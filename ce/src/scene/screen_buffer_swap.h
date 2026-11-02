#ifndef FALLOUT_SCENE_SCREEN_BUFFER_SWAP_H
#define FALLOUT_SCENE_SCREEN_BUFFER_SWAP_H

#include "../geometry.h"

namespace fallout {
struct Window;
namespace scene {

// A prepared backing buffer. Exchanging twice restores the original state;
// destruction frees whichever buffer is no longer installed.
class ScreenBufferSwap {
public:
    ScreenBufferSwap() = default;
    ~ScreenBufferSwap();
    ScreenBufferSwap(const ScreenBufferSwap&) = delete;
    ScreenBufferSwap& operator=(const ScreenBufferSwap&) = delete;

    bool prepare(int width, int height, bool buffered);
    bool exchange(unsigned char*& screenBuffer, Rect& screenBounds, Window* root);

private:
    unsigned char* buffer_ = nullptr;
    Rect bounds_ = {};
    bool buffered_ = false;
    bool prepared_ = false;
};

} // namespace scene
} // namespace fallout

#endif
