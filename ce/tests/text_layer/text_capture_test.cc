#include "scene/text_capture.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "text_font.h"

namespace fallout {

FontManagerDrawTextProc* fontDrawTextPtr = nullptr;
FontManagerGetLineHeightProc* fontGetLineHeight = nullptr;
FontManagerGetCharacterWidthProc* fontGetCharacterWidth = nullptr;
FontManagerGetLetterSpacingProc* fontGetLetterSpacing = nullptr;
FontManagerGetMonospacedCharacterWidth* fontGetMonospacedCharacterWidth = nullptr;
int gCurrentFont = 101;
Color _colorTable[COLOR_PALETTE_SIZE_15BIT] = {};
TextWindowSnapshot gTestWindow = {};
TextCursorSnapshot gTestCursor = {};

int fakeLineHeight() { return 2; }
int fakePreviewLineHeight() { return 9; }
int fakeCharacterWidth(int) { return 1; }
int fakeLetterSpacing() { return 0; }
int fakeMonospacedWidth() { return 1; }
int wideCharacterWidth(int character) { return character == 'w' ? 3 : 1; }
int fivePixelMonospacedWidth() { return 5; }

int fontGetCurrent()
{
    return gCurrentFont;
}

int fontGetCurrentGlyphCount()
{
    return 256;
}

bool textCaptureGetWindowSnapshot(int index, TextWindowSnapshot* snapshot, int* count)
{
    if (count != nullptr) *count = 1;
    if (index != 0 || snapshot == nullptr) return false;
    *snapshot = gTestWindow;
    return true;
}

bool textCaptureGetCursorSnapshot(TextCursorSnapshot* state)
{
    if (state == nullptr) return false;
    *state = gTestCursor;
    return true;
}

void drawTwoGlyphs(unsigned char* buffer, const char* string, int length, int pitch, ColorWithFlags color)
{
    if (string == nullptr || length < 2 || pitch < 2) return;
    buffer[0] = static_cast<unsigned char>(color & 0xFF);
    buffer[1] = static_cast<unsigned char>((color & 0xFF) + 1);
}

} // namespace fallout

namespace {

int drawCalls = 0;
int copyCalls = 0;
int failures = 0;

void check(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

void fakeFontDraw(unsigned char* buffer, const char* string, int length, int pitch, fallout::ColorWithFlags color)
{
    drawCalls++;
    if ((color & fallout::DRAW_TEXT_FLAG_SHADOWED) != 0) {
        fallout::fontDrawText(buffer + pitch + 1, string, length, pitch,
            static_cast<fallout::ColorWithFlags>(color & ~fallout::DRAW_TEXT_FLAG_SHADOWED));
    }
    if (string != nullptr && *string != '\0' && length > 0) buffer[0] = static_cast<unsigned char>(color & 0xFF);
}

void fakeCopy(unsigned char* destination, int destinationPitch, const unsigned char* source, int sourcePitch, int width, int height)
{
    copyCalls++;
    for (int y = 0; y < height; y++) {
        std::memmove(destination, source, width);
        destination += destinationPitch;
        source += sourcePitch;
    }
}

void fakeTransparentCopy(unsigned char* destination, int destinationPitch, const unsigned char* source, int sourcePitch, int width, int height)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (source[x] != 0) destination[x] = source[x];
        }
        destination += destinationPitch;
        source += sourcePitch;
    }
}

void countingTransparentCopy(unsigned char* destination, int destinationPitch, const unsigned char* source, int sourcePitch, int width, int height)
{
    copyCalls++;
    fakeTransparentCopy(destination, destinationPitch, source, sourcePitch, width, height);
}

void transparentCopyThenZeroSource(unsigned char* destination, int destinationPitch, const unsigned char* source, int sourcePitch, int width, int height)
{
    countingTransparentCopy(destination, destinationPitch, source, sourcePitch, width, height);
    auto* mutableSource = const_cast<unsigned char*>(source);
    for (int y = 0; y < height; y++) {
        std::fill(mutableSource, mutableSource + width, 0);
        mutableSource += sourcePitch;
    }
}

void zeroSourceThenTransparentCopy(unsigned char* destination, int destinationPitch, const unsigned char* source, int sourcePitch, int width, int height)
{
    copyCalls++;
    auto* mutableSource = const_cast<unsigned char*>(source);
    for (int y = 0; y < height; y++) {
        std::fill(mutableSource, mutableSource + width, 0);
        mutableSource += sourcePitch;
    }
    fakeTransparentCopy(destination, destinationPitch, source, sourcePitch, width, height);
}

