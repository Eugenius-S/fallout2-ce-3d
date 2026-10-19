#include "map_clip.h"

#include <cmath>

namespace fallout {
namespace scene_renderer {

MapClip makeMapClip(const MapClipPoint points[4], MapClipSource source)
{
    MapClip clip;
    clip.source = source;
    if (points == nullptr || source == MapClipSource::Disabled) return clip;

    double centerX = 0.0;
    double centerZ = 0.0;
    clip.left = clip.top = 1e300;
    clip.right = clip.bottom = -1e300;
    for (int index = 0; index < 4; index++) {
        if (!std::isfinite(points[index].x) || !std::isfinite(points[index].z)) return MapClip{};
        centerX += points[index].x * 0.25;
        centerZ += points[index].z * 0.25;
        if (points[index].x < clip.left) clip.left = points[index].x;
        if (points[index].x > clip.right) clip.right = points[index].x;
        if (points[index].z < clip.top) clip.top = points[index].z;
        if (points[index].z > clip.bottom) clip.bottom = points[index].z;
    }

    double areaTwice = 0.0;
    for (int index = 0; index < 4; index++) {
        const MapClipPoint& a = points[index];
        const MapClipPoint& b = points[(index + 1) % 4];
        areaTwice += a.x * b.z - b.x * a.z;
    }
    if (std::abs(areaTwice) < 1e-8 || clip.right - clip.left < 1e-8 || clip.bottom - clip.top < 1e-8) {
        return MapClip{};
    }

    for (int index = 0; index < 4; index++) {
        const MapClipPoint& a = points[index];
        const MapClipPoint& b = points[(index + 1) % 4];
        double nx = -(b.z - a.z);
        double nz = b.x - a.x;
        const double length = std::sqrt(nx * nx + nz * nz);
        if (!std::isfinite(length) || length < 1e-8) return MapClip{};
        nx /= length;
        nz /= length;
        double offset = -(nx * a.x + nz * a.z);
        if (nx * centerX + nz * centerZ + offset < 0.0) {
            nx = -nx;
            nz = -nz;
            offset = -offset;
        }
        clip.planes[index] = {
            static_cast<float>(nx), static_cast<float>(nz), static_cast<float>(offset),
        };
    }
    clip.enabled = true;
    return clip;
}

MapClip makeMapClipRect(double left, double top, double right, double bottom, MapClipSource source)
{
    if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) || !std::isfinite(bottom)) return {};
    if (left > right) {
        const double swap = left;
        left = right;
        right = swap;
    }
    if (top > bottom) {
        const double swap = top;
        top = bottom;
        bottom = swap;
    }
    const MapClipPoint points[4] = {
        { left, top }, { right, top }, { right, bottom }, { left, bottom },
    };
    return makeMapClip(points, source);
}

bool mapClipContains(const MapClip& clip, double x, double z)
{
    if (!clip.enabled) return true;
    if (!std::isfinite(x) || !std::isfinite(z)) return false;
    for (const MapClipPlane& plane : clip.planes) {
        if (plane.x * x + plane.z * z + plane.offset < -1e-5) return false;
    }
    return true;
}

void edgeOffsetToGamePixel(double edgeX, double edgeY, double anchorEdgeX, double anchorEdgeY,
    double anchorGameX, double anchorGameY, double* gameX, double* gameY)
{
    if (gameX != nullptr) *gameX = anchorGameX + (anchorEdgeX - edgeX);
    if (gameY != nullptr) *gameY = anchorGameY + (edgeY - anchorEdgeY);
}

} // namespace scene_renderer
} // namespace fallout
