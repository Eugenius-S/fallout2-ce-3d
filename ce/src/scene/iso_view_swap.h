#ifndef FALLOUT_SCENE_ISO_VIEW_SWAP_H
#define FALLOUT_SCENE_ISO_VIEW_SWAP_H

#include <utility>
#include "../geometry.h"

namespace fallout::scene {

// Swap non-owning aliases only. The corresponding WindowBufferSwap must stay
// alive until the entire frame transaction is committed or rolled back.
class IsoViewSwap {
public:
    bool prepare(unsigned char* buffer, const Rect& rect)
    {
        if (prepared_ || buffer == nullptr || rect.left != 0 || rect.top != 0
            || rect.right < 0 || rect.bottom < 0 || rect.right >= 7680 || rect.bottom >= 4320) return false;
        buffer_ = buffer;
        rect_ = rect;
        prepared_ = true;
        return true;
    }
    bool exchange(unsigned char*& buffer, Rect& rect)
    {
        if (!prepared_ || buffer == nullptr) return false;
        std::swap(buffer_, buffer);
        std::swap(rect_, rect);
        return true;
    }
private:
    unsigned char* buffer_ = nullptr;
    Rect rect_ = {};
    bool prepared_ = false;
};

// Floating text retains its objects and only changes the drawing target.
class TextViewSwap {
public:
    bool prepare(unsigned char* buffer, int width, int height)
    {
        if (prepared_ || buffer == nullptr || width < 1 || height < 1 || width > 7680 || height > 4320) return false;
        buffer_ = buffer;
        width_ = width;
        height_ = height;
        prepared_ = true;
        return true;
    }
    bool exchange(unsigned char*& buffer, int& width, int& height)
    {
        if (!prepared_ || buffer == nullptr) return false;
        std::swap(buffer_, buffer);
        std::swap(width_, width);
        std::swap(height_, height);
        return true;
    }
private:
    unsigned char* buffer_ = nullptr;
    int width_ = 0, height_ = 0;
    bool prepared_ = false;
};

} // namespace fallout::scene

#endif
