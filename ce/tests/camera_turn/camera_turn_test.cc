#include "camera_turn.h"

#include <cstdio>
#include <cmath>

using namespace fallout::scene;

namespace {

const double kDt = 1.0 / 144.0;

// Runs the eased turn to its target, one frame at a time. Fails on a backward
// move, a jump past the target or a turn that never lands.
double runToTarget(double& at, double& to, double& speed, double* peakSpeed,
    double* lastFrameSpeed, int* frames, double* largestSpeedDelta = nullptr)
{
    *peakSpeed = 0.0;
    *lastFrameSpeed = 0.0;
    *frames = 0;
    if (largestSpeedDelta != nullptr) *largestSpeedDelta = 0.0;
    double prev = at;
    double prevSpeed = speed;
    while (at != to && *frames < 100000) {
        const bool wasMovingTowardTarget = (to - prev) * prevSpeed >= -1e-12;
        const bool landed = cameraTurnEaseStep(at, to, speed, kDt);
        if (landed != (at == to)) {
            std::fprintf(stderr, "CHECK failed: landed flag disagrees with arrival\n");
            return 1.0;
        }
        if (wasMovingTowardTarget && (to >= prev ? at < prev - 1e-12 : at > prev + 1e-12)) {
            std::fprintf(stderr, "CHECK failed: the turn moved backward\n");
            return 1.0;
        }
        if ((to - at) * (to - prev) < -1e-12) {
            std::fprintf(stderr, "CHECK failed: the turn overshot the target\n");
            return 1.0;
        }
        prev = at;
        if (largestSpeedDelta != nullptr) {
            *largestSpeedDelta = std::max(*largestSpeedDelta, std::fabs(speed - prevSpeed));
        }
        prevSpeed = speed;
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
        // The longer zero-to-zero arc gives the turn room to ease in and out.
        const double seconds = frames * kDt;
        CHECK(seconds >= 0.20);
        CHECK(seconds <= 0.35);
        // Frame-to-frame angular speed stays bounded by the acceleration.
        double maxDelta = 0.0;
        at = 0.0; to = 10.0; speed = 0.0;
        CHECK(runToTarget(at, to, speed, &peak, &last, &frames, &maxDelta) == 0.0);
        CHECK(maxDelta <= kCameraTurnAccel * kDt * 1.15);
        CHECK(peak <= kCameraTurnMaxSpeed + 1e-9);
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
        int presses = 8;
        double maxSpeedDelta = 0.0;
        double previousSpeed = 0.0;
        for (int frame = 0; frame < 10000; frame++) {
            if (frame % 12 == 0 && presses > 0) {
                if (presses < 8) CHECK(speed > 0.0);
                to = cameraTurnTarget(to, 1, true); // a held key's repeat
                presses--;
            }
            cameraTurnEaseStep(at, to, speed, kDt);
            if (presses > 0 && speed + 1e-9 < previousSpeed) {
                std::fprintf(stderr, "held speed fell at frame %d: %.9f -> %.9f, target %.9f, angle %.9f\n",
                    frame, previousSpeed, speed, to, at);
                CHECK(false);
            }
            maxSpeedDelta = std::max(maxSpeedDelta, std::fabs(speed - previousSpeed));
            previousSpeed = speed;
            if (presses == 0 && at == to) break;
        }
        CHECK(presses == 0);
        CHECK(at == to && to == 80.0);
        CHECK(speed == 0.0);
        CHECK(maxSpeedDelta <= kCameraTurnAccel * kDt * 1.15);
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
