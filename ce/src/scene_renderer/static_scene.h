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
// Drops the loaded scene (floor, walls, projection, live objects, the hero)
// so another map's can be loaded.
void unloadStaticScene();
void staticWallStats(unsigned int* triangles, unsigned int* pages);

// Which roof squares the game shows now: 100 x 100 bytes in the engine's
// square order (y * 100 + x), nonzero shown.
void setStaticRoofVisibility(const unsigned char* visible);
// The game's see-through circle around the hero (its egg) over roofs: the
// egg's mask (0: the roof shows whole, else its share in 128ths) and its
// top-left corner on screen, in game logical pixels. The mask must stay alive.
void setStaticEgg(const unsigned char* mask, int width, int height, int left, int top);
void clearStaticEgg();
// The game's light: each hex's intensity (HEX_GRID_SIZE values, 65536 full)
// and the ambient level. The scene is lit as the engine paints: the floor
// by its hexes, walls and sprites by their own hex, roofs by the ambient.
void setStaticLight(const int* tileIntensity, int ambient);
void clearStaticLight();

// Objects walls.bin already draws (baked.bin): the caller only picks them.
bool staticObjectBaked(int tile, int fid);
// Things standing on a solid (an item on a table) are moved onto it, along the
// game's view ray: GL offset (x, y up, z). False when the object stays put.
bool staticObjectLift(int tile, int fid, float lift[3]);
// The walls and scenery walls.bin draws that the hero stands behind, so the egg
// cuts them as the game does: begin, mark each, end (uploads on a change).
void beginStaticWallSeeThrough();
void markStaticWallSeeThrough(int tile, int fid);
// A baked object the engine paints at full light (OBJECT_TRANS_WALL), this frame.
void markStaticWallFullLight(int tile, int fid);
void endStaticWallSeeThrough();

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_STATIC_SCENE_H */
