#ifndef FALLOUT_SCENE_RENDERER_TEXT_PRESENT_H
#define FALLOUT_SCENE_RENDERER_TEXT_PRESENT_H

#include "text_draw.h"

struct SDL_Surface;

namespace fallout {
namespace scene_renderer {

// Validates live text records and builds the restored game frame plus sharp
// glyph overlay without changing the indexed game buffer.
bool prepareSharpTextFrame(const unsigned char* indices, int width, int height, int indexPitch,
    const unsigned char* rgba, int rgbaPitch, int physicalWidth, int physicalHeight,
    const unsigned char* paletteRgb, int paletteEntries, bool enabled);
bool prepareSharpTextForPresent(SDL_Surface* indexedSurface, SDL_Surface* rgbaSurface, bool enabled);
void disableSharpText(const char* message);
const PreparedSharpTextFrame& preparedSharpTextFrame();

} // namespace scene_renderer
} // namespace fallout

#endif
