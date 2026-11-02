#include "screen_buffer_swap.h"

#include <limits>
#include <cstring>
#include <utility>

#include "../memory.h"
#include "../window_manager.h"
#include "text_capture.h"

namespace fallout::scene {

ScreenBufferSwap::~ScreenBufferSwap() { if (buffer_ != nullptr) internal_free(buffer_); }

bool ScreenBufferSwap::prepare(int width, int height, bool buffered)
{
    if (prepared_ || width < 640 || height < 480 || width > 7680 || height > 4320) return false;
    const auto w = static_cast<size_t>(width);
    const auto h = static_cast<size_t>(height);
    if (w > std::numeric_limits<size_t>::max() / h) return false;
    if (buffered) {
        buffer_ = static_cast<unsigned char*>(internal_malloc(w * h));
        if (buffer_ == nullptr) return false;
    }
    bounds_ = { 0, 0, width - 1, height - 1 };
    buffered_ = buffered;
    prepared_ = true;
    return true;
}

bool ScreenBufferSwap::exchange(unsigned char*& screenBuffer, Rect& screenBounds, Window* root)
{
    if (!prepared_ || root == nullptr || (screenBuffer != nullptr) != buffered_) return false;
    std::swap(screenBuffer, buffer_);
    std::swap(screenBounds, bounds_);
    root->rect = screenBounds;
    root->width = screenBounds.right - screenBounds.left + 1;
    root->height = screenBounds.bottom - screenBounds.top + 1;
    return true;
}

WindowBufferSwap::~WindowBufferSwap()
{
    if (buffer_ != nullptr) {
        if (ownerWindow_ >= 0) textCaptureForgetWindow(ownerWindow_, buffer_);
        internal_free(buffer_);
    }
}

bool WindowBufferSwap::prepare(int width, int height)
{
    if (buffer_ != nullptr || width < 1 || height < 1 || width > 7680 || height > 4320) return false;
    const auto w = static_cast<size_t>(width);
    const auto h = static_cast<size_t>(height);
    if (w > std::numeric_limits<size_t>::max() / h) return false;
    buffer_ = static_cast<unsigned char*>(internal_malloc(w * h));
    if (buffer_ == nullptr) return false;
    std::memset(buffer_, 0, w * h);
    width_ = width;
    height_ = height;
    return true;
}

bool WindowBufferSwap::exchange(Window* window)
{
    if (window == nullptr || window->id == 0 || window->buffer == nullptr || buffer_ == nullptr
        || (ownerWindow_ >= 0 && ownerWindow_ != window->id)
        || window->buttonListHead != nullptr || window->menuBar != nullptr) return false;
    ownerWindow_ = window->id;
    std::swap(window->buffer, buffer_);
    std::swap(window->width, width_);
    std::swap(window->height, height_);
    window->rect.right = window->rect.left + window->width - 1;
    window->rect.bottom = window->rect.top + window->height - 1;
    textCaptureMarkWindowBuffer(window->buffer, window->id);
    return true;
}

} // namespace fallout::scene
