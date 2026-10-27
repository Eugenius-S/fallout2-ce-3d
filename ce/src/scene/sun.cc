#include "sun.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace fallout {
namespace scene {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTicksPerHour = 36000.0;
constexpr double kSunrise = 6.0;
constexpr double kSunset = 18.0;
// The sun stays HIGH and near-overhead (the owner's 082): straight up at
// noon, the swing degrees off vertical at the day's ends, linear between. No
// long shadows exist at all; a 2-unit wall casts at most 2 * tan(swing)
// units of shadow. The swing is the [3d] sun_swing setting (082 round 2,
// default 30, was a 15 constant).
constexpr double kNoonHeight = 90.0; // degrees, noon
// The vertical plane the sun swings in, as the game camera's screen
// horizontal: the GL ground direction (x, z) that moves a point across the
// screen and never up or down it. From the donor projection (kladwtwn's
// projection.bin: one ground u step is (-48, 12) game pixels, one v step
// (32, 24)), one screen pixel right is the ground step (-1/64, 1/128), the
// unit vector (-2, 1) / sqrt(5). Shadows fall screen-right of things in the
// morning (tilt < 0) and screen-left in the evening, never towards or away
// from the viewer.
constexpr double kSunAxis[2] = { -0.8944271909999159, 0.4472135954999579 };
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

unsigned int stepClockHour(unsigned int gameTime, int direction)
{
    constexpr unsigned int hour = 36000;
    if (direction > 0) return gameTime > std::numeric_limits<unsigned int>::max() - hour
        ? std::numeric_limits<unsigned int>::max() : gameTime + hour;
    if (direction < 0) return gameTime <= hour ? 1u : gameTime - hour;
    return gameTime;
}

Sun sunAt(unsigned int gameTime, double ambient, bool daylight, double swingDegrees)
{
    Sun sun;
    const double swing = std::clamp(swingDegrees, 0.0, 60.0);
    const double hour = std::fmod(gameTime / kTicksPerHour, 24.0);
    const double day = (hour - kSunrise) / (kSunset - kSunrise); // 0 sunrise, 1 sunset
    // One fixed vertical plane, the tilt linear through the day: -swing at
    // sunrise, 0 at noon, +swing at sunset (082). Outside the day the sun is
    // down and the tilt stays at the day's end.
    sun.tilt = std::clamp(-swing + 2.0 * swing * day, -swing, swing);
    const double tilt = sun.tilt * kPi / 180.0;
    const double sinTilt = std::sin(tilt);
    sun.height = kNoonHeight - std::fabs(sun.tilt);
    // Where the sun leans on the ground: along kSunAxis, to one side at
    // dawn, the other at dusk, nowhere at noon.
    sun.azimuth = sinTilt == 0.0
        ? 0.0
        : std::atan2(kSunAxis[1] * sinTilt, kSunAxis[0] * sinTilt) * 180.0 / kPi;
    sun.direction[0] = static_cast<float>(sinTilt * kSunAxis[0]);
    sun.direction[1] = static_cast<float>(std::cos(tilt));
    sun.direction[2] = static_cast<float>(sinTilt * kSunAxis[1]);

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

const Sun& SunTracker::update(unsigned int gameTime, double ambient, bool daylight, double seconds,
    double swingDegrees)
{
    const Sun target = sunAt(gameTime, ambient, daylight, swingDegrees);
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
