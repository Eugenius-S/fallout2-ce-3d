#ifndef FALLOUT_SCENE_RENDERER_STATIC_SCENE_H
#define FALLOUT_SCENE_RENDERER_STATIC_SCENE_H

#include <filesystem>
#include <string>

namespace fallout {
namespace scene_renderer {

// The exported scene of one map elevation, from one cache directory: the
// versioned floor.bin sidecar with tiles.png, and walls.bin with its atlas
// pages and baked.bin.
bool loadStaticFloor(const std::filesystem::path& cacheDirectory, std::string* error);
bool loadStaticWalls(const std::filesystem::path& cacheDirectory, std::string* error);
void staticWallStats(unsigned int* triangles, unsigned int* pages);

// Objects walls.bin already draws (baked.bin): the caller only picks them.
bool staticObjectBaked(int tile, int fid);
// Things standing on a solid (an item on a table) are moved onto it, along the
// game's view ray: GL offset (x, y up, z). False when the object stays put.
bool staticObjectLift(int tile, int fid, float lift[3]);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_STATIC_SCENE_H */
