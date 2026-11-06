#ifndef FALLOUT_SCENE_DISPLAY_WINDOW_TRANSACTION_H
#define FALLOUT_SCENE_DISPLAY_WINDOW_TRANSACTION_H

#include <SDL.h>
#include "../svga.h"

namespace fallout::scene {

// Keeps the original SDL window state available until the caller commits the
// rest of the framebuffer and window-layout transaction.
class DisplayWindowTransaction {
public:
    enum class Result { Applied, RejectedRestored, RestoreFailed, InvalidState };
    explicit DisplayWindowTransaction(SDL_Window* window);
    ~DisplayWindowTransaction();
    DisplayWindowTransaction(const DisplayWindowTransaction&) = delete;
    DisplayWindowTransaction& operator=(const DisplayWindowTransaction&) = delete;

    Result apply(WindowMode mode, int width, int height);
    bool rollback();
    void commit();

private:
    SDL_Window* window_ = nullptr;
    SDL_DisplayMode displayMode_ = {};
    Uint32 fullscreenFlags_ = 0;
    int x_ = 0, y_ = 0, width_ = 0, height_ = 0;
    bool valid_ = false;
    bool changed_ = false;
};

} // namespace fallout::scene

#endif
