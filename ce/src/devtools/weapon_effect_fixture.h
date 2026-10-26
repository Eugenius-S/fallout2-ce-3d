#ifndef FALLOUT_DEVTOOLS_WEAPON_EFFECT_FIXTURE_H
#define FALLOUT_DEVTOOLS_WEAPON_EFFECT_FIXTURE_H

#include <string>

namespace fallout {
namespace devtools {

bool configureWeaponEffectFixture(const char* name);
void setWeaponEffectFrame(int frame);
void requestWeaponEffectLightCheck();
bool startWeaponEffectFixture(int tile, int elevation);
bool focusWeaponEffectFixture();
std::string weaponEffectMetadata();

} // namespace devtools
} // namespace fallout

#endif
