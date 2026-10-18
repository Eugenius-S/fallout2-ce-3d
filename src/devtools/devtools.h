#ifndef FALLOUT_DEVTOOLS_H
#define FALLOUT_DEVTOOLS_H

#include <SDL.h>

namespace fallout {
namespace devtools {

// Developer map-explorer mode (sprite-lighting mod, spec edition 2, P1).
//
// Enabled with `--dev-tools`. In that mode:
// - `--dev-map=<name>` / `--dev-elevation=<0..2>` request a map to travel to
//   right after the startup save is loaded (via the engine's own safe map
//   transition);
// - `--dev-list-maps` prints the maps available in the game resources and
//   exits;
// - Alt+D toggles an in-game panel (map filter/list, elevation choice,
//   prev/next map, go, snapshot). The current map/elevation and the active
//   renderer are displayed. A snapshot saves the PNG plus a sidecar with
//   metadata (map, elevation, renderer, light, resolutions, CE commit,
//   SHA-256 of the running EXE).

// Consumes --dev-tools / --dev-map / --dev-elevation / --dev-list-maps.
// Returns true when the argument belonged to devtools.
bool parseOption(const char* arg);

bool isEnabled();
bool wantsListMaps();

// Prints the maps available in game resources (stdout). Requires the game
// (db) to be initialized.
void listMaps();

// Applies the --dev-map/--dev-elevation target once a game is running.
// Called by main.cc right after the startup save is loaded.
void applyStartupTarget();

// Per-frame tick (called from the presentation hook): applies pending
// elevation corrections after map loads settle.
void update();

// Requests a map change through the engine's own safe transition
// (mapSetTransition); the load itself happens in the game loop. Refused
// during combat/dialog/movies.
bool requestMapChange(const char* mapFileName, int elevation);

bool panelOpen();
bool handleKey(int key);
bool handleClick(int mouseX, int mouseY);

// Draws the panel into the 8-bit framebuffer (before palette expansion).
void draw(unsigned char* surface, int width, int height, int pitch);

// Saves a PNG of the current frame plus a JSON sidecar with the metadata
// of the actual state.
void takeSnapshot();

// Metadata helpers.
void currentMapName(char* buffer, size_t size);
const char* activeRendererName();

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_H */