void testAllocatorGenerationAndShadowGuard()
{
    using namespace fallout;
    std::vector<unsigned char> pixels(32, 2);
    textCaptureAllocationCreated(pixels.data(), pixels.size());
    textCaptureMarkBufferOwned(pixels.data());
    const scene::TextBufferId first = textCaptureBufferId(pixels.data());
    fontDrawTextPtr = fakeFontDraw;
    drawCalls = 0;
    fontDrawText(pixels.data() + 1, "ab", 8, 8,
        static_cast<ColorWithFlags>(5 | DRAW_TEXT_FLAG_SHADOWED));
    const auto records = scene::textCaptureRecords(first);
    check(records.size() == 1, "recursive shadow is captured as one outer run");
    check(records.size() == 1 && records.front().run.text == "ab", "accepted text is retained");
    check(records.size() == 1 && records.front().run.font == 101, "active font is retained");
    check(records.size() == 1 && records.front().run.lineHeight == 2, "line height is retained");
    check(drawCalls == 2, "shadow and main draw both reach the original proc");
    textCaptureAllocationResized(pixels.data(), nullptr, pixels.size() * 2);
    check(textCaptureBufferId(pixels.data()) == first, "failed realloc preserves its live generation");
    check(scene::textCaptureRecords(first).size() == 1, "failed realloc preserves its text records");
    textCaptureAllocationResized(pixels.data(), pixels.data(), pixels.size() * 2);
    const scene::TextBufferId resized = textCaptureBufferId(pixels.data());
    check(resized != first, "successful same-address realloc creates a new generation");
    check(scene::textCaptureRecords(first).empty(), "successful realloc clears old records");
    textCaptureAllocationDestroyed(pixels.data());
    textCaptureAllocationCreated(pixels.data(), pixels.size());
    const scene::TextBufferId reused = textCaptureBufferId(pixels.data());
    check(reused != first && reused != resized, "freed address reuse creates a new generation");
    textCaptureAllocationDestroyed(pixels.data());
}

void testFont101SharpEligibility()
{
    using namespace fallout;
    _colorTable[rgb555(0x00FF00)] = static_cast<Color>(43);
    _colorTable[rgb555(0xFFFFFF)] = static_cast<Color>(44);
    _colorTable[rgb555(0xFFFF5A)] = static_cast<Color>(45);
    _colorTable[rgb555(0xFF0000)] = static_cast<Color>(46);
    std::vector<unsigned char> source(32, 1);
    std::vector<unsigned char> destination(32, 2);
    textCaptureRegisterExternalBuffer(source.data(), source.size(), 8, 4, 8, false);
    textCaptureRegisterExternalBuffer(destination.data(), destination.size(), 8, 4, 8, false);
    textCaptureMarkBufferOwned(source.data());
    textCaptureMarkBufferOwned(destination.data());
    const scene::TextBufferId sourceId = textCaptureBufferId(source.data());
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());
    fontDrawTextPtr = drawTwoGlyphs;
    gCurrentFont = 101;
    const ColorWithFlags font101Colors[] = {
        static_cast<ColorWithFlags>(COLOR_GREEN | DRAW_TEXT_FLAG_NO_BG),
        static_cast<ColorWithFlags>(COLOR_WHITE),
        static_cast<ColorWithFlags>(COLOR_LIGHT_YELLOW),
        static_cast<ColorWithFlags>(COLOR_RED),
        static_cast<ColorWithFlags>(COLOR_GREEN | DRAW_TEXT_FLAG_REFRESH),
    };
    for (std::size_t index = 0; index < std::size(font101Colors); index++) {
        fontDrawText(source.data() + index * 2, "ab", 8, 8, font101Colors[index]);
    }
    const int otherFonts[] = { 102, 103, 104 };
    for (std::size_t index = 0; index < std::size(otherFonts); index++) {
        gCurrentFont = otherFonts[index];
        fontDrawText(source.data() + 10 + index * 2, "ab", 8, 8,
            static_cast<ColorWithFlags>(COLOR_GREEN));
    }

    const auto rawRecords = scene::textCaptureRecords(sourceId);
    const auto sharpRecords = scene::textCaptureSharpRecords(sourceId);
    const auto sharpRestore = scene::textCaptureSharpRestoration(sourceId);
    check(rawRecords.size() == 8, "raw diagnostics retain all captured font IDs");
    check(sharpRecords.size() == 5, "every font-101 palette color is eligible without a conversation tag");
    bool allColorsPreserved = sharpRecords.size() == std::size(font101Colors);
    for (std::size_t index = 0; index < sharpRecords.size() && allColorsPreserved; index++) {
        allColorsPreserved = sharpRecords[index].run.font == 101
            && sharpRecords[index].run.sharpEligible
            && sharpRecords[index].run.colorAndFlags == static_cast<unsigned int>(font101Colors[index])
            && sharpRecords[index].run.lineHeight == 2
            && sharpRecords[index].run.width == 2
            && sharpRecords[index].run.characterX.size() == 2;
    }
    check(allColorsPreserved, "font ID, palette flags, and original font metrics are preserved");
    check(std::all_of(rawRecords.begin() + 5, rawRecords.end(), [](const scene::TextRecord& record) {
        return !record.run.sharpEligible && record.run.colorRole == scene::TextColorRole::Palette;
    }), "font IDs 102, 103, and 104 stay on the pixel-text path and keep original colors");
    check(sharpRestore.size() == 10,
        "sharp restoration includes all font-101 colors and excludes other font IDs");

    textCaptureCopy(source.data(), 8, 4, 8, destination.data(), 8, false, fakeCopy);
    const auto copiedRaw = scene::textCaptureRecords(destinationId);
    const auto copiedSharp = scene::textCaptureSharpRecords(destinationId);
    const auto copiedRestore = scene::textCaptureSharpRestoration(destinationId);
    check(copiedRaw.size() == 8 && copiedSharp.size() == 5,
        "font-101 eligibility survives image-buffer copies for all original colors");
    check(copiedRestore.size() == 10,
        "copied font-101 text contributes to sharp restoration; other font IDs do not");

    textCaptureUnregisterExternalBuffer(source.data());
    textCaptureUnregisterExternalBuffer(destination.data());
    gCurrentFont = 101;
}

