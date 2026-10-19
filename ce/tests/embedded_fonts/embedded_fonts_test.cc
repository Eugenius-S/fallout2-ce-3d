#include "stb_truetype.h"
#include "embedded_fonts.h"

#include <algorithm>
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
    const EmbeddedFont semiBold = embeddedFont(InterfaceFontWeight::SemiBold);
    const EmbeddedFont bold = embeddedFont(InterfaceFontWeight::Bold);
    const EmbeddedFont unknown = embeddedFont(static_cast<InterfaceFontWeight>(-1));
    if (semiBold.data == bold.data || unknown.data != nullptr || unknown.size != 0
        || !checkFont(semiBold.data, semiBold.size) || !checkFont(bold.data, bold.size)) {
        std::fprintf(stderr, "Embedded font initialization, glyph lookup or rasterization failed\n");
        return 1;
    }
    std::printf("Both embedded fonts load; all 30 required glyphs rasterize\n");
    return 0;
}
