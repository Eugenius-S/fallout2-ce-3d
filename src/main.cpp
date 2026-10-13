#include "app.h"
#include "gpu.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

f2native::Gpu g_gpu;
uint64_t g_smokeFrames = 0; // 0 = run until the window is closed
uint64_t g_framesDone = 0;

void onWindowResize(uint32_t width, uint32_t height) {
    g_gpu.resize(width, height);
}

void printUsage() {
    std::printf(
        "f2native — Fallout 2 native DLSS5 prototype viewer (G0 skeleton)\n"
        "  --smoke-frames N   render N frames, then exit 0 (G0 smoke test)\n"
        "  --width W          window client width  (default 1280)\n"
        "  --height H         window client height (default 720)\n");
}

} // namespace

int main(int argc, char** argv) {
    uint32_t width = 1280;
    uint32_t height = 720;

    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            auto next = [&]() -> const char* {
                if (i + 1 >= argc) throw std::invalid_argument("missing value for " + arg);
                return argv[++i];
            };
            if (arg == "--smoke-frames") {
                g_smokeFrames = std::strtoull(next(), nullptr, 10);
            } else if (arg == "--width") {
                width = static_cast<uint32_t>(std::strtoul(next(), nullptr, 10));
            } else if (arg == "--height") {
                height = static_cast<uint32_t>(std::strtoul(next(), nullptr, 10));
            } else if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            } else {
                throw std::invalid_argument("unknown argument: " + arg);
            }
        }

        f2native::App app;
        const HWND hwnd = app.createWindow(L"F2Native — native DLSS5 prototype (G0)", width,
                                           height, onWindowResize);
        g_gpu.initialize(hwnd, width, height);

        while (app.pumpMessages()) {
            g_gpu.renderFrame();
            g_framesDone++;
            if (g_smokeFrames > 0 && g_framesDone >= g_smokeFrames) break;
        }

        g_gpu.waitGpu();
        g_gpu.shutdown();
        app.shutdown();
        std::printf("SMOKE OK: rendered %llu frames\n",
                    static_cast<unsigned long long>(g_framesDone));
        return 0;
    } catch (const std::exception& e) {
        std::fflush(stdout);
        std::fprintf(stderr, "FATAL: %s\n", e.what());
        return 3; // GPU error exit code per plan section 1
    }
}
