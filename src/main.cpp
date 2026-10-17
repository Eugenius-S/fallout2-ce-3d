#include "app.h"
#include "render.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <windows.h>
#include <dbghelp.h>

namespace {

// Crash forensics: symbolize the faulting address via dbghelp so the exact
// line is known without an interactive debugger.
LONG WINAPI crashFilter(EXCEPTION_POINTERS* info) {
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    SymInitialize(GetCurrentProcess(), nullptr, TRUE);
    void* addr = info->ExceptionRecord->ExceptionAddress;
    std::fprintf(stderr, "\n[crash] code=0x%08lX at %p\n",
                 static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode), addr);
    DWORD64 moduleBase = 0;
    char moduleName[MAX_PATH] = {};
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCSTR>(addr),
                           reinterpret_cast<HMODULE*>(&moduleBase))) {
        GetModuleFileNameA(reinterpret_cast<HMODULE>(moduleBase), moduleName, MAX_PATH);
        moduleBase = reinterpret_cast<DWORD64>(GetModuleHandleA(
            moduleName[0] ? moduleName : nullptr));
        std::fprintf(stderr, "[crash] module %s + 0x%llx\n", moduleName,
                     static_cast<unsigned long long>(reinterpret_cast<DWORD64>(addr) -
                                                     moduleBase));
    }
    DWORD64 symAddr = reinterpret_cast<DWORD64>(addr);
    alignas(SYMBOL_INFO) char symBuf[sizeof(SYMBOL_INFO) + 256] = {};
    SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(symBuf);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 255;
    DWORD64 disp = 0;
    if (SymFromAddr(GetCurrentProcess(), symAddr, &disp, sym)) {
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        DWORD lineDisp = 0;
        BOOL haveLine = SymGetLineFromAddr64(GetCurrentProcess(), symAddr, &lineDisp, &line);
        std::fprintf(stderr, "[crash] symbol %s + %llu (%s:%lu)\n", sym->Name,
                     static_cast<unsigned long long>(disp),
                     haveLine ? line.FileName : "?", haveLine ? line.LineNumber : 0);
    } else {
        std::fprintf(stderr, "[crash] no symbol (SymFromAddr failed)\n");
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

namespace {

using namespace f2native;

struct Options {
    RenderMode mode = RenderMode::Native;
    Route route = Route::Static;
    CameraKind camera = CameraKind::Free;
    uint64_t smokeFrames = 0;     // render N frames then exit (G0 smoke)
    double warmupSeconds = 0.0;
    double measureSeconds = 0.0;
    uint32_t width = 1280;
    uint32_t height = 720;
    bool selftest = false;
    bool novsync = false;
    std::filesystem::path runDir;
};

Options parseArgs(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) throw std::invalid_argument("missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--mode") {
            const std::string v = next();
            if (v == "native") o.mode = RenderMode::Native;
            else if (v == "carrier") o.mode = RenderMode::Carrier;
            else if (v == "neural") o.mode = RenderMode::Neural;
            else throw std::invalid_argument("--mode must be native|carrier|neural");
        } else if (arg == "--route") {
            const std::string v = next();
            if (v == "static") o.route = Route::Static;
            else if (v == "pan") o.route = Route::Pan;
            else throw std::invalid_argument("--route must be static|pan");
        } else if (arg == "--camera") {
            const std::string v = next();
            if (v == "free") o.camera = CameraKind::Free;
            else if (v == "iso") o.camera = CameraKind::Iso;
            else throw std::invalid_argument("--camera must be free|iso");
        } else if (arg == "--smoke-frames") {
            o.smokeFrames = std::strtoull(next(), nullptr, 10);
        } else if (arg == "--warmup") {
            o.warmupSeconds = std::strtod(next(), nullptr);
        } else if (arg == "--seconds") {
            o.measureSeconds = std::strtod(next(), nullptr);
        } else if (arg == "--width") {
            o.width = std::strtoul(next(), nullptr, 10);
        } else if (arg == "--height") {
            o.height = std::strtoul(next(), nullptr, 10);
        } else if (arg == "--run-dir") {
            o.runDir = next();
        } else if (arg == "--novsync") {
            o.novsync = true;
        } else if (arg == "--selftest") {
            o.selftest = true;
        } else if (arg == "--help" || arg == "-h") {
            std::printf(
                "f2native - Fallout 2 native DLSS5 prototype\n"
                "  --mode native|carrier|neural   (default native)\n"
                "  --route static|pan             (default static)\n"
                "  --smoke-frames N               render N frames, exit 0\n"
                "  --warmup S / --seconds S       timed benchmark, CSV frame times\n"
                "  --run-dir DIR                  CSV/log output directory\n"
                "  --width W --height H           window client size (default 1280x720)\n"
                "  --selftest                     numerical contract checks, exit 4 on fail\n");
            exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + arg);
        }
    }
    if (o.width == 0 || o.height == 0) throw std::invalid_argument("width/height must be > 0");
    return o;
}

} // namespace

