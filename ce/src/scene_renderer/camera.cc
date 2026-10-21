#include "camera.h"

#include <algorithm>
#include <cmath>

#include "ground_pick.h"
#include "scene_internal.h"
#include "../tile.h"

namespace fallout {
namespace scene_renderer {

namespace internal {

bool gHeroProjectionLoaded = false;
HeroProjection gHeroProjection;

} // namespace internal

using namespace internal;

namespace {

// The hex at the center of the game's screen (gCenterTile); with the hero
// projection loaded the wall/hero view is the game's own projection there.
int gCameraCenterTile = -1;
// The camera glides towards the game's view center instead of jumping a hex
// at a time: the game pixel at the screen center now, and when it last moved.
double gCameraPixel[2] = {};
Uint64 gCameraMoved = 0;

void multiply4x4(const float* a, const float* b, float* out)
{
    float result[16] = {};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            for (int k = 0; k < 4; k++) {
                result[col * 4 + row] += a[k * 4 + row] * b[col * 4 + k];
            }
        }
    }
    std::copy(result, result + 16, out);
}

// Eye distance of the game view (units): it sets how strong the perspective
// is. The 25-degree camera's 36.9 was asked milder twice, closer to the game's
// own parallel projection: 52, then 100.
double gViewDistance = 100.0;

// The 3D view zoom around the screen center (1 = the game's own size).
double gZoom = 1.0;

double gameViewDistance()
{
    return gViewDistance;
}

bool gFlying = false;
StaticFlyCamera gFly = {};
constexpr float kFlyFovY = 50.0f;
// The game's GL world is the donor's ground reflected (u, z, v): its screen
// right is reflected too, as the game view's.
constexpr bool kFlyScreenRight = true;

} // namespace

namespace internal {

SpriteCardProjection cardProjection()
{
    return {
        { gHeroProjection.squ[0], gHeroProjection.squ[1] },
        { gHeroProjection.sqv[0], gHeroProjection.sqv[1] },
        { gHeroProjection.origin[0], gHeroProjection.origin[1] },
        gHeroProjection.hpx, gHeroProjection.bbPx, gHeroProjection.rowTilt, gHeroProjection.stack,
    };
}

void makeFloorMvp(float aspect, const StaticPreviewCamera& camera, float* out)
{
    constexpr float kPi = 3.14159265358979323846f;
    const float f = 1.0f / std::tan(camera.fovY * kPi / 360.0f);
    const float kNear = camera.nearPlane;
    const float kFar = camera.farPlane;
    float projection[16] = {};
    projection[0] = f / aspect;
    projection[5] = f;
    projection[10] = (kFar + kNear) / (kNear - kFar);
    projection[11] = -1.0f;
    projection[14] = (2.0f * kFar * kNear) / (kNear - kFar);

    // Fixed preview camera above the square floor, aimed at map center.
    const float* eye = camera.eye;
    const GroundPickCamera basisCamera = {
        { eye[0], eye[1], eye[2] },
        { camera.target[0], camera.target[1], camera.target[2] }, camera.fovY, camera.donorScreenRight,
    };
    double forwardD[3], sideD[3], cameraUpD[3];
    if (!previewCameraBasis(basisCamera, forwardD, sideD, cameraUpD)) {
        std::fill(out, out + 16, 0.0f);
        return;
    }
    float forward[3], side[3], cameraUp[3];
    for (int i = 0; i < 3; i++) {
        forward[i] = static_cast<float>(forwardD[i]);
        side[i] = static_cast<float>(sideD[i]);
        cameraUp[i] = static_cast<float>(cameraUpD[i]);
    }
    float view[16] = {
        side[0], cameraUp[0], -forward[0], 0.0f,
        side[1], cameraUp[1], -forward[1], 0.0f,
        side[2], cameraUp[2], -forward[2], 0.0f,
        -(side[0] * eye[0] + side[1] * eye[1] + side[2] * eye[2]),
        -(cameraUp[0] * eye[0] + cameraUp[1] * eye[1] + cameraUp[2] * eye[2]),
        forward[0] * eye[0] + forward[1] * eye[1] + forward[2] * eye[2],
        1.0f,
    };
    multiply4x4(projection, view, out);
}

bool currentGameView(GameView* view)
{
    if (!gHeroProjectionLoaded || gCameraCenterTile < 0) return false;
    // Whole screen pixels keep the texel grid steady while the camera glides.
    int x, y, width, height;
    double screenPerGame = 1.0;
    if (fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &x, &y, &width, &height)) {
        screenPerGame = static_cast<double>(width) / gLogicalWidth;
    }
    *view = {
        { gHeroProjection.squ[0], gHeroProjection.squ[1] },
        { gHeroProjection.sqv[0], gHeroProjection.sqv[1] },
        { gHeroProjection.origin[0], gHeroProjection.origin[1] },
        gHeroProjection.hpx,
        { std::round(gCameraPixel[0] * screenPerGame) / screenPerGame,
            std::round(gCameraPixel[1] * screenPerGame) / screenPerGame },
        gLogicalWidth, gLogicalHeight, gameViewDistance(),
    };
    view->zoom = gZoom;
    // The 2D's own place for the center hex, so the 3D frames the map as 2D does.
    int screenX, screenY;
    if (tileToScreenXY(gCameraCenterTile, &screenX, &screenY) == 0) {
        view->screenCenterSet = true;
        view->screenCenter[0] = screenX + 16.0;
        view->screenCenter[1] = screenY + 8.0;
    }
    return true;
}

