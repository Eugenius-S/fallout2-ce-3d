#include "screen_buffer_swap.h"

#include <limits>
#include <utility>

#include "../memory.h"
#include "../window_manager.h"

namespace fallout::scene {

ScreenBufferSwap::~ScreenBufferSwap() { if (buffer_ != nullptr) internal_free(buffer_); }

bool ScreenBufferSwap::prepare(int width, int height, bool buffered)
{
    if (prepared_ || width < 640 || height < 480) return false;
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

} // namespace fallout::scene
