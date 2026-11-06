#include "perf.h"

#include <SDL.h>

#include <algorithm>
#include <mutex>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../dev_output.h"
#include "../settings.h"
#include "gl_api.h"

namespace fallout {
namespace scene_renderer {

namespace {

constexpr int kRing = 4;
constexpr int kMaxMarks = 16;
constexpr unsigned int kTimestamp = 0x8E28; // GL_TIMESTAMP
constexpr unsigned int kQueryResult = 0x8866;
constexpr unsigned int kQueryAvailable = 0x8867;
constexpr double kWindowSeconds = 2.0;

struct Slot {
    unsigned int queries[kMaxMarks + 1] = {};
    const char* names[kMaxMarks] = {};
    int marks = -1; // -1: no frame recorded here
};

struct Toggle {
    std::string key;
    int value;
    double at;
    bool done;
};

struct Span {
    const char* name;
    double sum;
};

bool gChecked = false;
bool gOn = false;
bool gQueriesReady = false;
bool gInFrame = false;
Slot gSlots[kRing];
int gCurrent = -1;
Uint64 gRunStart = 0;
Uint64 gLastEnd = 0;
Uint64 gFrameStart = 0;
Uint64 gWindowStart = 0;
std::vector<Toggle> gToggles;
std::string gToggleNote;
std::vector<Span> gSpans;
double gGpuSum = 0.0;
unsigned gGpuFrames = 0;
double gCpuSum = 0.0;
std::vector<double> gWindow; // frame intervals this window, ms
std::vector<double> gRun; // every frame interval, ms
Uint64 gLoadStart = 0;
std::vector<const char*> gLoadMarks; // names marked since the chain's start

double seconds(Uint64 ticks)
{
    return static_cast<double>(ticks) / static_cast<double>(SDL_GetPerformanceFrequency());
}

void parseToggles(const char* text)
{
    // "shadows=0@3,glow=1@6": key, value, seconds into the run.
    const char* p = text;
    while (*p != '\0') {
        char key[32] = {};
        int value = 0;
        double at = 0.0;
        int used = 0;
        if (std::sscanf(p, "%31[a-z_]=%d@%lf%n", key, &value, &at, &used) < 3 || used <= 0) break;
        gToggles.push_back({ key, value, at, false });
        p += used;
        if (*p == ',') p++;
    }
}

void applyToggle(const Toggle& toggle)
{
    if (toggle.key == "shadows") settings.scene3d.shadows = toggle.value != 0;
    else if (toggle.key == "glow") settings.scene3d.glow = toggle.value != 0;
    else if (toggle.key == "clouds") settings.scene3d.cloud_shadows = toggle.value != 0;
    else if (toggle.key == "lights") settings.scene3d.lights = toggle.value != 0;
    else if (toggle.key == "sun") settings.scene3d.sun = toggle.value != 0;
}

void writeLog(const std::string& line)
{
    // The warm thread pool marks art reads concurrently (172): one append at
    // a time.
    static std::mutex logMutex;
    std::lock_guard<std::mutex> guard(logMutex);
    if (FILE* log = std::fopen((dev_output::outputDirectory("logs") / "devperf.log").string().c_str(), "a")) {
        std::fputs(line.c_str(), log);
        std::fclose(log);
    }
}

bool initPerf()
{
    if (!gChecked) {
        gChecked = true;
        const char* flag = SDL_getenv("F2CE_PERF");
        gOn = flag != nullptr && flag[0] != '\0' && flag[0] != '0';
        const char* toggles = SDL_getenv("F2CE_PERF_TOGGLE");
        if (gOn && toggles != nullptr) parseToggles(toggles);
    }
    if (!gOn) return false;
    if (!gQueriesReady) {
        if (glGenQueries == nullptr) {
            gOn = false;
            return false;
        }
        for (Slot& slot : gSlots) glGenQueries(kMaxMarks + 1, slot.queries);
        gQueriesReady = true;
    }
    return true;
}

// Mean of the slowest 1% of the frames (at least one).
double slowestShare(std::vector<double> frames)
{
    if (frames.empty()) return 0.0;
    std::sort(frames.begin(), frames.end(), [](double a, double b) { return a > b; });
    const size_t count = std::max<size_t>(1, frames.size() / 100);
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) sum += frames[i];
    return sum / static_cast<double>(count);
}

double mean(const std::vector<double>& frames)
{
    if (frames.empty()) return 0.0;
    double sum = 0.0;
    for (double value : frames) sum += value;
    return sum / static_cast<double>(frames.size());
}

void addSpan(const char* name, double ms)
{
    for (Span& span : gSpans) {
        if (std::strcmp(span.name, name) == 0) {
            span.sum += ms;
            return;
        }
    }
    gSpans.push_back({ name, ms });
}

void collect(Slot& slot)
{
    if (slot.marks < 0) return;
    int ready = 0;
    glGetQueryObjectiv(slot.queries[slot.marks], kQueryAvailable, &ready);
    if (ready == 0) {
        slot.marks = -1;
        return;
    }
    unsigned long long stamps[kMaxMarks + 1] = {};
    for (int i = 0; i <= slot.marks; i++) glGetQueryObjectui64v(slot.queries[i], kQueryResult, &stamps[i]);
    for (int i = 0; i < slot.marks; i++) addSpan(slot.names[i], static_cast<double>(stamps[i + 1] - stamps[i]) / 1e6);
    gGpuSum += static_cast<double>(stamps[slot.marks] - stamps[0]) / 1e6;
    gGpuFrames++;
    slot.marks = -1;
}

void flushWindow(double elapsed)
{
    const unsigned frames = static_cast<unsigned>(gWindow.size());
    if (frames == 0) return;
    char head[512];
    std::snprintf(head, sizeof head,
        "perf fps=%.1f frame_avg_ms=%.2f frame_low1_ms=%.2f frame_max_ms=%.2f cpu_ms=%.2f gpu_ms=%.2f"
        " run_avg_ms=%.2f run_low1_ms=%.2f",
        frames / elapsed, mean(gWindow), slowestShare(gWindow), *std::max_element(gWindow.begin(), gWindow.end()),
        gCpuSum / frames, gGpuFrames > 0 ? gGpuSum / gGpuFrames : 0.0, mean(gRun), slowestShare(gRun));
    std::string line = head;
    for (const Span& span : gSpans) {
        char part[96];
        std::snprintf(part, sizeof part, " %s_ms=%.2f", span.name, gGpuFrames > 0 ? span.sum / gGpuFrames : 0.0);
        line += part;
    }
    line += "\n";
    writeLog(line);
    gWindow.clear();
    gSpans.clear();
    gGpuSum = 0.0;
    gGpuFrames = 0;
    gCpuSum = 0.0;
}

} // namespace

bool perfEnabled()
{
    if (!gChecked) {
        const char* flag = SDL_getenv("F2CE_PERF");
        return flag != nullptr && flag[0] != '\0' && flag[0] != '0';
    }
    return gOn;
}

void perfBegin()
{
    gInFrame = false;
    if (!initPerf()) return;
    const Uint64 now = SDL_GetPerformanceCounter();
    if (gRunStart == 0) {
        gRunStart = now;
        gWindowStart = now;
    }
    const double elapsed = seconds(now - gRunStart);
    for (Toggle& toggle : gToggles) {
        if (toggle.done || elapsed < toggle.at) continue;
        toggle.done = true;
        applyToggle(toggle);
        char note[96];
        std::snprintf(note, sizeof note, "%s%s=%d", gToggleNote.empty() ? "" : " ", toggle.key.c_str(), toggle.value);
        gToggleNote += note;
    }
    gCurrent = (gCurrent + 1) % kRing;
    Slot& slot = gSlots[gCurrent];
    slot.marks = 0;
    glQueryCounter(slot.queries[0], kTimestamp);
    gFrameStart = now;
    gInFrame = true;
}

void perfMark(const char* name)
{
    if (!gInFrame) return;
    Slot& slot = gSlots[gCurrent];
    if (slot.marks >= kMaxMarks) return;
    slot.names[slot.marks] = name;
    slot.marks++;
    glQueryCounter(slot.queries[slot.marks], kTimestamp);
}

void perfEnd()
{
    if (!gInFrame) return;
    gInFrame = false;
    const Uint64 now = SDL_GetPerformanceCounter();
    gCpuSum += seconds(now - gFrameStart) * 1000.0;
    if (gLastEnd != 0) {
        const double interval = seconds(now - gLastEnd) * 1000.0;
        gWindow.push_back(interval);
        gRun.push_back(interval);
        if (!gToggleNote.empty()) {
            // The frame a toggle landed in: its length is the stall.
            char line[160];
            std::snprintf(line, sizeof line, "perf toggle %s frame_ms=%.2f\n", gToggleNote.c_str(), interval);
            writeLog(line);
            gToggleNote.clear();
        }
    }
    gLastEnd = now;
    collect(gSlots[(gCurrent + 1) % kRing]);
    const double sinceWindow = seconds(now - gWindowStart);
    if (sinceWindow >= kWindowSeconds) {
        flushWindow(sinceWindow);
        gWindowStart = now;
    }
}

bool perfHolding()
{
    if (!perfEnabled()) return false;
    const char* hold = SDL_getenv("F2CE_PERF_HOLD");
    if (hold == nullptr || gRunStart == 0) return hold != nullptr;
    return seconds(SDL_GetPerformanceCounter() - gRunStart) < std::atof(hold);
}

PerfScope::PerfScope(const char* what)
    : what_(what)
    , started_(perfEnabled() ? SDL_GetPerformanceCounter() : 0)
{
}

PerfScope::~PerfScope()
{
    if (started_ == 0) return;
    char line[160];
    std::snprintf(line, sizeof line, "perf load %s ms=%.2f\n", what_, seconds(SDL_GetPerformanceCounter() - started_) * 1000.0);
    writeLog(line);
}

void perfLoadStart(const char* what)
{
    if (!perfEnabled()) return;
    gLoadStart = SDL_GetPerformanceCounter();
    gLoadMarks.clear();
    char line[128];
    std::snprintf(line, sizeof line, "perf load chain %s begin\n", what);
    writeLog(line);
}

void perfLoadMark(const char* what)
{
    if (!perfEnabled() || gLoadStart == 0) return;
    for (const char* marked : gLoadMarks) {
        if (std::strcmp(marked, what) == 0) return;
    }
    gLoadMarks.push_back(what);
    char line[160];
    std::snprintf(line, sizeof line, "perf load %s ms=%.2f since chain start\n", what,
        seconds(SDL_GetPerformanceCounter() - gLoadStart) * 1000.0);
    writeLog(line);
}

// The owner shot reads the recent frame rate whatever F2CE_PERF says (183):
// a plain ring of the last intervals, ticked once per presented frame and
// read on the main thread only, so no lock.
constexpr int kFrameRing = 300;
double gFrameRing[kFrameRing] = {};
int gFrameRingCount = 0;
int gFrameRingAt = 0;
Uint64 gFrameTickLast = 0;

void perfFrameTick()
{
    const Uint64 now = SDL_GetPerformanceCounter();
    if (gFrameTickLast != 0) {
        gFrameRing[gFrameRingAt] = seconds(now - gFrameTickLast) * 1000.0;
        gFrameRingAt = (gFrameRingAt + 1) % kFrameRing;
        if (gFrameRingCount < kFrameRing) gFrameRingCount++;
    }
    gFrameTickLast = now;
}

bool perfFrameStats(double* fps, double* averageMs, double* low1Ms){
    if (gFrameRingCount < 2) return false;
    std::vector<double> frames;
    frames.reserve(gFrameRingCount);
    double sum = 0.0;
    for (int i = 0; i < gFrameRingCount; i++) {
        const double ms = gFrameRing[(gFrameRingAt + kFrameRing - 1 - i) % kFrameRing];
        frames.push_back(ms);
        sum += ms;
    }
    std::sort(frames.begin(), frames.end(), [](double a, double b) { return a > b; });
    const size_t count = std::max<size_t>(1, frames.size() / 100);
    double slowest = 0.0;
    for (size_t i = 0; i < count; i++) slowest += frames[i];
    *fps = static_cast<double>(frames.size()) * 1000.0 / sum;
    *averageMs = sum / static_cast<double>(frames.size());
    *low1Ms = slowest / static_cast<double>(count);
    return true;
}

void perfLog(const char* line)
{
    if (!perfEnabled()) return;
    writeLog(line);
}

} // namespace scene_renderer
} // namespace fallout
