#ifndef FALLOUT_SCENE_RENDERER_POST_H
#define FALLOUT_SCENE_RENDERER_POST_H

#include <string>

namespace fallout {
namespace scene_renderer {

// The glow (bloom) pass: the scene is rendered into an offscreen framebuffer,
// its bright parts are extracted, blurred and composited back over the
// window. Within OpenGL 4.1 core. Driven by [3d] glow and [3d]
// glow_strength; switches itself off and logs when it cannot start.

bool initGlow(std::string* error);
// Recreates the offscreen targets when the window size changes; no-op when
// the size is unchanged.
void resizeGlow();
bool glowActive();
void setGlowDisabled();

// Binds the offscreen framebuffer for the scene draw.
bool beginSceneCapture();
// Extracts and blurs the bright parts of the scene framebuffer, then
// composites the scene and the glow over the window.
void compositeGlow();

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_POST_H */
