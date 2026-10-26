#ifndef FALLOUT_SCENE_EFFECT_EMISSION_H
#define FALLOUT_SCENE_EFFECT_EMISSION_H

#include <algorithm>

namespace fallout {
namespace scene {

// Native art identities, not arbitrary bright scenery. Alpha tags are internal
// material markers 240..244 carry the owner's per-kind calibration at the
// common default 2; 248..250 remain developer/legacy tags. All are self-lit.
struct EffectEmission {
    unsigned char alpha = 255;
    int distance = 0;
    float intensity = 0.0f;
    bool sign = false;
    bool flame = false;
    bool cycling = false;
};

inline bool gEffectEmissionEnabled = true; // developer comparison only

inline EffectEmission effectEmission(int fid)
{
    if (!gEffectEmissionEnabled) return {};
    const int type = (fid >> 24) & 15;
    const int art = fid & 0xFFF;
    if (type == 2 && (art == 1750 || art == 1751)) return { 249, 4, 0.8f, true, false }; // SIGN40/41
    if (type != 5) return {};
    switch (art) {
    case 5: return { 240, 3, 0.65f }; // laser: former shared 0.7
    case 6: return { 241, 3, 0.8f, false, false, true }; // plasma bolt: 1.5
    case 7: case 8: return { 240, 3, 0.8f }; // plasma pistol/rifle: 0.7
    case 9: return { 242, 3, 0.65f, false, true }; // rocket exhaust: 1
    case 14: return { 240, 3, 0.8f }; // electricity: corrected to 0.7
    case 26: return { 243, 4, 0.9f, false, true }; // flamethrower: 1
    case 2: case 10: case 29: case 30: case 31:
        return { 244, 5, 1.0f }; // all explosions: former shared 0.7
    default: return {};
    }
}

// 6-bit palette. Keep smoke, shadows, metal and dim fading frames unlit.
// The same mask locates/colors the point light and marks the sprite material.
inline bool effectHotPixel(const EffectEmission& effect, unsigned char index, const unsigned char* palette)
{
    if (effect.distance == 0 || index == 0 || palette == nullptr) return false;
    if (effect.cycling) return (index >= 229 && index <= 247) || index == 254;
    const int r = palette[index * 3], g = palette[index * 3 + 1], b = palette[index * 3 + 2];
    const int peak = std::max({ r, g, b });
    const int low = std::min({ r, g, b });
    if (effect.sign || effect.flame) return r >= 32 && r > b * 3 / 2 && r >= g;
    return (peak >= 28 && peak - low >= 12) || low >= 42;
}

} // namespace scene
} // namespace fallout
#endif
