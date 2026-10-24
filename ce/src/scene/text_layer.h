#ifndef FALLOUT_SCENE_TEXT_LAYER_H
#define FALLOUT_SCENE_TEXT_LAYER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

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

struct TextPixels {
    const unsigned char* data = nullptr;
    int width = 0;
    int height = 0;
    int pitch = 0;
    int originX = 0;
    int originY = 0;
};

struct TextValidationPixels {
    TextBufferId buffer = 0;
    TextPixels pixels;
};

struct TextRunInfo {
    std::string text;
    int font = 0;
    unsigned int colorAndFlags = 0;
    int x = 0;
    int y = 0;
    // Accepted logical advance and mono cell grid remain unchanged when a
    // copied run is clipped to a smaller destination rectangle.
    int width = 0;
    int cellWidth = 0;
    int lineHeight = 0;
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

struct TextVisibilityMask {
    TextRect bounds;
    std::vector<unsigned char> pixels;

    bool visibleAt(int x, int y) const
    {
        if (x < bounds.x || y < bounds.y || x >= bounds.x + bounds.width || y >= bounds.y + bounds.height) return false;
        return pixels[static_cast<std::size_t>(y - bounds.y) * bounds.width + (x - bounds.x)] != 0;
    }
};

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
    // text rectangle or copied fragment. Legacy changed pixels determine
    // whether the record is still valid; blank mask cells may be occluded.
    std::vector<TextPixelCoverage> coverage;
    std::vector<TextPixelRecord> pixels;
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
