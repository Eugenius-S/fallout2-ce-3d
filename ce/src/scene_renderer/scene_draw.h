#ifndef FALLOUT_SCENE_RENDERER_SCENE_DRAW_H
#define FALLOUT_SCENE_RENDERER_SCENE_DRAW_H

#include <filesystem>
#include <string>

namespace fallout {
namespace scene_renderer {

// The loaded scene into the back buffer: the floor alone (fixed preview
// camera), with walls, or with walls, live objects and the hero.
void drawStaticFloor();
void drawStaticWallPreview();
void drawStaticHeroPreview();

// The same picture into a new PNG; with the hero it includes the game's
// picture over the scene as last presented.
bool captureStaticFloorPng(const std::filesystem::path& path, int* width, int* height, std::string* error);
bool captureStaticPreviewPng(const std::filesystem::path& path, bool withWalls, bool withHero,
    int* width, int* height, std::string* error);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SCENE_DRAW_H */
