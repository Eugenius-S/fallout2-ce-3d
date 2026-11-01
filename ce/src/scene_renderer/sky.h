#ifndef FALLOUT_SCENE_RENDERER_SKY_H
#define FALLOUT_SCENE_RENDERER_SKY_H

namespace fallout {
namespace scene_renderer {

// Draws the time-of-day sky behind a turned or flying 3D scene.
void drawSceneSky(float aspect, int width, int height, bool flying);
void destroySceneSky();

} // namespace scene_renderer
} // namespace fallout

#endif
