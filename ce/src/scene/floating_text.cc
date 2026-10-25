#include "floating_text.h"

#include <cstring>

#include "../draw.h"
#include "../memory.h"
#include "../text_font.h"
#include "text_capture.h"

namespace fallout {
void tileSceneShift(int tile, int* dx, int* dy);

namespace scene {

bool floatingTextCreateData(char* string, const short* lineStarts, int linesCount,
    int font, ColorWithFlags color, ColorWithFlags outlineColor,
    int width, int height, FloatingTextOutlineLayout* layout, unsigned char** data)
{
    if (string == nullptr || lineStarts == nullptr || linesCount <= 0 || layout == nullptr || data == nullptr) {
        return false;
    }

    *layout = floatingTextOutlineLayout(width, height, fontGetLineHeight(), linesCount,
        outlineColor != COLOR_INVALID, font == 101 && outlineColor != COLOR_INVALID);
    *data = nullptr;

    int size = layout->bufferWidth * layout->bufferHeight;
    unsigned char* buffer = static_cast<unsigned char*>(internal_malloc(size));
    if (buffer == nullptr) {
        return false;
    }

    std::memset(buffer, 0, size);
    for (int index = 0; index < linesCount; index++) {
        char* beginning = string + lineStarts[index];
        char* ending = string + lineStarts[index + 1];
        if (ending[-1] == ' ') {
            --ending;
        }

        char c = *ending;
        *ending = '\0';

        int lineWidth = fontGetStringWidth(beginning);
        unsigned char* destination = buffer + layout->lineStartOffset(index)
            + (width - lineWidth) / 2;
        if (font == 101 && outlineColor != COLOR_INVALID) {
            ScopedTextCaptureTag barkTag(TextCaptureTag::FloatingCharacterBark,
                static_cast<ColorWithFlags>(outlineColor & COLOR_LAST), layout->sharpLineOffset(index));
            fontDrawText(destination, beginning, width, layout->bufferWidth, color);
        } else {
            fontDrawText(destination, beginning, width, layout->bufferWidth, color);
        }

        *ending = c;
    }

    if (outlineColor != COLOR_INVALID) {
        bufferOutline(buffer, layout->bufferWidth, layout->bufferHeight,
            layout->bufferWidth, static_cast<Color>(outlineColor & COLOR_LAST));
    }

    *data = buffer;
    return true;
}

void floatingTextRenderInRect(const FloatingTextOutlineLayout& layout,
    const unsigned char* data, int x, int y, const Rect& clip,
    unsigned char* destination, int destinationPitch)
{
    if (data == nullptr || destination == nullptr || destinationPitch <= 0) {
        return;
    }

    const TextRect clipped = layout.clipBounds(x, y,
        { clip.left, clip.top, clip.right - clip.left + 1, clip.bottom - clip.top + 1 });
    if (clipped.width <= 0 || clipped.height <= 0) {
        return;
    }

    const std::size_t sourceOffset = layout.sourceOffset(clipped, x, y);
    blitBufferToBufferTrans(data + sourceOffset,
        clipped.width,
        clipped.height,
        layout.bufferWidth,
        destination + destinationPitch * clipped.y + clipped.x,
        destinationPitch);
}

Rect floatingTextRefreshRect(const FloatingTextOutlineLayout& layout,
    int x, int y, int tile)
{
    int sceneOffsetX;
    int sceneOffsetY;
    tileSceneShift(tile, &sceneOffsetX, &sceneOffsetY);
    const TextRect bounds = layout.refreshBounds(x, y, sceneOffsetX, sceneOffsetY);
    return { bounds.x, bounds.y,
        bounds.x + bounds.width - 1, bounds.y + bounds.height - 1 };
}

} // namespace scene
} // namespace fallout
