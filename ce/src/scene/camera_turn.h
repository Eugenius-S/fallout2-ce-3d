#ifndef FALLOUT_SCENE_CAMERA_TURN_H
#define FALLOUT_SCENE_CAMERA_TURN_H

#include <algorithm>
#include <cmath>

namespace fallout {
namespace scene {

// One press of [ or ] turns the camera this many degrees.
constexpr double kCameraTurnStep = 10.0;
// Without camera_rotation_unlimited the turn stops at this many degrees either way.
constexpr double kCameraTurnLimit = 30.0;

inline double limitedCameraTurn(double degrees)
{
    return std::clamp(std::remainder(degrees, 360.0), -kCameraTurnLimit, kCameraTurnLimit);
}

inline double cameraTurnTarget(double current, int steps, bool unlimited)
{
    // Clamp before wrapping: repeated input at an endpoint must stay there.
    const double target = current + kCameraTurnStep * steps;
    return unlimited ? target : std::clamp(target, -kCameraTurnLimit, kCameraTurnLimit);
}

// The game scrolls the map along the unturned screen axes. With the camera
// turned by `turnDegrees` (positive adds to the render yaw, see
// updateCameraTurn), the edge the mouse is at points elsewhere on the ground.
// The requested step (dx right, dy down, each -1..1, as the game's edge
// scroll gives it) is a screen direction of the turned camera. Its right/up
// components are rotated by the turn into the unturned frame, where the
// game's scroll lives, and snapped to the nearest of the game's eight
// directions. A zero turn or a zero step returns the step untouched.
inline void cameraTurnScrollStep(int dx, int dy, double turnDegrees, int* outDx, int* outDy)
{
    *outDx = dx;
    *outDy = dy;
    if (turnDegrees == 0.0 || (dx == 0 && dy == 0)) return;
    const double pi = 3.14159265358979323846;
    const double radians = turnDegrees * pi / 180.0;
    const double right = dx;
    const double up = -dy;
    // The turned camera's right and up axes lie at +T from the unturned ones
    // (yaw adds T; right = (forward.z, -forward.x)), so the requested
    // (right, up) turns by +T, counterclockwise with right as x and up as y.
    const double rotatedRight = right * std::cos(radians) - up * std::sin(radians);
    const double rotatedUp = right * std::sin(radians) + up * std::cos(radians);
    const int sector = static_cast<int>(std::lround(std::atan2(rotatedUp, rotatedRight) / (pi / 4.0)));
    static const int kRight[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int kUp[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    const int index = ((sector % 8) + 8) % 8;
    *outDx = kRight[index];
    *outDy = -kUp[index];
}

} // namespace scene
} // namespace fallout

#endif
