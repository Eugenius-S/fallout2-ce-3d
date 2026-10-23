#include "text_layer.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>

namespace fallout {
namespace scene {
namespace {

bool validPixels(TextPixels pixels)
{
    return pixels.data != nullptr && pixels.width > 0 && pixels.height > 0
        && pixels.pitch >= pixels.width;
}

bool contains(TextPixels pixels, int x, int y)
{
    return validPixels(pixels) && x >= pixels.originX && y >= pixels.originY
        && static_cast<std::int64_t>(x) < static_cast<std::int64_t>(pixels.originX) + pixels.width
        && static_cast<std::int64_t>(y) < static_cast<std::int64_t>(pixels.originY) + pixels.height;
}

unsigned char pixelAt(TextPixels pixels, int x, int y)
{
    return pixels.data[static_cast<std::size_t>(y - pixels.originY) * pixels.pitch + (x - pixels.originX)];
}

TextRect intersection(TextRect a, TextRect b)
{
    const std::int64_t left = std::max<std::int64_t>(a.x, b.x);
    const std::int64_t top = std::max<std::int64_t>(a.y, b.y);
    const std::int64_t right = std::min<std::int64_t>(static_cast<std::int64_t>(a.x) + a.width,
        static_cast<std::int64_t>(b.x) + b.width);
    const std::int64_t bottom = std::min<std::int64_t>(static_cast<std::int64_t>(a.y) + a.height,
        static_cast<std::int64_t>(b.y) + b.height);
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0
        || right <= left || bottom <= top
        || left < INT32_MIN || top < INT32_MIN || right > INT32_MAX || bottom > INT32_MAX) {
        return {};
    }
    return { static_cast<int>(left), static_cast<int>(top),
        static_cast<int>(right - left), static_cast<int>(bottom - top) };
}

TextRect coverageBounds(const std::vector<TextPixelCoverage>& pixels)
{
    if (pixels.empty()) return {};
    int left = pixels.front().x;
    int right = left;
    int top = pixels.front().y;
    int bottom = top;
    for (const auto& pixel : pixels) {
        left = std::min(left, pixel.x);
        right = std::max(right, pixel.x);
        top = std::min(top, pixel.y);
        bottom = std::max(bottom, pixel.y);
    }
    return { left, top, right - left + 1, bottom - top + 1 };
}

std::uint64_t pixelKey(int x, int y)
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) << 32)
        | static_cast<std::uint32_t>(x);
}

bool traceEnabled()
{
    static const bool enabled = []() {
        const char* value = std::getenv("F2CE_TEXT_TRACE");
        return value != nullptr && value[0] == '1' && value[1] == '\0';
    }();
    return enabled;
}

bool isTraceTarget(const TextRunInfo& run)
{
    static constexpr unsigned char target[] = {
        0xCD, 0xEE, 0xF0, 0xEC, 0xE0, 0xEB, 0xFC, 0xED, 0xEE,
    };
    return run.text.size() == sizeof(target)
        && std::equal(run.text.begin(), run.text.end(), reinterpret_cast<const char*>(target));
}

void traceRun(const char* stage, TextBufferId buffer, const TextRunInfo& run, const char* format, ...)
{
    static std::size_t eventCount = 0;
    if (!traceEnabled() || !isTraceTarget(run) || eventCount >= 1000) return;
    eventCount++;
    std::fprintf(stderr, "F2CE_TEXT_TRACE stage=%s key=%llu:%d:%d:CD-EE-F0-EC-E0-EB-FC-ED-EE font=%d ",
        stage, static_cast<unsigned long long>(buffer), run.x, run.y, run.font);
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
}

} // namespace

TextLayer::TextLayer(std::size_t recordLimit)
    : recordLimit_(std::max<std::size_t>(recordLimit, 1))
{
}

