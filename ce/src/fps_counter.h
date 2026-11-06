#ifndef FALLOUT_FPS_COUNTER_H
#define FALLOUT_FPS_COUNTER_H

namespace fallout {

// 127: the frame-time log ([3d] frame_log, --dev-frame-log): every frame's
// time to a CSV, a summary (average FPS, the 1% low, the worst frame) when
// the log closes. The readout itself lives in renderPresent (svga.cc).
void fpsCounterOnFrame();
void fpsCounterFlushLog();

} // namespace fallout

#endif /* FALLOUT_FPS_COUNTER_H */
