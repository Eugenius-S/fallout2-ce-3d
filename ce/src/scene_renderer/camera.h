#ifndef FALLOUT_SCENE_RENDERER_CAMERA_H
#define FALLOUT_SCENE_RENDERER_CAMERA_H

#include <filesystem>
#include <string>

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

// The game's projection of the scene (projection.bin from the cache).
bool loadStaticProjection(const std::filesystem::path& cacheDirectory, std::string* error);

// The wall/hero view is the game's own projection centered on this hex (the
// engine's gCenterTile), with a mild perspective (game_view.h). -1 = the
// fixed preview camera.
void setStaticCameraCenterTile(int tile);

StaticPreviewCamera staticPreviewCamera(bool withWalls);

// The game view's eye distance (units, 100 by default); a far eye (thousands)
// is the game's own parallel projection, for comparing with 2D.
void setStaticViewDistance(double units);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_CAMERA_H */
