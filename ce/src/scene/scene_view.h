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

// Whether the scene of the current map and elevation is loaded (the exporter
// builds one per map elevation into mod-cache/scene/<map>/<elevation>).
bool showsCurrentMap();

// Asks for `view` on every map from now on and loads the current map's scene.
// False when the current map cannot show it: `message` says why, the game
// shows it in 2D, and the view still comes on for maps that have a scene.
bool enable(View view, std::string* message);
// Back to the game's own picture. False when the scene was off already.
bool disable();

// Follows the game to another map or elevation: the old scene is dropped and
// the new one loaded. A map without a scene, or with a broken one, stays 2D
// with one log line. Called before each presented frame.
void syncToMap();

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
