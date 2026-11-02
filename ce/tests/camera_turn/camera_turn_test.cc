#include "camera_turn.h"

#include <cstdio>
#include <cmath>

using namespace fallout::scene;

namespace {

const double kDt = 1.0 / 144.0;

// Runs the eased turn to its target, one frame at a time. Fails on a backward
// move, a jump past the target or a turn that never lands.
double runToTarget(double& at, double& to, double& speed, double* peakSpeed,
    double* lastFrameSpeed, int* frames)
{
    *peakSpeed = 0.0;
    *lastFrameSpeed = 0.0;
    *frames = 0;
    double prev = at;
    while (at != to && *frames < 100000) {
        const bool landed = cameraTurnEaseStep(at, to, speed, kDt);
        if (landed != (at == to)) {
            std::fprintf(stderr, "CHECK failed: landed flag disagrees with arrival\n");
            return 1.0;
        }
        if (to >= prev ? at < prev - 1e-12 : at > prev + 1e-12) {
            std::fprintf(stderr, "CHECK failed: the turn moved backward\n");
            return 1.0;
        }
        if ((to - at) * (to - prev) < -1e-12) {
            std::fprintf(stderr, "CHECK failed: the turn overshot the target\n");
            return 1.0;
        }
        prev = at;
        *lastFrameSpeed = speed;
        *peakSpeed = std::max(*peakSpeed, std::fabs(speed));
        (*frames)++;
    }
    return 0.0;
}

} // namespace

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

    // The step target and the limit keep their old math.
    CHECK(cameraTurnTarget(0.0, 1, false) == 10.0);
    CHECK(cameraTurnTarget(25.0, 1, false) == 30.0);
    CHECK(cameraTurnTarget(30.0, 1, false) == 30.0);
    CHECK(cameraTurnTarget(350.0, 1, true) == 360.0);

    // A single step starts and ends at zero speed and lands exactly.
    {
        double at = 0.0, to = 10.0, speed = 0.0;
        double peak = 0.0, last = 0.0;
        int frames = 0;
        CHECK(runToTarget(at, to, speed, &peak, &last, &frames) == 0.0);
        CHECK(at == to);
        CHECK(speed == 0.0);
        // Ease in: the first frame is well below the peak. Ease out: so is
        // the last one.
        CHECK(peak > 0.0);
        double firstAt = 0.0, firstTo = 10.0, firstSpeed = 0.0;
        cameraTurnEaseStep(firstAt, firstTo, firstSpeed, kDt);
        CHECK(std::fabs(firstSpeed) < 0.75 * peak);
        CHECK(last < 0.75 * peak);
        // About the old time per step (10 degrees at kCameraTurnSpeed).
        const double oldSeconds = kCameraTurnStep / kCameraTurnSpeed;
        const double seconds = frames * kDt;
        CHECK(seconds > 0.5 * oldSeconds);
        CHECK(seconds < 1.5 * oldSeconds);
    }

    // A press mid-turn retargets without a speed jump: the next frame's speed
    // differs by at most one frame of acceleration.
    {
        double at = 0.0, to = 10.0, speed = 0.0;
        cameraTurnEaseStep(at, to, speed, kDt);
        cameraTurnEaseStep(at, to, speed, kDt);
        const double before = speed;
        to = 20.0; // the second press
        cameraTurnEaseStep(at, to, speed, kDt);
        CHECK(std::fabs(speed - before) <= kCameraTurnAccel * kDt + 1e-9);
        // And the double press is one arc: monotone, no overshoot, exact stop.
        double peak = 0.0, last = 0.0;
        int frames = 0;
        CHECK(runToTarget(at, to, speed, &peak, &last, &frames) == 0.0);
        CHECK(at == 20.0);
        CHECK(speed == 0.0);
    }

    // A press that reverses the direction sheds the old speed through zero
    // instead of flipping it in one frame.
    {
        double at = 0.0, to = 10.0, speed = 0.0;
        cameraTurnEaseStep(at, to, speed, kDt);
        cameraTurnEaseStep(at, to, speed, kDt);
        const double before = speed;
        CHECK(before > 0.0);
        to = 0.0; // the turn back
        cameraTurnEaseStep(at, to, speed, kDt);
        CHECK(speed < before); // losing speed toward the new direction
        CHECK(speed > before - kCameraTurnAccel * kDt - 1e-9); // bounded, not flipped
        double peak = 0.0, last = 0.0;
        int frames = 0;
        CHECK(runToTarget(at, to, speed, &peak, &last, &frames) == 0.0);
        CHECK(at == 0.0);
        CHECK(speed == 0.0);
    }

    // Held keys keep extending the target; the turn still lands exactly.
    {
        double at = 0.0, to = 0.0, speed = 0.0;
        int presses = 4;
        for (int frame = 0; frame < 10000; frame++) {
            if (frame % 20 == 0 && presses > 0) {
                to = cameraTurnTarget(to, 1, true); // a held key's repeat
                presses--;
            }
            cameraTurnEaseStep(at, to, speed, kDt);
            if (presses == 0 && at == to) break;
        }
        CHECK(presses == 0);
        CHECK(at == to && to == 40.0);
        CHECK(speed == 0.0);
    }

    // A zero step and a zero first frame change nothing.
    {
        double at = 4.0, to = 10.0, speed = 300.0;
        CHECK(!cameraTurnEaseStep(at, to, speed, 0.0));
        CHECK(at == 4.0 && speed == 300.0);
        double peak = 0.0, last = 0.0;
        int frames = 0;
        CHECK(runToTarget(at, to, speed, &peak, &last, &frames) == 0.0);
        CHECK(at == 10.0 && speed == 0.0);
    }
}