void testSemanticGreenCaptureRoles()
{
    using namespace fallout;
    _colorTable[rgb555(0x00FF00)] = static_cast<Color>(43);
    _colorTable[rgb555(0x007300)] = static_cast<Color>(44);
    _colorTable[rgb555(0x429C21)] = static_cast<Color>(45);
    _colorTable[rgb555(0x00C500)] = static_cast<Color>(46);
    _colorTable[rgb555(0x007B00)] = static_cast<Color>(47);
    _colorTable[rgb555(0x084A08)] = static_cast<Color>(48);
    _colorTable[rgb555(0x8CBD8C)] = static_cast<Color>(49);
    _colorTable[rgb555(0x7B7B29)] = static_cast<Color>(50);
    _colorTable[rgb555(0xEFFF08)] = static_cast<Color>(51);
    _colorTable[rgb555(0xADAD5A)] = static_cast<Color>(52);

    std::vector<unsigned char> source(128, 1);
    std::vector<unsigned char> destination(128, 2);
    textCaptureRegisterExternalBuffer(source.data(), source.size(), 64, 2, 64, false);
    textCaptureRegisterExternalBuffer(destination.data(), destination.size(), 64, 2, 64, false);
    textCaptureMarkBufferOwned(source.data());
    textCaptureMarkBufferOwned(destination.data());
    const scene::TextBufferId sourceId = textCaptureBufferId(source.data());
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());
    const Color greenColors[] = { COLOR_GREEN, COLOR_LIGHT_GREEN, COLOR_LIGHT_GREEN_2,
        COLOR_LIGHT_GREEN_3, COLOR_DARK_GREEN, COLOR_DARK_GREEN_2, COLOR_SAGE_GREEN };
    fontDrawTextPtr = drawTwoGlyphs;
    gCurrentFont = 101;
    for (std::size_t index = 0; index < std::size(greenColors); ++index) {
        fontDrawText(source.data() + index * 2, "ab", 8, 64,
            static_cast<ColorWithFlags>(greenColors[index]));
    }
    {
        ScopedTextCaptureTag captureTag(TextCaptureTag::HudMessageLog);
        fontDrawText(source.data() + 14, "ab", 8, 64, static_cast<ColorWithFlags>(COLOR_GREEN));
        fontDrawText(source.data() + 16, "ab", 8, 64, static_cast<ColorWithFlags>(COLOR_YELLOW_GREEN));
    }
    fontDrawText(source.data() + 18, "ab", 8, 64, static_cast<ColorWithFlags>(COLOR_GREEN));
    fontDrawText(source.data() + 20, "ab", 8, 64, static_cast<ColorWithFlags>(COLOR_GRASS_GREEN));
    fontDrawText(source.data() + 22, "ab", 8, 64, static_cast<ColorWithFlags>(COLOR_OLIVE));

    const auto captured = scene::textCaptureRecords(sourceId);
    check(captured.size() == 12, "all font-101 semantic-color test runs are captured");
    bool rolesMatch = captured.size() == 12;
    for (std::size_t index = 0; index < std::size(greenColors) && rolesMatch; ++index) {
        rolesMatch = captured[index].run.colorRole == scene::TextColorRole::InterfaceGreen;
    }
    if (captured.size() == 12) {
        rolesMatch = rolesMatch
            && captured[7].run.colorRole == scene::TextColorRole::HudMessageLogGreen
            && captured[8].run.colorRole == scene::TextColorRole::Palette
            && captured[9].run.colorRole == scene::TextColorRole::InterfaceGreen
            && captured[10].run.colorRole == scene::TextColorRole::Palette
            && captured[11].run.colorRole == scene::TextColorRole::Palette;
    }
    check(rolesMatch, "semantic greens are tagged, HUD role is scoped, and olive/yellow-green stay original");

    textCaptureCopy(source.data(), 64, 2, 64, destination.data(), 64, false, fakeCopy);
    const auto copied = scene::textCaptureRecords(destinationId);
    bool copiedRolesMatch = copied.size() == captured.size();
    for (std::size_t index = 0; index < copied.size() && copiedRolesMatch; ++index) {
        copiedRolesMatch = copied[index].run.colorRole == captured[index].run.colorRole;
    }
    check(copiedRolesMatch, "green and HUD-log roles survive buffer copies");

    textCaptureUnregisterExternalBuffer(source.data());
    textCaptureUnregisterExternalBuffer(destination.data());
    gCurrentFont = 101;
}

