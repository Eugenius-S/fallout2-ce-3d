#ifndef FALLOUT_SCENE_TEXT_LAYER_H
#define FALLOUT_SCENE_TEXT_LAYER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
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
};

struct TextRunInfo {
    std::string text;
    int font = 0;
    unsigned int colorAndFlags = 0;
    int x = 0;
    int y = 0;
    int lineHeight = 0;
    std::vector<int> characterX;
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

struct TextRecord {
    TextBufferId buffer = 0;
    bool orphan = false;
    TextRunInfo run;
    TextRect bounds;
    // Per-game-pixel clip for the modern glyph. It covers the captured text
    // rectangle, so later rendering can hide only overwritten portions.
    std::vector<TextPixelCoverage> coverage;
    std::vector<TextPixelRecord> pixels;
};

struct TextRestorePixel {
    int x = 0;
    int y = 0;
    unsigned char paletteIndex = 0;
};

// CPU-only capture, copy tracking, invalidation, and restoration bookkeeping.
// Pixel views are consumed synchronously and are never retained.
class TextLayer {
public:
    explicit TextLayer(std::size_t recordLimit = 4096);

    bool capture(TextBufferId buffer, bool orphan, const TextRunInfo& run,
        TextRect bounds, TextPixels before, TextPixels after);

    // Call after a blit with a snapshot of the source at blit time and the
    // destination values just before and after it. The rectangle maps
    // sourceRect onto destinationX/Y; clipped pixels retain their coverage.
    // All views are read only during this call.
    void copy(TextBufferId source, TextBufferId destination, bool destinationOrphan,
        TextRect sourceRect, int destinationX, int destinationY,
        bool transparent, unsigned char transparentIndex,
        TextPixels sourceSnapshot, TextPixels destinationBefore, TextPixels destinationAfter);

    // Drop every record for a dead buffer generation.
    void eraseBuffer(TextBufferId buffer);

    // Validate records against the live buffer and return only pixels which
    // still contain their recorded text. Orphan records are never presented.
    std::vector<TextRestorePixel> restorationPixels(TextBufferId buffer,
        TextPixels current, bool includeOrphans = false);

    const std::vector<TextRecord>& records() const;
    std::size_t size() const;

private:
    void trim();
    void removePixels(TextBufferId buffer, const std::unordered_set<std::uint64_t>& coverage);

    std::size_t recordLimit_;
    std::vector<TextRecord> records_;
};

} // namespace scene
} // namespace fallout

#endif // FALLOUT_SCENE_TEXT_LAYER_H
