#ifndef FALLOUT_LIGHTING_QUAD_BLIT_H
#define FALLOUT_LIGHTING_QUAD_BLIT_H

#include "../color.h"
#include "../geometry.h"

namespace fallout {
namespace lighting {

// Projected corners of a full source image:
// index 0 = top-left, 1 = top-right, 2 = bottom-right, 3 = bottom-left.
// `depth` is the camera-space depth used for perspective-correct
// interpolation of source coordinates.
struct Quad {
    float x[4];
    float y[4];
    float depth[4];
};

// Perspective-correct blit of `src` (srcWidth x srcHeight, row stride
// srcPitch) into `dest` (stride destPitch), clipped to `clipRect` (window
// coordinates; both buffers share their origin). Palette index 0 is
// transparent.
//
// Shading follows the vanilla conventions:
// - `intensity` uses the _dark_trans_buf_to_buf scale (0x10000 = full
//   brightness), dimming through intensityColorTable;
// - when `intensityMap` is not null it is sampled per source pixel instead
//   (stride `intensityMapStride`, values shifted right by 9), matching the
//   vanilla floor renderer's per-pixel light map;
// - when `blendTable`/`colorTable` are not null the translucent path
//   (_dark_translucent_trans_buf_to_buf) is used.
void quadBlit(const unsigned char* src,
    int srcWidth,
    int srcHeight,
    int srcPitch,
    const Quad& quad,
    const Rect* clipRect,
    unsigned char* dest,
    int destPitch,
    int intensity,
    const int* intensityMap = nullptr,
    int intensityMapStride = 0,
    Color* blendTable = nullptr,
    Color* colorTable = nullptr);

} // namespace lighting
} // namespace fallout

#endif /* FALLOUT_LIGHTING_QUAD_BLIT_H */
