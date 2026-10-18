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
bool pickStaticGroundTile(int logicalX, int logicalY, GroundPickResult* result);
void setStaticGroundCursor(int tile);
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