void testSafeClippedCaptureAndBlit()
{
    using namespace fallout;
    std::vector<unsigned char> source(16, 3);
    std::vector<unsigned char> destination(16, 1);
    textCaptureAllocationCreated(source.data(), source.size());
    textCaptureAllocationCreated(destination.data(), destination.size());
    textCaptureMarkBufferOwned(destination.data());
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());
    fontDrawTextPtr = fakeFontDraw;
    fontDrawText(source.data() + 14, "abcd", 8, 8, static_cast<ColorWithFlags>(5));
    const auto sourceId = textCaptureBufferId(source.data());
    const auto sourceRecords = scene::textCaptureRecords(sourceId);
    check(sourceRecords.empty(), "orphan records are not exposed to live rendering");
    // Source runs are intentionally orphaned, but their records must still be copied.
    textCaptureCopy(source.data() + 8, 4, 1, 8, destination.data() + 8, 8, false, fakeCopy);
    check(scene::textCaptureRecords(destinationId).empty(), "unrelated orphan copy does not create live text");
    textCaptureCopy(source.data() + 14, 2, 1, 8, destination.data() + 6, 8, false, fakeCopy);
    const auto clippedCopy = scene::textCaptureRecords(destinationId);
    check(clippedCopy.size() == 1 && clippedCopy.front().bounds.x == 6 && clippedCopy.front().bounds.y == 0,
        "a clipped orphan run propagates only through its actual blit rectangle");
    // A full valid source capture propagates through the registered blit adapter.
    source[0] = 7;
    fontDrawText(source.data(), "ab", 8, 8, static_cast<ColorWithFlags>(5));
    textCaptureCopy(source.data(), 2, 1, 8, destination.data(), 8, false, fakeCopy);
    const auto destinationRecords = scene::textCaptureRecords(destinationId);
    check(destinationRecords.size() == 2, "registered source text is propagated through the blit adapter");
    check(destinationRecords.size() == 2 && destinationRecords.back().run.text == "ab", "blit preserves run metadata");
    check(destination[0] == source[0], "copy adapter leaves pixel result unchanged");
    textCaptureAllocationDestroyed(source.data());
    textCaptureAllocationDestroyed(destination.data());
    check(scene::textCaptureRecords(destinationId).empty(), "destroyed destination generation is erased");
}

void testUnknownBuffersKeepPixelBehavior()
{
    using namespace fallout;
    std::vector<unsigned char> source(8, 4);
    std::vector<unsigned char> destination(8, 1);
    fontDrawTextPtr = fakeFontDraw;
    drawCalls = 0;
    fontDrawText(source.data(), "x", 8, 8, static_cast<ColorWithFlags>(6));
    textCaptureCopy(source.data(), 4, 1, 4, destination.data(), 4, false, fakeCopy);
    check(drawCalls == 1, "unknown targets still invoke the original font proc once");
    check(destination[0] == 6 && destination[1] == 4,
        "unknown buffers retain the original drawing and blit pixel result");
}

void testImageBlitMasksDestinationTextWithoutSourceText()
{
    using namespace fallout;
    std::vector<unsigned char> source(4, 40);
    std::vector<unsigned char> destination(4, 1);
    textCaptureAllocationCreated(source.data(), source.size());
    textCaptureAllocationCreated(destination.data(), destination.size());
    textCaptureMarkBufferOwned(destination.data());
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());
    fontDrawTextPtr = drawTwoGlyphs;
    fontDrawText(destination.data(), "ab", 2, 2, static_cast<ColorWithFlags>(5));
    check(scene::textCaptureRecords(destinationId).size() == 1, "destination has a captured text run before image blit");
    textCaptureCopy(source.data(), 2, 2, 2, destination.data(), 2, false, fakeCopy);
    check(destination == source, "opaque image blit retains original pixel output");
    check(scene::textCaptureRecords(destinationId).empty(),
        "opaque source without text removes only destination text pixels it covers");
    textCaptureAllocationDestroyed(source.data());
    textCaptureAllocationDestroyed(destination.data());
}

void testUntrackedImageCopyUsesPixelFastPath()
{
    using namespace fallout;
    std::vector<unsigned char> source(4, 40);
    std::vector<unsigned char> destination(4, 1);
    textCaptureAllocationCreated(source.data(), source.size());
    textCaptureAllocationCreated(destination.data(), destination.size());
    const int before = copyCalls;
    textCaptureCopy(source.data(), 2, 2, 2, destination.data(), 2, false, fakeCopy);
    check(destination == source, "untracked image copy preserves its pixel output");
    check(copyCalls == before + 1, "untracked image copy reaches the pixel blitter once without tracking work");
    textCaptureAllocationDestroyed(source.data());
    textCaptureAllocationDestroyed(destination.data());
}

void testTransparentImageBlitMasksOnlyOpaqueShape()
{
    using namespace fallout;
    std::vector<unsigned char> source = { 5, 0, 0, 0 };
    std::vector<unsigned char> destination(4, 1);
    textCaptureAllocationCreated(source.data(), source.size());
    textCaptureAllocationCreated(destination.data(), destination.size());
    textCaptureMarkBufferOwned(destination.data());
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());
    fontDrawTextPtr = drawTwoGlyphs;
    fontDrawText(destination.data(), "ab", 2, 2, static_cast<ColorWithFlags>(5));
    textCaptureCopy(source.data(), 2, 2, 2, destination.data(), 2, true, fakeTransparentCopy);
    const auto records = scene::textCaptureRecords(destinationId);
    check(destination[0] == 5 && destination[1] == 6,
        "transparent image blit keeps its exact palette result");
    check(records.size() == 1 && records.front().pixels.size() == 1
            && records.front().pixels.front().x == 1,
        "transparent image blit masks an unchanged opaque cell and preserves blank source cells");
    textCaptureAllocationDestroyed(source.data());
    textCaptureAllocationDestroyed(destination.data());
}

