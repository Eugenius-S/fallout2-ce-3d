#include "scene/text_capture.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
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
TextWindowSnapshot gTestWindow = {};

int fakeLineHeight() { return 2; }
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
    *state = {};
    return true;
}

} // namespace fallout

namespace {

int drawCalls = 0;
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
    for (int y = 0; y < height; y++) {
        std::memmove(destination, source, width);
        destination += destinationPitch;
        source += sourcePitch;
    }
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

} // namespace

int main()
{
    fallout::fontGetLineHeight = fallout::fakeLineHeight;
    fallout::fontGetCharacterWidth = fallout::fakeCharacterWidth;
    fallout::fontGetLetterSpacing = fallout::fakeLetterSpacing;
    fallout::fontGetMonospacedCharacterWidth = fallout::fakeMonospacedWidth;
    testAllocatorGenerationAndShadowGuard();
    testSafeClippedCaptureAndBlit();
    testAllocationEdgeBounds();
    testMonospacedClippedRunMetadata();
    testUnknownBuffersKeepPixelBehavior();
    testVisibilityClearsHiddenAndStaleOwners();
    if (failures == 0) std::printf("text_capture_tests: all passed\n");
    return failures == 0 ? 0 : 1;
}