bool TextLayer::capture(TextBufferId buffer, bool orphan, const TextRunInfo& run,
    TextRect bounds, TextPixels before, TextPixels after, int ownerWindow, TextBufferId ownerGeneration)
{
    if (buffer == 0 || !validPixels(before) || !validPixels(after)
        || before.width != after.width || before.height != after.height
        || before.pitch != after.pitch || before.originX != after.originX || before.originY != after.originY) {
        return false;
    }

    const TextRect clipped = intersection(bounds, { before.originX, before.originY, before.width, before.height });
    if (clipped.width <= 0 || clipped.height <= 0) return false;

    TextRecord record;
    record.buffer = buffer;
    record.orphan = orphan;
    record.ownerWindow = ownerWindow;
    record.ownerGeneration = ownerGeneration;
    record.run = run;
    record.bounds = clipped;
    std::unordered_map<std::uint64_t, std::pair<unsigned char, unsigned char>> previous;
    for (const auto& oldRecord : records_) {
        if (oldRecord.buffer != buffer) continue;
        for (const auto& oldPixel : oldRecord.pixels) {
            previous[pixelKey(oldPixel.x, oldPixel.y)] = { oldPixel.after, oldPixel.before };
        }
    }

    for (int y = clipped.y; y < clipped.y + clipped.height; y++) {
        for (int x = clipped.x; x < clipped.x + clipped.width; x++) {
            const unsigned char oldValue = pixelAt(before, x, y);
            const unsigned char newValue = pixelAt(after, x, y);
            record.coverage.push_back({ x, y, newValue });
            if (oldValue == newValue) continue;

            unsigned char underlying = oldValue;
            const auto previousPixel = previous.find(pixelKey(x, y));
            if (previousPixel != previous.end() && previousPixel->second.first == oldValue) {
                underlying = previousPixel->second.second;
            }
            record.pixels.push_back({ x, y, underlying, newValue });
        }
    }

    if (record.pixels.empty()) {
        traceRun("capture_no_ink", buffer, record.run,
            "owner=%d generation=%llu bounds=%d,%d,%d,%d coverage=%zu", record.ownerWindow,
            static_cast<unsigned long long>(record.ownerGeneration), record.bounds.x, record.bounds.y,
            record.bounds.width, record.bounds.height, record.coverage.size());
        return false;
    }
    // A run's bounds include blank line-height and advance space. Replace an
    // older run only when the new draw actually changed one of its glyph pixels.
    std::unordered_set<std::uint64_t> changedPixels;
    changedPixels.reserve(record.pixels.size());
    for (const auto& pixel : record.pixels) changedPixels.insert(pixelKey(pixel.x, pixel.y));
    records_.erase(std::remove_if(records_.begin(), records_.end(), [buffer, &changedPixels, &record](const TextRecord& oldRecord) {
        if (oldRecord.buffer != buffer) return false;
        const bool inkOverlap = std::any_of(oldRecord.pixels.begin(), oldRecord.pixels.end(), [&changedPixels](const TextPixelRecord& pixel) {
            return changedPixels.find(pixelKey(pixel.x, pixel.y)) != changedPixels.end();
        });
        if (inkOverlap) {
            traceRun("retire_ink_overlap", oldRecord.buffer, oldRecord.run,
                "replaced_by=%d:%d new_ink_pixels=%zu old_ink_pixels=%zu", record.run.x, record.run.y,
                record.pixels.size(), oldRecord.pixels.size());
        }
        return inkOverlap;
    }), records_.end());
    traceRun("capture", buffer, record.run,
        "owner=%d generation=%llu bounds=%d,%d,%d,%d coverage=%zu ink=%zu", record.ownerWindow,
        static_cast<unsigned long long>(record.ownerGeneration), record.bounds.x, record.bounds.y,
        record.bounds.width, record.bounds.height, record.coverage.size(), record.pixels.size());
    records_.push_back(std::move(record));
    trim();
    return true;
}

