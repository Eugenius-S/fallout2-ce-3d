#ifndef FALLOUT_SCENE_MUZZLE_FLASH_H
#define FALLOUT_SCENE_MUZZLE_FLASH_H
#include "muzzle_mask.h"
namespace fallout { struct Object; namespace scene {
struct MuzzleMaterial {
    bool allowNative = true;
    bool extra = false;
    int weaponPid = 0;
    float color[3] = {1.0f, 1.0f, 1.0f};
};
bool muzzleFlashesEnabled();
void setMuzzleFlashPreview(bool enabled);
void setEnergyMuzzlePreview(bool enabled);
MuzzleMaterial muzzleMaterial(Object* object);
void tintMuzzlePixel(unsigned char* rgba, const MuzzleMaterial& material);
const MuzzleMask* muzzleFlashFor(int fid, int frame, int rotation, bool respectSwitch = true);
} }
#endif
