#ifndef FALLOUT_SCENE_RENDERER_MAP_CLIP_H
#define FALLOUT_SCENE_RENDERER_MAP_CLIP_H

namespace fallout {
namespace scene_renderer {

enum class MapClipSource {
    Disabled,
    Authored,
    Floor,
    FloorGridFallback,
};

struct MapClipPoint {
    double x;
    double z;
};

struct MapClipPlane {
    float x;
    float z;
    float offset;
};

// A convex four-sided cut on the ground plane. Plane values point inward, so
// the fragment and picking tests share the same inclusive boundary rule.
struct MapClip {
    bool enabled = false;
    MapClipSource source = MapClipSource::Disabled;
    MapClipPlane planes[4] = {};
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;
};

// Builds inward-facing planes for four points in perimeter order. Invalid,
// non-finite, or degenerate geometry returns a disabled clip.
MapClip makeMapClip(const MapClipPoint points[4], MapClipSource source);

// Builds a normalized axis-aligned rectangle. Coordinates are map world X/Z.
MapClip makeMapClipRect(double left, double top, double right, double bottom, MapClipSource source);

// Builds a straight game-pixel rectangle from occupied floor-square centers,
// inset by half a projected square so the outermost tile rim is cut cleanly.
// The returned quadrilateral is in map world X/Z; invalid or tiny inputs fail closed.
MapClip makeProjectedFloorMapClip(const unsigned char* occupied, int width, int height,
    const double squ[2], const double sqv[2], const double origin[2]);

bool mapClipContains(const MapClip& clip, double x, double z);

// Extend one authored clip plane to an aligned floor plane. If the floor has
// no plane with the same inward normal, leave the clip unchanged.
bool extendMapClipPlaneToFloor(MapClip& clip, const MapClip& floor, int planeIndex);

// Whether the floor sidecar has a drawn square at this world point. The grid
// origin is centered at (0, 0), like floor.bin's 100 x 100 map.
bool floorGridContainsWorldPoint(const unsigned char* occupied, int width, int height, double x, double z);

// EDG v1 pixelRect uses map-edge offsets. Convert them to game-art pixels by
// anchoring both coordinate systems at the same known hex center.
void edgeOffsetToGamePixel(double edgeX, double edgeY, double anchorEdgeX, double anchorEdgeY,
    double anchorGameX, double anchorGameY, double* gameX, double* gameY);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_MAP_CLIP_H */
