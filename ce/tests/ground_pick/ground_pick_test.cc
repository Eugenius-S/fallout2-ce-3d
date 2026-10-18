#include "ground_pick.h"

#include <cmath>
#include <cstdio>
#include <limits>

using namespace fallout::scene_renderer;

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)
    const GroundPickProjection projection = { { -48, 12 }, { 32, 24 }, { 4784, -1190 } };
    const GroundPickCamera camera = { { -3.9561, 11, 21.11265 }, { -11.9561, 1, 9.11265 }, 50 };
    const GroundPickViewport viewport = { 640, 480, 640, 480, 640, 480 };
    double x = 0, y = 0;
    CHECK(projectGroundPoint(projection, camera, viewport, -12.541666667, 12.6875, &x, &y));
    // Independent camera/MVP golden, not a project-then-unproject round trip.
    CHECK(std::abs(x - 240.252317) < 0.01 && std::abs(y - 315.271943) < 0.01);
    GroundPickResult picked = {};
    CHECK(pickGroundTile(projection, camera, viewport, x, y, &picked));
    CHECK(picked.tile == 25075);
    CHECK(std::abs(picked.worldX + 12.541666667) < 1e-6);
    CHECK(std::abs(picked.floorV - 62.6875) < 1e-6);

    // CE direction order is NE,E,SE,SW,W,NW. The one-step tile-center
    // deltas below come from tile.cc's parity table at odd column 75.
    // The donor ground projection maps those screen deltas to these world
    // deltas; project them through the production camera and require the
    // original CE left/right facing to remain on the corresponding side.
    const double worldStep[6][2] = {
        { -0.5, -0.25 }, { -0.5, 0.25 }, { 0.0, 0.5 },
        { 0.5, 0.25 }, { 0.5, -0.25 }, { 0.0, -0.5 },
    };
    const int ceScreenRight[6] = { 1, 1, 1, -1, -1, -1 };
    const GroundPickCamera closeCamera = staticPreviewCameraParameters(true);
    CHECK(closeCamera.fovY == 25.0 && closeCamera.donorScreenRight);
    const double closeScale = std::tan(25.0 * 3.14159265358979323846 / 180.0)
        / std::tan(12.5 * 3.14159265358979323846 / 180.0);
    CHECK(std::abs((closeCamera.eye[0] - closeCamera.target[0]) / 8.0 - closeScale) < 1e-12);
    CHECK(std::abs((closeCamera.eye[1] - closeCamera.target[1]) / 10.0 - closeScale) < 1e-12);
    CHECK(std::abs((closeCamera.eye[2] - closeCamera.target[2]) / 12.0 - closeScale) < 1e-12);
    CHECK(std::abs(closeCamera.eye[0] - 4.8709233517) < 1e-8);
    CHECK(std::abs(closeCamera.eye[1] - 22.0337791896) < 1e-8);
    CHECK(std::abs(closeCamera.eye[2] - 34.3531850275) < 1e-8);
    CHECK(projectGroundPoint(projection, closeCamera, viewport,
        -12.541666667, 12.6875, &x, &y));
    CHECK(std::abs(x - 395.741122340) < 0.01 && std::abs(y - 311.490245737) < 0.01);
    const double expectedScreen[6][2] = {
        { 403.304148192, 302.306432956 }, { 412.599923932, 310.165588789 },
        { 405.068989698, 319.528854259 }, { 388.005263085, 320.883930814 },
        { 378.827816792, 312.819185258 }, { 386.592170623, 303.605823885 },
    };
    const GroundPickCamera overviewCamera = staticPreviewCameraParameters(false);
    CHECK(overviewCamera.fovY == 50.0 && !overviewCamera.donorScreenRight);
    CHECK(overviewCamera.eye[0] == 0.0 && overviewCamera.eye[1] == 88.0 && overviewCamera.eye[2] == 118.0);
    CHECK(overviewCamera.target[0] == 0.0 && overviewCamera.target[1] == 0.0 && overviewCamera.target[2] == 0.0);
    for (int direction = 0; direction < 6; direction++) {
        double neighborX = 0, neighborY = 0;
        CHECK(projectGroundPoint(projection, closeCamera, viewport,
            -12.541666667 + worldStep[direction][0],
            12.6875 + worldStep[direction][1], &neighborX, &neighborY));
        CHECK((neighborX - x) * ceScreenRight[direction] > 0.0);
        CHECK(std::abs(neighborX - expectedScreen[direction][0]) < 0.02);
        CHECK(std::abs(neighborY - expectedScreen[direction][1]) < 0.02);
    }

    // Exact donor hex mask edge: move the art pixel into its adjacent cell.
    CHECK(projectGroundPoint(projection, camera, viewport, -11.541666667, 12.6875, &x, &y));
    CHECK(std::abs(x - 265.557556) < 0.01 && std::abs(y - 327.991920) < 0.01);
    CHECK(donorHexFromPixel(4992, 764) == 25075);
    CHECK(donorHexFromPixel(5008, 764) == 25074);

    // SDL mouse coordinates are already logical; high-DPI drawable scaling is applied once.
    const GroundPickViewport highDpi = { 640, 480, 1280, 960, 640, 480 };
    CHECK(pickGroundTile(projection, camera, highDpi, 240.252317, 315.271943, &picked));
    CHECK(picked.tile == 25075);
    const GroundPickViewport letterboxed = { 640, 480, 1000, 600, 1000, 600 };
    CHECK(!pickGroundTile(projection, camera, letterboxed, 10, 240, &picked));
    const GroundPickViewport squareWindow = { 640, 480, 800, 800, 800, 800 };
    CHECK(projectGroundPoint(projection, camera, squareWindow, -12.541666667, 12.6875, &x, &y));
    CHECK(std::abs(x - 240.252317) < 0.02 && std::abs(y - 296.453957) < 0.02);
    CHECK(pickGroundTile(projection, camera, squareWindow, x, y, &picked) && picked.tile == 25075);
    CHECK(!pickGroundTile(projection, camera, squareWindow, 320, 10, &picked));
    CHECK(!pickGroundTile(projection, camera, viewport, 0, 0, &picked));

    CHECK(!pickGroundTile(projection, camera, viewport, std::numeric_limits<double>::quiet_NaN(), 4, &picked));
    const GroundPickCamera horizon = { { 0, 1, 0 }, { 1, 1, 0 }, 50 };
    CHECK(!pickGroundTile(projection, horizon, viewport, 320, 240, &picked));
    const GroundPickCamera beyondMap = { { 59, 11, 21.11265 }, { 51, 1, 9.11265 }, 50 };
    CHECK(!pickGroundTile(projection, beyondMap, viewport, 320, 240, &picked));
    GroundPickProjection invalidProjection = projection;
    invalidProjection.squ[0] = std::numeric_limits<double>::infinity();
    CHECK(!pickGroundTile(invalidProjection, camera, viewport, 320, 240, &picked));

    // Following the game's view center moves eye and target together: the
    // angle and distance stay, and the screen center now picks that point.
    const GroundPickCamera base = staticPreviewCameraParameters(true);
    GroundPickCamera followed = {};
    CHECK(followGroundPoint(base, -20.25, 4.5, &followed));
    for (int axis = 0; axis < 3; axis++) {
        CHECK(std::abs((followed.eye[axis] - followed.target[axis]) - (base.eye[axis] - base.target[axis])) < 1e-12);
    }
    CHECK(followed.target[1] == base.target[1] && followed.fovY == base.fovY);
    CHECK(pickGroundTile(projection, followed, viewport, 320, 240, &picked));
    CHECK(std::abs(picked.worldX + 20.25) < 1e-6 && std::abs(picked.worldZ - 4.5) < 1e-6);
    const GroundPickCamera level = { { 0, 1, 0 }, { 1, 1, 0 }, 25 };
    CHECK(!followGroundPoint(level, 0, 0, &followed));
}
