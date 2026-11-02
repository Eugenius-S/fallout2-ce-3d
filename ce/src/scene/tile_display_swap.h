#ifndef FALLOUT_SCENE_TILE_DISPLAY_SWAP_H
#define FALLOUT_SCENE_TILE_DISPLAY_SWAP_H

#include <utility>

#include "../geometry.h"

namespace fallout::scene {

// Rebinds the existing tile renderer to another already allocated iso buffer.
// After exchange the caller recalculates the center and performs a full redraw.
class TileDisplaySwap {
public:
    bool prepare(unsigned char* buffer, int width, int height, int pitch)
    {
        if (prepared_ || buffer == nullptr || width < 1 || height < 1 || pitch < width
            || width > 7680 || height > 4320) return false;
        buffer_ = buffer;
        width_ = width;
        height_ = height;
        pitch_ = pitch;
        bounds_ = { 0, 0, width - 1, height - 1 };
        prepared_ = true;
        return true;
    }

    bool exchange(unsigned char*& buffer, int& width, int& height, int& pitch, Rect& bounds)
    {
        if (!prepared_ || buffer == nullptr) return false;
        std::swap(buffer_, buffer);
        std::swap(width_, width);
        std::swap(height_, height);
        std::swap(pitch_, pitch);
        std::swap(bounds_, bounds);
        return true;
    }

private:
    unsigned char* buffer_ = nullptr;
    int width_ = 0, height_ = 0, pitch_ = 0;
    Rect bounds_ = {};
    bool prepared_ = false;
};

} // namespace fallout::scene

#endif
