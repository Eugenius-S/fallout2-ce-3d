#ifndef FALLOUT_SCENE_RENDERER_PERF_H
#define FALLOUT_SCENE_RENDERER_PERF_H

namespace fallout {
namespace scene_renderer {

// Dev-only frame profiler (task 155). Off unless F2CE_PERF is set in the
// environment; then devperf.log in the run's log folder gets, every two
// seconds, the frame rate, the average, 99th percentile and worst frame time,
// the CPU time spent submitting a frame, and the GPU time of each span between
// two marks.
//
// A frame is perfBegin(), any number of perfMark("name") (each ends the span
// that began at the previous mark and names it), perfEnd() after the swap.
// F2CE_PERF_TOGGLE="shadows=0@3,shadows=1@6" flips [3d] shadows, glow or
// clouds at the given seconds into the run, to measure what a toggle costs.
bool perfEnabled();
void perfBegin();
void perfMark(const char* name);
void perfEnd();
// F2CE_PERF_HOLD=N: true until N seconds after the first measured frame, so a
// one-shot test run keeps drawing long enough to measure.
bool perfHolding();

// A load stage's wall time, logged when the profiler is on.
class PerfScope {
public:
    explicit PerfScope(const char* what);
    ~PerfScope();
    PerfScope(const PerfScope&) = delete;
    PerfScope& operator=(const PerfScope&) = delete;

private:
    const char* what_;
    unsigned long long started_;
};

// A load chain's wall-clock marks (a map change, task 172): perfLoadStart
// begins the chain and each perfLoadMark logs the wall time since the chain's
// start, once per name per chain, so one map change reads as one table:
// map_load, scene_loaded, first_frame. Off with the profiler.
void perfLoadStart(const char* what);
void perfLoadMark(const char* what);

// One tick per presented frame, always on (the owner shot, 183): the ring of
// the last frame intervals, whatever F2CE_PERF says. Main thread only.
void perfFrameTick();
// The recent frames over that ring: the frame rate, the mean frame time and
// the mean of the slowest 1% (the 1% low). False until two frames passed.
bool perfFrameStats(double* fps, double* averageMs, double* low1Ms);
// One line into devperf.log (the profiler's file), so an in-game event lands
// in the same timeline as the frame windows around it.
void perfLog(const char* line);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_PERF_H */