namespace {
Gpu* g_gpu = nullptr;   // single-window resize plumbing
Render* g_render = nullptr;

void onWindowResize(uint32_t width, uint32_t height) {
    // THE FIX (verifier point 4): the render targets and the NGX feature are
    // size-bound; resizing the swapchain alone left them stale.
    if (g_gpu) g_gpu->resize(width, height);
    if (g_render) g_render->resize(width, height);
}

} // namespace

int main(int argc, char** argv) {
    SetUnhandledExceptionFilter(crashFilter);
    Options opts;
    try {
        opts = parseArgs(argc, argv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FATAL: %s\n", e.what());
        return 2;
    }

    Gpu gpu;
    g_gpu = &gpu;
    gpu.setVsync(!opts.novsync);
    Render render;
    g_render = &render;

    try {
        f2native::App app;
        const HWND hwnd = app.createWindow(L"F2Native - native DLSS5 prototype", opts.width,
                                           opts.height, onWindowResize);
        gpu.initialize(hwnd, opts.width, opts.height);
        render.initialize(gpu, opts.mode, opts.camera, opts.width, opts.height);

        if (opts.selftest) {
            const SelftestResult st = render.runSelftest();
            std::fputs(st.details.c_str(), stderr);
            std::printf("%s\n", st.allPassed ? "SELFTEST OK" : "SELFTEST FAILED");
            render.shutdown();
            gpu.shutdown();
            app.shutdown();
            return st.allPassed ? 0 : 4;
        }

        std::ofstream csv;
        if (!opts.runDir.empty()) {
            std::filesystem::create_directories(opts.runDir);
            csv.open(opts.runDir / "frames.csv");
            csv << "frame,cpu_ms\n";
        }

        auto tick = [] { return std::chrono::steady_clock::now(); };
        const auto runStart = tick();
        const auto measureStart = runStart + std::chrono::duration<double>(opts.warmupSeconds);
        uint64_t frames = 0;
        uint64_t measuredFrames = 0;
        double measuredSum = 0.0;
        std::vector<double> measured;
        auto last = tick();
        bool resetHistory = false;

        for (;;) {
            if (!app.pumpMessages()) break;
            const double tSeconds =
                std::chrono::duration<double>(tick() - runStart).count();
            if (!render.renderFrame(tSeconds, opts.route, resetHistory)) break;
            ++frames;

            const auto now = tick();
            const double cpuMs = std::chrono::duration<double, std::milli>(now - last).count();
            last = now;
            if (now >= measureStart) {
                ++measuredFrames;
                measuredSum += cpuMs;
                measured.push_back(cpuMs);
                if (csv) csv << frames << "," << cpuMs << "\n";
                if (opts.measureSeconds > 0.0 &&
                    std::chrono::duration<double>(now - measureStart).count() >=
                        opts.measureSeconds) {
                    break;
                }
            }
            if (opts.smokeFrames > 0 && frames >= opts.smokeFrames) break;
        }

        if (!measured.empty()) {
            std::vector<double> sorted = measured;
            std::sort(sorted.begin(), sorted.end());
            const double median = sorted[sorted.size() / 2];
            const double p95 = sorted[static_cast<size_t>(sorted.size() * 0.95) % sorted.size()];
            const double fps = measuredFrames * 1000.0 / measuredSum;
            std::printf("RUN OK: mode=%s frames=%llu measured=%llu fps=%.1f median=%.3fms p95=%.3fms evals=%llu\n",
                        opts.mode == RenderMode::Native ? "native"
                            : opts.mode == RenderMode::Carrier ? "carrier" : "neural",
                        static_cast<unsigned long long>(frames),
                        static_cast<unsigned long long>(measuredFrames), fps, median, p95,
                        static_cast<unsigned long long>(render.neural().evaluations()));
            if (csv) {
                std::ofstream summary(opts.runDir / "summary.txt");
                summary << "frames " << frames << "\nmeasured " << measuredFrames
                        << "\nfps " << fps << "\nmedian_ms " << median << "\np95_ms " << p95
                        << "\nevals " << render.neural().evaluations() << "\n";
            }
        } else {
            std::printf("SMOKE OK: rendered %llu frames\n",
                        static_cast<unsigned long long>(frames));
        }

        render.shutdown();
        gpu.shutdown();
        app.shutdown();
        return 0;
    } catch (const std::exception& e) {
        std::fflush(stdout);
        std::fprintf(stderr, "FATAL: %s\n", e.what());
        return 3;
    }
}
