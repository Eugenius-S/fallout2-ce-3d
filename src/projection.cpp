#include "projection.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace f2native {

bool Proj::fromJsonString(const std::string& text, Proj& out) {
    try {
        const auto j = nlohmann::json::parse(text);
        if (!j.is_object()) return false;
        if (!j.contains("squ") || !j.contains("sqv") || !j.contains("origin") ||
            !j.contains("hpx") || !j.contains("roofPx") || !j.contains("bbPx")) {
            return false;
        }
        out.squ[0] = j.at("squ")[0].get<double>();
        out.squ[1] = j.at("squ")[1].get<double>();
        out.sqv[0] = j.at("sqv")[0].get<double>();
        out.sqv[1] = j.at("sqv")[1].get<double>();
        out.origin[0] = j.at("origin")[0].get<double>();
        out.origin[1] = j.at("origin")[1].get<double>();
        out.hpx = j.at("hpx").get<double>();
        out.roofPx = j.at("roofPx").get<double>();
        out.bbPx = j.at("bbPx").get<double>();
        out.rowTilt = j.value("rowTilt", 0.0);
        out.stack = j.value("stack", 0.0);
        out.det = out.squ[0] * out.sqv[1] - out.sqv[0] * out.squ[1];
        out.roofZ = out.roofPx / out.hpx;
        double d[2];
        out.deltaToGround(0, 1, d);
        out.groundPerPx = std::hypot(d[0], d[1]);
        return out.det != 0.0 && out.hpx != 0.0;
    } catch (...) {
        return false;
    }
}

SpriteCard card(const Proj& proj, int x, int y, int w, int h, int oy) {
    (void)x;
    (void)y;
    (void)w;
    const double top = oy, bottom = top + h;
    SpriteCard c;
    if (bottom <= 0) {
        c.z0 = -bottom / proj.hpx;
        c.z1 = -top / proj.hpx;
        c.up0 = c.z0;
        c.up1 = c.z1;
        return c;
    }
    const double far = std::min(bottom, std::max(0.0, top));
    c.shift = (bottom + far) / 2;
    c.lean = (bottom - far) / 2 * proj.groundPerPx;
    c.z0 = 0;
    c.z1 = (far - top) / proj.hpx;
    c.up0 = 0;
    c.up1 = (far - top + kCardFoot * (bottom - far)) / proj.hpx;
    return c;
}

int hexFromPx(double x, double y) {
    x = std::floor(x);
    y = std::floor(y);
    static const auto kMask = [] {
        std::array<uint8_t, 32 * 12> m = {};
        for (int row = 0; row < 4; row++) {
            for (int i = 0; i < 16; i++) {
                if (64 - 4 * i > 16 * row) m[32 * row + i] = 1;
                if (4 * i > 16 * row) m[32 * row + 16 + i] = 2;
            }
        }
        return m;
    }();
    const int v3 = static_cast<int>(std::floor(y / 12));
    const double v4d = x - 16 * v3;
    const double v5d = y - 12 * v3;
    const int v6 = static_cast<int>(std::floor(v4d / 64));
    int v10 = v6 + v3;
    double v8d = v4d - v6 * 64;
    int v11 = 2 * v6;
    if (v8d >= 32) { v8d -= 32; v11 += 1; }
    const int v8 = static_cast<int>(v8d);
    const int v5 = static_cast<int>(v5d);
    if (v8 < 0 || v8 >= 32 || v5 < 0 || v5 >= 12) return -1;
    const int corner = kMask[32 * v5 + v8];
    if (corner == 2) {
        v11 += 1;
        if (v11 & 1) v10 -= 1;
    } else if (corner == 1) {
        v10 -= 1;
    }
    const int hx = kHexW - 1 - v11;
    return hx >= 0 && hx < kHexW && v10 >= 0 && v10 < kHexW ? v10 * kHexW + hx : -1;
}

double rowPlane(const Proj& proj, double u, double v, int stack) {
    double px[2];
    proj.groundToPx(u, v, 0, px);
    const int tile = hexFromPx(px[0], px[1]);
    if (tile < 0) return v;
    const int hx = tile % kHexW;
    double nudgeCap = proj.stack != 0 ? proj.rowTilt / proj.stack - 1 : 0;
    const double nudge = proj.rowTilt * hx + proj.stack * std::min(stack, static_cast<int>(nudgeCap));
    double centre[2];
    hexPx(tile - (hx & 1), centre);
    double out[2];
    proj.pxToGround(centre[0], centre[1], out);
    return out[1] + nudge;
}

} // namespace f2native
