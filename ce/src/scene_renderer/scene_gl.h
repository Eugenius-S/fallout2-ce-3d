#ifndef FALLOUT_SCENE_RENDERER_SCENE_GL_H
#define FALLOUT_SCENE_RENDERER_SCENE_GL_H

#include <SDL.h>
#include <cstddef>
#include <filesystem>
#include <string>

#include "ground_pick.h"

namespace fallout {
namespace scene_renderer {

struct StaticPreviewCamera {
    float eye[3];
    float target[3];
    float fovY;
    float nearPlane;
    float farPlane;
    bool donorScreenRight;
};

struct StaticHeroFrame {
    int tile;
    int elevation;
    int fid;
    int frame;
    int rotation;
    int objectX;
    int objectY;
    int artXOffset;
    int artYOffset;
    int width;
    int height;
    const unsigned char* rgba;
    size_t rgbaSize;
    int stack = 0; // sprites the game paints on the same hex before this one
};

// One live map object (critter, item, scenery, door) in its current frame.
// `rgba` may be null when staticObjectArtCached() says the frame is uploaded.
struct StaticObjectFrame {
    int tile;
    int fid;
    int frame;
    int rotation;
    int objectX;
    int objectY;
    int artXOffset;
    int artYOffset;
    int width;
    int height;
    bool flat;
    const unsigned char* rgba;
    size_t rgbaSize;
    int stack = 0; // sprites the game paints on the same hex before this one
    // The engine's translucent look (_dark_translucent_trans_buf_to_buf): the
    // tint's RGB, and whether the glass gray table is used; tint < 0: opaque.
    float tint[3] = { -1.0f, -1.0f, -1.0f };
    bool glassGray = false;
};

struct StaticHeroInfo {
    int tile;
    int fid;
    int frame;
    int rotation;
    double worldU;
    double worldV;
};

// Takes over presentation of the CE window with an OpenGL 3.3 context.
// `logicalWidth/Height` is the size of the game framebuffer (the vanilla
// SDL_Renderer logical size equivalent): the frame is scaled into the window
// with aspect preserved and letterboxed, like SDL_RenderSetLogicalSize did.
bool init(SDL_Window* window, int logicalWidth, int logicalHeight);
void shutdown();
bool isActive();

// Presents one RGBA8 frame (the palette-expanded game buffer) as a
// full-viewport quad, then swaps. Pixels are top-down, `pitch` in bytes.
void presentFrame(const unsigned char* rgba, int width, int height, int pitch);

// Optional overlay drawn over the last presented frame (movie frames): an
// RGBA image mapped into `dstX/dstY/dstW/dstH` in LOGICAL coordinates.
void drawOverlay(const unsigned char* rgba, int width, int height, int pitch, int dstX, int dstY, int dstW, int dstH);

// Swaps after overlay drawing.
void swap();

// Loads a fixed static-floor preview into independent GL resources. The
// versioned floor.bin sidecar and tiles.png are read from one cache directory.
bool loadStaticFloor(const std::filesystem::path& cacheDirectory, std::string* error);
bool loadStaticWalls(const std::filesystem::path& cacheDirectory, std::string* error);
bool loadStaticProjection(const std::filesystem::path& cacheDirectory, std::string* error);
void staticWallStats(unsigned int* triangles, unsigned int* pages);
bool updateStaticHero(const StaticHeroFrame& frame, std::string* error);
void clearStaticHero();
bool staticHeroInfo(StaticHeroInfo* info);
// Live objects are rebuilt every presented frame between begin and end.
// Objects walls.bin already draws (baked.bin) are skipped by the caller.
bool staticObjectBaked(int tile, int fid);
// Things standing on a solid (an item on a table) are moved onto it, along the
// game's view ray: GL offset (x, y up, z). False when the object stays put.
bool staticObjectLift(int tile, int fid, float lift[3]);
bool staticObjectArtCached(int fid, int frame, int rotation);
void beginStaticObjects();
bool addStaticObject(const StaticObjectFrame& frame, std::string* error);
void endStaticObjects();
void clearStaticObjects();
unsigned int staticObjectCount();
bool pickStaticGroundTile(int logicalX, int logicalY, GroundPickResult* result);
void setStaticGroundCursor(int tile);
// The wall/hero view is the game's own projection centered on this hex (the
// engine's gCenterTile), with a mild perspective (game_view.h). -1 = the
// fixed preview camera.
void setStaticCameraCenterTile(int tile);
void drawStaticFloor();
void drawStaticWallPreview();
void drawStaticHeroPreview();
StaticPreviewCamera staticPreviewCamera(bool withWalls);
bool captureStaticFloorPng(const std::filesystem::path& path, int* width, int* height, std::string* error);
bool captureStaticPreviewPng(const std::filesystem::path& path, bool withWalls, bool withHero,
    int* width, int* height, std::string* error);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SCENE_GL_H */
