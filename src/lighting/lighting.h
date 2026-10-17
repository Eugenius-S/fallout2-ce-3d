#ifndef FALLOUT_LIGHTING_H
#define FALLOUT_LIGHTING_H

namespace fallout {
namespace lighting {

// Perspective camera layer (CE modification for the sprite-lighting mod).
//
// The vanilla renderer draws the world into the 8-bit framebuffer with a
// fixed linear isometric projection. This module treats those vanilla screen
// coordinates as coordinates of a virtual ground plane and re-projects them
// with a tilted perspective camera. The azimuth is unchanged, so sprites
// keep exactly the sides they were drawn for; heights above the plane
// (roofs, the vertical extent of sprites) are expressed in vanilla pixels
// and foreshorten consistently.
//
// world (vanilla screen coords) -> perspective screen:
//   zc  = d - (y - Cy)*cos(pitch) - h*sin(pitch)   [camera-space depth]
//   sx' = Cx + d*(x - Cx) / zc
//   sy' = Cy + d*((y - Cy)*sin(pitch) - h*cos(pitch)) / zc
// where (Cx, Cy) is the map window center, d = [lighting] distance,
// pitch = [lighting] pitch in degrees (90 deg gives the vanilla projection
// for the ground plane). Smaller d means stronger perspective.

// Must be called when the map window dimensions are known.
void init(int windowWidth, int windowHeight);

bool isPerspective();

void projectElevated(float x, float y, float h, float* outX, float* outY, float* outDepth = nullptr);
void projectGround(float x, float y, float* outX, float* outY);

// Inverse of projectGround; used to map the mouse position back onto the
// ground plane so tile picking keeps working under perspective.
void inverseGround(float x, float y, float* outX, float* outY);

// How many vanilla-screen pixels above the window top still project into the
// visible window under the current perspective (0 when off / none).
int farMargin();

// Top and side margins (in vanilla screen pixels) that must be added to the
// vanilla render rectangle so every point visible under the perspective
// camera gets rendered. Both are 0 when the perspective is off.
void renderMargins(int* top, int* sides);

} // namespace lighting
} // namespace fallout

#endif /* FALLOUT_LIGHTING_H */
