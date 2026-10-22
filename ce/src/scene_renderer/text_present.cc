#include "text_present.h"

#include <algorithm>
#include <stdio.h>

#include <SDL.h>

#include "../scene/text_capture.h"
#include "../settings.h"
#include "context.h"

namespace fallout {
namespace scene_renderer {
namespace {

bool gSharpTextFailureLogged = false;

PreparedSharpTextFrame& preparedFrameStorage()
{
    static PreparedSharpTextFrame value;
    return value;
}

} // namespace

void disableSharpText(const char* message)
{
    settings.scene3d.sharp_text = false;
    if (gSharpTextFailureLogged) return;
    gSharpTextFailureLogged = true;
    fprintf(stderr, "sharp text: %s; using pixel text\n", message != nullptr ? message : "rendering failed");
    fflush(stderr);
}

bool prepareSharpTextForPresent(SDL_Surface* indexedSurface, SDL_Surface* rgbaSurface, bool enabled)
{
    if (!enabled || !settings.scene3d.sharp_text || indexedSurface == nullptr || rgbaSurface == nullptr) {
        prepareSharpTextFrame(nullptr, 0, 0, 0, nullptr, 0, 0, 0, nullptr, 0, false);
        return false;
    }

    const scene::TextBufferId buffer = textCaptureBufferId(indexedSurface->pixels);
    if (buffer == 0) return false;
    textCaptureRestoration(buffer);
    if (scene::textCaptureRecords(buffer).empty()) {
        prepareSharpTextFrame(nullptr, 0, 0, 0, nullptr, 0, 0, 0, nullptr, 0, false);
        return false;
    }
    if (SDL_BlitSurface(indexedSurface, nullptr, rgbaSurface, nullptr) != 0) {
        disableSharpText("palette expansion failed");
        return false;
    }
    int physicalWidth = 0;
    int physicalHeight = 0;
    if (!sharpTextOutputSize(&physicalWidth, &physicalHeight)) {
        disableSharpText("could not determine output size");
        return false;
    }
    SDL_Palette* palette = indexedSurface->format != nullptr ? indexedSurface->format->palette : nullptr;
    if (palette == nullptr || palette->colors == nullptr || palette->ncolors <= 0) {
        disableSharpText("game palette unavailable");
        return false;
    }
    unsigned char paletteRgb[256 * 3] = {};
    const int paletteEntries = std::min(256, palette->ncolors);
    for (int index = 0; index < paletteEntries; ++index) {
        paletteRgb[index * 3] = palette->colors[index].r;
        paletteRgb[index * 3 + 1] = palette->colors[index].g;
        paletteRgb[index * 3 + 2] = palette->colors[index].b;
    }
    if (!prepareSharpTextFrame(static_cast<const unsigned char*>(indexedSurface->pixels),
            indexedSurface->w, indexedSurface->h, indexedSurface->pitch,
            static_cast<const unsigned char*>(rgbaSurface->pixels), rgbaSurface->pitch,
            physicalWidth, physicalHeight, paletteRgb, paletteEntries, true)) {
        disableSharpText("capture or glyph rendering failed");
        return false;
    }
    const PreparedSharpTextFrame& frame = preparedSharpTextFrame();
    if (!frame.active) return false;
    if (!uploadSharpTextOverlay(frame.overlay.pixels.data(), frame.overlay.width,
            frame.overlay.height, frame.overlay.pitch)) {
        disableSharpText("OpenGL texture upload failed");
        return false;
    }
    return true;
}

bool prepareSharpTextFrame(const unsigned char* indices, int width, int height, int indexPitch,
    const unsigned char* rgba, int rgbaPitch, int physicalWidth, int physicalHeight,
    const unsigned char* paletteRgb, int paletteEntries, bool enabled)
{
    PreparedSharpTextFrame& output = preparedFrameStorage();
    output.active = false;
    if (!enabled) return true;
    if (indices == nullptr || rgba == nullptr || width <= 0 || height <= 0
        || indexPitch < width || rgbaPitch < width * 4 || physicalWidth <= 0 || physicalHeight <= 0
        || paletteRgb == nullptr || paletteEntries <= 0) return false;
    output.width = width;
    output.height = height;
    output.indexPitch = indexPitch;
    output.rgbaPitch = rgbaPitch;
    output.originalIndices.resize(static_cast<std::size_t>(indexPitch) * height);
    output.originalRgba.resize(static_cast<std::size_t>(rgbaPitch) * height);
    output.restoredIndices.resize(static_cast<std::size_t>(indexPitch) * height);
    output.restoredRgba.resize(static_cast<std::size_t>(rgbaPitch) * height);
    for (int y = 0; y < height; ++y) {
        std::copy(indices + static_cast<std::size_t>(y) * indexPitch,
            indices + static_cast<std::size_t>(y) * indexPitch + indexPitch,
            output.originalIndices.data() + static_cast<std::size_t>(y) * indexPitch);
        std::copy(rgba + static_cast<std::size_t>(y) * rgbaPitch,
            rgba + static_cast<std::size_t>(y) * rgbaPitch + rgbaPitch,
            output.originalRgba.data() + static_cast<std::size_t>(y) * rgbaPitch);
        std::copy(output.originalIndices.data() + static_cast<std::size_t>(y) * indexPitch,
            output.originalIndices.data() + static_cast<std::size_t>(y) * indexPitch + indexPitch,
            output.restoredIndices.data() + static_cast<std::size_t>(y) * indexPitch);
        std::copy(output.originalRgba.data() + static_cast<std::size_t>(y) * rgbaPitch,
            output.originalRgba.data() + static_cast<std::size_t>(y) * rgbaPitch + rgbaPitch,
            output.restoredRgba.data() + static_cast<std::size_t>(y) * rgbaPitch);
    }

    const scene::TextBufferId buffer = textCaptureBufferId(indices);
    if (buffer == 0) return false;
    textCaptureRestoration(buffer); // Validate and prune overwritten records against the live indexed buffer.
    const std::vector<scene::TextRecord> records = textCaptureRecords(buffer);
    if (records.empty()) return true;

    std::vector<scene::TextVisibilityMask> masks(records.size());
    for (std::size_t index = 0; index < records.size(); ++index) {
        if (!textCaptureVisibilityMask(records[index], width, height, &masks[index])) return false;
    }
    if (!renderSharpText(records, width, height, physicalWidth, physicalHeight,
            paletteRgb, paletteEntries, masks, &output.overlay)) return false;

    for (std::size_t index = 0; index < records.size(); ++index) {
        const scene::TextRecord& record = records[index];
        const scene::TextVisibilityMask& mask = masks[index];
        for (const scene::TextPixelRecord& pixel : record.pixels) {
            if (pixel.x < 0 || pixel.y < 0 || pixel.x >= width || pixel.y >= height
                || pixel.x >= mask.bounds.x + mask.bounds.width || pixel.y >= mask.bounds.y + mask.bounds.height
                || !mask.visibleAt(pixel.x, pixel.y)
                || indices[static_cast<std::size_t>(pixel.y) * indexPitch + pixel.x] != pixel.after
                || pixel.before >= paletteEntries) continue;
            output.restoredIndices[static_cast<std::size_t>(pixel.y) * indexPitch + pixel.x] = pixel.before;
            const unsigned char* color = paletteRgb + pixel.before * 3;
            unsigned char* rgbaPixel = output.restoredRgba.data()
                + static_cast<std::size_t>(pixel.y) * rgbaPitch + pixel.x * 4;
            rgbaPixel[0] = color[0];
            rgbaPixel[1] = color[1];
            rgbaPixel[2] = color[2];
            rgbaPixel[3] = 255;
        }
    }
    output.active = true;
    return true;
}

const PreparedSharpTextFrame& preparedSharpTextFrame()
{
    return preparedFrameStorage();
}

} // namespace scene_renderer
} // namespace fallout
