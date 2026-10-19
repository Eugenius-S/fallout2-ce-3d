#ifndef FALLOUT_SCENE_SCENE_OVERLAY_H
#define FALLOUT_SCENE_SCENE_OVERLAY_H

namespace fallout {
namespace scene {

// The 3D scene in play: the game's own picture (interface, mouse cursor,
// floating text) is shown over it, and the mouse asks the scene what lies
// under it. Called first thing each presented frame: turns the tile window's
// scene overlay on or off (tileSetSceneOverlay) and redraws it. Returns
// whether it is on.
bool syncOverlay();

// Clears (transparent, premultiplied) the pixels of the palette-expanded
// frame where the scene shows through: the tile window's own colour-0 pixels.
void maskOverlay(const unsigned char* indices, int indexPitch, unsigned char* rgba, int rgbaPitch,
    int width, int height);

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_SCENE_OVERLAY_H */
