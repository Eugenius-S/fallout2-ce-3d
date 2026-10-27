#include "../../src/scene/muzzle_mask.h"
#include <cstdio>
using namespace fallout::scene;

int main()
{
    unsigned char palette[768] = {};
    palette[3] = palette[4] = palette[5] = 20;
    palette[6] = palette[7] = palette[8] = 63;
    palette[9] = 63; palette[10] = 55; palette[11] = 25;
    MuzzleImage aim;
    aim.width = 40; aim.height = 48; aim.anchorX = 10; aim.anchorY = 47;
    aim.indices.assign(40 * 48, 0);
    for (int y = 14; y < 47; ++y) for (int x = 8; x <= 12; ++x) aim.indices[y * 40 + x] = 1;
    for (int x = 10; x <= 23; ++x) aim.indices[22 * 40 + x] = 1;
    aim.indices[15 * 40 + 10] = 2; // persistent white eye/armor
    auto flash = aim;
    for (int y = 20; y <= 22; ++y) for (int x = 24; x <= 27; ++x) flash.indices[y * 40 + x] = 3;
    std::vector<MuzzleImage> frames(9, aim);
    for (int i : {1,3,5}) frames[i] = flash;
    const auto burst = detectMuzzleMasks(frames, aim, 1, 0, false, false, palette);
    for (int i = 0; i < 9; ++i) {
        const bool expected = i == 1 || i == 3 || i == 5;
        if ((burst[i].corePixels > 0) != expected) return 1;
        if (expected && (burst[i].x < 24.0 || burst[i].mask[15 * 40 + 10])) return 2;
    }
    const auto single = detectMuzzleMasks(frames, aim, 1, 1, true, false, palette);
    for (int i = 0; i < 9; ++i) if ((single[i].corePixels > 0) != (i == 1)) return 3;
    auto isolated = aim; isolated.indices[20 * 40 + 30] = 2;
    if (detectMuzzleMasks({isolated}, aim, 1, 0, false, false, palette)[0].corePixels != 0) return 4;
    auto malformed = aim; malformed.indices.clear();
    if (detectMuzzleMasks({malformed}, aim, 1, 0, false, false, palette)[0].corePixels != 0) return 5;
    std::puts("pixel-driven pulses, quiet glints, isolated pixels and malformed art checked");
    return 0;
}
