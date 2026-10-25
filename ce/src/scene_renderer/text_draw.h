#ifndef FALLOUT_SCENE_RENDERER_TEXT_DRAW_H
#define FALLOUT_SCENE_RENDERER_TEXT_DRAW_H

#include <string>
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
    unsigned int runCount = 0;
    unsigned int glyphCount = 0;
    unsigned int restoredPixelCount = 0;
    double prepareMilliseconds = 0.0;
    bool drawn = false;
    bool active = false;
};

bool renderSharpText(const std::vector<scene::TextRecord>& records,
    int logicalWidth, int logicalHeight, int physicalWidth, int physicalHeight,
    const unsigned char* paletteRgb, int paletteEntries,
    const std::vector<scene::TextVisibilityMask>& logicalVisibility, SharpTextSurface* output);
bool sharpTextLanguageSupported(const std::string& language);
// Logical pixel row nearest the rendered font-101 lowercase x ink center.
bool sharpTextXHeightCenter(int lineHeight, int ownerWindow, double physicalScale, int* center);

} // namespace scene_renderer
} // namespace fallout

#endif
