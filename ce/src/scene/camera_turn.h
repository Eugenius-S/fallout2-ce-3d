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
// The old constant-speed turn: a 10-degree step took kCameraTurnStep over
// this, about 28 ms. The eased step keeps about that time.
constexpr double kCameraTurnSpeed = 360.0;
// The eased step peaks at twice the old constant speed, so its speed arc
// (zero at both ends) covers the step in about the old time.
constexpr double kCameraTurnMaxSpeed = 2.0 * kCameraTurnSpeed;
// And ramps at the rate that makes the arc exactly that: braking from the
// peak covers kCameraTurnStep of distance (v^2 = 2 * a * s).
constexpr double kCameraTurnAccel = kCameraTurnMaxSpeed * kCameraTurnMaxSpeed / kCameraTurnStep;

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

// One frame of the eased camera turn: moves the angle `at` toward the target
// `to` over `dt` seconds, easing the signed `speed` in from zero and back out
// to zero at the target. A longer turn cruises at kCameraTurnMaxSpeed between
// the ease in and the ease out. A press mid-turn retargets `to` while the
// speed carries over, so two quick presses are one smooth arc. Returns true
// when the camera stands on the target: `at == to`, `speed == 0`.
inline bool cameraTurnEaseStep(double& at, double& to, double& speed, double dt)
{
    if (dt <= 0.0) return at == to;
    const double dist = to - at;
    if (dist == 0.0) {
        speed = 0.0;
        return true;
    }
    const double dir = dist > 0.0 ? 1.0 : -1.0;
    // Ease in: gain speed toward the target. A press that reverses the
    // direction sheds the opposite speed through zero at the same rate,
    // so the turn bends around without a jump.
    double s = speed + dir * kCameraTurnAccel * dt;
    s = dir > 0.0 ? std::min(s, kCameraTurnMaxSpeed) : std::max(s, -kCameraTurnMaxSpeed);
    if (s * dir > 0.0) {
        // Ease out: never faster than the speed that lands on the target.
        const double stop = std::sqrt(2.0 * kCameraTurnAccel * std::fabs(dist));
        s = dir > 0.0 ? std::min(s, stop) : std::max(s, -stop);
    }
    const double move = s * dt;
    if (std::fabs(move) >= std::fabs(dist)) {
        at = to;
        speed = 0.0;
        return true;
    }
    speed = s;
    at += move;
    return false;
}

// The game scrolls the map along its own eight directions: a step (dx right,
// dy down, each -1..1) moves the view kScrollStepX * dx by kScrollStepY * dy
// screen pixels of the unturned game view (mapScroll). The two sizes differ
// and the game's ground projection is oblique, so the eight steps are not
// eight equal, 45-degree screen directions.
constexpr double kScrollStepX = 32.0;
constexpr double kScrollStepY = 24.0;
constexpr double kPi = 3.14159265358979323846;

// What the unturned game view does to the ground, and how the camera looks at
// it. horizontal and vertical are the ground (GL x, z) displacements of one
// kScrollStepX pixel step right and one kScrollStepY pixel step down at the
// view center; yaw and pitch (radians) are the unturned 3D camera's.
struct ScrollGroundBasis {
    double horizontal[2];
    double vertical[2];
    double yaw;
    double pitch;
};

// Where the camera turned by `turnDegrees` (positive adds to the render yaw,
// see updateCameraTurn) shows a ground displacement (GL x, z): (right, up) in
// screen units, equal on both axes. At the view center the perspective only
// scales both alike, so the direction needs no distance.
inline void groundToTurnedScreen(const ScrollGroundBasis& basis, double turnDegrees,
    double groundX, double groundZ, double* outRight, double* outUp)
{
    const double yaw = basis.yaw + turnDegrees * kPi / 180.0;
    *outRight = groundX * std::sin(yaw) - groundZ * std::cos(yaw);
    *outUp = -std::sin(basis.pitch) * (groundX * std::cos(yaw) + groundZ * std::sin(yaw));
}

// The angle between two screen directions, 0..pi.
inline double screenAngleBetween(double rightA, double upA, double rightB, double upB)
{
    return std::abs(std::remainder(std::atan2(upA, rightA) - std::atan2(upB, rightB), 2.0 * kPi));
}

// With the camera turned, the edge the mouse is at is a direction on the
// turned screen. Of the game's eight steps, the one that really moves the view
// closest to that direction on the turned screen: each step's ground
// displacement (basis) is projected through the turned camera and compared by
// angle with the edge. A zero turn or a zero step returns the step untouched.
inline void cameraTurnScrollStep(int dx, int dy, double turnDegrees, const ScrollGroundBasis& basis,
    int* outDx, int* outDy)
{
    *outDx = dx;
    *outDy = dy;
    if (turnDegrees == 0.0 || (dx == 0 && dy == 0)) return;
    const double wantRight = dx;
    const double wantUp = -dy;
    double best = 4.0 * kPi;
    for (int candidateDy = -1; candidateDy <= 1; ++candidateDy) {
        for (int candidateDx = -1; candidateDx <= 1; ++candidateDx) {
            if (candidateDx == 0 && candidateDy == 0) continue;
            const double groundX = candidateDx * basis.horizontal[0] + candidateDy * basis.vertical[0];
            const double groundZ = candidateDx * basis.horizontal[1] + candidateDy * basis.vertical[1];
            double right, up;
            groundToTurnedScreen(basis, turnDegrees, groundX, groundZ, &right, &up);
            if (right == 0.0 && up == 0.0) continue;
            const double angle = screenAngleBetween(right, up, wantRight, wantUp);
            if (angle < best - 1e-9) {
                best = angle;
                *outDx = candidateDx;
                *outDy = candidateDy;
            }
        }
    }
}

} // namespace scene
} // namespace fallout

#endif
