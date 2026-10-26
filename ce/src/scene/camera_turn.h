#ifndef FALLOUT_SCENE_CAMERA_TURN_H
#define FALLOUT_SCENE_CAMERA_TURN_H

#include <algorithm>
#include <cmath>

namespace fallout {
namespace scene {

inline double limitedCameraTurn(double degrees)
{
    return std::clamp(std::remainder(degrees, 360.0), -45.0, 45.0);
}

inline double cameraTurnTarget(double current, int steps, bool unlimited)
{
    // Clamp before wrapping: repeated input at an endpoint must stay there.
    const double target = current + 45.0 * steps;
    return unlimited ? target : std::clamp(target, -45.0, 45.0);
}

} // namespace scene
} // namespace fallout

#endif
