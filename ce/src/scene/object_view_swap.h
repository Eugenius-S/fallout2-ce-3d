#ifndef FALLOUT_SCENE_OBJECT_VIEW_SWAP_H
#define FALLOUT_SCENE_OBJECT_VIEW_SWAP_H

#include "../geometry.h"
#include "../obj_types.h"

namespace fallout::scene {

struct ObjectViewState {
    unsigned char* buffer = nullptr;
    int width = 0, height = 0, pitch = 0, bufferSize = 0;
    Rect windowRect = {}, updateBounds = {};
    int hexWidth = 0, hexHeight = 0, hexSize = 0, centerToUpperLeft = 0;
    int* offsets[2] = {};
    int* order[2] = {};
    int* div = nullptr;
    int* mod = nullptr;
    ObjectListNode** render = nullptr;
};

// The tile view must already target the proposed buffer when prepare is called.
// No game frame may be processed between rebinding the tile and object views.
class ObjectViewSwap {
public:
    ObjectViewSwap() = default;
    ~ObjectViewSwap();
    ObjectViewSwap(const ObjectViewSwap&) = delete;
    ObjectViewSwap& operator=(const ObjectViewSwap&) = delete;
    bool prepare(unsigned char* buffer, int width, int height, int pitch);
    bool exchange(ObjectViewState& state);
private:
    ObjectViewState state_;
    bool prepared_ = false;
};

} // namespace fallout::scene

#endif
