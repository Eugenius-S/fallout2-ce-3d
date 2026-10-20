#include "map_clip.h"

#include <algorithm>
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

MapClip makeProjectedFloorMapClip(const unsigned char* occupied, int width, int height,
    const double squ[2], const double sqv[2], const double origin[2])
{
    if (occupied == nullptr || width <= 0 || height <= 0 || squ == nullptr || sqv == nullptr || origin == nullptr) {
        return {};
    }
    const double determinant = squ[0] * sqv[1] - sqv[0] * squ[1];
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-9) return {};

    double minX = 1e300, maxX = -1e300, minY = 1e300, maxY = -1e300;
    bool hasCell = false;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (occupied[row * width + col] == 0) continue;
            const double u = col + 0.5;
            const double v = row + 0.5;
            const double pixelX = origin[0] + u * squ[0] + v * sqv[0];
            const double pixelY = origin[1] + u * squ[1] + v * sqv[1];
            if (!std::isfinite(pixelX) || !std::isfinite(pixelY)) return {};
            minX = std::min(minX, pixelX);
            maxX = std::max(maxX, pixelX);
            minY = std::min(minY, pixelY);
            maxY = std::max(maxY, pixelY);
            hasCell = true;
        }
    }
    if (!hasCell) return {};

    const double insetX = (std::abs(squ[0]) + std::abs(sqv[0])) * 0.5;
    const double insetY = (std::abs(squ[1]) + std::abs(sqv[1])) * 0.5;
    minX += insetX;
    maxX -= insetX;
    minY += insetY;
    maxY -= insetY;
    if (maxX - minX < 1e-6 || maxY - minY < 1e-6) return {};

    const double pixels[4][2] = {
        { minX, minY }, { maxX, minY }, { maxX, maxY }, { minX, maxY },
    };
    MapClipPoint points[4];
    for (int index = 0; index < 4; index++) {
        const double dx = pixels[index][0] - origin[0];
        const double dy = pixels[index][1] - origin[1];
        const double u = (sqv[1] * dx - sqv[0] * dy) / determinant;
        const double v = (squ[0] * dy - squ[1] * dx) / determinant;
        points[index] = { u - 50.0, v - 50.0 };
    }
    return makeMapClip(points, MapClipSource::Floor);
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
