#include "text_draw.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <tuple>

#include "embedded_fonts.h"
#include "stb_truetype.h"

#include "../color.h"

namespace fallout {
namespace scene_renderer {
namespace {

unsigned int decodeCp1251(unsigned char value)
{
    static const unsigned short upper[64] = {
        0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
        0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
        0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
        0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
        0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
        0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
        0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457
    };
    if (value < 0x80) return value;
    if (value < 0xC0) return upper[value - 0x80];
    return 0x0410 + value - 0xC0;
}

struct FontVerticalBounds {
    int top = 0;
    int bottom = 0;
    bool valid = false;
};

const FontVerticalBounds& getFontVerticalBounds(bool bold, stbtt_fontinfo* font)
{
    static FontVerticalBounds bounds[2];
    static bool initialized[2] = {};
    const int index = bold ? 1 : 0;
    if (initialized[index]) return bounds[index];
    initialized[index] = true;

    FontVerticalBounds& result = bounds[index];
    result.top = std::numeric_limits<int>::min();
    result.bottom = std::numeric_limits<int>::max();
    for (unsigned int byte = 0x20; byte <= 0xFF; ++byte) {
        if (byte == 0x7F) continue;
        const unsigned int codepoint = decodeCp1251(static_cast<unsigned char>(byte));
        if (codepoint == 0xFFFD || stbtt_FindGlyphIndex(font, static_cast<int>(codepoint)) == 0) continue;
        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;
        if (!stbtt_GetCodepointBox(font, static_cast<int>(codepoint), &x0, &y0, &x1, &y1)
            || (x0 == x1 && y0 == y1)) continue;
        result.top = std::max(result.top, y1);
        result.bottom = std::min(result.bottom, y0);
        result.valid = true;
    }
    return result;
}

struct GlyphBitmap {
    int width = 0;
    int height = 0;
    int xoff = 0;
    int yoff = 0;
    std::vector<unsigned char> pixels;
};

using GlyphKey = std::tuple<unsigned int, unsigned int, unsigned int, int>;

std::map<GlyphKey, GlyphBitmap>& glyphCache()
{
    static std::map<GlyphKey, GlyphBitmap> value;
    return value;
}

std::size_t& glyphCacheBytes()
{
    static std::size_t value = 0;
    return value;
}

bool& glyphCacheResetPending()
{
    static bool value = false;
    return value;
}


void beginGlyphFrame()
{
    // Clear only before any positioned glyph pointers are taken. The caps
    // below also bound growth within a single unusually varied frame.
    if (glyphCacheResetPending() || glyphCache().size() >= 4096 || glyphCacheBytes() >= 8 * 1024 * 1024) {
        glyphCache().clear();
        glyphCacheBytes() = 0;
        glyphCacheResetPending() = false;
    }
}

stbtt_fontinfo* getFont(bool bold)
{
    static stbtt_fontinfo fonts[2] = {};
    static bool initialized[2] = {};
    static bool valid[2] = {};
    const int index = bold ? 1 : 0;
    if (!initialized[index]) {
        initialized[index] = true;
        const EmbeddedFont embedded = embeddedFont(bold ? InterfaceFontWeight::Bold : InterfaceFontWeight::SemiBold);
        valid[index] = embedded.data != nullptr && embedded.size > 0
            && stbtt_InitFont(&fonts[index], embedded.data, 0) != 0;
    }
    return valid[index] ? &fonts[index] : nullptr;
}

unsigned int floatBits(float value)
{
    unsigned int bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

const GlyphBitmap* getGlyph(unsigned int codepoint, float scaleX, float scaleY, bool bold)
{
    auto& glyphs = glyphCache();
    const GlyphKey key { codepoint, floatBits(scaleX), floatBits(scaleY), bold ? 104 : 101 };
    auto found = glyphs.find(key);
    if (found != glyphs.end()) return &found->second;
    stbtt_fontinfo* font = getFont(bold);
    if (font == nullptr) return nullptr;
    int width = 0;
    int height = 0;
    int xoff = 0;
    int yoff = 0;
    unsigned char* bitmap = stbtt_GetCodepointBitmap(font, scaleX, scaleY,
        static_cast<int>(codepoint), &width, &height, &xoff, &yoff);
    GlyphBitmap value;
    value.width = width;
    value.height = height;
    value.xoff = xoff;
    value.yoff = yoff;
    if (bitmap != nullptr && width > 0 && height > 0) {
        value.pixels.assign(bitmap, bitmap + static_cast<std::size_t>(width) * height);
        stbtt_FreeBitmap(bitmap, nullptr);
    } else if (bitmap != nullptr) {
        stbtt_FreeBitmap(bitmap, nullptr);
    }
    const std::size_t bytes = value.pixels.size();
    if (glyphs.size() >= 4096 || bytes > 8 * 1024 * 1024 - glyphCacheBytes()) {
        glyphCacheResetPending() = true;
        return nullptr;
    }
    glyphCacheBytes() += bytes;
    return &glyphs.emplace(key, std::move(value)).first->second;
}

void blendPixel(unsigned char* pixel, const unsigned char* rgb, unsigned char alpha)
{
    if (alpha == 0) return;
    const unsigned int inverse = 255 - alpha;
    pixel[0] = static_cast<unsigned char>((rgb[0] * alpha + pixel[0] * inverse + 127) / 255);
    pixel[1] = static_cast<unsigned char>((rgb[1] * alpha + pixel[1] * inverse + 127) / 255);
    pixel[2] = static_cast<unsigned char>((rgb[2] * alpha + pixel[2] * inverse + 127) / 255);
    pixel[3] = static_cast<unsigned char>(alpha + (pixel[3] * inverse + 127) / 255);
}

bool equalsAsciiCaseInsensitive(const std::string& value, const char* expected)
{
    const std::size_t expectedSize = std::strlen(expected);
    if (value.size() != expectedSize) return false;
    for (std::size_t index = 0; index < expectedSize; index++) {
        unsigned char character = static_cast<unsigned char>(value[index]);
        if (character >= 'A' && character <= 'Z') character = static_cast<unsigned char>(character - 'A' + 'a');
        if (character != static_cast<unsigned char>(expected[index])) return false;
    }
    return true;
}

void drawGlyph(SharpTextSurface& surface, const GlyphBitmap& glyph, int x, int baseline,
    int offsetX, int offsetY, const unsigned char* rgb, const scene::TextVisibilityMask& visibility,
    int logicalWidth, int logicalHeight, double sx, double sy)
{
    for (int by = 0; by < glyph.height; ++by) {
            const int py = baseline + glyph.yoff + by + offsetY;
        if (py < 0 || py >= surface.height) continue;
        const int logicalY = std::min(logicalHeight - 1, static_cast<int>(py / sy));
        for (int bx = 0; bx < glyph.width; ++bx) {
            const int px = x + glyph.xoff + bx + offsetX;
            if (px < 0 || px >= surface.width) continue;
            const int logicalX = std::min(logicalWidth - 1, static_cast<int>(px / sx));
            if (!visibility.visibleAt(logicalX, logicalY)) continue;
            const unsigned char coverage = glyph.pixels[static_cast<std::size_t>(by) * glyph.width + bx];
            if (coverage == 0) continue;
            blendPixel(surface.pixels.data() + static_cast<std::size_t>(py) * surface.pitch + px * 4, rgb, coverage);
        }
    }
}

} // namespace

bool sharpTextLanguageSupported(const std::string& language)
{
    // This renderer decodes captured bytes as CP1251. Do not erase original
    // text in locales that require another legacy encoding.
    return equalsAsciiCaseInsensitive(language, "english")
        || equalsAsciiCaseInsensitive(language, "russian");
}

bool renderSharpText(const std::vector<scene::TextRecord>& records,
    int logicalWidth, int logicalHeight, int physicalWidth, int physicalHeight,
    const unsigned char* paletteRgb, int paletteEntries,
    const std::vector<scene::TextVisibilityMask>& logicalVisibility, SharpTextSurface* output)
{
    if (output == nullptr || logicalWidth <= 0 || logicalHeight <= 0 || physicalWidth <= 0 || physicalHeight <= 0
        || paletteRgb == nullptr || paletteEntries <= 0 || logicalVisibility.size() != records.size()) return false;
    beginGlyphFrame();
    output->width = physicalWidth;
    output->height = physicalHeight;
    output->pitch = physicalWidth * 4;
    output->pixels.assign(static_cast<std::size_t>(output->pitch) * physicalHeight, 0);
    const double sx = static_cast<double>(physicalWidth) / logicalWidth;
    const double sy = static_cast<double>(physicalHeight) / logicalHeight;
    const unsigned char black[3] = { 0, 0, 0 };

    for (std::size_t recordIndex = 0; recordIndex < records.size(); ++recordIndex) {
        const scene::TextRecord& record = records[recordIndex];
        const scene::TextRunInfo& run = record.run;
        const scene::TextVisibilityMask& visibility = logicalVisibility[recordIndex];
        if (visibility.pixels.size() != static_cast<std::size_t>(visibility.bounds.width) * visibility.bounds.height
            || run.text.empty() || run.lineHeight <= 0 || run.width <= 0) continue;
        const bool bold = run.font == 103 || run.font == 104;
        stbtt_fontinfo* font = getFont(bold);
        if (font == nullptr) return false;
        const FontVerticalBounds& verticalBounds = getFontVerticalBounds(bold, font);
        if (!verticalBounds.valid) continue;
        const int fitHeight = verticalBounds.top - verticalBounds.bottom;
        if (fitHeight <= 0) continue;
        // Leave half a logical pixel for bitmap rounding at both line edges.
        const double fitLineHeight = std::max(1.0, static_cast<double>(run.lineHeight) - 0.5);
        const double logicalScale = fitLineHeight / fitHeight;
        const double fontScaleX = logicalScale * sx;
        const double fontScaleY = logicalScale * sy;
        const int baseline = static_cast<int>(std::lround((run.y + verticalBounds.top * logicalScale) * sy));

        std::vector<unsigned int> codepoints;
        std::vector<double> advances;
        double naturalWidth = 0.0;
        for (unsigned char character : run.text) {
            const unsigned int codepoint = decodeCp1251(character);
            int advance = 0;
            int bearing = 0;
            stbtt_GetCodepointHMetrics(font, static_cast<int>(codepoint), &advance, &bearing);
            codepoints.push_back(codepoint);
            const double logicalAdvance = advance * logicalScale;
            advances.push_back(logicalAdvance);
            naturalWidth += logicalAdvance;
        }
        const bool monospaced = (run.colorAndFlags & DRAW_TEXT_FLAG_MONOSPACED) != 0
            && run.characterCellX.size() == codepoints.size() && run.cellWidth > 0;
        const double proportionalCondense = naturalWidth <= 0.0 ? 1.0 : std::min(1.0, run.width / naturalWidth);
        const unsigned int colorIndex = static_cast<unsigned int>(run.colorAndFlags) & 0xFF;
        if (colorIndex >= static_cast<unsigned int>(paletteEntries)) continue;
        const unsigned char* rgb = paletteRgb + colorIndex * 3;
        struct PositionedGlyph {
            const GlyphBitmap* bitmap;
            int x;
        };
        std::vector<PositionedGlyph> positioned;
        positioned.reserve(codepoints.size());
        double cursorX = run.x;
        for (std::size_t index = 0; index < codepoints.size(); ++index) {
            int advanceUnits = 0;
            int bearingUnits = 0;
            stbtt_GetCodepointHMetrics(font, static_cast<int>(codepoints[index]), &advanceUnits, &bearingUnits);
            const double logicalAdvance = advances[index];
            const double cellCondense = monospaced && logicalAdvance > run.cellWidth
                ? run.cellWidth / logicalAdvance : 1.0;
            int anchorX = 0;
            if (monospaced) {
                const double glyphAdvance = logicalAdvance * cellCondense;
                anchorX = static_cast<int>(std::lround((run.characterCellX[index]
                    + (run.cellWidth - glyphAdvance) * 0.5) * sx));
            } else {
                anchorX = static_cast<int>(std::lround(cursorX * sx));
            }
            const double condense = monospaced ? cellCondense : proportionalCondense;
            const GlyphBitmap* glyph = getGlyph(codepoints[index], static_cast<float>(fontScaleX * condense),
                static_cast<float>(fontScaleY), bold);
            if (glyph == nullptr) return false;
            positioned.push_back({ glyph, anchorX });
            if (!monospaced) cursorX += logicalAdvance * proportionalCondense;
        }
        const bool shadowed = (run.colorAndFlags & DRAW_TEXT_FLAG_SHADOWED) != 0;
        if (shadowed) {
            const int shadowX = std::max(1, static_cast<int>(std::lround(sx)));
            const int shadowY = std::max(1, static_cast<int>(std::lround(sy)));
            for (const PositionedGlyph& glyph : positioned) {
                if (!glyph.bitmap->pixels.empty()) drawGlyph(*output, *glyph.bitmap, glyph.x, baseline, shadowX, shadowY,
                    black, visibility, logicalWidth, logicalHeight, sx, sy);
            }
        }
        for (const PositionedGlyph& glyph : positioned) {
            if (!glyph.bitmap->pixels.empty()) drawGlyph(*output, *glyph.bitmap, glyph.x, baseline, 0, 0,
                rgb, visibility, logicalWidth, logicalHeight, sx, sy);
        }
        if ((run.colorAndFlags & DRAW_TEXT_FLAG_UNDERLINED) != 0) {
            const int y = static_cast<int>(std::lround((run.y + run.lineHeight - 1) * sy));
            const int left = std::max(0, static_cast<int>(std::lround(run.x * sx)));
            const int right = std::min(physicalWidth, static_cast<int>(std::lround((run.x + run.width) * sx)));
            for (int px = left; px < right && y >= 0 && y < physicalHeight; ++px) {
                const int logicalX = std::min(logicalWidth - 1, static_cast<int>(px / sx));
                const int logicalY = std::min(logicalHeight - 1, static_cast<int>(y / sy));
                if (visibility.visibleAt(logicalX, logicalY)) {
                    blendPixel(output->pixels.data() + static_cast<std::size_t>(y) * output->pitch + px * 4, rgb, 255);
                }
            }
        }
    }
    return true;
}

} // namespace scene_renderer
} // namespace fallout
