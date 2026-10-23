#include "text_layer.h"

#include <cstdio>
#include <vector>

using fallout::scene::TextBufferId;
using fallout::scene::TextLayer;
using fallout::scene::TextPixels;
using fallout::scene::TextRect;
using fallout::scene::TextRunInfo;

namespace {

bool check(bool condition, const char* what)
{
    if (!condition) std::fprintf(stderr, "FAIL: %s\n", what);
    return condition;
}

TextPixels view(const std::vector<unsigned char>& pixels, int width, int height, int pitch)
{
    return { pixels.data(), width, height, pitch };
}

std::vector<unsigned char> blit(const std::vector<unsigned char>& source, int sourceWidth, int sourceHeight,
    const std::vector<unsigned char>& destinationBefore, int destinationWidth, int destinationHeight,
    TextRect sourceRect, int destinationX, int destinationY, bool transparent, unsigned char transparentIndex)
{
    std::vector<unsigned char> destinationAfter = destinationBefore;
    for (int sy = sourceRect.y; sy < sourceRect.y + sourceRect.height; sy++) {
        for (int sx = sourceRect.x; sx < sourceRect.x + sourceRect.width; sx++) {
            const int dx = destinationX + sx - sourceRect.x;
            const int dy = destinationY + sy - sourceRect.y;
            if (sx < 0 || sy < 0 || sx >= sourceWidth || sy >= sourceHeight
                || dx < 0 || dy < 0 || dx >= destinationWidth || dy >= destinationHeight) continue;
            const unsigned char value = source[sy * sourceWidth + sx];
            if (transparent && value == transparentIndex) continue;
            destinationAfter[dy * destinationWidth + dx] = value;
        }
    }
    return destinationAfter;
}

TextRunInfo run(const char* text, int x, int y)
{
    TextRunInfo value;
    value.text = text;
    value.font = 101;
    value.colorAndFlags = 7;
    value.x = x;
    value.y = y;
    value.lineHeight = 8;
    value.characterX = { x, x + 2 };
    return value;
}

bool capture(TextLayer& layer, TextBufferId id, bool orphan,
    const std::vector<unsigned char>& before, const std::vector<unsigned char>& after,
    int width, int height, TextRect bounds, const char* text = "AB",
    int ownerWindow = -1, TextBufferId ownerGeneration = 0)
{
    const auto beforeView = view(before, width, height, width);
    const auto afterView = view(after, width, height, width);
    TextRunInfo capturedRun = run(text, bounds.x, bounds.y);
    capturedRun.width = bounds.width;
    return layer.capture(id, orphan, capturedRun, bounds, beforeView, afterView,
        ownerWindow, ownerGeneration);
}

} // namespace

