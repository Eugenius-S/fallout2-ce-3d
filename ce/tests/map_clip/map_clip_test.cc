#include "map_clip.h"

#include <cmath>
#include <cstdio>
#include <limits>

namespace {

bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "FAIL: %s\n", message);
    return condition;
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
    return 0;
}
