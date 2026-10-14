#pragma once

// Exact port of vendor/fallout2-first-person/viewer/geom.js (commit 0da1cd4).
// World: (u, v) = floor-square coordinates, z = up, 1 unit = one square side.
// All constants arrive from scene.json "proj" - nothing here repeats values.

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace f2native {

struct Proj {
    double squ[2] = {0, 0};
    double sqv[2] = {0, 0};
    double origin[2] = {0, 0};
    double hpx = 1.0;
    double roofPx = 96.0;
    double bbPx = 57.2;
    double rowTilt = 0.0;
    double stack = 0.0;

    double det = 0.0;
    double roofZ = 0.0;
    double groundPerPx = 0.0;

    // Builds from the parsed "proj" object text (squ, sqv, origin, hpx, roofPx,
    // bbPx, rowTilt, stack). Returns false if a field is missing.
    static bool fromJsonString(const std::string& text, Proj& out);

    // Ground offset (du, dv) of a screen offset between two floor points.
    void deltaToGround(double dx, double dy, double out[2]) const {
        out[0] = (sqv[1] * dx - sqv[0] * dy) / det;
        out[1] = (squ[0] * dy - squ[1] * dx) / det;
    }

    void pxToGround(double x, double y, double out[2]) const {
        deltaToGround(x - origin[0], y - origin[1], out);
    }

    // Screen pixel (x down) of a world floor point at height z.
    void groundToPx(double u, double v, double z, double out[2]) const {
        out[0] = origin[0] + u * squ[0] + v * sqv[0];
        out[1] = origin[1] + u * squ[1] + v * sqv[1] - hpx * z;
    }

    void groundDir(double dx, double dy, double out[2]) const {
        double uv[2];
        deltaToGround(dx, dy, uv);
        const double n = std::hypot(uv[0], uv[1]);
        out[0] = uv[0] / n;
        out[1] = uv[1] / n;
    }
};

// How a camera-facing sprite stands in the world (card() in geom.js, FOOT = 0.5).
// sprite = {page, x, y, w, h, ox, oy}; (ox, oy) = top-left pixel relative to the
// hex-centre anchor.
struct SpriteCard {
    double shift = 0, lean = 0, z0 = 0, z1 = 0, up0 = 0, up1 = 0;
};

constexpr double kCardFoot = 0.5;

SpriteCard card(const Proj& proj, int x, int y, int w, int h, int oy);

// ------------------------------------------------------------------ hex grid
constexpr int kHexW = 200;

// Continuous world pixel of a hex centre (hexPx in geom.js).
inline void hexPx(int tile, double out[2]) {
    const int hx = tile % kHexW, hy = tile / kHexW;
    const int a = kHexW - 1 - hx;
    out[0] = 48.0 * (a >> 1) + 32.0 * (a & 1) + 16.0 * hy + 16;
    out[1] = -12.0 * (a >> 1) + 12.0 * hy + 8;
}

// Tile whose hexagon contains world pixel (x, y), or -1 (hexFromPx).
int hexFromPx(double x, double y);

// Depth-sort v of an upright plane along u (rowPlane in geom.js).
double rowPlane(const Proj& proj, double u, double v, int stack = 0);

} // namespace f2native