int main()
{
    int failures = 0;

    // An opaque button-image copy restores the source background, not whatever
    // happened to be under the destination before the blit.
    {
        TextLayer layer;
        std::vector<unsigned char> sourceBefore(12, 10);
        std::vector<unsigned char> sourceAfter = sourceBefore;
        sourceAfter[5] = 20;
        sourceAfter[6] = 21;
        failures += !check(capture(layer, 1, true, sourceBefore, sourceAfter, 4, 3, { 0, 0, 4, 3 }, "AB", 4, 12),
            "capture creates an orphan record");

        std::vector<unsigned char> destBefore(20, 77);
        const auto destAfter = blit(sourceAfter, 4, 3, destBefore, 5, 4,
            { 0, 0, 4, 3 }, 1, 0, false, 0);
        layer.copy(1, 2, false, { 0, 0, 4, 3 }, 1, 0, false, 0,
            view(sourceAfter, 4, 3, 4),
            view(destBefore, 5, 4, 5), view(destAfter, 5, 4, 5));
        const auto restored = layer.restorationPixels(2, view(destAfter, 5, 4, 5));
        failures += !check(restored.size() == 2, "opaque copy keeps both glyph pixels");
        failures += !check(restored.size() == 2 && restored[0].paletteIndex == 10 && restored[1].paletteIndex == 10,
            "opaque copy carries source underlying pixels");
        failures += !check(layer.records().size() == 2 && layer.records().back().run.characterX[0] == 1,
            "copy translates run geometry");
        failures += !check(layer.records().back().ownerWindow == 4 && layer.records().back().ownerGeneration == 12,
            "copy preserves owner provenance until a destination owner is assigned");
    }

    // Transparent source background leaves the destination's own background
    // under a copied glyph.
    {
        TextLayer layer;
        std::vector<unsigned char> sourceBefore(4, 0);
        std::vector<unsigned char> sourceAfter = sourceBefore;
        sourceAfter[1] = 6;
        capture(layer, 3, true, sourceBefore, sourceAfter, 2, 2, { 0, 0, 2, 2 }, "A");
        std::vector<unsigned char> destBefore(4, 55);
        const auto destAfter = blit(sourceAfter, 2, 2, destBefore, 2, 2,
            { 0, 0, 2, 2 }, 0, 0, true, 0);
        layer.copy(3, 4, false, { 0, 0, 2, 2 }, 0, 0, true, 0,
            view(sourceAfter, 2, 2, 2),
            view(destBefore, 2, 2, 2), view(destAfter, 2, 2, 2));
        const auto restored = layer.restorationPixels(4, view(destAfter, 2, 2, 2));
        failures += !check(restored.size() == 1 && restored[0].paletteIndex == 55,
            "transparent copy retains destination underlying pixel");
        failures += !check(layer.records().back().coverage.size() == 4,
            "transparent copy keeps blank source cells in the modern glyph mask");
    }

    // A rightward overlapping scroll uses a source snapshot from before the
    // blit and preserves both the old visible fragment and the translated one.
    {
        TextLayer layer;
        std::vector<unsigned char> before = { 1, 1, 1, 1 };
        std::vector<unsigned char> after = { 9, 8, 1, 1 };
        capture(layer, 12, false, before, after, 4, 1, { 0, 0, 4, 1 }, "AB");
        const std::vector<unsigned char> sourceSnapshot = after;
        const std::vector<unsigned char> destinationBefore = after;
        const auto destinationAfter = blit(sourceSnapshot, 4, 1, destinationBefore, 4, 1,
            { 0, 0, 3, 1 }, 1, 0, false, 0);
        layer.copy(12, 12, false, { 0, 0, 3, 1 }, 1, 0, false, 0,
            view(sourceSnapshot, 4, 1, 4), view(destinationBefore, 4, 1, 4), view(destinationAfter, 4, 1, 4));
        failures += !check(layer.records().size() == 2 && layer.records()[0].coverage.size() == 1
                && layer.records()[1].pixels.size() == 2,
            "overlapping blit snapshots source records before replacing target coverage");
        const auto restored = layer.restorationPixels(12, view(destinationAfter, 4, 1, 4));
        failures += !check(restored.size() == 3,
            "overlapping scroll keeps the unclipped and translated glyph pixels");
    }

    // Clipping a scrolling copy retains only the visible part of the line.
    {
        TextLayer layer;
        std::vector<unsigned char> before(15, 3);
        std::vector<unsigned char> after = before;
        after[6] = 9;
        after[7] = 10;
        after[8] = 11;
        capture(layer, 5, false, before, after, 5, 3, { 0, 0, 5, 3 });
        std::vector<unsigned char> destBefore(6, 4);
        const auto destAfter = blit(after, 5, 3, destBefore, 3, 2,
            { 1, 1, 3, 1 }, -1, 0, false, 0);
        layer.copy(5, 6, false, { 1, 1, 3, 1 }, -1, 0, false, 0,
            view(after, 5, 3, 5),
            view(destBefore, 3, 2, 3), view(destAfter, 3, 2, 3));
        failures += !check(layer.records().size() == 2 && layer.records().back().pixels.size() == 2,
            "clipped copy keeps the visible pixel fragment");
        failures += !check(layer.records().back().bounds.x == 0 && layer.records().back().bounds.width == 2,
            "clipped fragment has clipped bounds");
    }

    // Bounds overlap because both runs include blank advance/line-height area,
    // but their actual changed glyph pixels are disjoint.
    {
        TextLayer layer;
        std::vector<unsigned char> base(4, 12);
        std::vector<unsigned char> first = base;
        first[0] = 30;
        capture(layer, 70, false, base, first, 2, 2, { 0, 0, 2, 2 }, "A");
        std::vector<unsigned char> second = first;
        second[1] = 40;
        capture(layer, 70, false, first, second, 2, 2, { 1, 0, 1, 1 }, "B");
        failures += !check(layer.records().size() == 2,
            "overlapping draw bounds preserve older runs when glyph pixels are disjoint");
        const auto restored = layer.restorationPixels(70, view(second, 2, 2, 2));
        failures += !check(restored.size() == 2,
            "both ink-disjoint runs remain available for sharp rendering");
    }

    // A new font call that touches an actual old glyph pixel replaces the
    // entire old run, while preserving the original restoration value.
    {
        TextLayer layer;
        std::vector<unsigned char> base(4, 12);
        std::vector<unsigned char> first = base;
        first[0] = 30;
        first[1] = 31;
        capture(layer, 7, false, base, first, 2, 2, { 0, 0, 2, 2 }, "AB");
        std::vector<unsigned char> second = first;
        second[0] = 40;
        second[1] = 12; // The window was repainted before the shorter label.
        capture(layer, 7, false, first, second, 2, 2, { 0, 0, 1, 1 }, "A");
        failures += !check(layer.records().size() == 1 && layer.records()[0].run.text == "A",
            "actual glyph overlap replaces the entire older run");
        failures += !check(layer.records()[0].pixels[0].before == 12,
            "redraw inherits the original underlying value");
        std::vector<unsigned char> overwritten = second;
        overwritten[1] = 99;
        const auto restored = layer.restorationPixels(7, view(overwritten, 2, 2, 2));
        failures += !check(restored.size() == 1 && restored[0].x == 0 && restored[0].paletteIndex == 12,
            "new run remains valid when unrelated pixels outside its box change");
    }

    // Any overwritten captured value invalidates a complete run. Matching
    // blank coverage must not keep a stale glyph mask alive.
    {
        TextLayer layer;
        std::vector<unsigned char> before(3, 12);
        std::vector<unsigned char> after = before;
        after[0] = 30;
        after[1] = 31;
        capture(layer, 18, false, before, after, 3, 1, { 0, 0, 3, 1 }, "AB");
        std::vector<unsigned char> repainted = after;
        repainted[0] = 99;
        const auto restored = layer.restorationPixels(18, view(repainted, 3, 1, 3));
        failures += !check(restored.empty() && layer.records().empty(),
            "one overwritten glyph pixel invalidates the run despite matching glyph and blank coverage");

        std::vector<unsigned char> oneGlyphBefore(2, 12);
        std::vector<unsigned char> oneGlyphAfter = oneGlyphBefore;
        oneGlyphAfter[0] = 40;
        capture(layer, 181, false, oneGlyphBefore, oneGlyphAfter, 2, 1, { 0, 0, 2, 1 }, "A");
        oneGlyphAfter[0] = 99;
        const auto blankCoverage = layer.restorationPixels(181, view(oneGlyphAfter, 2, 1, 2));
        failures += !check(blankCoverage.empty() && layer.records().empty(),
            "matching blank coverage cannot keep a fully overwritten glyph run alive");
    }

    // An occluder may touch a blank cell inside the modern glyph bounds. It
    // must not invalidate the other unchanged legacy glyph pixels.
    {
        TextLayer layer;
        std::vector<unsigned char> before(3, 12);
        std::vector<unsigned char> after = before;
        after[0] = 30;
        after[1] = 31;
        capture(layer, 182, false, before, after, 3, 1, { 0, 0, 3, 1 }, "AB");
        std::vector<unsigned char> cursorOverBlank = after;
        cursorOverBlank[2] = 99;
        const auto restored = layer.restorationPixels(182, view(cursorOverBlank, 3, 1, 3));
        failures += !check(restored.size() == 2 && layer.records().size() == 1,
            "occlusion over blank mask cell preserves valid glyph pixels");
    }

    // A known opaque image blit removes only the covered part of an existing
    // text run, even when one copied palette value matches the old pixel.
    {
        TextLayer layer;
        std::vector<unsigned char> before(4, 3);
        std::vector<unsigned char> after = { 20, 21, 22, 23 };
        capture(layer, 183, false, before, after, 4, 1, { 0, 0, 4, 1 });
        layer.overwrite(183, { 1, 0, 2, 1 });
        const auto restored = layer.restorationPixels(183, view(after, 4, 1, 4));
        failures += !check(layer.records().size() == 1 && layer.records().front().pixels.size() == 2
                && layer.records().front().run.width == 4,
            "opaque image blit clips covered cells but preserves original run geometry");
        failures += !check(restored.size() == 2 && restored[0].x == 0 && restored[1].x == 3,
            "opaque image block leaves only uncovered text pixels for sharp drawing");
    }

    // A transparent sprite masks every nontransparent source cell, including
    // cells whose palette value happens to match the destination already.
    {
        TextLayer layer;
        std::vector<unsigned char> before(4, 3);
        std::vector<unsigned char> after = { 20, 21, 22, 23 };
        capture(layer, 184, false, before, after, 4, 1, { 0, 0, 4, 1 });
        std::vector<unsigned char> sprite = { 20, 0, 0, 22 };
        layer.overwrite(184, { 0, 0, 4, 1 }, view(sprite, 4, 1, 4), true, 0);
        const auto restored = layer.restorationPixels(184, view(after, 4, 1, 4));
        failures += !check(layer.records().size() == 1 && layer.records().front().coverage.size() == 2,
            "transparent sprite masks its shape even when an opaque pixel was unchanged");
        failures += !check(restored.size() == 2 && restored[0].x == 1 && restored[1].x == 2,
            "transparent sprite leaves only uncovered glyph pixels for sharp drawing");
        std::vector<unsigned char> stale = after;
        stale[2] = 99;
        failures += !check(layer.restorationPixels(184, view(stale, 4, 1, 4)).empty()
                && layer.records().empty(),
            "an unrelated stale glyph delta still invalidates the remaining run atomically");
    }

    // A tracked blit whose source contains no text still retires overwritten
    // destination text cells. Transparent source blanks preserve them.
    {
        TextLayer layer;
        std::vector<unsigned char> before(4, 2);
        std::vector<unsigned char> after = { 8, 9, 10, 11 };
        capture(layer, 185, false, before, after, 4, 1, { 0, 0, 4, 1 });
        std::vector<unsigned char> sprite = { 50, 0, 52, 0 };
        layer.overwrite(185, { 0, 0, 4, 1 }, view(sprite, 4, 1, 4), true, 0);
        failures += !check(layer.records().size() == 1 && layer.records().front().pixels.size() == 2
                && layer.records().front().pixels[0].x == 1 && layer.records().front().pixels[1].x == 3,
            "transparent blit without source text clips only opaque destination cells");
    }

    // A compact source snapshot may cover only a copied fragment. The source
    // rectangle can be larger, but validation and cloning must stay in bounds.
    {
        TextLayer layer;
        std::vector<unsigned char> before = { 3, 3, 3, 3 };
        std::vector<unsigned char> after = { 3, 20, 21, 3 };
        capture(layer, 186, true, before, after, 4, 1, { 0, 0, 4, 1 }, "AB", 5, 15);
        std::vector<unsigned char> compact = { 20, 21 };
        std::vector<unsigned char> destinationBefore(4, 9);
        std::vector<unsigned char> destinationAfter = { 3, 20, 21, 3 };
        TextPixels compactView { compact.data(), 2, 1, 2, 1, 0 };
        layer.copy(186, 187, false, { 0, 0, 4, 1 }, 0, 0, false, 0,
            compactView, view(destinationBefore, 4, 1, 4), view(destinationAfter, 4, 1, 4));
        const auto records = layer.records();
        failures += !check(records.size() == 2 && records.back().bounds.x == 1 && records.back().bounds.width == 2,
            "copy with a compact source snapshot retains only safely sampled glyph geometry");
    }

    {
        TextLayer layer;
        std::vector<unsigned char> before(4, 3);
        std::vector<unsigned char> after = { 20, 3, 3, 21 };
        capture(layer, 188, true, before, after, 4, 1, { 0, 0, 4, 1 }, "AB", 5, 16);
        std::vector<unsigned char> compact = { 3, 3 };
        std::vector<unsigned char> destinationBefore(4, 9);
        std::vector<unsigned char> destinationAfter = after;
        TextPixels compactView { compact.data(), 2, 1, 2, 1, 0 };
        layer.copy(188, 189, false, { 0, 0, 4, 1 }, 0, 0, false, 0,
            compactView, view(destinationBefore, 4, 1, 4), view(destinationAfter, 4, 1, 4));
        failures += !check(layer.records().size() == 1,
            "a compact source snapshot missing changed glyph pixels cannot clone an unvalidated record");
    }

    // Orphan memory is checked only during the live copy, and a single stale
    // glyph delta rejects the whole clipped fragment.
    {
        TextLayer layer;
        std::vector<unsigned char> before(2, 2);
        std::vector<unsigned char> after = { 8, 9 };
        capture(layer, 19, true, before, after, 2, 1, { 0, 0, 2, 1 }, "AB");
        std::vector<unsigned char> staleSource = after;
        staleSource[1] = 99;
        std::vector<unsigned char> destinationBefore(2, 5);
        const auto destinationAfter = blit(staleSource, 2, 1, destinationBefore, 2, 1,
            { 0, 0, 2, 1 }, 0, 0, false, 0);
        layer.copy(19, 20, false, { 0, 0, 2, 1 }, 0, 0, false, 0,
            view(staleSource, 2, 1, 2), view(destinationBefore, 2, 1, 2), view(destinationAfter, 2, 1, 2));
        failures += !check(layer.records().size() == 1 && layer.records().front().buffer == 19,
            "orphan blit rejects the whole fragment when one glyph delta is stale");
        failures += !check(layer.restorationPixels(20, view(destinationAfter, 2, 1, 2)).empty(),
            "stale orphan fragment cannot restore destination pixels");
    }

    // Orphan records are skipped without reading their former memory, but can
    // be copied while a live blit supplies destination snapshots.
    {
        TextLayer layer;
        std::vector<unsigned char> before(4, 2);
        std::vector<unsigned char> after = before;
        after[0] = 8;
        capture(layer, 8, true, before, after, 2, 2, { 0, 0, 1, 1 }, "A");
        failures += !check(layer.restorationPixels(8, {}).empty() && layer.size() == 1,
            "orphan is not touched at presentation");
        std::vector<unsigned char> destBefore(4, 5);
        const auto destAfter = blit(after, 2, 2, destBefore, 2, 2,
            { 0, 0, 1, 1 }, 0, 0, false, 0);
        layer.copy(8, 9, false, { 0, 0, 1, 1 }, 0, 0, false, 0,
            view(after, 2, 2, 2),
            view(destBefore, 2, 2, 2), view(destAfter, 2, 2, 2));
        failures += !check(layer.restorationPixels(9, view(destAfter, 2, 2, 2)).size() == 1,
            "orphan record can be cloned during a live copy");
        std::vector<unsigned char> secondBefore(4, 6);
        const auto secondAfter = blit(destAfter, 2, 2, secondBefore, 2, 2,
            { 0, 0, 1, 1 }, 0, 0, false, 0);
        layer.copy(9, 10, true, { 0, 0, 1, 1 }, 0, 0, false, 0,
            view(destAfter, 2, 2, 2), view(secondBefore, 2, 2, 2), view(secondAfter, 2, 2, 2));
        failures += !check(layer.restorationPixels(10, {}).empty(),
            "a copied off-screen destination remains an orphan");
        layer.eraseBuffer(8);
        failures += !check(layer.records().size() == 2 && layer.records()[0].buffer == 9 && layer.records()[1].buffer == 10,
            "destroyed owner generation is removed");
    }

    // Copying into previously tracked text retains the source's own
    // background. A stale source record is ignored, and identical destination
    // before/after values still permit the tracked run to be established.
    {
        TextLayer layer;
        std::vector<unsigned char> sourceBefore(1, 5);
        std::vector<unsigned char> sourceAfter(1, 20);
        capture(layer, 14, false, sourceBefore, sourceAfter, 1, 1, { 0, 0, 1, 1 }, "S");
        std::vector<unsigned char> targetBase(1, 1);
        std::vector<unsigned char> targetText(1, 2);
        capture(layer, 15, false, targetBase, targetText, 1, 1, { 0, 0, 1, 1 }, "T");
        const auto targetAfter = blit(sourceAfter, 1, 1, targetText, 1, 1,
            { 0, 0, 1, 1 }, 0, 0, false, 0);
        layer.copy(14, 15, false, { 0, 0, 1, 1 }, 0, 0, false, 0,
            view(sourceAfter, 1, 1, 1), view(targetText, 1, 1, 1), view(targetAfter, 1, 1, 1));
        failures += !check(layer.records().size() == 2 && layer.records().back().pixels[0].before == 5,
            "opaque source restoration value survives overlapping destination text");

        std::vector<unsigned char> unchangedBefore(1, 20);
        const auto unchangedAfter = blit(sourceAfter, 1, 1, unchangedBefore, 1, 1,
            { 0, 0, 1, 1 }, 0, 0, false, 0);
        layer.copy(14, 16, false, { 0, 0, 1, 1 }, 0, 0, false, 0,
            view(sourceAfter, 1, 1, 1), view(unchangedBefore, 1, 1, 1), view(unchangedAfter, 1, 1, 1));
        failures += !check(layer.records().size() == 3 && layer.records().back().pixels[0].before == 5,
            "unchanged destination value still receives a text record");

        std::vector<unsigned char> staleSource(1, 99);
        std::vector<unsigned char> staleDestination(1, 5);
        const auto staleAfter = blit(staleSource, 1, 1, staleDestination, 1, 1,
            { 0, 0, 1, 1 }, 0, 0, false, 0);
        layer.copy(14, 17, false, { 0, 0, 1, 1 }, 0, 0, false, 0,
            view(staleSource, 1, 1, 1), view(staleDestination, 1, 1, 1), view(staleAfter, 1, 1, 1));
        failures += !check(layer.records().size() == 3,
            "overwritten source pixels are not copied from stale records");
    }

    // The oldest record is discarded at the configured cap.
    {
        TextLayer layer;
        std::vector<unsigned char> before = { 4 };
        std::vector<unsigned char> after = { 7 };
        TextPixels beforeRegion { before.data(), 1, 1, 1, 3, 2 };
        TextPixels afterRegion { after.data(), 1, 1, 1, 3, 2 };
        failures += !check(layer.capture(20, true, run("O", 3, 2), { 3, 2, 1, 1 }, beforeRegion, afterRegion),
            "compact capture snapshots retain allocation coordinates");
        std::vector<unsigned char> destinationBefore(24, 9);
        std::vector<unsigned char> destinationAfter = destinationBefore;
        destinationAfter[1 * 6 + 4] = 7;
        TextPixels sourceSnapshot { after.data(), 1, 1, 1, 3, 2 };
        TextPixels destBeforeRegion { destinationBefore.data() + 1 * 6 + 4, 1, 1, 1, 4, 1 };
        TextPixels destAfterRegion { destinationAfter.data() + 1 * 6 + 4, 1, 1, 1, 4, 1 };
        layer.copy(20, 21, false, { 3, 2, 1, 1 }, 4, 1, false, 0,
            sourceSnapshot, destBeforeRegion, destAfterRegion);
        const auto restored = layer.restorationPixels(21, view(destinationAfter, 6, 4, 6));
        failures += !check(restored.size() == 1 && restored[0].x == 4 && restored[0].y == 1
                && restored[0].paletteIndex == 4,
            "compact blit snapshots preserve source and destination origins");
    }

    // Raw records stay available while sharp restoration selects tagged runs.
    {
        TextLayer layer;
        std::vector<unsigned char> before(6, 1);
        std::vector<unsigned char> after = before;
        after[0] = 7;
        after[2] = 8;
        after[4] = 9;
        for (int index = 0; index < 3; ++index) {
            TextRunInfo tagged = run("A", index * 2, 0);
            tagged.sharpEligible = index == 0;
            const bool captured = layer.capture(30, false, tagged, { index * 2, 0, 1, 1 },
                { before.data() + index * 2, 1, 1, 1, index * 2, 0 },
                { after.data() + index * 2, 1, 1, 1, index * 2, 0 });
            failures += !check(captured, "tagged and untagged records remain available to raw capture");
        }
        const auto eligibleRestore = layer.restorationPixels(30, view(after, 6, 1, 6), false, true);
        failures += !check(layer.records().size() == 3, "eligibility does not hide raw capture records");
        failures += !check(eligibleRestore.size() == 1 && eligibleRestore[0].x == 0,
            "sharp restoration returns only semantically eligible text");

        std::vector<unsigned char> destinationBefore(6, 2);
        std::vector<unsigned char> destinationAfter = destinationBefore;
        destinationAfter[3] = 7;
        TextRunInfo eligible = run("A", 0, 0);
        eligible.sharpEligible = true;
        TextLayer copyLayer;
        std::vector<unsigned char> sourceBefore(1, 1);
        std::vector<unsigned char> sourceAfter(1, 7);
        copyLayer.capture(31, false, eligible, { 0, 0, 1, 1 }, view(sourceBefore, 1, 1, 1), view(sourceAfter, 1, 1, 1));
        copyLayer.copy(31, 32, false, { 0, 0, 1, 1 }, 3, 0, false, 0,
            view(sourceAfter, 1, 1, 1), view(destinationBefore, 6, 1, 6), view(destinationAfter, 6, 1, 6));
        const auto copiedRestore = copyLayer.restorationPixels(32, view(destinationAfter, 6, 1, 6), false, true);
        failures += !check(copyLayer.records().back().run.sharpEligible && copiedRestore.size() == 1
                && copiedRestore[0].x == 3,
            "semantic eligibility survives buffer copies and remains restorable");
    }

    // The oldest record is discarded at the configured cap.
    {
        TextLayer layer(1);
        std::vector<unsigned char> before(1, 1);
        std::vector<unsigned char> a(1, 2);
        capture(layer, 10, false, before, a, 1, 1, { 0, 0, 1, 1 }, "A");
        std::vector<unsigned char> b(1, 3);
        capture(layer, 11, false, before, b, 1, 1, { 0, 0, 1, 1 }, "B");
        failures += !check(layer.size() == 1 && layer.records()[0].buffer == 11,
            "record cap evicts the oldest record");
    }

    if (failures == 0) std::printf("text_layer_tests: all passed\n");
    return failures == 0 ? 0 : 1;
}