void testTransparentInteriorImageViewMasksTrackedText()
{
    using namespace fallout;
    scene::setFont101PreviewScale(1.0, -1);
    const std::size_t allocationSize = 640 * 8;
    std::vector<unsigned char> source(allocationSize, 0);
    std::vector<unsigned char> destination(16, 1);
    textCaptureAllocationCreated(source.data(), source.size());
    textCaptureMarkBufferOwned(source.data());
    textCaptureAllocationCreated(destination.data(), destination.size());
    textCaptureMarkBufferOwned(destination.data());
    fontDrawTextPtr = fakeFontDraw;
    fontDrawText(source.data(), "s", 640, 640, static_cast<ColorWithFlags>(5));
    const scene::TextBufferId sourceId = textCaptureBufferId(source.data());
    check(sourceId != 0, "large source allocation registers its inferred 640-pixel pitch");

    fontDrawTextPtr = drawTwoGlyphs;
    fontDrawText(destination.data(), "ab", 2, 4, static_cast<ColorWithFlags>(5));
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());
    check(scene::textCaptureRecords(destinationId).size() == 1,
        "destination has text coverage before an interior FRM view blit");

    const std::size_t sourceOffset = 640 * 2;
    unsigned char* const interior = source.data() + sourceOffset;
    interior[0] = 5; // Same palette value as the first glyph, but still opaque.
    interior[1] = 9; // Changed opaque pixel.
    interior[46] = 0; // Transparent pixels preserve the destination text mask.
    interior[47] = 0;
    const int beforeCopyCalls = copyCalls;
    textCaptureCopy(interior, 2, 2, 46, destination.data(), 4, true, transparentCopyThenZeroSource);

    check(copyCalls == beforeCopyCalls + 1, "interior image view still uses the original copy proc once");
    check(destination[0] == 5 && destination[1] == 9 && destination[4] == 1 && destination[5] == 1,
        "bounded transparent snapshot preserves same-color, changed-color and zero pixels");
    const auto records = scene::textCaptureRecords(destinationId);
    check(records.size() == 1 && records.front().coverage.size() == 2
            && records.front().coverage[0].y == 1 && records.front().coverage[1].y == 1
            && records.front().pixels.empty(),
        "tracked source-range fallback retires opaque coverage and preserves transparent zero coverage");

    textCaptureAllocationDestroyed(source.data());
    textCaptureAllocationDestroyed(destination.data());
    scene::setFont101PreviewScale(1.29, -1);
}

void testTransparentInteriorImageViewRejectsOutOfRangeFootprint()
{
    using namespace fallout;
    scene::setFont101PreviewScale(1.0, -1);
    const std::size_t registeredSize = 640 * 4;
    std::vector<unsigned char> source(registeredSize + 64, 0);
    std::vector<unsigned char> destination(16, 1);
    textCaptureAllocationCreated(source.data(), registeredSize);
    textCaptureMarkBufferOwned(source.data());
    textCaptureAllocationCreated(destination.data(), destination.size());
    textCaptureMarkBufferOwned(destination.data());
    fontDrawTextPtr = fakeFontDraw;
    fontDrawText(source.data(), "s", 640, 640, static_cast<ColorWithFlags>(5));
    fontDrawTextPtr = drawTwoGlyphs;
    fontDrawText(destination.data(), "ab", 2, 4, static_cast<ColorWithFlags>(5));
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());

    const std::size_t sourceOffset = registeredSize - 36;
    unsigned char* const interior = source.data() + sourceOffset;
    interior[0] = 9;
    interior[1] = 10;
    interior[46] = 11;
    interior[47] = 12;
    const int beforeCopyCalls = copyCalls;
    textCaptureCopy(interior, 2, 2, 46, destination.data(), 4, true, zeroSourceThenTransparentCopy);

    check(copyCalls == beforeCopyCalls + 1 && destination[0] == 5 && destination[1] == 6,
        "unprovable image view keeps the original copy proc result without reading outside its allocation");
    const auto records = scene::textCaptureRecords(destinationId);
    check(records.size() == 1 && records.front().coverage.size() == 4 && records.front().pixels.size() == 2,
        "source footprint extending past its registered allocation keeps conservative text tracking");

    textCaptureAllocationDestroyed(source.data());
    textCaptureAllocationDestroyed(destination.data());
    scene::setFont101PreviewScale(1.29, -1);
}

void testAllocationEdgeBounds()
{
    using namespace fallout;
    std::vector<unsigned char> pixels(16, 2);
    textCaptureAllocationCreated(pixels.data(), pixels.size());
    textCaptureMarkBufferOwned(pixels.data());
    const scene::TextBufferId id = textCaptureBufferId(pixels.data());
    fontDrawTextPtr = fakeFontDraw;
    fontDrawText(pixels.data() + 14, "abcd", 8, 8, static_cast<ColorWithFlags>(5));
    const auto records = scene::textCaptureRecords(id);
    check(records.size() == 1 && records.front().bounds.x == 6 && records.front().bounds.y == 1
            && records.front().bounds.width == 2 && records.front().bounds.height == 1,
        "capture clips its compact snapshot at the registered allocation edge");
    textCaptureAllocationDestroyed(pixels.data());
}

