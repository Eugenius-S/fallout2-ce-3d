#ifndef FALLOUT_DEVTOOLS_TEST_RUN_STATE_H
#define FALLOUT_DEVTOOLS_TEST_RUN_STATE_H

// A test run with its own output folder (F2CE_DEV_OUT, dev_output.h) keeps
// the game's own state there: master_patches moves to DIR/data, and the
// configured patches folder is still read, minus the state another game
// left in it. That state is what a new game erases from master_patches:
// saved maps (MAPS\*.SAV), the automap (MAPS\AUTOMAP.DB), the world map
// (worldmap.dat) and saved protos (proto\critters\*.pro, proto\items\*.pro).
// Unset, every call here does nothing.

namespace fallout {
namespace devtools {

// Before the databases open: points master_patches at the run's folder.
void testRunStateRedirectPatches();

// Just before master_patches opens: mounts the configured patches folder
// below it, with the game state hidden.
void testRunStateMountSharedPatches();

// True when the directory xbase at [xbasePath] must not answer for
// [filePath] (relative to it; enumeration passes its pattern).
bool testRunStateHidesFile(const char* xbasePath, const char* filePath);

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_TEST_RUN_STATE_H */
