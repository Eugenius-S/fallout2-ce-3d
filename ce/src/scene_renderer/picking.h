#ifndef FALLOUT_SCENE_RENDERER_PICKING_H
#define FALLOUT_SCENE_RENDERER_PICKING_H

#include "ground_pick.h"

namespace fallout {
namespace scene_renderer {

// The floor hex the scene shows at a screen pixel (game logical pixels).
bool pickStaticGroundTile(int logicalX, int logicalY, GroundPickResult* result);

// The objects whose cards show an opaque texel at a screen pixel (game logical
// pixels), nearest first, as the scene draws them: up to `capacity` pick ids
// that `wanted` accepts (null: all). Returns how many.
int pickStaticObjects(int logicalX, int logicalY, bool (*wanted)(int pickId, void* context), void* context,
    int* pickIds, int capacity);

// Screen pixel (game logical pixels) where the scene shows the floor point the
// game draws at a game pixel.
bool staticScreenFromGamePixel(double pixelX, double pixelY, double* logicalX, double* logicalY);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_PICKING_H */
