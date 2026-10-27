#ifndef FALLOUT_SCENE_SUN_H
#define FALLOUT_SCENE_SUN_H

namespace fallout {
namespace scene {

// The sun over a map under the sky, by game time (36000 ticks an hour). It
// shines from 6:00 to 18:00 in one fixed vertical plane (082): straight
// overhead at noon, `swingDegrees` off vertical at the day's ends (the [3d]
// sun_swing setting, kSunSwingDefault by default), linear between; its
// strength follows the ambient light the map scripts set, so the game's own
// day and night decide how bright it is.
struct Sun {
    bool up = false; // it shines (strength above 0)
    double tilt = 0.0; // degrees off vertical in its plane: - at dawn, + at dusk
    double height = 0.0; // degrees above the horizon (90 - |tilt|)
    double azimuth = 0.0; // degrees in the GL ground plane, from +x towards +z: where the sun leans
    float direction[3] = { 0.0f, 1.0f, 0.0f }; // towards the sun, GL (x, y up, z)
    float color[3] = { 1.0f, 1.0f, 1.0f };
    float strength = 0.0f; // 0 none, 1 full
};

// The sun swing the default arguments carry: the [3d] sun_swing setting's
// default (082 round 2).
constexpr double kSunSwingDefault = 30.0;

// Step the debug clock one hour without discarding its day or minute.
unsigned int stepClockHour(unsigned int gameTime, int direction);

// Where the sun is at `gameTime` with the map's ambient light (0 to 1, the
// engine's LIGHT_INTENSITY_MAX is 1) on a map under the sky (`daylight`).
Sun sunAt(unsigned int gameTime, double ambient, bool daylight,
    double swingDegrees = kSunSwingDefault);

// The sun of the frame: it follows game time; a change of the ambient light
// fades its strength over about a second, and a jump of game time (waiting,
// travel, a load) or a new map sets it at once.
class SunTracker {
public:
    const Sun& update(unsigned int gameTime, double ambient, bool daylight, double seconds,
        double swingDegrees = kSunSwingDefault);
    const Sun& sun() const { return current; }
    void reset() { started = false; }

private:
    bool started = false;
    unsigned int lastTime = 0;
    bool lastDaylight = false;
    Sun current;
};

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_SUN_H */
