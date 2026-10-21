#ifndef FALLOUT_SCENE_RENDERER_POST_H
#define FALLOUT_SCENE_RENDERER_POST_H

#include <string>

namespace fallout {
namespace scene_renderer {

// The glow (bloom) pass: the scene renders into an offscreen framebuffer with
// a second attachment for what its texels shine by themselves (a sprite's
// palette-cycling colours, marked alpha 254); that buffer blurs wide and adds
// back over the picture. Within OpenGL 4.1 core. Driven by [3d] glow and
// [3d] glow_strength; switches itself off and logs when it cannot start.

bool initGlow(std::string* error);
// Recreates the offscreen targets when the window size changes; no-op when
// the size is unchanged.
void resizeGlow();
bool glowActive();
void setGlowDisabled();
// Frees everything the glow holds; a later beginSceneCapture starts it anew.
void shutdownGlow();

// Binds the offscreen framebuffer for the scene draw; false when the glow is
// off (the scene draws straight to the window, byte for byte as before).
bool beginSceneCapture();
// Blurs the emissive buffer and composites the scene and the glow over the
// window.
void compositeGlow();

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_POST_H */
