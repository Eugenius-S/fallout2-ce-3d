#ifndef FALLOUT_SCENE_RENDERER_STATIC_SCENE_H
#define FALLOUT_SCENE_RENDERER_STATIC_SCENE_H

#include <filesystem>
#include <string>
#include <vector>

#include "map_clip.h"

namespace fallout {
namespace scene_renderer {

struct SpriteCardProjection;

// The exported scene of one map elevation, from one cache directory: the
// versioned floor.bin sidecar with tiles.png, and walls.bin with its atlas
// pages and baked.bin.
bool loadStaticFloor(const std::filesystem::path& cacheDirectory, std::string* error);
bool loadStaticWalls(const std::filesystem::path& cacheDirectory, std::string* error);
// Drops the loaded scene (floor, walls, projection, live objects, the hero)
// so another map's can be loaded.
void unloadStaticScene();
void staticWallStats(unsigned int* triangles, unsigned int* pages);
MapClip staticFloorMapClip();
bool setStaticFloorMapProjection(const SpriteCardProjection& projection);
void setStaticMapClip(const MapClip& clip);
MapClip staticMapClip();
void bindStaticMapClipUniforms();

// Which roof squares the game shows now: 100 x 100 bytes in the engine's
// square order (y * 100 + x), nonzero shown.
void setStaticRoofVisibility(const unsigned char* visible);
// The game's see-through circle around the hero (its egg) over roofs: the
// egg's mask (0: the roof shows whole, else its share in 128ths) and its
// top-left corner on screen, in game logical pixels. The mask must stay alive.
void setStaticEgg(const unsigned char* mask, int width, int height, int left, int top);
void clearStaticEgg();
// The egg as the scene shows it now, for the snapshot metadata: false when no
// egg is shown; else its rect and the shown zoom it is sampled through.
bool staticEggInfo(int* left, int* top, int* width, int* height, double* zoom);
// How many walls.bin object rows the game marked see-through this frame, and
// whether the scene has the see-through wall data at all.
void staticWallSeeThroughStats(int* markedRows, bool* any);
// A light-carrying object of the game, lit in the scene as a point light.
struct StaticPointLight {
    int tile = -1;
    int objectX = 0; // the object's pixel movement off its hex
    int objectY = 0;
    // Where it shines from: game pixels right of and below its anchor
    // (negative: above); the height is how far above.
    double right = 0.0;
    double down = 0.0;
    float color[3] = { 1.0f, 1.0f, 1.0f }; // the brightest channel 1
    float intensity = 0.0f; // the object's lightIntensity / 65536
    int distance = 0; // the object's lightDistance, hexes
    std::vector<int> reach; // the hexes its light reaches as the engine spreads it
};

// The game's light: the ambient level (65536 full); the light of the many
// small lights per hex (HEX_GRID_SIZE values, 65536 full, without the
// engine's base) with their colour (rgb per hex, the brightest channel 1);
// and the big lights, each lit from its own point. The scene adds them up as
// the engine does, in their colours; big lights have no hex steps and walls
// take more of one they face. Roofs keep the ambient level.
void setStaticLight(int ambient, std::vector<int> hexLight, std::vector<float> hexColor,
    std::vector<StaticPointLight> lights);
void clearStaticLight();

// Objects walls.bin already draws (baked.bin): the caller only picks them.
bool staticObjectBaked(int tile, int fid);
// Things standing on a solid (an item on a table) are moved onto it, along the
// game's view ray: GL offset (x, y up, z). False when the object stays put.
bool staticObjectLift(int tile, int fid, float lift[3]);
// Items and scenery the donor lays on the floor (low piles): drawn flat, as
// the game's FLAT objects are.
bool staticObjectFlat(int tile, int fid);
// Doors and gates: GL x and z of a point of their wall's plane and its
// direction (radians); under the fly camera they lie on it.
bool staticObjectPlane(int tile, int fid, float plane[3]);
// Low cards (a cabbage, a tin): flat under the fly camera.
bool staticObjectFlyFlat(int tile, int fid);
// Dev (--dev-solo): draw only the things within radius hexes of tile, without
// walls and roofs, to look at one thing from every side. Set before a map loads.
void setStaticSolo(int tile, int radius);
// The same on a loaded map: walls.bin is read again (dev batch snapshots).
bool resoloStaticWalls(int tile, int radius);
bool staticSoloKeeps(int tile);
// The walls and scenery walls.bin draws that the hero stands behind, so the egg
// cuts them as the game does: begin, mark each, end (uploads on a change).
void beginStaticWallSeeThrough();
void markStaticWallSeeThrough(int tile, int fid);
// A baked object the engine paints at full light (OBJECT_TRANS_WALL), this frame.
void markStaticWallFullLight(int tile, int fid);
// Known luminous sign: only its colored tubes shine, not the backing board.
void markStaticWallEmissive(int tile, int fid);
void endStaticWallSeeThrough();

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_STATIC_SCENE_H */