void testMonospacedClippedRunMetadata()
{
    using namespace fallout;
    std::vector<unsigned char> source(16, 2);
    std::vector<unsigned char> destination(16, 1);
    textCaptureAllocationCreated(source.data(), source.size());
    textCaptureAllocationCreated(destination.data(), destination.size());
    textCaptureMarkBufferOwned(source.data());
    textCaptureMarkBufferOwned(destination.data());
    fontGetCharacterWidth = wideCharacterWidth;
    fontGetMonospacedCharacterWidth = fivePixelMonospacedWidth;
    fontDrawTextPtr = fakeFontDraw;
    const int flags = DRAW_TEXT_FLAG_MONOSPACED;

    // The width cap accepts only the first character, while its cell remains
    // the full five pixels and its glyph is centered inside that cell.
    fontDrawText(source.data(), "aw", 7, 8, static_cast<ColorWithFlags>(5 | flags));
    const scene::TextBufferId sourceId = textCaptureBufferId(source.data());
    const auto sourceRecords = scene::textCaptureRecords(sourceId);
    check(sourceRecords.size() == 1, "mono prefix capture creates one source run");
    if (sourceRecords.size() == 1) {
        const auto& run = sourceRecords.front().run;
        check(run.text == "a", "mono run retains only the accepted prefix");
        check(run.width == 5 && run.cellWidth == 5, "mono run keeps accepted advance and original cell width");
        check(run.characterX.size() == 1 && run.characterX[0] == 2,
            "mono glyph position is centered inside the original cell");
        check(run.characterCellX.size() == 1 && run.characterCellX[0] == 0,
            "mono run retains the original cell origin");
    }

    // Copy only one raster column. The geometric fragment must not change
    // the logical run width or condense the glyph to the fragment bounds.
    textCaptureCopy(source.data(), 1, 1, 8, destination.data() + 4, 8, false, fakeCopy);
    const scene::TextBufferId destinationId = textCaptureBufferId(destination.data());
    const auto destinationRecords = scene::textCaptureRecords(destinationId);
    check(destinationRecords.size() == 1, "clipped mono blit retains the run record");
    if (destinationRecords.size() == 1) {
        const auto& run = destinationRecords.front().run;
        check(run.text == "a" && run.width == 5 && run.cellWidth == 5,
            "clipped mono blit preserves accepted width and cell width");
        check(run.x == 4 && run.characterX.size() == 1 && run.characterX[0] == 6,
            "clipped mono blit shifts the glyph position");
        check(run.characterCellX.size() == 1 && run.characterCellX[0] == 4,
            "clipped mono blit shifts the cell origin");
    }
    textCaptureAllocationDestroyed(source.data());
    textCaptureAllocationDestroyed(destination.data());
    fontGetCharacterWidth = fakeCharacterWidth;
    fontGetMonospacedCharacterWidth = fakeMonospacedWidth;
}

void testVisibilityClearsHiddenAndStaleOwners()
{
    using namespace fallout;
    std::vector<unsigned char> pixels(4, 2);
    std::vector<unsigned char> replacement(4, 2);
    textCaptureRegisterExternalBuffer(pixels.data(), pixels.size(), 2, 2, 2, false);
    textCaptureMarkWindowBuffer(pixels.data(), 7);
    gTestWindow = {};
    gTestWindow.id = 7;
    gTestWindow.pixels = pixels.data();
    gTestWindow.generation = textCaptureBufferId(pixels.data());
    gTestWindow.width = 2;
    gTestWindow.height = 2;
    gTestWindow.pitch = 2;
    fontDrawTextPtr = fakeFontDraw;
    fontDrawText(pixels.data(), "x", 2, 2, static_cast<ColorWithFlags>(5));
    const auto records = scene::textCaptureRecords(textCaptureBufferId(pixels.data()));
    check(records.size() == 1 && records.front().ownerWindow == 7,
        "window-backed font capture stores its owner generation");
    if (records.size() == 1) {
        scene::TextVisibilityMask mask;
        const bool visible = scene::textCaptureVisibilityMask(records.front(), 2, 2, &mask);
        check(visible && mask.bounds.width == 1 && mask.bounds.height == 2
                && mask.pixels.size() == 2 && mask.visibleAt(0, 0),
            "visible owner retains compact per-record text coverage");
        gTestWindow.hidden = true;
        mask.pixels.assign(4, 1);
        check(scene::textCaptureVisibilityMask(records.front(), 2, 2, &mask)
                && std::none_of(mask.pixels.begin(), mask.pixels.end(), [](unsigned char value) { return value != 0; }),
            "hidden owner clears previously valid coverage");
        gTestWindow.hidden = false;
        gTestWindow.pixels = replacement.data();
        gTestWindow.generation = 0;
        mask.pixels.assign(4, 1);
        check(scene::textCaptureVisibilityMask(records.front(), 2, 2, &mask)
                && std::none_of(mask.pixels.begin(), mask.pixels.end(), [](unsigned char value) { return value != 0; }),
            "stale owner generation clears previously valid coverage");
    }
    textCaptureUnregisterExternalBuffer(pixels.data());
    gTestWindow = {};
}

