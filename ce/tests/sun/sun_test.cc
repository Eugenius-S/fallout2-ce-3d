#include "sun.h"

#include <cmath>
#include <cstdio>

using fallout::scene::Sun;
using fallout::scene::SunTracker;
using fallout::scene::sunAt;

namespace {

constexpr unsigned int kHour = 36000;

bool check(bool condition, const char* what)
{
    if (!condition) std::printf("FAIL: %s\n", what);
    return condition;
}

} // namespace

int main()
{
    int failures = 0;
    const unsigned int day = 3 * 24 * kHour; // some day of the game

    const Sun dawn = sunAt(day + 6 * kHour, 1.0, true);
    failures += !check(!dawn.up && std::fabs(dawn.height) < 1e-6, "6:00: on the horizon, not shining yet");

    const Sun morning = sunAt(day + 7 * kHour, 1.0, true);
    failures += !check(morning.up && morning.height > 5.0 && morning.height < 20.0, "7:00: low");
    failures += !check(morning.color[2] < 0.8f, "7:00: warm");

    const Sun noon = sunAt(day + 12 * kHour, 1.0, true);
    failures += !check(noon.up && std::fabs(noon.height - 60.0) < 1e-6, "12:00: at its highest");
    failures += !check(std::fabs(noon.strength - 1.0f) < 1e-6f, "12:00 full ambient: full strength");
    failures += !check(noon.color[2] > 0.85f, "12:00: nearly white");
    const double length = std::sqrt(noon.direction[0] * noon.direction[0] + noon.direction[1] * noon.direction[1]
        + noon.direction[2] * noon.direction[2]);
    failures += !check(std::fabs(length - 1.0) < 1e-5, "the direction is a unit vector");

    const Sun evening = sunAt(day + 17 * kHour, 1.0, true);
    failures += !check(evening.up && evening.height < 20.0, "17:00: low again");
    failures += !check(std::fabs(evening.azimuth - morning.azimuth - 150.0) < 1e-6,
        "it crosses the sky: 7:00 and 17:00 are 150 degrees apart");

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
