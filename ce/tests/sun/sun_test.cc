#include "sun.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>

using fallout::scene::Sun;
using fallout::scene::SunTracker;
using fallout::scene::sunAt;

namespace {

constexpr unsigned int kHour = 36000;
// The sun's plane (082): it holds the game camera's screen horizontal, the
// ground unit vector (-2, 1) / sqrt(5) in GL (x, z). A normal of that plane.
constexpr double kPlaneNormal[2] = { -0.4472135954999579, -0.8944271909999159 };

bool check(bool condition, const char* what)
{
    if (!condition) std::printf("FAIL: %s\n", what);
    return condition;
}

bool close(double a, double b, double eps) { return std::fabs(a - b) < eps; }

} // namespace

int main()
{
    int failures = 0;
    const unsigned int day = 3 * 24 * kHour; // some day of the game

    // 082: one vertical plane, the tilt linear through the day: -15 degrees
    // at 6:00, 0 at noon, +15 at 18:00.
    const Sun dawn = sunAt(day + 6 * kHour, 1.0, true);
    failures += !check(!dawn.up, "6:00: not shining yet");
    failures += !check(close(dawn.tilt, -15.0, 1e-9), "6:00: tilted -15 degrees");
    failures += !check(close(dawn.height, 75.0, 1e-9), "6:00: 75 degrees above the horizon");

    const Sun noon = sunAt(day + 12 * kHour, 1.0, true);
    failures += !check(noon.up && close(noon.tilt, 0.0, 1e-9), "12:00: straight overhead");
    failures += !check(close(noon.direction[0], 0.0, 1e-6) && close(noon.direction[1], 1.0, 1e-6)
            && close(noon.direction[2], 0.0, 1e-6),
        "12:00: the direction is straight up");
    failures += !check(std::fabs(noon.strength - 1.0f) < 1e-6f, "12:00 full ambient: full strength");
    failures += !check(noon.color[2] > 0.85f, "12:00: nearly white, the sun stays high");

    const Sun dusk = sunAt(day + 18 * kHour, 1.0, true);
    failures += !check(!dusk.up, "18:00: the day's end, no light");
    failures += !check(close(dusk.tilt, 15.0, 1e-9), "18:00: tilted +15 degrees");

    // Linear between, in one plane, always a unit vector.
    const Sun morning = sunAt(day + 7 * kHour, 1.0, true);
    const Sun evening = sunAt(day + 17 * kHour, 1.0, true);
    failures += !check(morning.up && close(morning.tilt, -12.5, 1e-9), "7:00: linear tilt of -12.5 degrees");
    failures += !check(evening.up && close(evening.tilt, 12.5, 1e-9), "17:00: linear tilt of +12.5 degrees");
    failures += !check(close(morning.height, 77.5, 1e-9) && morning.height > 70.0 && morning.height < 90.0,
        "7:00: low and high at once");
    failures += !check(morning.color[2] > 0.85f, "7:00: nearly white, the sun stays high");
    for (const Sun* sun : { &dawn, &morning, &noon, &evening, &dusk }) {
        const double length = std::sqrt(sun->direction[0] * sun->direction[0]
            + sun->direction[1] * sun->direction[1] + sun->direction[2] * sun->direction[2]);
        failures += !check(std::fabs(length - 1.0) < 1e-5, "the direction is a unit vector");
        const double offPlane = sun->direction[0] * kPlaneNormal[0] + sun->direction[2] * kPlaneNormal[1];
        failures += !check(std::fabs(offPlane) < 1e-5, "the direction stays in the sun's one plane");
    }
    failures += !check(morning.direction[0] > 0.0f && morning.direction[2] < 0.0f,
        "7:00: the sun leans to the other screen side than at 17:00");
    failures += !check(close(std::fabs(morning.azimuth - evening.azimuth), 180.0, 1e-5),
        "dawn and dusk lean to opposite sides of the plane");

    failures += !check(!sunAt(day + 19 * kHour, 1.0, true).up, "19:00: set");
    failures += !check(!sunAt(day, 1.0, true).up, "0:00: none");
    failures += !check(!sunAt(day + 12 * kHour, 1.0, false).up, "underground: none");
    failures += !check(!sunAt(day + 12 * kHour, 0.25, true).up, "the night ambient: none");
    const Sun dim = sunAt(day + 12 * kHour, 0.625, true);
    failures += !check(dim.up && std::fabs(dim.strength - 0.5f) < 1e-5f, "half the ambient over night: half strength");

    // The ambient changes: the strength fades; time jumps: it is set at once.
    SunTracker tracker;
    tracker.update(day + 12 * kHour, 1.0, true, 0.016);
    failures += !check(std::fabs(tracker.sun().strength - 1.0f) < 1e-6f, "the first frame is set at once");
    tracker.update(day + 12 * kHour + 10, 0.625, true, 0.25);
    failures += !check(tracker.sun().strength > 0.6f && tracker.sun().strength < 0.95f, "an ambient change fades");
    tracker.update(day + 12 * kHour + 20, 0.625, true, 2.0);
    failures += !check(std::fabs(tracker.sun().strength - 0.5f) < 1e-5f, "and arrives");
    tracker.update(day + 12 * kHour + 30, 1.0, true, 0.016);
    tracker.update(day + 20 * kHour, 1.0, true, 0.016);
    failures += !check(!tracker.sun().up, "waiting until 20:00 sets it at once");
    tracker.update(day + 12 * kHour, 1.0, true, 0.016);
    failures += !check(std::fabs(tracker.sun().strength - 1.0f) < 1e-6f, "a load back to noon sets it at once");

    if (failures == 0) std::printf("sun_tests: all passed\n");
    return failures == 0 ? 0 : 1;
}
