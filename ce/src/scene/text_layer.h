#ifndef FALLOUT_SCENE_TEXT_LAYER_H
#define FALLOUT_SCENE_TEXT_LAYER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../color.h"

namespace fallout {
namespace scene {

// A key identifies one lifetime of a pixel buffer. Callers must use a new key
// when an address is reused; the layer never stores or dereferences addresses.
using TextBufferId = std::uint64_t;

struct TextRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct FloatingTextOutlineLayout {
    int legacyWidth = 0;
    int legacyHeight = 0;
    int bufferWidth = 0;
    int bufferHeight = 0;
    int offsetX = 0;
    int offsetY = 0;
    int legacyTopMargin = 0;
    int lineHeight = 0;
    int linePitch = 0;
    int lineOffset = 0;

    TextRect renderBounds(int x, int y) const;
    TextRect clipBounds(int x, int y, TextRect clip) const;
    std::size_t sourceOffset(TextRect clipped, int x, int y) const;
    std::size_t lineStartOffset(int line) const;
    int sharpLineOffset(int line) const;
};

FloatingTextOutlineLayout floatingTextOutlineLayout(int legacyWidth, int legacyHeight,
    int lineHeight, int lines, bool hasOutline, bool modernOutline);

struct TextPixels {
    const unsigned char* data = nullptr;
    int width = 0;
    int height = 0;
    int pitch = 0;
    int originX = 0;
    int originY = 0;
};

enum class TextColorRole : std::uint8_t {
    Palette,
    InterfaceGreen,
    HudMessageLogGreen,
    InactiveInjuryStatusGreen,
};

struct TextValidationPixels {
    TextBufferId buffer = 0;
    TextPixels pixels;
};

enum class TextSizeRole {
    Default,
    CompactUi,
};

struct TextRunInfo {
    std::string text;
    int font = 0;
    TextSizeRole sizeRole = TextSizeRole::Default;
    unsigned int colorAndFlags = 0;
    TextColorRole colorRole = TextColorRole::Palette;
    unsigned int outlineColor = 0;
    bool hasOutline = false;
    int x = 0;
    int y = 0;
    // Accepted logical advance and mono cell grid remain unchanged when a
    // copied run is clipped to a smaller destination rectangle.
    int width = 0;
    int cellWidth = 0;
    int lineHeight = 0;
    // Optional fixed content box for per-run vertical centering. The box moves
    // with the run when the record is copied to another buffer.
    int verticalAlignmentBoxY = 0;
    int verticalAlignmentBoxHeight = 0;
    // Logical vertical shift for the modern glyph overlay only.
    int sharpTextOffsetY = 0;
    bool sharpEligible = false;
    std::vector<int> characterX;
    std::vector<int> characterCellX;
};

struct TextPixelRecord {
    int x = 0;
    int y = 0;
    unsigned char before = 0;
    unsigned char after = 0;
};

struct TextPixelCoverage {
    int x = 0;
    int y = 0;
    unsigned char expected = 0;
};

struct TextOwnerWitness {
    int x = 0;
    int y = 0;
    unsigned char expected = 0;
};

struct TextVisibilityMask {
    TextRect bounds;
    std::vector<unsigned char> pixels;