void resetCamera()
{
    gHeroProjectionLoaded = false;
    gHeroProjection = {};
    gCameraCenterTile = -1;
    gCameraMoved = 0;
}

bool flyCameraRight(double* x, double* z)
{
    if (!gFlying) return false;
    float forward[3], right[3];
    staticFlyCameraAxes(gFly, forward, right);
    *x = right[0];
    *z = right[2];
    return true;
}

bool flyCameraMvp(float aspect, float* out)
{
    if (!gFlying) return false;
    float forward[3], right[3];
    staticFlyCameraAxes(gFly, forward, right);
    const StaticPreviewCamera camera = {
        { gFly.eye[0], gFly.eye[1], gFly.eye[2] },
        { gFly.eye[0] + forward[0], gFly.eye[1] + forward[1], gFly.eye[2] + forward[2] },
        gFly.fovY > 0.0f ? gFly.fovY : kFlyFovY, 0.05f, 400.0f, kFlyScreenRight,
    };
    makeFloorMvp(aspect, camera, out);
    return true;
}

} // namespace internal

void setCameraZoom(double zoom)
{
    if (!std::isfinite(zoom) || zoom < 0.1 || zoom > 10.0) return;
    gZoom = zoom;
}

bool currentGameProjection(SpriteCardProjection* projection)
{
    if (!internal::gHeroProjectionLoaded || projection == nullptr) return false;
    *projection = internal::cardProjection();
    return true;
}

void setStaticViewDistance(double units)
{
    gViewDistance = units;
}

void setStaticFlyCamera(const StaticFlyCamera* camera)
{
    gFlying = camera != nullptr;
    if (camera != nullptr) gFly = *camera;
}

bool staticFlyCameraActive()
{
    return gFlying;
}

void staticFlyCameraAxes(const StaticFlyCamera& camera, float forward[3], float right[3])
{
    forward[0] = std::cos(camera.pitch) * std::cos(camera.yaw);
    forward[1] = std::sin(camera.pitch);
    forward[2] = std::cos(camera.pitch) * std::sin(camera.yaw);
    // previewCameraBasis with the reflected screen right.
    const float length = std::max(std::sqrt(forward[0] * forward[0] + forward[2] * forward[2]), 1e-6f);
    right[0] = forward[2] / length;
    right[1] = 0.0f;
    right[2] = -forward[0] / length;
}

bool staticGameViewPose(StaticFlyCamera* camera)
{
    GameView view;
    double eye[3], centerX, centerZ;
    if (!currentGameView(&view) || !gameViewEye(view, eye) || !gameViewCenter(view, &centerX, &centerZ)) return false;
    const double to[3] = { centerX - eye[0], -eye[1], centerZ - eye[2] };
    const double length = std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    if (!(length > 0.0)) return false;
    // Start three quarters of the way to the ground: the game view's eye is far up its ray.
    const double start = 0.75;
    for (int i = 0; i < 3; i++) camera->eye[i] = static_cast<float>(eye[i] + to[i] * start);
    camera->yaw = static_cast<float>(std::atan2(to[2], to[0]));
    camera->pitch = static_cast<float>(std::asin(to[1] / length));
    return true;
}

bool staticGameViewCamera(StaticFlyCamera* camera)
{
    // The game's view is no plain perspective (world to game pixels to the
    // screen), so the camera is put where a plain one, looking the same way at
    // the same point, shows the same width of floor across the screen's middle.
    GameView view;
    if (!currentGameView(&view) || !staticGameViewPose(camera) || view.logicalHeight <= 0) return false;
    const double middleX = view.screenCenterSet ? view.screenCenter[0] : view.logicalWidth / 2.0;
    const double middleY = view.screenCenterSet ? view.screenCenter[1] : view.logicalHeight / 2.0;
    double leftX, leftZ, rightX, rightZ, centerX, centerZ;
    if (!gameViewPickGround(view, 0.0, middleY, &leftX, &leftZ)
        || !gameViewPickGround(view, view.logicalWidth, middleY, &rightX, &rightZ)
        || !gameViewPickGround(view, middleX, middleY, &centerX, &centerZ)) {
        return false;
    }
    const double width = std::hypot(rightX - leftX, rightZ - leftZ);
    camera->fovY = staticPreviewCamera(true).fovY;
    const double aspect = static_cast<double>(view.logicalWidth) / view.logicalHeight;
    const double distance = width / (2.0 * aspect * std::tan(camera->fovY * 3.14159265358979323846 / 360.0));
    float forward[3], right[3];
    staticFlyCameraAxes(*camera, forward, right);
    const float center[3] = { static_cast<float>(centerX), 0.0f, static_cast<float>(centerZ) };
    for (int i = 0; i < 3; i++) camera->eye[i] = center[i] - forward[i] * static_cast<float>(distance);
    return true;
}

