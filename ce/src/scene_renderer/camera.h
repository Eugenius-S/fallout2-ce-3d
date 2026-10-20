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

// A free camera for looking around a map (dev tools): a GL eye and where it
// looks, by yaw and pitch (radians). While it flies the scene is drawn from
// it, with no egg and nothing of the game's own picture over it.
struct StaticFlyCamera {
    float eye[3];
    float yaw;
    float pitch;
};

// nullptr: back to the game view.
void setStaticFlyCamera(const StaticFlyCamera* camera);
bool staticFlyCameraActive();
// Where the game view looks from and at (GL), as a fly camera's start; false
// before the view is known.
bool staticGameViewPose(StaticFlyCamera* camera);
// The direction a fly camera looks in, and its screen right, in GL world.
void staticFlyCameraAxes(const StaticFlyCamera& camera, float forward[3], float right[3]);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_CAMERA_H */
