#ifndef FALLOUT_SCENE_RENDERER_SCENE_DRAW_H
#define FALLOUT_SCENE_RENDERER_SCENE_DRAW_H

#include <filesystem>
#include <string>
#include <vector>

namespace fallout {
namespace scene_renderer {

// The loaded scene into the back buffer: the floor alone (fixed preview
// camera), with walls, or with walls, live objects and the hero.
void drawStaticFloor();
void drawStaticWallPreview();
void drawStaticHeroPreview();

// The same picture into a new PNG; with the hero it includes the game's
// picture over the scene as last presented. Dev comparisons can omit the
// overlay so UI readiness cannot change between captures in one run.
bool captureStaticFloorPng(const std::filesystem::path& path, int* width, int* height, std::string* error);
bool captureStaticPreviewPng(const std::filesystem::path& path, bool withWalls, bool withHero,
    int* width, int* height, std::string* error, bool includeOverlay = true);
// Captures the same static scene to a top-down RGBA buffer without writing a file.
bool captureStaticPreviewRgba(bool withWalls, bool withHero, std::vector<unsigned char>* rgba,
    int* width, int* height, std::string* error, bool includeOverlay = true);
// The game's own picture (palette-expanded RGBA, top-down) as presentFrame
// shows it in the window, into a new PNG at the window's size.
bool captureFramePng(const std::filesystem::path& path, const unsigned char* rgba, int frameWidth, int frameHeight,
    int pitch, int* width, int* height, std::string* error);
bool capturePresentedFramePng(const std::filesystem::path& path, int* width, int* height, std::string* error);

// The same captures without a file write: the GL readback and the redraw it
// needs stay on the main thread; the PNG encode and the file writes go to a
// worker (183, the owner shot). Top-down RGBA rows at the window's size.
bool captureStaticPreviewBuffer(bool withWalls, bool withHero, bool includeOverlay,
    std::vector<unsigned char>& rgba, int* width, int* height, std::string* error);
bool captureFrameBuffer(const unsigned char* rgba, int frameWidth, int frameHeight, int pitch,
    std::vector<unsigned char>& out, int* width, int* height, std::string* error);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SCENE_DRAW_H */
