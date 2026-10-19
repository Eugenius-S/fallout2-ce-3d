#ifndef FALLOUT_SCENE_SCENE_VIEW_H
#define FALLOUT_SCENE_SCENE_VIEW_H

#include <string>

namespace fallout {
namespace scene {

// What the 3D scene shows in place of the game's world picture.
enum class View {
    Off,
    Floor, // the floor alone, from the fixed preview camera
    Walls, // floor and walls in the game's view
    Full, // the game in 3D: walls, live objects, the hero, the game's interface over them
};

// Whether a built scene exists for the current map and elevation (today only
// KLADWTWN elevation 0).
bool showsCurrentMap();

// Loads the current map's scene and shows it as `view`. On failure the view
// stays as it was and `message` says why.
bool enable(View view, std::string* message);
// Back to the game's own picture. False when the scene was off already.
bool disable();

// The view asked for, whatever map is loaded now.
View requestedView();
// The requested view while the scene shows the current map, else Off.
View activeView();

// True while the active view may replace the game's picture: not during
// dialogs, combat, movies and other modal states (it comes back after them).
bool shouldPresent();
// Why the scene is not presented now, or "available" / "not-requested".
const char* availability();

// Off for deterministic snapshots: the camera jumps to the view center
// instead of gliding there.
void setCameraGlide(bool glide);

struct RefreshResult {
    bool heroUpdated; // the hero card shows the hero's current frame
    double seconds; // time spent rebuilding live objects and the hero
};

// Brings the full view up to date right before it is drawn: the camera
// follows the game's view center, live objects and the hero take their
// current frames.
RefreshResult refreshForPresent();

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_SCENE_VIEW_H */
