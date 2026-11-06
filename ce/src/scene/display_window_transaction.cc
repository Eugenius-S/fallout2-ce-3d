#include "display_window_transaction.h"

namespace fallout::scene {

DisplayWindowTransaction::DisplayWindowTransaction(SDL_Window* window)
    : window_(window)
{
    if (window_ == nullptr || SDL_GetWindowDisplayMode(window_, &displayMode_) != 0) return;
    fullscreenFlags_ = SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_GetWindowPosition(window_, &x_, &y_);
    SDL_GetWindowSize(window_, &width_, &height_);
    valid_ = width_ > 0 && height_ > 0;
}

DisplayWindowTransaction::~DisplayWindowTransaction()
{
    if (changed_) rollback();
}

DisplayWindowTransaction::Result DisplayWindowTransaction::apply(WindowMode mode, int width, int height)
{
    if (!valid_ || changed_) return Result::InvalidState;
    if (width < 640 || height < 480) return Result::RejectedRestored;
    if (mode != WindowMode::Windowed && mode != WindowMode::Fullscreen
        && mode != WindowMode::WindowedFullscreen) return Result::RejectedRestored;

    changed_ = true;
    const auto reject = [this]() { return rollback() ? Result::RejectedRestored : Result::RestoreFailed; };
    if (SDL_SetWindowFullscreen(window_, 0) != 0) return reject();
    SDL_SetWindowSize(window_, width, height);
    if (mode == WindowMode::Fullscreen) {
        SDL_DisplayMode requested = displayMode_;
        requested.w = width;
        requested.h = height;
        if (SDL_SetWindowDisplayMode(window_, &requested) != 0
            || SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN) != 0) return reject();
    } else if (mode == WindowMode::WindowedFullscreen) {
        if (SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) return reject();
    }

    const Uint32 flags = SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    const Uint32 expected = mode == WindowMode::Windowed ? 0
        : mode == WindowMode::Fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP;
    int actualWidth = 0, actualHeight = 0;
    SDL_GetWindowSize(window_, &actualWidth, &actualHeight);
    if (flags != expected || (mode == WindowMode::Windowed
            && (actualWidth != width || actualHeight != height))) return reject();
    if (mode == WindowMode::Fullscreen) {
        SDL_DisplayMode actualMode = {};
        if (SDL_GetWindowDisplayMode(window_, &actualMode) != 0 || actualMode.w != width || actualMode.h != height) return reject();
    }
    return Result::Applied;
}

bool DisplayWindowTransaction::rollback()
{
    if (!valid_ || !changed_) return valid_;
    bool ok = SDL_SetWindowFullscreen(window_, 0) == 0;
    ok = SDL_SetWindowDisplayMode(window_, &displayMode_) == 0 && ok;
    SDL_SetWindowSize(window_, width_, height_);
    SDL_SetWindowPosition(window_, x_, y_);
    if (fullscreenFlags_ != 0) ok = SDL_SetWindowFullscreen(window_, fullscreenFlags_) == 0 && ok;
    int restoredWidth = 0, restoredHeight = 0;
    SDL_GetWindowSize(window_, &restoredWidth, &restoredHeight);
    const Uint32 restoredFlags = SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    ok = restoredFlags == fullscreenFlags_ && restoredWidth == width_ && restoredHeight == height_ && ok;
    if (fullscreenFlags_ == SDL_WINDOW_FULLSCREEN) {
        SDL_DisplayMode restoredMode = {};
        ok = SDL_GetWindowDisplayMode(window_, &restoredMode) == 0
            && restoredMode.w == displayMode_.w && restoredMode.h == displayMode_.h && ok;
    }
    if (fullscreenFlags_ == 0) {
        int restoredX = 0, restoredY = 0;
        SDL_GetWindowPosition(window_, &restoredX, &restoredY);
        ok = restoredX == x_ && restoredY == y_ && ok;
    }
    if (ok) changed_ = false;
    return ok;
}

void DisplayWindowTransaction::commit() { changed_ = false; }

} // namespace fallout::scene
