#ifndef FALLOUT_SCENE_RENDERER_BOARD_SHADOWS_H
#define FALLOUT_SCENE_RENDERER_BOARD_SHADOWS_H

#include <filesystem>
#include <string>

namespace fallout {
namespace scene_renderer {

// shadows.bin: the boards whose art carries a painted shadow, with the raw
// RGBA of the sprite without it (shadows.bin, task 046). With the scene's sun
// shadows on, the board's card draws these pixels instead of its own art, so
// the painted shadow does not double the real one; with them off, nothing of
// the module is consulted.
bool loadBoardShadows(const std::filesystem::path& cacheDirectory, std::string* error);
void unloadBoardShadows();
// The shadow-free art of the map object, when the scene has one for it in
// this exact frame and rotation; null otherwise.
const unsigned char* staticObjectShadowFreeArt(int tile, unsigned int fid, int frame, int rotation,
    int* width, int* height, int* artXOffset, int* artYOffset);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_BOARD_SHADOWS_H */
