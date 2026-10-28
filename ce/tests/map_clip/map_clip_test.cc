#include "map_clip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace {

bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

void pixelToWorld(double px, double py, const double squ[2], const double sqv[2], const double origin[2],
    double* x, double* z)
{
    const double dx = px - origin[0];
    const double dy = py - origin[1];
    const double determinant = squ[0] * sqv[1] - sqv[0] * squ[1];
    const double u = (sqv[1] * dx - sqv[0] * dy) / determinant;
    const double v = (squ[0] * dy - squ[1] * dx) / determinant;
    *x = u - 50.0;
    *z = v - 50.0;
}

} // namespace

int main()
{
    using namespace fallout::scene_renderer;

    // EDG v2's format may store X backwards (for example left=99,right=0).
    const MapClip reversedSquare = makeMapClipRect(99.0, 10.0, 0.0, 20.0, MapClipSource::Authored);
    if (!check(reversedSquare.enabled && reversedSquare.source == MapClipSource::Authored,
            "reversed EDG square rectangle is normalized")) return 1;
    if (!check(mapClipContains(reversedSquare, 0.0, 10.0)
            && mapClipContains(reversedSquare, 99.0, 20.0)
            && mapClipContains(reversedSquare, 49.5, 15.0), "all rectangle edges are inclusive")) return 1;
    if (!check(!mapClipContains(reversedSquare, -0.01, 15.0)
            && !mapClipContains(reversedSquare, 50.0, 20.01), "points outside each rectangle axis are rejected")) return 1;

    // A projected v1 EDG rectangle can be a parallelogram in ground X/Z.
    const MapClipPoint corners[4] = {
        { -2.0, -1.0 }, { 3.0, 1.0 }, { 2.0, 5.0 }, { -3.0, 3.0 },
    };
    const MapClip authored = makeMapClip(corners, MapClipSource::Authored);
    if (!check(authored.enabled && mapClipContains(authored, 0.0, 2.0), "projected authored envelope contains its center")) return 1;
    if (!check(!mapClipContains(authored, -2.9, -0.5) && !mapClipContains(authored, 2.5, 4.0),
            "projected authored envelope rejects points beyond slanted sides")) return 1;
    const MapClipPoint reverseWinding[4] = { corners[3], corners[2], corners[1], corners[0] };
    const MapClip reverse = makeMapClip(reverseWinding, MapClipSource::Floor);
    if (!check(reverse.enabled && mapClipContains(reverse, 0.0, 2.0)
            && !mapClipContains(reverse, -2.9, -0.5), "clip planes are inward for either winding")) return 1;

    const MapClip degenerate = makeMapClipRect(3.0, 4.0, 3.0, 8.0, MapClipSource::Floor);
    if (!check(!degenerate.enabled && mapClipContains(degenerate, 1e9, -1e9), "degenerate clip safely disables the feature")) return 1;
    const MapClipPoint invalidCorners[4] = {
        { 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, std::numeric_limits<double>::infinity() }, { 0.0, 1.0 },
    };
    if (!check(!makeMapClip(invalidCorners, MapClipSource::Authored).enabled,
            "non-finite authored geometry is rejected")) return 1;
    if (!check(!mapClipContains(authored, std::numeric_limits<double>::quiet_NaN(), 0.0),
            "non-finite picking coordinates are rejected")) return 1;

    double gameX = 0.0, gameY = 0.0;
    edgeOffsetToGamePixel(3184.0, 24.0, 3200.0, 0.0, 4800.0, -1180.0, &gameX, &gameY);
    if (!check(gameX == 4816.0 && gameY == -1156.0,
            "v1 edge pixels use the anchored inverted-X and normal-Y conversion")) return 1;

    // A diamond in floor-grid coordinates projects to a straight pixel rectangle.
    // The cut follows that rectangle, not the much larger world-grid AABB.
    unsigned char diamond[7 * 7] = {};
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 7; col++) {
            diamond[row * 7 + col] = std::abs(row - 3) + std::abs(col - 3) <= 3 ? 1 : 0;
        }
    }
    const double squ[2] = { -2.0, 1.0 };
    const double sqv[2] = { 2.0, 1.0 };
    const double origin[2] = { 10.0, -5.0 };
    const MapClip projectedFloor = makeProjectedFloorMapClip(diamond, 7, 7, squ, sqv, origin);
    double insideX = 0.0, insideZ = 0.0, outsideX = 0.0, outsideZ = 0.0;
    pixelToWorld(10.0, 3.5, squ, sqv, origin, &insideX, &insideZ);
    pixelToWorld(14.1, 1.0, squ, sqv, origin, &outsideX, &outsideZ);
    if (!check(projectedFloor.enabled && projectedFloor.source == MapClipSource::Floor,
            "diamond floor mask produces a projected four-sided floor cut")) return 1;
    if (!check(mapClipContains(projectedFloor, insideX, insideZ)
            && !mapClipContains(projectedFloor, outsideX, outsideZ),
            "projected cell centers and half-cell inset produce the straight pixel rectangle")) return 1;
    if (!check(std::abs(projectedFloor.planes[0].x) > 0.01f && std::abs(projectedFloor.planes[0].z) > 0.01f,
            "projected pixel edge remains a rotated ground-plane edge")) return 1;

    const double singularSqu[2] = { 1.0, 2.0 };
    const double singularSqv[2] = { 2.0, 4.0 };
    if (!check(!makeProjectedFloorMapClip(diamond, 7, 7, singularSqu, singularSqv, origin).enabled,
            "singular floor projection disables the clip")) return 1;
    unsigned char emptyFloor[4] = {};
    unsigned char oneCell[1] = { 1 };
    if (!check(!makeProjectedFloorMapClip(emptyFloor, 2, 2, squ, sqv, origin).enabled
            && !makeProjectedFloorMapClip(oneCell, 1, 1, squ, sqv, origin).enabled,
            "empty and too-small floor masks disable the clip")) return 1;
    unsigned char floorGrid[2 * 2] = { 1, 0, 0, 1 };
    if (!check(floorGridContainsWorldPoint(floorGrid, 2, 2, -0.5, -0.5)
            && floorGridContainsWorldPoint(floorGrid, 2, 2, 0.5, 0.5)
            && !floorGridContainsWorldPoint(floorGrid, 2, 2, 0.5, -0.5)
            && !floorGridContainsWorldPoint(floorGrid, 2, 2, 1.0, 0.0),
            "bottom-edge ground checks use the occupied floor cells and reject void")) return 1;
    const unsigned char fullFloor[16] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };
    const double rotatedSqu[2] = { 2.0, 1.0 };
    const double rotatedSqv[2] = { -1.0, 2.0 };
    const double rotatedOrigin[2] = { 0.0, 0.0 };
    const MapClip diagonalFloor = makeProjectedFloorMapClip(fullFloor, 4, 4,
        rotatedSqu, rotatedSqv, rotatedOrigin);
    const MapClip squareAuthored = makeMapClipRect(-1.0, -1.0, 1.0, 1.0, MapClipSource::Authored);
    double bestFloorNormalDot = -1.0;
    for (const MapClipPlane& plane : diagonalFloor.planes) {
        bestFloorNormalDot = std::max(bestFloorNormalDot,
            static_cast<double>(squareAuthored.planes[2].x) * plane.x
                + static_cast<double>(squareAuthored.planes[2].z) * plane.z);
    }
    MapClip unmatchedSquare = squareAuthored;
    const bool unmatchedExtended = extendMapClipPlaneToFloor(unmatchedSquare, diagonalFloor, 2);
    const MapClip alignedFloor = makeMapClipRect(-1.0, -1.0, 1.0, 3.0, MapClipSource::Floor);
    MapClip extendedSquare = squareAuthored;
    const bool didExtendSquare = extendMapClipPlaneToFloor(extendedSquare, alignedFloor, 2);
    if (!check(diagonalFloor.enabled && bestFloorNormalDot < 0.999,
            "square and diagonal floor normals do not match")) return 1;
    if (!check(!unmatchedExtended
            && unmatchedSquare.planes[0].offset == squareAuthored.planes[0].offset
            && unmatchedSquare.planes[1].offset == squareAuthored.planes[1].offset
            && unmatchedSquare.planes[2].offset == squareAuthored.planes[2].offset
            && unmatchedSquare.planes[3].offset == squareAuthored.planes[3].offset,
            "an unmatched square edge does not extend any plane")) return 1;
    if (!check(didExtendSquare && extendedSquare.planes[2].offset > squareAuthored.planes[2].offset
            && extendedSquare.planes[0].offset == squareAuthored.planes[0].offset
            && extendedSquare.planes[1].offset == squareAuthored.planes[1].offset
            && extendedSquare.planes[3].offset == squareAuthored.planes[3].offset
            && mapClipContains(extendedSquare, 0.0, 3.0),
            "an aligned floor edge extends only the selected plane")) return 1;
    return 0;
}
