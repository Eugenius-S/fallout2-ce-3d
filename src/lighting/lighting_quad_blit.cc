#include "lighting_quad_blit.h"

#include <algorithm>
#include <cmath>

namespace fallout {
namespace lighting {

namespace {

inline float edgeFunction(float ax, float ay, float bx, float by, float px, float py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

// Rasterizes one triangle of the quad. `u`, `v`, `depth` are per-corner
// source coordinates and camera depths. Pixel centers are sampled; the
// barycentric weights come from the edge functions.
void rasterTriangle(const Quad& quad,
    int i0,
    int i1,
    int i2,
    float u0,
    float v0,
    float u1,
    float v1,
    float u2,
    float v2,
    const unsigned char* src,
    int srcWidth,
    int srcHeight,
    int srcPitch,
    int minX,
    int minY,
    int maxX,
    int maxY,
    unsigned char* dest,
    int destPitch,
    int intensityIndex,
    const int* intensityMap,
    int intensityMapStride,
    Color* blendTable,
    Color* colorTable)
{
    float area = edgeFunction(quad.x[i0], quad.y[i0], quad.x[i1], quad.y[i1], quad.x[i2], quad.y[i2]);
    if (std::fabs(area) < 0.00001f) {
        return;
    }

    const int x0 = std::max(minX, static_cast<int>(std::floor(std::min({ quad.x[i0], quad.x[i1], quad.x[i2] }))));
    const int x1 = std::min(maxX, static_cast<int>(std::ceil(std::max({ quad.x[i0], quad.x[i1], quad.x[i2] }))));
    const int y0 = std::max(minY, static_cast<int>(std::floor(std::min({ quad.y[i0], quad.y[i1], quad.y[i2] }))));
    const int y1 = std::min(maxY, static_cast<int>(std::ceil(std::max({ quad.y[i0], quad.y[i1], quad.y[i2] }))));
    if (x0 > x1 || y0 > y1) {
        return;
    }

    // Perspective-correct attribute plane: u/w, v/w and 1/w are affine in
    // screen space.
    const float iw0 = 1.0f / quad.depth[i0];
    const float iw1 = 1.0f / quad.depth[i1];
    const float iw2 = 1.0f / quad.depth[i2];
    const float a0 = u0 * iw0;
    const float a1 = u1 * iw1;
    const float a2 = u2 * iw2;
    const float b0 = v0 * iw0;
    const float b1 = v1 * iw1;
    const float b2 = v2 * iw2;

    for (int y = y0; y <= y1; y++) {
        const float py = y + 0.5f;
        unsigned char* destRow = dest + destPitch * y;
        for (int x = x0; x <= x1; x++) {
            const float px = x + 0.5f;
            const float w0 = edgeFunction(quad.x[i1], quad.y[i1], quad.x[i2], quad.y[i2], px, py);
            const float w1 = edgeFunction(quad.x[i2], quad.y[i2], quad.x[i0], quad.y[i0], px, py);
            const float w2 = edgeFunction(quad.x[i0], quad.y[i0], quad.x[i1], quad.y[i1], px, py);
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) {
                continue;
            }

            const float l0 = w0 / area;
            const float l1 = w1 / area;
            const float l2 = w2 / area;

            const float invDepth = l0 * iw0 + l1 * iw1 + l2 * iw2;
            if (invDepth <= 0.0f) {
                continue;
            }
            const float depth = 1.0f / invDepth;
            int su = static_cast<int>((l0 * a0 + l1 * a1 + l2 * a2) * depth);
            int sv = static_cast<int>((l0 * b0 + l1 * b1 + l2 * b2) * depth);
            if (su < 0) su = 0;
            if (su >= srcWidth) su = srcWidth - 1;
            if (sv < 0) sv = 0;
            if (sv >= srcHeight) sv = srcHeight - 1;

            const unsigned char srcByte = src[srcPitch * sv + su];
            if (srcByte == 0) {
                continue;
            }

            unsigned char& destByte = destRow[x];
            if (blendTable != nullptr) {
                unsigned int index = colorTable[srcByte] << 8;
                index = blendTable[index + destByte];
                destByte = intensityColorTable[index][intensityIndex];
            } else {
                int ii = intensityIndex;
                if (intensityMap != nullptr) {
                    ii = intensityMap[intensityMapStride * sv + su] >> 9;
                }
                destByte = srcByte < 0xE5 ? intensityColorTable[srcByte][ii] : srcByte;
            }
        }
    }
}

} // namespace

void quadBlit(const unsigned char* src,
    int srcWidth,
    int srcHeight,
    int srcPitch,
    const Quad& quad,
    const Rect* clipRect,
    unsigned char* dest,
    int destPitch,
    int intensity,
    const int* intensityMap,
    int intensityMapStride,
    Color* blendTable,
    Color* colorTable)
{
    int minX = 0;
    int minY = 0;
    int maxX = clipRect->right;
    int maxY = clipRect->bottom;
    if (clipRect->left > minX) minX = clipRect->left;
    if (clipRect->top > minY) minY = clipRect->top;

    float fx0 = quad.x[0];
    float fx1 = quad.x[0];
    float fy0 = quad.y[0];
    float fy1 = quad.y[0];
    for (int i = 1; i < 4; i++) {
        fx0 = std::min(fx0, quad.x[i]);
        fx1 = std::max(fx1, quad.x[i]);
        fy0 = std::min(fy0, quad.y[i]);
        fy1 = std::max(fy1, quad.y[i]);
    }

    if (fx1 < minX || fx0 > maxX || fy1 < minY || fy0 > maxY) {
        return;
    }
    if (fx0 < -4096.0f || fx1 > 8192.0f || fy0 < -4096.0f || fy1 > 8192.0f) {
        return; // degenerate projection (behind the camera) - skip
    }

    const int intensityIndex = intensity / 512;

    // Two triangles sharing the TL-BR diagonal. Source uv corners:
    // 0 = (0,0), 1 = (W,0), 2 = (W,H), 3 = (0,H).
    rasterTriangle(quad, 0, 1, 2, 0.0f, 0.0f,
        static_cast<float>(srcWidth), 0.0f,
        static_cast<float>(srcWidth), static_cast<float>(srcHeight),
        src, srcWidth, srcHeight, srcPitch,
        minX, minY, maxX, maxY,
        dest, destPitch, intensityIndex,
        intensityMap, intensityMapStride, blendTable, colorTable);
    rasterTriangle(quad, 0, 2, 3, 0.0f, 0.0f,
        static_cast<float>(srcWidth), static_cast<float>(srcHeight),
        0.0f, static_cast<float>(srcHeight),
        src, srcWidth, srcHeight, srcPitch,
        minX, minY, maxX, maxY,
        dest, destPitch, intensityIndex,
        intensityMap, intensityMapStride, blendTable, colorTable);
}

} // namespace lighting
} // namespace fallout
