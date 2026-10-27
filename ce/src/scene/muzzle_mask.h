#ifndef FALLOUT_SCENE_MUZZLE_MASK_H
#define FALLOUT_SCENE_MUZZLE_MASK_H

#include <array>
#include <vector>

namespace fallout { namespace scene {

struct MuzzleImage {
    int width = 0, height = 0;
    int anchorX = 0, anchorY = 0; // native anchor in image pixels, including frame deltas
    std::vector<unsigned char> indices;
};
struct MuzzleMask {
    int width = 0, height = 0;
    int corePixels = 0;
    double x = 0.0, y = 0.0;
    double rootX = 0.0, rootY = 0.0;
    std::array<int, 256> colors = {};
    std::vector<unsigned char> mask;
    std::vector<unsigned char> extraMask;
};

// Timing and coordinates come from the actual firing frames and the quiet
// aim pose. The action frame is the native FRM header cue, never a weapon table.
std::vector<MuzzleMask> detectMuzzleMasks(const std::vector<MuzzleImage>& frames,
    const MuzzleImage& aim, int rotation, int actionFrame, bool single, bool continuous,
    const unsigned char* palette);

} }
#endif
