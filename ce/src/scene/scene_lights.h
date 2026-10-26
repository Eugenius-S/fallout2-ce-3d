#ifndef FALLOUT_SCENE_SCENE_LIGHTS_H
#define FALLOUT_SCENE_SCENE_LIGHTS_H

#include "sun.h"

namespace fallout {
struct Object;
namespace scene {

// The game's light into the scene, each frame: the ambient level and every
// light-carrying object on the current elevation as a point light. It shines
// from its sprite's shining pixels (the engine's animated colours: fire,
// slime, monitors, alarms; else its brightest pixels) in their colour; its
// reach is the engine's own spread, so walls stop it as in the game.
// `[3d] lights` off: the scene is unlit.
void updateLightsFromGame();
// Developer lifetime check against the lights actually submitted this frame.
bool effectLightMatches(const Object* object, int tile, int x, int y);

// Whether the loaded map elevation is under the sky (scene.json `daylight`).
void setSceneDaylight(bool daylight);
bool sceneDaylight();
// The sun of this frame from game time and the ambient level; `[3d] sun`
// off, indoors and at night: none.
void updateSunFromGame();
const Sun& currentSun();

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_SCENE_LIGHTS_H */