void TextLayer::copy(TextBufferId source, TextBufferId destination, bool destinationOrphan,
    TextRect sourceRect, int destinationX, int destinationY,
    bool transparent, unsigned char transparentIndex,
    TextPixels sourceSnapshot, TextPixels destinationBefore, TextPixels destinationAfter,
    int destinationOwnerWindow, TextBufferId destinationOwnerGeneration)
{
    if (source == 0 || destination == 0 || !validPixels(sourceSnapshot) || !validPixels(destinationBefore)
        || !validPixels(destinationAfter) || destinationBefore.width != destinationAfter.width
        || destinationBefore.height != destinationAfter.height || destinationBefore.pitch != destinationAfter.pitch
        || sourceRect.width <= 0 || sourceRect.height <= 0) {
        return;
    }

    // Restrict source coordinates to the destination's valid area after translation.
    const std::int64_t destinationLeft = destinationBefore.originX;
    const std::int64_t destinationTop = destinationBefore.originY;
    const std::int64_t destinationRight = destinationLeft + destinationBefore.width;
    const std::int64_t destinationBottom = destinationTop + destinationBefore.height;
    const std::int64_t sourceLeft = static_cast<std::int64_t>(sourceRect.x)
        + std::max<std::int64_t>(0, destinationLeft - destinationX);
    const std::int64_t sourceTop = static_cast<std::int64_t>(sourceRect.y)
        + std::max<std::int64_t>(0, destinationTop - destinationY);
    const std::int64_t sourceRight = static_cast<std::int64_t>(sourceRect.x)
        + std::min<std::int64_t>(sourceRect.width, destinationRight - destinationX);
    const std::int64_t sourceBottom = static_cast<std::int64_t>(sourceRect.y)
        + std::min<std::int64_t>(sourceRect.height, destinationBottom - destinationY);
    if (sourceRight <= sourceLeft || sourceBottom <= sourceTop
        || sourceLeft < INT32_MIN || sourceTop < INT32_MIN
        || sourceRight > INT32_MAX || sourceBottom > INT32_MAX) {
        return;
    }

    std::vector<TextRecord> copies;
    for (const auto& sourceRecord : records_) {
        if (sourceRecord.buffer != source) continue;
        TextRecord clone;
        clone.buffer = destination;
        clone.orphan = destinationOrphan;
        clone.ownerWindow = destinationOwnerWindow >= 0 ? destinationOwnerWindow : sourceRecord.ownerWindow;
        clone.ownerGeneration = destinationOwnerWindow >= 0 ? destinationOwnerGeneration : sourceRecord.ownerGeneration;
        clone.run = sourceRecord.run;
        const std::int64_t shiftX = static_cast<std::int64_t>(destinationX) - sourceRect.x;
        const std::int64_t shiftY = static_cast<std::int64_t>(destinationY) - sourceRect.y;
        clone.ownerOffsetX = destinationOwnerWindow >= 0 ? 0 : sourceRecord.ownerOffsetX + static_cast<int>(shiftX);
        clone.ownerOffsetY = destinationOwnerWindow >= 0 ? 0 : sourceRecord.ownerOffsetY + static_cast<int>(shiftY);
        clone.run.x = static_cast<int>(static_cast<std::int64_t>(clone.run.x) + shiftX);
        clone.run.y = static_cast<int>(static_cast<std::int64_t>(clone.run.y) + shiftY);
        for (int& characterX : clone.run.characterX) characterX = static_cast<int>(static_cast<std::int64_t>(characterX) + shiftX);
        for (int& characterCellX : clone.run.characterCellX) characterCellX = static_cast<int>(static_cast<std::int64_t>(characterCellX) + shiftX);

        // Validate only actual legacy glyph deltas. Blank cells belong to the
        // modern glyph mask but may be covered by a cursor or another layer.
        // A stale changed pixel invalidates the whole copied fragment.
        bool validFragment = true;
        bool hasGlyphPixel = false;
        int staleX = -1;
        int staleY = -1;
        int staleExpected = -1;
        int staleActual = -1;
        for (const auto& pixel : sourceRecord.pixels) {
            if (pixel.x < sourceLeft || pixel.x >= sourceRight || pixel.y < sourceTop || pixel.y >= sourceBottom) continue;
            if (!contains(sourceSnapshot, pixel.x, pixel.y)
                || pixelAt(sourceSnapshot, pixel.x, pixel.y) != pixel.after) {
                validFragment = false;
                staleX = pixel.x;
                staleY = pixel.y;
                staleExpected = pixel.after;
                staleActual = contains(sourceSnapshot, pixel.x, pixel.y) ? pixelAt(sourceSnapshot, pixel.x, pixel.y) : -1;
                break;
            }
            if (!(transparent && pixel.after == transparentIndex)) hasGlyphPixel = true;
        }
        if (!validFragment) {
            traceRun("copy_skip_stale", sourceRecord.buffer, sourceRecord.run,
                "destination=%llu coord=%d,%d expected=%d actual=%d", static_cast<unsigned long long>(destination),
                staleX, staleY, staleExpected, staleActual);
            continue;
        }
        if (!hasGlyphPixel) {
            traceRun("copy_skip_no_glyph", sourceRecord.buffer, sourceRecord.run,
                "destination=%llu transparent=%d", static_cast<unsigned long long>(destination), transparent ? 1 : 0);
            continue;
        }

        for (const auto& maskPixel : sourceRecord.coverage) {
            if (maskPixel.x < sourceLeft || maskPixel.x >= sourceRight || maskPixel.y < sourceTop || maskPixel.y >= sourceBottom) continue;
            const std::int64_t translatedX = static_cast<std::int64_t>(destinationX) + (maskPixel.x - sourceRect.x);
            const std::int64_t translatedY = static_cast<std::int64_t>(destinationY) + (maskPixel.y - sourceRect.y);
            if (translatedX < INT32_MIN || translatedX > INT32_MAX || translatedY < INT32_MIN || translatedY > INT32_MAX
                || !contains(destinationBefore, static_cast<int>(translatedX), static_cast<int>(translatedY))) continue;
            const int destX = static_cast<int>(translatedX);
            const int destY = static_cast<int>(translatedY);
            if (!contains(sourceSnapshot, maskPixel.x, maskPixel.y)
                || pixelAt(sourceSnapshot, maskPixel.x, maskPixel.y) != maskPixel.expected
                || !contains(destinationBefore, destX, destY)
                || !contains(destinationAfter, destX, destY)) continue;
            const bool skippedByTransparency = transparent && maskPixel.expected == transparentIndex;
            if (skippedByTransparency) {
                if (pixelAt(destinationAfter, destX, destY) != pixelAt(destinationBefore, destX, destY)) continue;
            } else if (pixelAt(destinationAfter, destX, destY) != maskPixel.expected) {
                continue;
            }
            clone.coverage.push_back({ destX, destY, pixelAt(destinationAfter, destX, destY) });
        }
        for (const auto& pixel : sourceRecord.pixels) {
            if (pixel.x < sourceLeft || pixel.x >= sourceRight || pixel.y < sourceTop || pixel.y >= sourceBottom) continue;
            const std::int64_t translatedX = static_cast<std::int64_t>(destinationX) + (pixel.x - sourceRect.x);
            const std::int64_t translatedY = static_cast<std::int64_t>(destinationY) + (pixel.y - sourceRect.y);
            if (translatedX < INT32_MIN || translatedX > INT32_MAX || translatedY < INT32_MIN || translatedY > INT32_MAX
                || !contains(destinationBefore, static_cast<int>(translatedX), static_cast<int>(translatedY))) continue;
            const int destX = static_cast<int>(translatedX);
            const int destY = static_cast<int>(translatedY);
            if (!contains(destinationBefore, destX, destY) || !contains(destinationAfter, destX, destY)) continue;
            if (!contains(sourceSnapshot, pixel.x, pixel.y) || pixelAt(sourceSnapshot, pixel.x, pixel.y) != pixel.after) continue;
            if (transparent && pixel.after == transparentIndex) continue;
            const unsigned char beforeValue = pixelAt(destinationBefore, destX, destY);
            const unsigned char afterValue = pixelAt(destinationAfter, destX, destY);
            if (afterValue != pixel.after) continue;

            // Opaque copies carry the source background with the glyph. A
            // transparent copy whose source background is itself transparent
            // leaves the destination background in place.
            unsigned char underlying = pixel.before;
            if (transparent && pixel.before == transparentIndex) {
                underlying = beforeValue;
                for (auto recordIt = records_.rbegin(); recordIt != records_.rend(); ++recordIt) {
                    if (recordIt->buffer != destination) continue;
                    const auto found = std::find_if(recordIt->pixels.begin(), recordIt->pixels.end(),
                        [destX, destY, beforeValue](const TextPixelRecord& oldPixel) {
                            return oldPixel.x == destX && oldPixel.y == destY && oldPixel.after == beforeValue;
                        });
                    if (found != recordIt->pixels.end()) {
                        underlying = found->before;
                        break;
                    }
                }
            }
            clone.pixels.push_back({ destX, destY, underlying, afterValue });
        }
        if (!clone.coverage.empty()) {
            clone.bounds = coverageBounds(clone.coverage);
            traceRun("copy_clone", sourceRecord.buffer, sourceRecord.run,
                "destination=%llu new_xy=%d,%d owner=%d generation=%llu bounds=%d,%d,%d,%d coverage=%zu ink=%zu",
                static_cast<unsigned long long>(destination), clone.run.x, clone.run.y, clone.ownerWindow,
                static_cast<unsigned long long>(clone.ownerGeneration), clone.bounds.x, clone.bounds.y,
                clone.bounds.width, clone.bounds.height, clone.coverage.size(), clone.pixels.size());
            copies.push_back(std::move(clone));
        } else {
            traceRun("copy_skip_empty_mask", sourceRecord.buffer, sourceRecord.run,
                "destination=%llu", static_cast<unsigned long long>(destination));
        }
    }

    // Retire only existing destination text cells covered by the blit. This
    // keeps full-frame image copies proportional to tracked text, not pixels.
    // Clones were already snapshotted above, so same-buffer scrolls are safe.
    for (auto recordIt = records_.begin(); recordIt != records_.end();) {
        if (recordIt->buffer != destination) {
            ++recordIt;
            continue;
        }
        auto wasWritten = [&](int x, int y) {
            const std::int64_t offsetX = static_cast<std::int64_t>(x) - destinationX;
            const std::int64_t offsetY = static_cast<std::int64_t>(y) - destinationY;
            if (offsetX < 0 || offsetY < 0 || offsetX >= sourceRect.width || offsetY >= sourceRect.height
                || !contains(destinationBefore, x, y) || !contains(destinationAfter, x, y)) return false;
            if (!transparent) return true;
            const std::int64_t sourcePixelX = static_cast<std::int64_t>(sourceRect.x) + offsetX;
            const std::int64_t sourcePixelY = static_cast<std::int64_t>(sourceRect.y) + offsetY;
            if (sourcePixelX < INT32_MIN || sourcePixelX > INT32_MAX || sourcePixelY < INT32_MIN || sourcePixelY > INT32_MAX) return false;
            const int sx = static_cast<int>(sourcePixelX);
            const int sy = static_cast<int>(sourcePixelY);
            return contains(sourceSnapshot, sx, sy) && pixelAt(sourceSnapshot, sx, sy) != transparentIndex;
        };
        const bool traceRelevant = traceEnabled() && isTraceTarget(recordIt->run)
            && intersection(recordIt->bounds, { destinationX, destinationY, sourceRect.width, sourceRect.height }).width > 0;
        const std::size_t oldCoverageCount = recordIt->coverage.size();
        const std::size_t oldPixelCount = recordIt->pixels.size();
        auto& mask = recordIt->coverage;
        mask.erase(std::remove_if(mask.begin(), mask.end(), [&](const TextPixelCoverage& pixel) {
            return wasWritten(pixel.x, pixel.y);
        }), mask.end());
        auto& pixels = recordIt->pixels;
        pixels.erase(std::remove_if(pixels.begin(), pixels.end(), [&](const TextPixelRecord& pixel) {
            return wasWritten(pixel.x, pixel.y);
        }), pixels.end());
        if (traceRelevant) {
            traceRun("known_blit_clip", recordIt->buffer, recordIt->run,
                "kind=copy source=%llu destination=%llu rect=%d,%d,%d,%d coverage_removed=%zu ink_removed=%zu remaining_coverage=%zu remaining_ink=%zu",
                static_cast<unsigned long long>(source), static_cast<unsigned long long>(destination), destinationX, destinationY,
                sourceRect.width, sourceRect.height, oldCoverageCount - mask.size(), oldPixelCount - pixels.size(),
                mask.size(), pixels.size());
        }
        recordIt->bounds = coverageBounds(mask);
        if (mask.empty()) recordIt = records_.erase(recordIt);
        else ++recordIt;
    }
    for (auto& clone : copies) records_.push_back(std::move(clone));
    trim();
}

