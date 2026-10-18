#include "ground_pick.h"

#include <array>
#include <cmath>
#include <limits>

namespace fallout {
namespace scene_renderer {
namespace {

constexpr int kHexWidth = 200;

bool finite(double v) { return std::isfinite(v); }

struct Basis { double forward[3], right[3], up[3]; };

bool makeBasis(const GroundPickCamera& c, Basis* b)
{
    return previewCameraBasis(c, b->forward, b->right, b->up);
}

bool valid(const GroundPickProjection& p)
{
    for (double v : p.squ) if (!finite(v) || std::abs(v) > 1e6) return false;
    for (double v : p.sqv) if (!finite(v) || std::abs(v) > 1e6) return false;
    for (double v : p.origin) if (!finite(v) || std::abs(v) > 1e7) return false;
    return std::abs(p.squ[0] * p.sqv[1] - p.sqv[0] * p.squ[1]) > 1e-9;
}

bool contentRect(const GroundPickViewport& v, double* x, double* y, double* w, double* h)
{
    if (v.logicalWidth <= 0 || v.logicalHeight <= 0 || v.drawableWidth <= 0 || v.drawableHeight <= 0 || v.windowWidth <= 0 || v.windowHeight <= 0) return false;
    int width = v.drawableWidth;
    int height = width * v.logicalHeight / v.logicalWidth;
    if (height > v.drawableHeight) { height = v.drawableHeight; width = height * v.logicalWidth / v.logicalHeight; }
    // SDL mouse coordinates are already CE-logical, mapped linearly from window points.
    // Scale back to drawable units only to test the same letterbox used by the GL viewport.
    const double sx = static_cast<double>(v.drawableWidth) / v.windowWidth;
    const double sy = static_cast<double>(v.drawableHeight) / v.windowHeight;
    const int left = (v.drawableWidth - width) / 2;
    const int top = (v.drawableHeight - height) / 2;
    *x = left / sx;
    *y = top / sy;
    *w = width / sx;
    *h = height / sy;
    return *w > 0 && *h > 0;
}

} // namespace

bool previewCameraBasis(const GroundPickCamera& camera, double forward[3],
    double screenRight[3], double screenUp[3])
{
    if (forward == nullptr || screenRight == nullptr || screenUp == nullptr) return false;
    double f[3] = {
        camera.target[0] - camera.eye[0],
        camera.target[1] - camera.eye[1],
        camera.target[2] - camera.eye[2],
    };
    for (double value : f) if (!finite(value)) return false;
    double length = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (!finite(length) || length < 1e-9) return false;
    for (double& value : f) value /= length;

    // The donor's ground coordinates are reflected when converted to GL
    // (u,v,z) -> (u,z,v). The close wall/hero/walk view opts into the
    // reflected horizontal axis; floor-only keeps its accepted overview.
    const double sign = camera.donorScreenRight ? -1.0 : 1.0;
    double r[3] = { -sign * f[2], 0.0, sign * f[0] };
    length = std::sqrt(r[0] * r[0] + r[2] * r[2]);
    if (!finite(length) || length < 1e-9) return false;
    for (double& value : r) value /= length;

    const double projection = f[1]; // dot(forward, worldUp)
    double u[3] = { -projection * f[0], 1.0 - projection * f[1], -projection * f[2] };
    length = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    if (!finite(length) || length < 1e-9) return false;
    for (double& value : u) value /= length;
    for (int i = 0; i < 3; i++) {
        forward[i] = f[i];
        screenRight[i] = r[i];
        screenUp[i] = u[i];
    }
    return true;
}

GroundPickCamera staticPreviewCameraParameters(bool withWalls)
{
    if (withWalls) {
        constexpr double kPi = 3.14159265358979323846;
        const double scale = std::tan(25.0 * kPi / 180.0) / std::tan(12.5 * kPi / 180.0);
        return {
            { -11.9561 + 8.0 * scale, 1.0 + 10.0 * scale, 9.11265 + 12.0 * scale },
            { -11.9561, 1.0, 9.11265 },
            25.0, true,
        };
    }
    return { { 0.0, 88.0, 118.0 }, { 0.0, 0.0, 0.0 }, 50.0, false };
}

int donorHexFromPixel(double x, double y)
{
    if (!finite(x) || !finite(y) || std::abs(x) > 1e7 || std::abs(y) > 1e7) return -1;
    x = std::floor(x); y = std::floor(y);
    static const auto mask = [] {
        std::array<unsigned char, 32 * 12> m = {};
        for (int row = 0; row < 4; row++) for (int i = 0; i < 16; i++) {
            if (64 - 4 * i > 16 * row) m[32 * row + i] = 1;
            if (4 * i > 16 * row) m[32 * row + 16 + i] = 2;
        }
        return m;
    }();
    const int row = static_cast<int>(std::floor(y / 12));
    const double shiftedX = x - 16 * row;
    const int band = static_cast<int>(std::floor(shiftedX / 64));
    int tileRow = band + row;
    double localX = shiftedX - band * 64;
    int tileColumn = 2 * band;
    if (localX >= 32) { localX -= 32; tileColumn++; }
    const int mx = static_cast<int>(localX), my = static_cast<int>(y - row * 12);
    if (mx < 0 || mx >= 32 || my < 0 || my >= 12) return -1;
    const int corner = mask[32 * my + mx];
    if (corner == 2) { tileColumn++; if ((tileColumn & 1) != 0) tileRow--; }
    else if (corner == 1) tileRow--;
    const int hx = kHexWidth - 1 - tileColumn;
    return hx >= 0 && hx < kHexWidth && tileRow >= 0 && tileRow < kHexWidth ? tileRow * kHexWidth + hx : -1;
}

bool projectGroundPoint(const GroundPickProjection& p, const GroundPickCamera& c,
    const GroundPickViewport& v, double worldX, double worldZ, double* logicalX, double* logicalY)
{
    if (logicalX == nullptr || logicalY == nullptr || !valid(p) || !finite(worldX) || !finite(worldZ)) return false;
    Basis b;
    if (!makeBasis(c, &b) || !finite(c.fovY) || c.fovY <= 0 || c.fovY >= 179) return false;
    double rx, ry, rw, rh;
    if (!contentRect(v, &rx, &ry, &rw, &rh)) return false;
    const double dx = worldX - c.eye[0], dy = -c.eye[1], dz = worldZ - c.eye[2];
    const double depth = dx * b.forward[0] + dy * b.forward[1] + dz * b.forward[2];
    if (depth <= 1e-9) return false;
    const double sx = static_cast<double>(v.drawableWidth) / v.windowWidth;
    const double sy = static_cast<double>(v.drawableHeight) / v.windowHeight;
    const double viewportWidth = rw * sx;
    const double viewportHeight = rh * sy;
    const double aspect = viewportWidth / viewportHeight;
    const double tanHalf = std::tan(c.fovY * 3.14159265358979323846 / 360.0);
    const double ndcX = (dx * b.right[0] + dy * b.right[1] + dz * b.right[2]) / (depth * tanHalf * aspect);
    const double ndcY = (dx * b.up[0] + dy * b.up[1] + dz * b.up[2]) / (depth * tanHalf);
    const double windowX = rx + (ndcX + 1.0) * 0.5 * rw;
    const double windowY = ry + (1.0 - ndcY) * 0.5 * rh;
    *logicalX = windowX * v.logicalWidth / v.windowWidth;
    *logicalY = windowY * v.logicalHeight / v.windowHeight;
    return finite(*logicalX) && finite(*logicalY);
}

bool pickGroundTile(const GroundPickProjection& p, const GroundPickCamera& c,
    const GroundPickViewport& v, double logicalX, double logicalY, GroundPickResult* out)
{
    if (out == nullptr || !valid(p) || !finite(logicalX) || !finite(logicalY)) return false;
    double rx, ry, rw, rh;
    if (!contentRect(v, &rx, &ry, &rw, &rh)) return false;
    const double sx = static_cast<double>(v.drawableWidth) / v.windowWidth;
    const double sy = static_cast<double>(v.drawableHeight) / v.windowHeight;
    const double px = logicalX * v.windowWidth / v.logicalWidth;
    const double py = logicalY * v.windowHeight / v.logicalHeight;
    if (px < rx || py < ry || px >= rx + rw || py >= ry + rh) return false;
    const double localX = px - rx;
    const double localY = py - ry;
    const double ndcX = 2.0 * localX / rw - 1.0;
    const double ndcY = 1.0 - 2.0 * localY / rh;
    Basis b;
    if (!makeBasis(c, &b) || !finite(c.fovY) || c.fovY <= 0 || c.fovY >= 179 || c.eye[1] <= 0) return false;
    const double tanHalf = std::tan(c.fovY * 3.14159265358979323846 / 360.0);
    const double aspect = (rw * sx) / (rh * sy);
    double ray[3];
    for (int i = 0; i < 3; i++) ray[i] = b.forward[i] + b.right[i] * ndcX * aspect * tanHalf + b.up[i] * ndcY * tanHalf;
    if (ray[1] >= -1e-10) return false;
    const double t = -c.eye[1] / ray[1];
    if (!finite(t) || t <= 0) return false;
    const double worldX = c.eye[0] + t * ray[0];
    const double worldZ = c.eye[2] + t * ray[2];
    const double u = worldX + 50.0, vWorld = worldZ + 50.0;
    // This preview is the exported 100x100 Kladwtwn square map only.
    if (u < 0.0 || u >= 100.0 || vWorld < 0.0 || vWorld >= 100.0) return false;
    const double artX = p.origin[0] + u * p.squ[0] + vWorld * p.sqv[0];
    const double artY = p.origin[1] + u * p.squ[1] + vWorld * p.sqv[1];
    const int tile = donorHexFromPixel(artX, artY);
    if (tile < 0) return false;
    out->tile = tile; out->worldX = worldX; out->worldZ = worldZ;
    out->floorU = u; out->floorV = vWorld; out->artX = artX; out->artY = artY;
    return true;
}

} // namespace scene_renderer
} // namespace fallout
