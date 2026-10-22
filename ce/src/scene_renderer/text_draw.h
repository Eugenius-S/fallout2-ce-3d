#ifndef FALLOUT_SCENE_RENDERER_TEXT_DRAW_H
#define FALLOUT_SCENE_RENDERER_TEXT_DRAW_H

#include <vector>

#include "../scene/text_layer.h"

namespace fallout {
namespace scene_renderer {

struct SharpTextSurface {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int pitch = 0;
};

bool renderSharpText(const std::vector<scene::TextRecord>& records,
    int logicalWidth, int logicalHeight, int physicalWidth, int physicalHeight,
    const unsigned char* paletteRgb, int paletteEntries,
    const std::vector<scene::TextVisibilityMask>& logicalVisibility, SharpTextSurface* output);

} // namespace scene_renderer
} // namespace fallout

#endif