    bool visibleAt(int x, int y) const
    {
        if (x < bounds.x || y < bounds.y || x >= bounds.x + bounds.width || y >= bounds.y + bounds.height) return false;
        return pixels[static_cast<std::size_t>(y - bounds.y) * bounds.width + (x - bounds.x)] != 0;
    }
};

// Process-local dev preview state. The owner window narrows the scale to a
// single fixture window; -1 applies it to CPU-only renderer tests.
bool setFont101PreviewScale(double scale, int ownerWindow);
double font101PreviewScaleForWindow(int ownerWindow);
double font101PreviewScaleForRun(int ownerWindow, TextSizeRole sizeRole);
int font101PreviewCoverageHeight(int lineHeight, int ownerWindow);
int font101PreviewCoverageHeight(int lineHeight, int ownerWindow, TextSizeRole sizeRole);
bool setFont101PreviewBold(bool bold, int ownerWindow);
bool font101PreviewUsesBold(int ownerWindow);

struct TextRecord {
    TextBufferId buffer = 0;
    bool orphan = false;
    int ownerWindow = -1;
    TextBufferId ownerGeneration = 0;
    // Current buffer coordinates minus authoritative owner-buffer coordinates.
    int ownerOffsetX = 0;
    int ownerOffsetY = 0;
    TextRunInfo run;
    TextRect bounds;
    // Per-game-pixel geometry for the modern glyph. It covers the captured
    // text rectangle or copied fragment. Legacy changed pixels and owner
    // witnesses determine validity; blank mask cells may be occluded.
    std::vector<TextPixelCoverage> coverage;
    std::vector<TextPixelRecord> pixels;
    // Derived coverage fragments can contain no legacy ink in their copy
    // rectangle. Keep the source run's changed pixels in owner coordinates so
    // those fragments remain tied to the live authoritative owner.
    std::vector<TextOwnerWitness> ownerWitnesses;
    int ownerWitnessWindow = -1;
    TextBufferId ownerWitnessGeneration = 0;
};

struct TextRestorePixel {
    int x = 0;
    int y = 0;
    unsigned char paletteIndex = 0;
    int ownerWindow = -1;
    TextBufferId ownerGeneration = 0;
};

// CPU-only capture, copy tracking, invalidation, and restoration bookkeeping.
// Pixel views are consumed synchronously and are never retained.
class TextLayer {
public:
    explicit TextLayer(std::size_t recordLimit = 4096);

    bool capture(TextBufferId buffer, bool orphan, const TextRunInfo& run,
        TextRect bounds, TextPixels before, TextPixels after,
        int ownerWindow = -1, TextBufferId ownerGeneration = 0);
    bool hasOutlinedRuns(TextBufferId buffer) const;
    bool refreshOutlinedRuns(TextBufferId buffer, TextPixels before, TextPixels after);

    // Call after a blit with a snapshot of the source at blit time and the
    // destination values just before and after it. The rectangle maps
    // sourceRect onto destinationX/Y; clipped pixels retain their coverage.
    // All views are read only during this call.
    void copy(TextBufferId source, TextBufferId destination, bool destinationOrphan,
        TextRect sourceRect, int destinationX, int destinationY,
        bool transparent, unsigned char transparentIndex,
        TextPixels sourceSnapshot, TextPixels destinationBefore, TextPixels destinationAfter,
        int destinationOwnerWindow = -1, TextBufferId destinationOwnerGeneration = 0,
        const std::vector<TextValidationPixels>& ownerPixels = {});

    // Retire only text coverage explicitly overwritten by a known image blit.
    // For a transparent blit, source pixels are aligned to destinationRect and
    // transparentIndex cells leave earlier text visible.
    void overwrite(TextBufferId destination, TextRect destinationRect,
        TextPixels sourceSnapshot = {}, bool transparent = false, unsigned char transparentIndex = 0);

    // Drop every record for a dead buffer generation.
    void eraseBuffer(TextBufferId buffer);
    void eraseOwner(int window, TextBufferId generation);

    // Validate actual legacy glyph pixels against the live buffer and return
    // restore pixels only when every changed value still matches.
    // Orphan records are never presented.
    std::vector<TextRestorePixel> restorationPixels(TextBufferId buffer,
        TextPixels current, bool includeOrphans = false, bool sharpEligibleOnly = false);
    std::vector<TextRestorePixel> restorationPixels(TextBufferId buffer,
        TextPixels current, const std::vector<TextValidationPixels>& ownerPixels,
        bool includeOrphans = false, bool sharpEligibleOnly = false);

    const std::vector<TextRecord>& records() const;
    std::size_t size() const;

private:
    void trim();

    std::size_t recordLimit_;
    std::vector<TextRecord> records_;
};

} // namespace scene
} // namespace fallout

#endif // FALLOUT_SCENE_TEXT_LAYER_H