StaticPreviewCamera staticPreviewCamera(bool withWalls)
{
    GroundPickCamera camera = staticPreviewCameraParameters(withWalls);
    GameView view;
    double eye[3], centerX, centerZ;
    if (withWalls && currentGameView(&view) && gameViewEye(view, eye) && gameViewCenter(view, &centerX, &centerZ)) {
        camera = { { eye[0], eye[1], eye[2] }, { centerX, 0.0, centerZ }, camera.fovY, camera.donorScreenRight };
    }
    return {
        { static_cast<float>(camera.eye[0]), static_cast<float>(camera.eye[1]), static_cast<float>(camera.eye[2]) },
        { static_cast<float>(camera.target[0]), static_cast<float>(camera.target[1]), static_cast<float>(camera.target[2]) },
        static_cast<float>(camera.fovY), 0.1f, 400.0f, camera.donorScreenRight,
    };
}

void setStaticCameraCenterTile(int tile)
{
    if (tile < 0 || tile >= HEX_GRID_SIZE) {
        gCameraCenterTile = -1;
        return;
    }
    double target[2];
    gameViewTilePixel(tile, target);
    const Uint64 now = SDL_GetPerformanceCounter();
    const bool first = gCameraCenterTile < 0 || gCameraMoved == 0;
    const double seconds = first ? 0.0 : static_cast<double>(now - gCameraMoved) / SDL_GetPerformanceFrequency();
    gameViewGlide(gCameraPixel, target, seconds, first);
    gCameraMoved = now;
    gCameraCenterTile = tile;
}

bool loadStaticProjection(const std::filesystem::path& cacheDirectory, std::string* error)
{
    if (gHeroProjectionLoaded) {
        if (error != nullptr) error->clear();
        return true;
    }
    auto fail = [&](const std::string& message) {
        gHeroProjectionLoaded = false;
        gHeroProjection = {};
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized) return fail("OpenGL renderer is unavailable");

    constexpr size_t kHeaderSize = 12;
    constexpr size_t kValueCount = 10;
    constexpr size_t kPayloadSize = kHeaderSize + kValueCount * sizeof(double);
    std::vector<unsigned char> bytes;
    if (!readBinary(cacheDirectory / "projection.bin", bytes)) {
        return fail("projection.bin is missing, unreadable, or exceeds 64 MB");
    }
    if (bytes.size() != kPayloadSize || bytes[0] != 'F' || bytes[1] != '2'
        || bytes[2] != 'P' || bytes[3] != 'J'
        || readU16(bytes.data() + 4) != 1 || readU16(bytes.data() + 6) != kHeaderSize
        || readU32(bytes.data() + 8) != kValueCount) {
        return fail("projection.bin has an invalid format or length");
    }
    double values[kValueCount];
    for (size_t index = 0; index < kValueCount; index++) {
        values[index] = readF64(bytes.data() + kHeaderSize + index * sizeof(double));
        if (!std::isfinite(values[index])) return fail("projection.bin contains a non-finite value");
    }
    const double determinant = values[0] * values[3] - values[2] * values[1];
    if (std::abs(determinant) < 1e-9
        || std::abs(values[0]) > 1000000.0 || std::abs(values[1]) > 1000000.0
        || std::abs(values[2]) > 1000000.0 || std::abs(values[3]) > 1000000.0
        || std::abs(values[4]) > 1000000.0 || std::abs(values[5]) > 1000000.0
        || values[6] < 1e-3 || values[6] > 4096.0
        || values[7] < 1e-3 || values[7] > 4096.0
        || values[8] < 0.0 || values[8] > 1.0
        || values[9] < 0.0 || values[9] > 1.0) {
        return fail("projection.bin values are out of bounds or not invertible");
    }
    gHeroProjection = { { values[0], values[1] }, { values[2], values[3] },
        { values[4], values[5] }, values[6], values[7], values[8], values[9] };
    gHeroProjectionLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

} // namespace scene_renderer
} // namespace fallout
