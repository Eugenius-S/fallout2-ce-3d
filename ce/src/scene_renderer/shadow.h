#ifndef FALLOUT_SCENE_RENDERER_SHADOW_H
#define FALLOUT_SCENE_RENDERER_SHADOW_H

namespace fallout {
namespace scene_renderer {

// Per-frame stats of the shadow pass for the dev perf log: whether the last
// bindStaticShadows rendered it, and the seconds it took since the last call.
void shadowFrameStats(bool* rendered, double* seconds);

namespace internal {

// Sun shadows from buildings and big things (task 045). One directional depth
// map over the whole map ground, cast by the static scene: wall batches (backs
// and ends included), all roof squares (hidden ones too) and the baked objects
// big enough to cast. The floor and the walls sample it; sprites do not (that
// is task 046).
//
// Set the casters once after walls and roofs load; the rest happens per frame
// in bindStaticShadows from the scene draw.

// Takes the caster triangles walls.cc kept and the tall enough of them; both
// null arguments are allowed.
void setShadowWallCasters(unsigned int vao, int vertexCount, float maxCasterHeight);
// Takes the all-squares roof buffer roofs.cc keeps for the shadow pass.
void setShadowRoofCasters(unsigned int vao, int vertexCount);
void destroyStaticShadows();

// Renders the depth map with the frame's sun and sets the scene shader's
// shadow uniforms; false (with them switched off) when the setting is off,
// the sun is down, or the casters or the depth map are missing.
bool bindStaticShadows();

} // namespace internal
} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SHADOW_H */
