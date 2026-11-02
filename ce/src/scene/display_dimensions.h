#ifndef FALLOUT_SCENE_DISPLAY_DIMENSIONS_H
#define FALLOUT_SCENE_DISPLAY_DIMENSIONS_H

#include <cmath>

namespace fallout {
namespace scene {

// A rejected display size must not replace the active framebuffer.
inline bool displayLogicalDimensions(int outputWidth, int outputHeight, double factor, int* width, int* height)
{
    if (width == nullptr || height == nullptr || outputWidth < 640 || outputHeight < 480
        || !std::isfinite(factor) || factor < 1.0) return false;
    const int candidateWidth = static_cast<int>(std::lround(outputWidth / factor));
    const int candidateHeight = static_cast<int>(std::lround(outputHeight / factor));
    if (candidateWidth < 640 || candidateHeight < 480) return false;
    *width = candidateWidth;
    *height = candidateHeight;
    return true;
}

} // namespace scene
} // namespace fallout

#endif
