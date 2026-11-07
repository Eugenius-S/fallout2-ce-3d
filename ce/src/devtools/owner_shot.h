#ifndef FALLOUT_DEVTOOLS_OWNER_SHOT_H
#define FALLOUT_DEVTOOLS_OWNER_SHOT_H

namespace fallout {
namespace owner_shot {

// The owner's feedback shot (task 183): one key in the game saves the frame
// as it is on screen with a JSON next to it, so an agent can read a batch of
// these shots with the owner's notes later. Lives outside the dev tools:
// the owner plays a build without --dev-tools.
//
// F9 takes the shot. Ctrl+F9 or Alt+F9 enters the region mark: the game
// freezes, the owner drags one or more rectangles over the defect, Enter or
// a second press saves the clean PNG, the marked PNG and the JSON with the
// regions, Esc cancels.
//
// With the dev tools on, F9 stays the dev snapshot (the tours and the panel
// keep their devshot format), so this handler runs after devtools::handleKey
// and only answers the keys left over.
bool handleKey(int key);

// The teleport (step 3): dev-teleport.txt next to the exe, one line
// "map.elev" (the safe pick) or "map.elev.hex" (the named hex), is consumed
// and the hero jumps, in the owner's running game. The dev panel's TP button
// drives the same engine.
bool requestTeleport(const char* mapName, int elevation, int tile);
void pumpTeleport();

// The rectangles over the game's own picture while the mark mode is on:
// drawn after the dev panel, before the scene presents.
void draw(unsigned char* surface, int width, int height, int pitch);

// The same rectangles as RGBA patches over the presented 3D scene (217):
// the sharp-text present path rebuilds the screen from captured text runs
// and drops raw indexed pixels, so the surface draw alone never reached the
// owner's screen. Call after the scene overlay, before the swap.
void drawSceneOverlay();

// Called from the present path right after drawSceneOverlay. While the drag
// bench (--dev-owner-mark-drag) runs, it reads the presented pixels back and
// proves the green outline is really in the frame - the 217 overlay was only
// ever verified through the saved files.
void verifySceneOverlayPresented();

// True while the mark mode holds the input: the main loop skips the game's
// update phases and only presents.
bool markMode();

// The frame pump: takes the pending --dev-owner-shot-once shot at the first
// present after the map loaded (how test runs capture acceptance shots).
void parseOption(const char* arg);
void pump();

} // namespace owner_shot
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_OWNER_SHOT_H */
