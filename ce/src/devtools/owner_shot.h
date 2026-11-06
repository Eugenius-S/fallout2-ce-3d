#ifndef FALLOUT_DEVTOOLS_OWNER_SHOT_H
#define FALLOUT_DEVTOOLS_OWNER_SHOT_H

namespace fallout {
namespace owner_shot {

// The owner's feedback shot (task 183): one key in the game saves the frame
// as it is on screen with a JSON next to it, so an agent can read a batch of
// these shots with the owner's notes later. Lives outside the dev tools:
// the owner plays a build without --dev-tools.
//
// F9 takes the shot. With the dev tools on, F9 stays the dev snapshot (the
// tours and the panel keep their devshot format), so this handler runs after
// devtools::handleKey and only answers the keys left over.
bool handleKey(int key);

// The teleport (step 3): dev-teleport.txt next to the exe, one line
// "map.elev" (the safe pick) or "map.elev.hex" (the named hex), is consumed
// and the hero jumps, in the owner's running game. The dev panel's TP button
// drives the same engine.
bool requestTeleport(const char* mapName, int elevation, int tile);
void pumpTeleport();

// The frame pump: takes the pending --dev-owner-shot-once shot at the first
// present after the map loaded (how test runs capture acceptance shots).
void parseOption(const char* arg);
void pump();

} // namespace owner_shot
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_OWNER_SHOT_H */
