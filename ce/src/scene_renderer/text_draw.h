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

struct PreparedSharpTextFrame {
    std::vector<unsigned char> originalIndices;
    std::vector<unsigned char> originalRgba;
    std::vector<unsigned char> restoredIndices;
    std::vector<unsigned char> restoredRgba;
    SharpTextSurface overlay;
    int indexPitch = 0;
    int rgbaPitch = 0;
    int width = 0;
    int height = 0;
    bool active = false;
};

bool renderSharpText(const std::vector<scene::TextRecord>& records,
    int logicalWidth, int logicalHeight, int physicalWidth, int physicalHeight,
    const unsigned char* paletteRgb, int paletteEntries,
    const std::vector<scene::TextVisibilityMask>& logicalVisibility, SharpTextSurface* output);

} // namespace scene_renderer
} // namespace fallout

#endif
