#ifndef FALLOUT_SCENE_FLOATING_TEXT_H
#define FALLOUT_SCENE_FLOATING_TEXT_H

#include "../geometry.h"
#include "text_layer.h"

namespace fallout {
namespace scene {

bool floatingTextCreateData(char* string, const short* lineStarts, int linesCount,
    int font, ColorWithFlags color, ColorWithFlags outlineColor,
    int width, int height, FloatingTextOutlineLayout* layout, unsigned char** data);
void floatingTextRenderInRect(const FloatingTextOutlineLayout& layout,
    const unsigned char* data, int x, int y, const Rect& clip,
    unsigned char* destination, int destinationPitch);
Rect floatingTextRefreshRect(const FloatingTextOutlineLayout& layout,
    int x, int y, int tile);

} // namespace scene
} // namespace fallout

#endif // FALLOUT_SCENE_FLOATING_TEXT_H
