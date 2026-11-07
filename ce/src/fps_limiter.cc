#include "fps_limiter.h"

#include <SDL.h>

namespace fallout {

FpsLimiter::FpsLimiter(unsigned int fps)
    : _fps(fps)
    , _ticks(0)
{
}

void FpsLimiter::mark()
{
    _ticks = SDL_GetTicks();
}

void FpsLimiter::throttle() const
{
    if (1000 / _fps > SDL_GetTicks() - _ticks) {
        SDL_Delay(1000 / _fps - (SDL_GetTicks() - _ticks));
    }
}

void FpsLimiter::throttle(double fps) const
{
    if (fps <= 0.0) return;
    if (fps < 10.0) {
        throttle();
        return;
    }
    // Sleep the bulk of the wait, then spin the last couple of milliseconds:
    // the delay wakes a millisecond late at best, and at 120 fps a couple of
    // late wakes are the difference between an even pace and a stutter.
    const double budgetMs = 1000.0 / fps;
    int remaining = static_cast<int>(budgetMs - (SDL_GetTicks() - _ticks));
    if (remaining <= 0) return;
    if (remaining > 2) SDL_Delay(remaining - 2);
    while (SDL_GetTicks() - _ticks < budgetMs) {
        SDL_Delay(0);
    }
}

} // namespace fallout