void testCursorOcclusionPreservesOwnedText()
{
    using namespace fallout;
    std::vector<unsigned char> windowPixels(4, 2);
    std::vector<unsigned char> screenPixels(8, 1);
    std::vector<unsigned char> cursorPixels(1, 9);
    textCaptureRegisterExternalBuffer(windowPixels.data(), windowPixels.size(), 2, 2, 2, false);
    textCaptureRegisterExternalBuffer(screenPixels.data(), screenPixels.size(), 4, 2, 4, false);
    textCaptureMarkWindowBuffer(windowPixels.data(), 8);
    const scene::TextBufferId windowId = textCaptureBufferId(windowPixels.data());
    const scene::TextBufferId screenId = textCaptureBufferId(screenPixels.data());
    gTestWindow = {};
    gTestWindow.id = 8;
    gTestWindow.x = 1;
    gTestWindow.y = 0;
    gTestWindow.width = 2;
    gTestWindow.height = 2;
    gTestWindow.pitch = 2;
    gTestWindow.pixels = windowPixels.data();
    gTestWindow.generation = windowId;
    gTestCursor = { 1, 1, 1, 1, 1, true, 0, cursorPixels.data() };
    fontDrawTextPtr = drawTwoGlyphs;
    fontDrawText(windowPixels.data(), "ab", 2, 2, static_cast<ColorWithFlags>(5));
    textCaptureCopy(windowPixels.data(), 2, 2, 2, screenPixels.data() + 1, 4, false, fakeCopy);
    const auto records = scene::textCaptureRecords(screenId);
    check(records.size() == 1 && records.front().ownerWindow == 8,
        "screen copy retains its window provenance");
    if (records.size() == 1) {
        screenPixels[5] = 9; // Opaque cursor covers a blank cell in the text bounds.
        auto restored = scene::textCaptureRestoration(screenId);
        scene::TextVisibilityMask mask;
        check(restored.size() == 2 && scene::textCaptureVisibilityMask(records.front(), 4, 2, &mask)
                && !mask.visibleAt(1, 1) && mask.visibleAt(1, 0) && mask.visibleAt(2, 0),
            "cursor over a blank cell masks only that cell and preserves sharp glyphs");

        screenPixels[5] = 6;
        screenPixels[1] = 9; // Opaque cursor now covers the first legacy glyph pixel.
        gTestCursor.x = 1;
        gTestCursor.y = 0;
        restored = scene::textCaptureRestoration(screenId);
        check(restored.size() == 2 && scene::textCaptureVisibilityMask(records.front(), 4, 2, &mask)
                && !mask.visibleAt(1, 0) && mask.visibleAt(2, 0),
            "cursor over a foreground glyph preserves the remaining sharp glyph pixels");

        std::vector<unsigned char> cursorCompositeCopy(8, 1);
        textCaptureRegisterExternalBuffer(cursorCompositeCopy.data(), cursorCompositeCopy.size(), 4, 2, 4, true);
        textCaptureCopy(screenPixels.data(), 4, 2, 4, cursorCompositeCopy.data(), 4, false, fakeCopy);
        std::vector<unsigned char> cursorCompositeOutput(8, 1);
        textCaptureRegisterExternalBuffer(cursorCompositeOutput.data(), cursorCompositeOutput.size(), 4, 2, 4, false);
        const scene::TextBufferId cursorCompositeOutputId = textCaptureBufferId(cursorCompositeOutput.data());
        textCaptureCopy(cursorCompositeCopy.data(), 4, 2, 4, cursorCompositeOutput.data(), 4, false, fakeCopy);
        const auto cursorCompositeRecords = scene::textCaptureRecords(cursorCompositeOutputId);
        check(cursorCompositeRecords.size() == 1 && cursorCompositeRecords.front().pixels.size() == 1
                && cursorCompositeRecords.front().pixels.front().x == 2
                && std::none_of(cursorCompositeRecords.front().coverage.begin(), cursorCompositeRecords.front().coverage.end(),
                    [](const scene::TextPixelCoverage& pixel) { return pixel.x == 1 && pixel.y == 0; }),
            "orphan cursor scratch copy keeps owner-validated glyph pixels without retaining its opaque cell");
        textCaptureUnregisterExternalBuffer(cursorCompositeOutput.data());
        textCaptureUnregisterExternalBuffer(cursorCompositeCopy.data());

        screenPixels[1] = 5;
        gTestCursor.visible = false;
        gTestWindow.generation = windowId + 1;
        restored = scene::textCaptureRestoration(screenId);
        scene::textCaptureVisibilityMask(records.front(), 4, 2, &mask);
        check(restored.empty()
                && std::none_of(mask.pixels.begin(), mask.pixels.end(), [](unsigned char value) { return value != 0; }),
            "stale owner generation cannot restore or display captured text");

        gTestWindow.generation = windowId;
        windowPixels[0] = 99; // Repaint the owner while the screen copy remains unchanged.
        restored = scene::textCaptureRestoration(screenId);
        check(restored.empty() && scene::textCaptureRecords(screenId).empty(),
            "owner-buffer overwrite invalidates a matching screen clone");
    }
    textCaptureUnregisterExternalBuffer(windowPixels.data());
    textCaptureUnregisterExternalBuffer(screenPixels.data());
    gTestWindow = {};
    gTestCursor = {};
}

