#ifndef FALLOUT_SCENE_RENDERER_GROUND_PICK_H
#define FALLOUT_SCENE_RENDERER_GROUND_PICK_H

namespace fallout {
namespace scene_renderer {

struct GroundPickProjection {
    double squ[2];
    double sqv[2];
    double origin[2];
};

struct GroundPickCamera {
    double eye[3];
    double target[3];
    double fovY;
    bool donorScreenRight = false;
};

// Camera basis shared by scene rendering, ground picking, and hero cards.
// screenRight keeps donor screen handedness after (u,v,z) -> (u,z,v).
bool previewCameraBasis(const GroundPickCamera& camera, double forward[3],
    double screenRight[3], double screenUp[3]);
GroundPickCamera staticPreviewCameraParameters(bool withWalls);
// The same camera moved over the ground, angle and distance kept, so the
// ground point at the center of the view becomes (groundX, groundZ).
bool followGroundPoint(const GroundPickCamera& camera, double groundX, double groundZ, GroundPickCamera* moved);

struct GroundPickViewport {
    int logicalWidth;
    int logicalHeight;
    int drawableWidth;
    int drawableHeight;
    int windowWidth;
    int windowHeight;
};

struct GroundPickResult {
    int tile;
    double worldX;
    double worldZ;
    double floorU;
    double floorV;
    double artX;
    double artY;
};

bool projectGroundPoint(const GroundPickProjection& projection, const GroundPickCamera& camera,
    const GroundPickViewport& viewport, double worldX, double worldZ, double* logicalX, double* logicalY);
bool pickGroundTile(const GroundPickProjection& projection, const GroundPickCamera& camera,
    const GroundPickViewport& viewport, double logicalX, double logicalY, GroundPickResult* result);
int donorHexFromPixel(double x, double y);

} // namespace scene_renderer
} // namespace fallout

#endif
