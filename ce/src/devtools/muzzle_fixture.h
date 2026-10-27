#ifndef FALLOUT_DEVTOOLS_MUZZLE_FIXTURE_H
#define FALLOUT_DEVTOOLS_MUZZLE_FIXTURE_H
#include <string>
namespace fallout { namespace devtools {
void configureMuzzleFixture(int weaponPid, int animation, int rotation);
bool startMuzzleFixture(int tile, int elevation);
bool focusMuzzleFixture();
bool advanceMuzzleFixture();
std::string muzzleFixtureMetadata();
} }
#endif
