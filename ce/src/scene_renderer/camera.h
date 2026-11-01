#ifndef FALLOUT_SCENE_RENDERER_CAMERA_H
#define FALLOUT_SCENE_RENDERER_CAMERA_H

#include <filesystem>
#include <string>

namespace fallout {
namespace scene_renderer {

struct GameView;
struct SpriteCardProjection;

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
// The current game-view projection, positioned at a candidate center hex.
bool gameViewAtCenterTile(int tile, GameView* view);

StaticPreviewCamera staticPreviewCamera(bool withWalls);

// The donor's pixel-to-ground projection, once projection.bin is loaded.
bool currentGameProjection(SpriteCardProjection* projection);

// The game view's eye distance (units, 100 by default); a far eye (thousands)
// is the game's own parallel projection, for comparing with 2D.
void setStaticViewDistance(double units);

// The 3D view zoom around the screen center; 1.0 is the game's own size.
void setCameraZoom(double zoom);

// A free camera for looking around a map (dev tools): a GL eye and where it
// looks, by yaw and pitch (radians). While it flies the scene is drawn from
// it, with no egg and nothing of the game's own picture over it.
struct StaticFlyCamera {
    float eye[3];
    float yaw;
    float pitch;
    float fovY = 0.0f; // degrees; 0: the fly camera's own
};

// nullptr: back to the game view.
void setStaticFlyCamera(const StaticFlyCamera* camera);
bool staticFlyCameraActive();
bool staticFlyCamera(StaticFlyCamera* camera);
// Where the game view looks from and at (GL), as a fly camera's start; false
// before the view is known.
bool staticGameViewPose(StaticFlyCamera* camera);
// The game view's camera itself: its eye and field of view (camera rotation
// circles it).
bool staticGameViewCamera(StaticFlyCamera* camera);
// The direction a fly camera looks in, and its screen right, in GL world.
void staticFlyCameraAxes(const StaticFlyCamera& camera, float forward[3], float right[3]);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_CAMERA_H */