void testFont101SizePreviewExtendsCaptureOnly()
{
    using namespace fallout;
    check(scene::font101PreviewScaleForWindow(12) == 1.29
            && !scene::font101PreviewUsesBold(12),
        "production font-101 defaults select 1.29 SemiBold");
    std::vector<unsigned char> pixels(20 * 40, 0);
    textCaptureRegisterExternalBuffer(pixels.data(), pixels.size(), 20, 40, 20, false);
    textCaptureMarkWindowBuffer(pixels.data(), 12);
    scene::TextBufferId generation = textCaptureBufferId(pixels.data());
    gTestWindow = {};
    gTestWindow.id = 12;
    gTestWindow.width = 20;
    gTestWindow.height = 40;
    gTestWindow.pitch = 20;
    gTestWindow.pixels = pixels.data();
    gTestWindow.generation = generation;
    gCurrentFont = 101;
    fontGetLineHeight = fakePreviewLineHeight;
    fontDrawTextPtr = fakeFontDraw;

    check(scene::setFont101PreviewScale(1.0, 12), "font-101 preview default scale is available");
    fontDrawText(pixels.data() + 2 * 20 + 2, "A", 1, 20, static_cast<ColorWithFlags>(7));
    auto records = scene::textCaptureRecords(generation);
    check(records.size() == 1 && records.front().bounds.height == 9,
        "default font-101 capture keeps the original line bounds");

    textCaptureUnregisterExternalBuffer(pixels.data());
    pixels.assign(pixels.size(), 0);
    textCaptureRegisterExternalBuffer(pixels.data(), pixels.size(), 20, 40, 20, false);
    textCaptureMarkWindowBuffer(pixels.data(), 12);
    generation = textCaptureBufferId(pixels.data());
    gTestWindow.generation = generation;
    scene::setFont101PreviewScale(1.15, 12);
    fontDrawText(pixels.data() + 2 * 20 + 2, "B", 1, 20, static_cast<ColorWithFlags>(7));
    records = scene::textCaptureRecords(generation);
    scene::TextVisibilityMask mask;
    check(records.size() == 1 && records.front().bounds.height == 11
            && records.front().pixels.size() == 1
            && scene::textCaptureVisibilityMask(records.front(), 20, 40, &mask)
            && mask.bounds.height == 11,
        "font-101 preview extends only captured coverage, not legacy pixels or visibility logic");

    scene::setFont101PreviewScale(1.29, 12);
    {
        ScopedTextCaptureTag captureTag(TextCaptureTag::SpecialStatDescription);
        fontDrawText(pixels.data() + 14 * 20 + 2, "C", 1, 20, static_cast<ColorWithFlags>(7));
    }
    const auto aligned = scene::textCaptureRecords(generation);
    const auto alignedRun = std::find_if(aligned.begin(), aligned.end(), [](const scene::TextRecord& record) {
        return record.run.text == "C";
    });
    check(alignedRun != aligned.end() && alignedRun->run.verticalAlignmentBoxY == 10
            && alignedRun->run.verticalAlignmentBoxHeight == 17
            && alignedRun->bounds.y == 10 && alignedRun->bounds.height == 17,
        "SPECIAL label captures its measured 17-row box above and below the original draw line");
    fontDrawText(pixels.data() + 28 * 20 + 2, "D", 1, 20, static_cast<ColorWithFlags>(7));
    const auto afterScope = scene::textCaptureRecords(generation);
    const auto plainRun = std::find_if(afterScope.begin(), afterScope.end(), [](const scene::TextRecord& record) {
        return record.run.text == "D";
    });
    check(plainRun != afterScope.end() && plainRun->run.verticalAlignmentBoxHeight == 0
            && plainRun->bounds.y == 28,
        "SPECIAL box capture tag does not leak to the next run");

    scene::setFont101PreviewScale(1.29, -1);
    fontGetLineHeight = fakeLineHeight;
    textCaptureUnregisterExternalBuffer(pixels.data());
    gTestWindow = {};
    gCurrentFont = 101;
}

} // namespace

int main()
{
    fallout::fontGetLineHeight = fallout::fakeLineHeight;
    fallout::fontGetCharacterWidth = fallout::fakeCharacterWidth;
    fallout::fontGetLetterSpacing = fallout::fakeLetterSpacing;
    fallout::fontGetMonospacedCharacterWidth = fallout::fakeMonospacedWidth;
    testAllocatorGenerationAndShadowGuard();
    testFont101SharpEligibility();
    testSemanticGreenCaptureRoles();
    testSafeClippedCaptureAndBlit();
    testAllocationEdgeBounds();
    testMonospacedClippedRunMetadata();
    testImageBlitMasksDestinationTextWithoutSourceText();
    testUntrackedImageCopyUsesPixelFastPath();
    testTransparentImageBlitMasksOnlyOpaqueShape();
    testTransparentInteriorImageViewMasksTrackedText();
    testTransparentInteriorImageViewRejectsOutOfRangeFootprint();
    testUnknownBuffersKeepPixelBehavior();
    testVisibilityClearsHiddenAndStaleOwners();
    testCursorOcclusionPreservesOwnedText();
    testFont101SizePreviewExtendsCaptureOnly();
    if (failures == 0) std::printf("text_capture_tests: all passed\n");
    return failures == 0 ? 0 : 1;
}
