#ifndef FALLOUT_SCENE_RENDERER_CONTEXT_H
#define FALLOUT_SCENE_RENDERER_CONTEXT_H

#include <SDL.h>

#include <vector>

namespace fallout {
namespace scene_renderer {

// Takes over presentation of the CE window with an OpenGL context.
// `logicalWidth/Height` is the size of the game framebuffer (the vanilla
// SDL_Renderer logical size equivalent): the frame is scaled into the window
// with aspect preserved and letterboxed, like SDL_RenderSetLogicalSize did.
bool init(SDL_Window* window, int logicalWidth, int logicalHeight);
void shutdown();
bool isActive();

// Presents one RGBA8 frame (the palette-expanded game buffer) as a
// full-viewport quad. Pixels are top-down, `pitch` in bytes.
void presentFrame(const unsigned char* rgba, int width, int height, int pitch);

// The game's own picture over the 3D scene (interface, mouse cursor, floating
// text): premultiplied RGBA, transparent where the world shows. Snapshots of
// the scene include it.
void drawSceneOverlay(const unsigned char* rgba, int width, int height, int pitch);

// Optional overlay drawn over the last presented frame (movie frames): an
// RGBA image mapped into `dstX/dstY/dstW/dstH` in LOGICAL coordinates.
void drawOverlay(const unsigned char* rgba, int width, int height, int pitch, int dstX, int dstY, int dstW, int dstH);

// Maps a point in LOGICAL coordinates to window pixels through the same fit
// the quads draw with. The mark-drag bench reads its overlay's pixels back.
bool overlayLogicalToWindow(int logicalX, int logicalY, int* windowX, int* windowY);

// Reads a rectangle of the default framebuffer as it stands before the swap.
// `windowY` is top-down, like the drawing coordinates; the returned pixels
// are RGBA, bottom-up (GL's own order).
bool readPresentPixels(int windowX, int windowY, int width, int height, std::vector<unsigned char>& out);

// Sharp UI text uses an independent texture so it cannot replace the cached
// game overlay used by static scene snapshots.
bool sharpTextOutputSize(int* width, int* height);
bool uploadSharpTextOverlay(const unsigned char* rgba, int width, int height, int pitch);
bool drawSharpTextOverlay();

// Swaps after overlay drawing.
void swap();

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_CONTEXT_H */
