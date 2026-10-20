#ifndef FALLOUT_SCENE_SCENE_LIGHTS_H
#define FALLOUT_SCENE_SCENE_LIGHTS_H

namespace fallout {
namespace scene {

// The game's light into the scene, each frame: the ambient level and every
// light-carrying object on the current elevation as a point light. It shines
// from its sprite's shining pixels (the engine's animated colours: fire,
// slime, monitors, alarms; else its brightest pixels) in their colour; its
// reach is the engine's own spread, so walls stop it as in the game.
// `[3d] lights` off: the scene is unlit.
void updateLightsFromGame();

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_SCENE_LIGHTS_H */
