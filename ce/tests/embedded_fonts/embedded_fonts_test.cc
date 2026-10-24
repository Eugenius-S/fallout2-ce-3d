#include "stb_truetype.h"
#include "embedded_fonts.h"
#include "scene_renderer/text_draw.h"
#include "color.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

bool checkFont(const unsigned char* data, std::size_t size)
{
    if (data == nullptr || size < 1024 || stbtt_GetNumberOfFonts(data) != 1) return false;
    stbtt_fontinfo font = {};
    const int offset = stbtt_GetFontOffsetForIndex(data, 0);
    if (offset < 0 || !stbtt_InitFont(&font, data, offset)) return false;

    // W, Cyrillic Zhe/yo/Shcha, question mark and every decimal digit.
    const int codepoints[] = { 'W', 0x0416, 0x0451, 0x0429, '?',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    const float scale = stbtt_ScaleForPixelHeight(&font, 20.0f);
    for (int codepoint : codepoints) {
        if (stbtt_FindGlyphIndex(&font, codepoint) == 0) return false;
        int width = 0, height = 0;
        unsigned char* pixels = stbtt_GetCodepointBitmap(&font, scale, scale, codepoint,
            &width, &height, nullptr, nullptr);
        const bool visible = pixels != nullptr && width > 0 && height > 0
            && std::any_of(pixels, pixels + width * height, [](unsigned char value) { return value != 0; });
        stbtt_FreeBitmap(pixels, nullptr);
        if (!visible) return false;
    }
    return true;
}

int main()
{
    using namespace fallout::scene_renderer;
    const char* supportedLanguages[] = { "english", "russian", "English", "RUSSIAN" };
    const char* unsupportedLanguages[] = { "french", "german", "italian", "spanish", "unknown", "" };
    for (const char* language : supportedLanguages) {
        if (!sharpTextLanguageSupported(language)) {
            std::fprintf(stderr, "Supported sharp-text language was rejected: %s\n", language);
            return 1;
        }
    }
    for (const char* language : unsupportedLanguages) {
        if (sharpTextLanguageSupported(language)) {
            std::fprintf(stderr, "Unsupported sharp-text language was accepted: %s\n", language);
            return 1;
        }
    }
    const EmbeddedFont semiBold = embeddedFont(InterfaceFontWeight::SemiBold);
    const EmbeddedFont bold = embeddedFont(InterfaceFontWeight::Bold);
    const EmbeddedFont unknown = embeddedFont(static_cast<InterfaceFontWeight>(-1));
    if (semiBold.data == bold.data || unknown.data != nullptr || unknown.size != 0
        || !checkFont(semiBold.data, semiBold.size) || !checkFont(bold.data, bold.size)) {
        std::fprintf(stderr, "Embedded font initialization, glyph lookup or rasterization failed\n");
        return 1;
    }
    fallout::scene::TextRecord record;
    record.run.text = std::string("\xA8 \xC6", 3); // CP1251: Yo, space, Zhe.
    record.run.font = 103;
    record.run.colorAndFlags = 255;
    record.run.x = 2;
    record.run.y = 2;
    record.run.lineHeight = 18;
    stbtt_fontinfo boldInfo = {};
    stbtt_InitFont(&boldInfo, bold.data, 0);
    int capMinY = 0, capTop = 0, descBottom = 0, descMaxY = 0;
    stbtt_GetCodepointBox(&boldInfo, 'H', nullptr, &capMinY, nullptr, &capTop);
    stbtt_GetCodepointBox(&boldInfo, 'g', nullptr, &descBottom, nullptr, &descMaxY);
    const double scale = 18.0 / (capTop - descBottom);
    int firstAdvance = 0, spaceAdvance = 0, lastAdvance = 0, bearing = 0;
    stbtt_GetCodepointHMetrics(&boldInfo, 0x0401, &firstAdvance, &bearing);
    stbtt_GetCodepointHMetrics(&boldInfo, ' ', &spaceAdvance, &bearing);
    stbtt_GetCodepointHMetrics(&boldInfo, 0x0416, &lastAdvance, &bearing);
    record.run.width = static_cast<int>(std::ceil((firstAdvance + spaceAdvance + lastAdvance) * scale)) + 8;
    record.run.characterX = { 2, 15, 28 };
    std::vector<fallout::scene::TextVisibilityMask> visibility(1);
    visibility[0].bounds = { 0, 0, 256, 32 };
    visibility[0].pixels.assign(256 * 32, 1);
    unsigned char palette[256 * 3] = {};
    palette[255 * 3] = 255;
    palette[255 * 3 + 1] = 255;
    palette[255 * 3 + 2] = 255;
    SharpTextSurface surface;
    if (!renderSharpText({ record }, 256, 32, 512, 64, palette, 256, visibility, &surface)
        || std::none_of(surface.pixels.begin(), surface.pixels.end(), [](unsigned char value) { return value != 0; })) {
        std::fprintf(stderr, "CP1251 sharp text did not rasterize\n");
        return 1;
    }
    fallout::scene::TextRecord coverageOnly = record;
    coverageOnly.run.text = "A";
    coverageOnly.run.font = 101;
    coverageOnly.run.x = 32;
    coverageOnly.run.y = 2;
    coverageOnly.run.width = 32;
    coverageOnly.pixels.clear();
    coverageOnly.coverage.clear();
    for (int y = 2; y < 21; ++y) {
        for (int x = 32; x < 64; ++x) coverageOnly.coverage.push_back({ x, y, 0 });
    }
    visibility[0].bounds = { 32, 2, 32, 19 };
    visibility[0].pixels.assign(32 * 19, 1);
    SharpTextSurface coverageSurface;
    if (!renderSharpText({ coverageOnly }, 256, 32, 512, 64, palette, 256, visibility, &coverageSurface)
        || std::none_of(coverageSurface.pixels.begin(), coverageSurface.pixels.end(), [](unsigned char value) { return value != 0; })) {
        std::fprintf(stderr, "Coverage-only sharp text fragment did not rasterize\n");
        return 1;
    }
    visibility[0].bounds = { 0, 0, 256, 32 };
    visibility[0].pixels.assign(256 * 32, 1);
    bool adjacentCoverage = false;
    int rightmost = -1;
    for (int y = 0; y < surface.height; ++y) {
        for (int x = 0; x < surface.width; ++x) {
            const unsigned char* pixel = surface.pixels.data() + (static_cast<size_t>(y) * surface.width + x) * 4;
            if (pixel[3] != 0) {
                rightmost = std::max(rightmost, x);
                if ((x + 1 < surface.width && pixel[3] != 0 && pixel[7] != 0)
                    || (y + 1 < surface.height && pixel[3] != 0
                        && surface.pixels[(static_cast<size_t>(y + 1) * surface.width + x) * 4 + 3] != 0)) {
                    adjacentCoverage = true;
                }
            }
        }
    }
    if (!adjacentCoverage) {
        std::fprintf(stderr, "Physical glyph bitmap has no adjacent covered pixels\n");
        return 1;
    }
    fallout::scene::TextRecord noSpace = record;
    noSpace.run.text = std::string("\xA8\xC6", 2);
    noSpace.run.characterX = { 2, 15 };
    if (!renderSharpText({ noSpace }, 256, 32, 512, 64, palette, 256, visibility, &surface)) return 1;
    int noSpaceRightmost = -1;
    for (int x = 0; x < surface.width; ++x) {
        for (int y = 0; y < surface.height; ++y) {
            if (surface.pixels[(static_cast<size_t>(y) * surface.width + x) * 4 + 3] != 0) noSpaceRightmost = x;
        }
    }
    if (rightmost <= noSpaceRightmost) {
        std::fprintf(stderr, "Space did not advance the next glyph (%d <= %d)\n", rightmost, noSpaceRightmost);
        return 1;
    }
    for (size_t index = 0; index < surface.pixels.size(); index += 4) {
        if (surface.pixels[index + 3] != 0) {
            const size_t pixel = index / 4;
            const int px = static_cast<int>(pixel % surface.width);
            const int py = static_cast<int>(pixel / surface.width);
            std::fill(visibility[0].pixels.begin(), visibility[0].pixels.end(), 1);
            visibility[0].pixels[static_cast<size_t>(py / 2) * 256 + px / 2] = 0;
            if (!renderSharpText({ record }, 256, 32, 512, 64, palette, 256, visibility, &surface)
                || surface.pixels[index + 3] != 0) {
                std::fprintf(stderr, "Per-record mask failed to clip a physical glyph pixel\n");
                return 1;
            }
            break;
        }
    }
    record.run.font = 103;
    std::fill(visibility[0].pixels.begin(), visibility[0].pixels.end(), 1);
    if (!renderSharpText({ record }, 256, 32, 512, 64, palette, 256, visibility, &surface)) {
        std::fprintf(stderr, "Font 103 did not rasterize\n");
        return 1;
    }
    const std::vector<unsigned char> font103Pixels = surface.pixels;
    record.run.font = 104;
    if (!renderSharpText({ record }, 256, 32, 512, 64, palette, 256, visibility, &surface)
        || surface.pixels != font103Pixels) {
        std::fprintf(stderr, "Fonts 103 and 104 do not share the bold face\n");
        return 1;
    }
    record.run.font = 101;
    if (!renderSharpText({ record }, 256, 32, 512, 64, palette, 256, visibility, &surface)
        || surface.pixels == font103Pixels) {
        std::fprintf(stderr, "Regular font unexpectedly uses the bold face\n");
        return 1;
    }

    fallout::scene::TextRecord fitRecord = record;
    fitRecord.run.text = "Hg";
    fitRecord.run.x = 4;
    fitRecord.run.y = 2;
    fitRecord.run.width = 40;
    fitRecord.run.lineHeight = 18;
    fitRecord.run.colorAndFlags = 255;
    if (!renderSharpText({ fitRecord }, 256, 32, 512, 64, palette, 256, visibility, &surface)) return 1;
    int top = surface.height;
    int bottom = -1;
    for (int y = 0; y < surface.height; ++y) {
        for (int x = 0; x < surface.width; ++x) {
            if (surface.pixels[(static_cast<size_t>(y) * surface.width + x) * 4 + 3] != 0) {
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
    }
    if (top < fitRecord.run.y * 2 || bottom < top
        || bottom >= (fitRecord.run.y + fitRecord.run.lineHeight) * 2) {
        std::fprintf(stderr, "CP1251 repertoire fit is outside the captured line: %d..%d\n", top, bottom);
        return 1;
    }

    fitRecord.run.font = 101;
    fitRecord.run.text = std::string("\xC8", 1); // CP1251: capital I.
    fitRecord.run.y = 2;
    fitRecord.run.lineHeight = 9; // Production font-101 reply line height.
    std::vector<fallout::scene::TextVisibilityMask> fullLineVisibility(1);
    fullLineVisibility[0].bounds = { 0, 0, 256, 32 };
    fullLineVisibility[0].pixels.assign(256 * 32, 1);
    std::vector<fallout::scene::TextVisibilityMask> clippedLineVisibility = fullLineVisibility;
    std::fill(clippedLineVisibility[0].pixels.begin(), clippedLineVisibility[0].pixels.end(), 0);
    for (int y = fitRecord.run.y; y < fitRecord.run.y + fitRecord.run.lineHeight; ++y) {
        std::fill(clippedLineVisibility[0].pixels.begin() + y * 256,
            clippedLineVisibility[0].pixels.begin() + (y + 1) * 256, 1);
    }
    for (int scale : { 2, 3 }) {
        auto renderLine = [&](const char* bytes, std::size_t length, SharpTextSurface* full, SharpTextSurface* clipped) {
            fitRecord.run.text.assign(bytes, length);
            return renderSharpText({ fitRecord }, 256, 32, 256 * scale, 32 * scale,
                    palette, 256, fullLineVisibility, full)
                && renderSharpText({ fitRecord }, 256, 32, 256 * scale, 32 * scale,
                    palette, 256, clippedLineVisibility, clipped)
                && full->pixels == clipped->pixels;
        };
        SharpTextSurface iFull, iClipped, shortIFull, shortIClipped;
        SharpTextSurface eFull, eClipped, diaeresisFull, diaeresisClipped;
        if (!renderLine("\xC8", 1, &iFull, &iClipped)
            || !renderLine("\xC9", 1, &shortIFull, &shortIClipped)
            || !renderLine("\xC5", 1, &eFull, &eClipped)
            || !renderLine("\xA8", 1, &diaeresisFull, &diaeresisClipped)
            || iFull.pixels == shortIFull.pixels || eFull.pixels == diaeresisFull.pixels) {
            std::fprintf(stderr, "CP1251 diacritics were clipped or collapsed at scale %d\n", scale);
            return 1;
        }
    }
    fitRecord.run.text = "Hg";
    fitRecord.run.lineHeight = 18;

    fitRecord.run.colorAndFlags = 43;
    palette[43 * 3] = 0;
    palette[43 * 3 + 1] = 255;
    palette[43 * 3 + 2] = 0;
    if (!renderSharpText({ fitRecord }, 256, 32, 512, 64, palette, 256, visibility, &surface)) return 1;
    const std::vector<unsigned char> greenPixels = surface.pixels;
    palette[43 * 3 + 1] = 0;
    if (!renderSharpText({ fitRecord }, 256, 32, 512, 64, palette, 256, visibility, &surface)) return 1;
    bool checkedFadeCoverage = false;
    for (std::size_t index = 0; index < surface.pixels.size(); index += 4) {
        if (greenPixels[index] != 0 || greenPixels[index + 1] != greenPixels[index + 3]
            || greenPixels[index + 2] != 0 || greenPixels[index + 3] != surface.pixels[index + 3]
            || surface.pixels[index] != 0 || surface.pixels[index + 1] != 0 || surface.pixels[index + 2] != 0) {
            std::fprintf(stderr, "Palette fade changed glyph coverage or retained RGB ink\n");
            return 1;
        }
        checkedFadeCoverage = checkedFadeCoverage || greenPixels[index + 3] != 0;
    }
    if (!checkedFadeCoverage) {
        std::fprintf(stderr, "Palette fade test found no glyph coverage\n");
        return 1;
    }
    palette[43 * 3] = 255;
    palette[43 * 3 + 1] = 255;
    palette[43 * 3 + 2] = 255;
    fitRecord.run.colorAndFlags = 255;

    const std::vector<unsigned char> foregroundPixels = surface.pixels;
    fitRecord.run.colorAndFlags = static_cast<unsigned int>(255 | fallout::DRAW_TEXT_FLAG_SHADOWED);
    if (!renderSharpText({ fitRecord }, 256, 32, 512, 64, palette, 256, visibility, &surface)) return 1;
    bool checkedOpaqueForeground = false;
    for (size_t index = 0; index < surface.pixels.size(); index += 4) {
        if (foregroundPixels[index + 3] == 255) {
            checkedOpaqueForeground = true;
            if (surface.pixels[index] != 255 || surface.pixels[index + 1] != 255 || surface.pixels[index + 2] != 255) {
                std::fprintf(stderr, "Shadow pass darkened a fully covered foreground pixel\n");
                return 1;
            }
        }
    }
    if (!checkedOpaqueForeground) {
        std::fprintf(stderr, "Shadow test found no fully covered foreground pixel\n");
        return 1;
    }
    std::fill(visibility[0].pixels.begin(), visibility[0].pixels.end(), 0);
    if (!renderSharpText({ record }, 256, 32, 512, 64, palette, 256, visibility, &surface)
        || std::any_of(surface.pixels.begin(), surface.pixels.end(), [](unsigned char value) { return value != 0; })) {
        std::fprintf(stderr, "Sharp text escaped the visibility mask\n");
        return 1;
    }
    std::printf("Both embedded fonts load; all 30 required glyphs rasterize\n");
    return 0;
}
