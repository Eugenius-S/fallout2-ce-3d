#ifndef FPS_LIMITER_H
#define FPS_LIMITER_H

namespace fallout {

class FpsLimiter {
public:
    FpsLimiter(unsigned int fps = 60);
    void mark();
    void throttle() const;
    // 157: pace to an explicit target. 0 removes the limit entirely; values
    // below 10 wait with the classic coarse delay, above it the wait finishes
    // with a short spin so the pace does not wobble with the timer grain.
    void throttle(double fps) const;

private:
    const unsigned int _fps;
    unsigned int _ticks;
};

} // namespace fallout

#endif /* FPS_LIMITER_H */
