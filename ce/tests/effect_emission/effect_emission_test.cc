#include "effect_emission.h"
#include <cstdio>

using namespace fallout::scene;
int main()
{
    unsigned char palette[768] = {};
    palette[3] = 45; // saturated red
    palette[6] = palette[7] = palette[8] = 20; // metal/smoke
    palette[9] = palette[10] = palette[11] = 60; // white core
    palette[12] = 60; palette[13] = 38; palette[14] = 5; // exhaust
    const auto laser = effectEmission(0x05000005);
    const auto rocket = effectEmission(0x05000009);
    const auto sign = effectEmission(0x020006D6);
    if (sign.alpha != 245 || laser.distance != 3 || !effectHotPixel(laser, 1, palette)
        || effectHotPixel(laser, 0, palette) || effectHotPixel(laser, 2, palette)
        || !effectHotPixel(laser, 3, palette) || !effectHotPixel(rocket, 4, palette)
        || effectHotPixel(rocket, 3, palette) || !effectHotPixel(sign, 1, palette)
        || effectHotPixel(sign, 3, palette)
        || effectEmission(0x02000005).distance || effectEmission(0x05000001).distance
        || effectEmission(0x01000005).distance
        || !effectEmission(0x05000006).cycling
        || effectEmission(0x05000006).alpha != 241
        || laser.alpha != 240 || rocket.alpha != 242
        || effectEmission(0x05000026).distance != 0
        || effectEmission(0x0500001A).alpha != 243
        || effectEmission(0x0500001D).alpha != 244) {
        std::fprintf(stderr, "effect material identity/core isolation failed\n");
        return 1;
    }
    gEffectEmissionEnabled = false;
    if (effectEmission(0x05000005).distance != 0) return 2;
    return 0;
}
