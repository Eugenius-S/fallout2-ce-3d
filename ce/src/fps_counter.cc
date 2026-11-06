#include "fps_counter.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <SDL.h>

#include "dev_output.h"
#include "platform_compat.h"
#include "settings.h"

namespace fallout {

namespace {

struct FrameLogState {
    bool open = false; // whether the open CSV belongs to the current setting
    FILE* stream = nullptr;
    std::vector<float> frameMilliseconds;
    unsigned int loggedFrames = 0;
};

FrameLogState& state()
{
    static FrameLogState value;
    return value;
}

void closeLog()
{
    FrameLogState& log = state();
    if (!log.open) return;
    log.open = false;
    if (log.stream != nullptr) {
        fclose(log.stream);
        log.stream = nullptr;
    }

    std::vector<float>& frames = log.frameMilliseconds;
    if (frames.empty()) return;

    std::vector<float> sorted = frames;
    std::sort(sorted.begin(), sorted.end());

    double total = 0.0;
    for (float ms : sorted) {
        total += ms;
    }
    const double averageFps = sorted.size() * 1000.0 / total;
    // The 1% low: the frame time the slowest 1% of the frames goes past.
    const std::size_t index = sorted.size() > 1 ? sorted.size() * 99 / 100 : 0;
    const double onePercentLowMilliseconds = sorted[index];
    const double worstMilliseconds = sorted.back();

    printf("frame log: %u frames, average %.1f fps, 1%% low %.1f fps (%.1f ms), worst %.1f ms\n",
        log.loggedFrames, averageFps, 1000.0 / onePercentLowMilliseconds,
        onePercentLowMilliseconds, worstMilliseconds);
    fflush(stdout);

    const std::filesystem::path summaryPath = dev_output::outputFile("frame_log.summary.txt");
    FILE* summary = compat_fopen(summaryPath.string().c_str(), "w");
    if (summary != nullptr) {
        fprintf(summary, "frames,%u\n", log.loggedFrames);
        fprintf(summary, "average_fps,%.2f\n", averageFps);
        fprintf(summary, "one_percent_low_fps,%.2f\n", 1000.0 / onePercentLowMilliseconds);
        fprintf(summary, "one_percent_low_ms,%.2f\n", onePercentLowMilliseconds);
        fprintf(summary, "worst_ms,%.2f\n", worstMilliseconds);
        fclose(summary);
    }

    frames.clear();
    log.loggedFrames = 0;
}

} // namespace

void fpsCounterOnFrame()
{
    FrameLogState& log = state();

    if (settings.scene3d.frame_log) {
        if (!log.open) {
            const std::filesystem::path path = dev_output::outputFile("frame_log.csv");
            log.stream = compat_fopen(path.string().c_str(), "w");
            if (log.stream != nullptr) {
                fprintf(log.stream, "frame,ms\n");
            }
            log.open = true;
        }
        if (log.stream != nullptr) {
            static Uint32 lastFrameTicks = 0;
            const Uint32 now = SDL_GetTicks();
            double milliseconds = 0.0;
            if (lastFrameTicks != 0) {
                milliseconds = static_cast<double>(now - lastFrameTicks);
            }
            lastFrameTicks = now;
            fprintf(log.stream, "%u,%.3f\n", log.loggedFrames, milliseconds);
            log.frameMilliseconds.push_back(static_cast<float>(milliseconds));
            log.loggedFrames++;
            if (log.loggedFrames % 100 == 0) {
                fflush(log.stream);
            }
        }
    } else if (log.open) {
        closeLog();
    }
}

void fpsCounterFlushLog()
{
    closeLog();
}

} // namespace fallout
