#ifndef FALLOUT_SCENE_RENDERER_SCENE_GL_H
#define FALLOUT_SCENE_RENDERER_SCENE_GL_H

#include <SDL.h>

namespace fallout {
namespace scene_renderer {

// Takes over presentation of the CE window with an OpenGL 3.3 context.
// `logicalWidth/Height` is the size of the game framebuffer (the vanilla
// SDL_Renderer logical size equivalent): the frame is scaled into the window
// with aspect preserved and letterboxed, like SDL_RenderSetLogicalSize did.
bool init(SDL_Window* window, int logicalWidth, int logicalHeight);
void shutdown();
bool isActive();

// Presents one RGBA8 frame (the palette-expanded game buffer) as a
// full-viewport quad, then swaps. Pixels are top-down, `pitch` in bytes.
void presentFrame(const unsigned char* rgba, int width, int height, int pitch);

// Optional overlay drawn over the last presented frame (movie frames): an
// RGBA image mapped into `dstX/dstY/dstW/dstH` in LOGICAL coordinates.
void drawOverlay(const unsigned char* rgba, int width, int height, int pitch, int dstX, int dstY, int dstW, int dstH);

// Swaps after overlay drawing.
void swap();

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SCENE_GL_H */
