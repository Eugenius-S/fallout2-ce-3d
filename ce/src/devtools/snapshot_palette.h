#ifndef FALLOUT_DEVTOOLS_SNAPSHOT_PALETTE_H
#define FALLOUT_DEVTOOLS_SNAPSHOT_PALETTE_H

#include <array>

namespace fallout {
namespace devtools {

// Rest phase of cycle.cc's ranges 229..254, matching the exporter's pal.py.
// color.pal itself leaves these entries unmapped; _cmap therefore has zeros.
inline std::array<unsigned char, 26 * 3> snapshotCyclePalette()
{
    std::array<unsigned char, 26 * 3> colors = {
        0,108,0, 11,115,7, 27,123,15, 43,131,27,
        107,107,111, 99,103,127, 87,107,143, 0,147,163, 107,187,255,
        255,0,0, 215,0,0, 147,43,11, 255,119,0, 255,59,0,
        71,0,0, 123,0,0, 179,0,0, 123,0,0, 71,0,0,
        83,63,43, 75,59,43, 67,55,39, 63,51,39, 55,47,35, 51,43,35,
        240,0,0,
    };
    for (auto& channel : colors) channel >>= 2;
    return colors;
}

} // namespace devtools
} // namespace fallout

#endif
