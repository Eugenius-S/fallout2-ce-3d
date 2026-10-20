#include "sun.h"

#include <algorithm>
#include <cmath>

namespace fallout {
namespace scene {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTicksPerHour = 36000.0;
constexpr double kSunrise = 6.0;
constexpr double kSunset = 18.0;
constexpr double kNoonHeight = 60.0; // degrees
// Where the sun stands at noon, degrees in the GL ground plane; it rises 90
// degrees before and sets 90 degrees after. To be tuned on the shadows (045).
constexpr double kNoonAzimuth = 225.0;
// It fades in over its first degrees above the horizon.
constexpr double kHorizonFade = 8.0;
// The engine's lowest ambient (LIGHT_INTENSITY_MIN): no sun at that level.
constexpr double kNightAmbient = 0.25;
// Low sun is warm, high sun nearly white.
constexpr float kLowColor[3] = { 1.0f, 0.62f, 0.36f };
constexpr float kHighColor[3] = { 1.0f, 0.96f, 0.88f };
constexpr double kWarmUntil = 30.0; // degrees
// A step of game time longer than this between frames is a jump.
constexpr unsigned int kJumpTicks = 6000; // ten game minutes
constexpr double kFadeSeconds = 1.0;

double smoothstep(double edge0, double edge1, double x)
{
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

} // namespace

Sun sunAt(unsigned int gameTime, double ambient, bool daylight)
{
    Sun sun;
    const double hour = std::fmod(gameTime / kTicksPerHour, 24.0);
    const double day = (hour - kSunrise) / (kSunset - kSunrise); // 0 sunrise, 1 sunset
    sun.height = kNoonHeight * std::sin(kPi * day);
    sun.azimuth = kNoonAzimuth + (day - 0.5) * 180.0;
    const double height = sun.height * kPi / 180.0;
    const double azimuth = sun.azimuth * kPi / 180.0;
    sun.direction[0] = static_cast<float>(std::cos(height) * std::cos(azimuth));
    sun.direction[1] = static_cast<float>(std::sin(height));
    sun.direction[2] = static_cast<float>(std::cos(height) * std::sin(azimuth));

    const double warm = smoothstep(0.0, kWarmUntil, sun.height);
    for (int channel = 0; channel < 3; channel++) {
        sun.color[channel] = static_cast<float>(kLowColor[channel] + (kHighColor[channel] - kLowColor[channel]) * warm);
    }

    const double light = std::clamp((ambient - kNightAmbient) / (1.0 - kNightAmbient), 0.0, 1.0);
    const double strength = daylight && day > 0.0 && day < 1.0
        ? smoothstep(0.0, kHorizonFade, sun.height) * light
        : 0.0;
    sun.strength = static_cast<float>(strength);
    sun.up = sun.strength > 0.0f;
    return sun;
}

const Sun& SunTracker::update(unsigned int gameTime, double ambient, bool daylight, double seconds)
{
    const Sun target = sunAt(gameTime, ambient, daylight);
    const bool jumped = !started || daylight != lastDaylight || gameTime < lastTime
        || gameTime - lastTime > kJumpTicks;
    const float strength = jumped
        ? target.strength
        : static_cast<float>(current.strength
            + (target.strength - current.strength) * std::clamp(seconds / kFadeSeconds, 0.0, 1.0));
    current = target;
    current.strength = strength;
    current.up = strength > 0.0f;
    started = true;
    lastTime = gameTime;
    lastDaylight = daylight;
    return current;
}

} // namespace scene
} // namespace fallout