void TextLayer::overwrite(TextBufferId destination, TextRect destinationRect,
    TextPixels sourceSnapshot, bool transparent, unsigned char transparentIndex)
{
    if (destination == 0 || destinationRect.width <= 0 || destinationRect.height <= 0) return;
    const std::int64_t right = static_cast<std::int64_t>(destinationRect.x) + destinationRect.width;
    const std::int64_t bottom = static_cast<std::int64_t>(destinationRect.y) + destinationRect.height;
    for (auto recordIt = records_.begin(); recordIt != records_.end();) {
        if (recordIt->buffer != destination) {
            ++recordIt;
            continue;
        }
        auto wasWritten = [&](int x, int y) {
            if (x < destinationRect.x || y < destinationRect.y || x >= right || y >= bottom) return false;
            if (!transparent) return true;
            const std::int64_t sourceX = static_cast<std::int64_t>(sourceSnapshot.originX) + x - destinationRect.x;
            const std::int64_t sourceY = static_cast<std::int64_t>(sourceSnapshot.originY) + y - destinationRect.y;
            if (sourceX < INT32_MIN || sourceX > INT32_MAX || sourceY < INT32_MIN || sourceY > INT32_MAX) return false;
            const int sx = static_cast<int>(sourceX);
            const int sy = static_cast<int>(sourceY);
            if (!contains(sourceSnapshot, sx, sy)) return false;
            return pixelAt(sourceSnapshot, sx, sy) != transparentIndex;
        };
        const bool traceRelevant = traceEnabled() && isTraceTarget(recordIt->run)
            && intersection(recordIt->bounds, destinationRect).width > 0;
        const std::size_t oldCoverageCount = recordIt->coverage.size();
        const std::size_t oldPixelCount = recordIt->pixels.size();
        auto& mask = recordIt->coverage;
        mask.erase(std::remove_if(mask.begin(), mask.end(), [&](const TextPixelCoverage& pixel) {
            return wasWritten(pixel.x, pixel.y);
        }), mask.end());
        auto& pixels = recordIt->pixels;
        pixels.erase(std::remove_if(pixels.begin(), pixels.end(), [&](const TextPixelRecord& pixel) {
            return wasWritten(pixel.x, pixel.y);
        }), pixels.end());
        if (traceRelevant) {
            traceRun("known_blit_clip", recordIt->buffer, recordIt->run,
                "kind=overwrite rect=%d,%d,%d,%d transparent=%d coverage_removed=%zu ink_removed=%zu remaining_coverage=%zu remaining_ink=%zu",
                destinationRect.x, destinationRect.y, destinationRect.width, destinationRect.height, transparent ? 1 : 0,
                oldCoverageCount - mask.size(), oldPixelCount - pixels.size(), mask.size(), pixels.size());
        }
        recordIt->bounds = coverageBounds(mask);
        if (mask.empty()) recordIt = records_.erase(recordIt);
        else ++recordIt;
    }
}

