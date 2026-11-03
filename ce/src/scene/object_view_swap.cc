#include "object_view_swap.h"

#include <cstdlib>
#include <cstring>
#include <utility>

#include "../memory.h"
#include "../tile.h"

namespace fallout::scene {
namespace {
const int* sortOffsets;
int compareOffsets(const void* a, const void* b)
{
    return sortOffsets[*static_cast<const int*>(a)] - sortOffsets[*static_cast<const int*>(b)];
}
}

ObjectViewSwap::~ObjectViewSwap()
{
    for (int i = 0; i < 2; i++) {
        if (state_.offsets[i] != nullptr) internal_free(state_.offsets[i]);
        if (state_.order[i] != nullptr) internal_free(state_.order[i]);
    }
    if (state_.div != nullptr) internal_free(state_.div);
    if (state_.mod != nullptr) internal_free(state_.mod);
    if (state_.render != nullptr) internal_free(state_.render);
}

bool ObjectViewSwap::prepare(unsigned char* buffer, int width, int height, int pitch)
{
    if (prepared_ || state_.offsets[0] != nullptr || buffer == nullptr || width < 1 || height < 1
        || pitch < width || width > 7680 || height > 4320 || gHexGridSize < 4) return false;
    state_.buffer = buffer;
    state_.width = width;
    state_.height = height;
    state_.pitch = pitch;
    state_.bufferSize = width * height;
    state_.windowRect = { 0, 0, width - 1, height - 1 };
    state_.updateBounds = { -320, -240, width + 320, height + 240 };
    state_.hexWidth = (width + 641) / 32 + 1;
    state_.hexHeight = (height + 481) / 12 + 1;
    state_.hexSize = state_.hexWidth * state_.hexHeight;
    const size_t ints = sizeof(int) * static_cast<size_t>(state_.hexSize);
    for (int i = 0; i < 2; i++) {
        state_.offsets[i] = static_cast<int*>(internal_malloc(ints));
        state_.order[i] = static_cast<int*>(internal_malloc(ints));
        if (state_.offsets[i] == nullptr || state_.order[i] == nullptr) return false;
    }
    state_.div = static_cast<int*>(internal_malloc(ints));
    state_.mod = static_cast<int*>(internal_malloc(ints));
    state_.render = static_cast<ObjectListNode**>(internal_malloc(sizeof(*state_.render) * state_.hexSize));
    if (state_.div == nullptr || state_.mod == nullptr || state_.render == nullptr) return false;

    const int savedCenter = gCenterTile;
    constexpr int flags = TILE_SET_CENTER_FLAG_IGNORE_BOUNDS | TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS;
    // Pick an interior tile to generate offset tables for both parities.
    bool ok = tileSetCenter(gHexGridSize / 2 + HEX_GRID_WIDTH / 2, flags) == 0;
    for (int parity = 0; ok && parity < 2; parity++) {
        const int origin = tileFromScreenXY(state_.updateBounds.left, state_.updateBounds.top);
        int x, y;
        if (origin < 0 || tileToScreenXY(origin, &x, &y) != 0) { ok = false; break; }
        int* output = state_.offsets[gCenterTile & 1];
        int shift = 16;
        x += 16;
        y += 8;
        if (x > state_.updateBounds.left) shift = -shift;
        int tileX = x;
        for (int row = 0; ok && row < state_.hexHeight; row++) {
            for (int col = 0; col < state_.hexWidth; col++) {
                int tile = tileFromScreenXY(tileX, y);
                if (tile < 0) { ok = false; break; }
                *output++ = tile - origin;
                tileX += 32;
            }
            tileX = shift + x;
            y += 12;
            shift = -shift;
        }
        if (parity == 0 && ok) ok = tileSetCenter(gCenterTile + 1, flags) == 0;
    }
    if (tileSetCenter(savedCenter, flags) != 0) ok = false;
    if (!ok) return false;
    state_.centerToUpperLeft = tileFromScreenXY(state_.updateBounds.left, state_.updateBounds.top) - gCenterTile;
    for (int i = 0; i < state_.hexSize; i++) {
        state_.div[i] = i / state_.hexWidth;
        state_.mod[i] = i % state_.hexWidth;
        state_.order[0][i] = i;
        state_.order[1][i] = i;
        state_.render[i] = nullptr;
    }
    for (int i = 0; i < 2; i++) {
        sortOffsets = state_.offsets[i];
        std::qsort(state_.order[i], state_.hexSize, sizeof(int), compareOffsets);
    }
    sortOffsets = nullptr;
    prepared_ = true;
    return true;
}

bool ObjectViewSwap::exchange(ObjectViewState& state)
{
    if (!prepared_ || state.buffer == nullptr || state.offsets[0] == nullptr || state.offsets[1] == nullptr
        || state.order[0] == nullptr || state.order[1] == nullptr || state.div == nullptr
        || state.mod == nullptr || state.render == nullptr) return false;
    std::swap(state_, state);
    return true;
}
} // namespace fallout::scene