void TextLayer::eraseBuffer(TextBufferId buffer)
{
    records_.erase(std::remove_if(records_.begin(), records_.end(), [buffer](const TextRecord& record) {
        if (record.buffer == buffer) {
            traceRun("erase_buffer", record.buffer, record.run, "owner=%d generation=%llu",
                record.ownerWindow, static_cast<unsigned long long>(record.ownerGeneration));
        }
        return record.buffer == buffer;
    }), records_.end());
}

void TextLayer::eraseOwner(int window, TextBufferId generation)
{
    records_.erase(std::remove_if(records_.begin(), records_.end(), [window, generation](const TextRecord& record) {
        if (record.ownerWindow == window && record.ownerGeneration == generation) {
            traceRun("erase_owner", record.buffer, record.run, "owner=%d generation=%llu",
                record.ownerWindow, static_cast<unsigned long long>(record.ownerGeneration));
        }
        return record.ownerWindow == window && record.ownerGeneration == generation;
    }), records_.end());
}

std::vector<TextRestorePixel> TextLayer::restorationPixels(TextBufferId buffer,
    TextPixels current, bool includeOrphans, bool sharpEligibleOnly)
{
    return restorationPixels(buffer, current, {}, includeOrphans, sharpEligibleOnly);
}

std::vector<TextRestorePixel> TextLayer::restorationPixels(TextBufferId buffer,
    TextPixels current, const std::vector<TextValidationPixels>& ownerPixels,
    bool includeOrphans, bool sharpEligibleOnly)
{
    std::vector<TextRestorePixel> result;
    if (buffer == 0 || !validPixels(current)) return result;
    for (auto recordIt = records_.begin(); recordIt != records_.end();) {
        if (recordIt->buffer != buffer || (recordIt->orphan && !includeOrphans)) {
            ++recordIt;
            continue;
        }
        const auto& pixels = recordIt->pixels;
        bool valid = !recordIt->coverage.empty() && !pixels.empty();
        const TextPixels* validation = &current;
        TextBufferId validationBuffer = buffer;
        int validationOffsetX = 0;
        int validationOffsetY = 0;
        if (recordIt->ownerWindow >= 0 && recordIt->ownerGeneration != buffer) {
            for (const TextValidationPixels& owner : ownerPixels) {
                if (owner.buffer == recordIt->ownerGeneration && validPixels(owner.pixels)) {
                    validation = &owner.pixels;
                    validationBuffer = owner.buffer;
                    validationOffsetX = recordIt->ownerOffsetX;
                    validationOffsetY = recordIt->ownerOffsetY;
                    break;
                }
            }
        }
        if (valid) {
            for (const auto& pixel : pixels) {
                const int validationX = pixel.x - validationOffsetX;
                const int validationY = pixel.y - validationOffsetY;
                if (!contains(*validation, validationX, validationY)
                    || pixelAt(*validation, validationX, validationY) != pixel.after) {
                    valid = false;
                    const int actual = contains(*validation, validationX, validationY)
                        ? pixelAt(*validation, validationX, validationY) : -1;
                    traceRun("validity_prune", recordIt->buffer, recordIt->run,
                        "reason=%s coord=%d,%d validation_coord=%d,%d expected=%u actual=%d validation_buffer=%llu owner=%d generation=%llu offset=%d,%d",
                        validationBuffer == recordIt->buffer ? "screen_pixel_mismatch" : "owner_pixel_mismatch",
                        pixel.x, pixel.y, validationX, validationY, static_cast<unsigned int>(pixel.after), actual,
                        static_cast<unsigned long long>(validationBuffer), recordIt->ownerWindow,
                        static_cast<unsigned long long>(recordIt->ownerGeneration), validationOffsetX, validationOffsetY);
                    break;
                }
            }
        }
        if (!valid) {
            if (pixels.empty() || recordIt->coverage.empty()) {
                traceRun("validity_prune", recordIt->buffer, recordIt->run,
                    "reason=empty_record coverage=%zu ink=%zu owner=%d generation=%llu",
                    recordIt->coverage.size(), pixels.size(), recordIt->ownerWindow,
                    static_cast<unsigned long long>(recordIt->ownerGeneration));
            }
            recordIt = records_.erase(recordIt);
            continue;
        }
        if (!sharpEligibleOnly || recordIt->run.sharpEligible) {
            for (const auto& pixel : pixels) {
                result.push_back({ pixel.x, pixel.y, pixel.before, recordIt->ownerWindow, recordIt->ownerGeneration });
            }
        }
        ++recordIt;
    }
    return result;
}

const std::vector<TextRecord>& TextLayer::records() const
{
    return records_;
}

std::size_t TextLayer::size() const
{
    return records_.size();
}

void TextLayer::trim()
{
    if (records_.size() > recordLimit_) {
        records_.erase(records_.begin(), records_.begin() + (records_.size() - recordLimit_));
    }
}

} // namespace scene
} // namespace fallout
