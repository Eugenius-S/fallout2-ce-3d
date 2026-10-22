#include "text_capture.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <string>
#include <utility>

#include "../text_font.h"

namespace fallout {
namespace {

struct BufferRange {
    scene::TextBufferId id = 0;
    std::uintptr_t base = 0;
    std::size_t size = 0;
    int width = 0;
    int height = 0;
    int pitch = 0;
    bool orphan = true;
    bool allocated = false;
    int externalReferences = 0;
};

struct PixelSnapshot {
    std::vector<unsigned char> pixels;
    scene::TextPixels view;

    PixelSnapshot() = default;
    PixelSnapshot(const PixelSnapshot&) = delete;
    PixelSnapshot& operator=(const PixelSnapshot&) = delete;
    PixelSnapshot(PixelSnapshot&& other) noexcept
        : pixels(std::move(other.pixels))
        , view(other.view)
    {
        view.data = pixels.data();
    }
    PixelSnapshot& operator=(PixelSnapshot&& other) noexcept
    {
        if (this != &other) {
            pixels = std::move(other.pixels);
            view = other.view;
            view.data = pixels.data();
        }
        return *this;
    }
};

std::map<std::uintptr_t, BufferRange>& ranges()
{
    static std::map<std::uintptr_t, BufferRange> value;
    return value;
}

std::recursive_mutex& captureMutex()
{
    static std::recursive_mutex value;
    return value;
}

scene::TextLayer& layer()
{
    static scene::TextLayer value;
    return value;
}

std::uint64_t& nextBufferId()
{
    static std::uint64_t value = 1;
    return value;
}

int& drawDepth()
{
    static int value = 0;
    return value;
}

BufferRange* findRange(const void* pointer)
{
    if (pointer == nullptr) return nullptr;
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pointer);
    auto it = ranges().upper_bound(address);
    if (it == ranges().begin()) return nullptr;
    --it;
    const std::uintptr_t base = it->second.base;
    if (address < base || address - base >= it->second.size) return nullptr;
    return &it->second;
}

bool setPitch(BufferRange& range, int pitch)
{
    if (pitch <= 0) return false;
    if (range.pitch != 0) return range.pitch == pitch;
    const std::size_t rows = range.size / static_cast<std::size_t>(pitch);
    if (rows == 0 || rows > static_cast<std::size_t>(std::numeric_limits<int>::max())) return false;
    range.width = pitch;
    range.height = static_cast<int>(rows);
    range.pitch = pitch;
    return true;
}

bool pointerPosition(const BufferRange& range, const void* pointer, int& x, int& y)
{
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pointer);
    if (address < range.base || address - range.base >= range.size || range.pitch <= 0) return false;
    const std::size_t offset = address - range.base;
    const std::size_t row = offset / static_cast<std::size_t>(range.pitch);
    const std::size_t column = offset % static_cast<std::size_t>(range.pitch);
    if (row >= static_cast<std::size_t>(range.height) || column >= static_cast<std::size_t>(range.width)) return false;
    x = static_cast<int>(column);
    y = static_cast<int>(row);
    return true;
}

scene::TextRect clippedRect(scene::TextRect rect, const BufferRange& range)
{
    const std::int64_t left = std::max<std::int64_t>(0, rect.x);
    const std::int64_t top = std::max<std::int64_t>(0, rect.y);
    const std::int64_t right = std::min<std::int64_t>(range.width, static_cast<std::int64_t>(rect.x) + rect.width);
    const std::int64_t bottom = std::min<std::int64_t>(range.height, static_cast<std::int64_t>(rect.y) + rect.height);
    if (rect.width <= 0 || rect.height <= 0 || right <= left || bottom <= top) return {};
    return { static_cast<int>(left), static_cast<int>(top), static_cast<int>(right - left), static_cast<int>(bottom - top) };
}

PixelSnapshot snapshot(const BufferRange& range, scene::TextRect rect)
{
    PixelSnapshot result;
    rect = clippedRect(rect, range);
    if (rect.width <= 0 || rect.height <= 0) return result;
    result.pixels.resize(static_cast<std::size_t>(rect.width) * rect.height);
    const auto* base = reinterpret_cast<const unsigned char*>(range.base);
    for (int y = 0; y < rect.height; ++y) {
        const std::size_t sourceOffset = static_cast<std::size_t>(rect.y + y) * range.pitch + rect.x;
        std::copy(base + sourceOffset, base + sourceOffset + rect.width,
            result.pixels.data() + static_cast<std::size_t>(y) * rect.width);
    }
    result.view = { result.pixels.data(), rect.width, rect.height, rect.width, rect.x, rect.y };
    return result;
}

BufferRange* rangeForCopy(const void* pointer, int pitch, int width, int height, int& x, int& y)
{
    BufferRange* range = findRange(pointer);
    if (range == nullptr || !setPitch(*range, pitch) || !pointerPosition(*range, pointer, x, y)) return nullptr;
    if (width <= 0 || height <= 0 || x + static_cast<std::int64_t>(width) > range->width
        || y + static_cast<std::int64_t>(height) > range->height) return nullptr;
    return range;
}

bool captureMetrics(const char* string, int length, ColorWithFlags color,
    std::string& acceptedText, std::vector<int>& characterOffsets,
    std::vector<int>& cellOffsets, int& width, int& cellWidth, int& lineHeight)
{
    if (string == nullptr || length < 0 || fontGetCharacterWidth == nullptr || fontGetLetterSpacing == nullptr
        || fontGetLineHeight == nullptr || fontGetMonospacedCharacterWidth == nullptr) return false;
    acceptedText.clear();
    characterOffsets.clear();
    cellOffsets.clear();
    width = 0;
    cellWidth = 0;
    lineHeight = fontGetLineHeight();
    if (lineHeight <= 0) return false;
    const bool monospaced = (color & DRAW_TEXT_FLAG_MONOSPACED) != 0;
    cellWidth = monospaced ? fontGetMonospacedCharacterWidth() : 0;
    if (monospaced && cellWidth <= 0) return false;
    const int spacing = fontGetLetterSpacing();
    const int glyphCount = fontGetCurrentGlyphCount();
    for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(string); *cursor != '\0'; ++cursor) {
        const unsigned char character = *cursor;
        if (character >= glyphCount) continue;
        const int glyphWidth = fontGetCharacterWidth(character);
        const int advance = monospaced ? cellWidth : glyphWidth + spacing;
        if (advance < 0 || static_cast<std::int64_t>(width) + advance > length) break;
        const int glyphOffset = monospaced ? width + (cellWidth - glyphWidth - spacing) / 2 : width;
        characterOffsets.push_back(glyphOffset);
        if (monospaced) cellOffsets.push_back(width);
        acceptedText.push_back(static_cast<char>(character));
        width += advance;
    }
    return true;
}

} // namespace

void textCaptureAllocationCreated(void* buffer, std::size_t size)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (buffer == nullptr || size == 0) return;
    const auto base = reinterpret_cast<std::uintptr_t>(buffer);
    auto existing = ranges().find(base);
    if (existing != ranges().end()) {
        layer().eraseBuffer(existing->second.id);
        ranges().erase(existing);
    }
    const std::uintptr_t end = base + size;
    if (end < base) return;
    auto next = ranges().lower_bound(base);
    if (next != ranges().end() && next->first < end) return;
    if (next != ranges().begin()) {
        const auto previous = std::prev(next);
        if (previous->second.base + previous->second.size > base) return;
    }
    BufferRange range;
    range.id = nextBufferId()++;
    if (range.id == 0) range.id = nextBufferId()++;
    range.base = base;
    range.size = size;
    range.allocated = true;
    ranges()[base] = range;
}

void textCaptureAllocationDestroyed(void* buffer)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (buffer == nullptr) return;
    const auto base = reinterpret_cast<std::uintptr_t>(buffer);
    auto it = ranges().find(base);
    if (it == ranges().end()) return;
    layer().eraseBuffer(it->second.id);
    ranges().erase(it);
}

void textCaptureAllocationResized(void* oldBuffer, void* newBuffer, std::size_t newSize)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (oldBuffer == nullptr) {
        textCaptureAllocationCreated(newBuffer, newSize);
    } else if (newSize == 0) {
        textCaptureAllocationDestroyed(oldBuffer);
    } else if (newBuffer != nullptr) {
        textCaptureAllocationDestroyed(oldBuffer);
        textCaptureAllocationCreated(newBuffer, newSize);
    }
}

void textCaptureRegisterExternalBuffer(void* buffer, std::size_t size, int width, int height, int pitch, bool orphan)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (buffer == nullptr || size == 0 || width <= 0 || height <= 0 || pitch < width
        || static_cast<std::uint64_t>(height - 1) * pitch + width > size) return;
    const auto base = reinterpret_cast<std::uintptr_t>(buffer);
    auto it = ranges().find(base);
    if (it != ranges().end()) {
        BufferRange& range = it->second;
        if (range.size < size || (range.width != 0 && (range.width != width || range.pitch != pitch))) return;
        range.width = width;
        range.height = height;
        range.pitch = pitch;
        range.orphan = range.orphan && orphan;
        range.externalReferences++;
        return;
    }
    const std::uintptr_t end = base + size;
    if (end < base) return;
    auto next = ranges().lower_bound(base);
    if (next != ranges().end() && next->first < end) return;
    if (next != ranges().begin()) {
        const auto previous = std::prev(next);
        if (previous->second.base + previous->second.size > base) return;
    }
    BufferRange range;
    range.id = nextBufferId()++;
    if (range.id == 0) range.id = nextBufferId()++;
    range.base = base;
    range.size = size;
    range.width = width;
    range.height = height;
    range.pitch = pitch;
    range.orphan = orphan;
    range.externalReferences = 1;
    ranges()[base] = range;
}

void textCaptureUnregisterExternalBuffer(void* buffer)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (buffer == nullptr) return;
    const auto base = reinterpret_cast<std::uintptr_t>(buffer);
    auto it = ranges().find(base);
    if (it == ranges().end() || it->second.externalReferences <= 0) return;
    it->second.externalReferences--;
    if (it->second.externalReferences == 0 && !it->second.allocated) {
        layer().eraseBuffer(it->second.id);
        ranges().erase(it);
    }
}

void textCaptureMarkBufferOwned(void* buffer)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (BufferRange* range = findRange(buffer)) range->orphan = false;
}

scene::TextBufferId textCaptureBufferId(const void* buffer)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    BufferRange* range = findRange(buffer);
    return range != nullptr ? range->id : 0;
}

void textCaptureDrawText(unsigned char* buffer, const char* string, int length, int pitch, ColorWithFlags color)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (fontDrawTextPtr == nullptr) return;
    if (drawDepth() != 0) {
        fontDrawTextPtr(buffer, string, length, pitch, color);
        return;
    }
    BufferRange* range = findRange(buffer);
    if (range == nullptr || !setPitch(*range, pitch)) {
        fontDrawTextPtr(buffer, string, length, pitch, color);
        return;
    }
    int x = 0;
    int y = 0;
    if (!pointerPosition(*range, buffer, x, y)) {
        fontDrawTextPtr(buffer, string, length, pitch, color);
        return;
    }

    std::string acceptedText;
    std::vector<int> characterOffsets;
    std::vector<int> cellOffsets;
    int width = 0;
    int cellWidth = 0;
    int lineHeight = 0;
    if (string == nullptr || length < 0 || !captureMetrics(string, length, color,
            acceptedText, characterOffsets, cellOffsets, width, cellWidth, lineHeight)) {
        fontDrawTextPtr(buffer, string, length, pitch, color);
        return;
    }
    const bool shadowed = (color & DRAW_TEXT_FLAG_SHADOWED) != 0;
    scene::TextRect bounds { x, y, std::max(1, width + (shadowed ? 1 : 0)), std::max(1, lineHeight + (shadowed ? 1 : 0)) };
    const scene::TextRect clipped = clippedRect(bounds, *range);
    PixelSnapshot before = snapshot(*range, clipped);

    drawDepth()++;
    fontDrawTextPtr(buffer, string, length, pitch, color);
    drawDepth()--;

    PixelSnapshot after = snapshot(*range, clipped);
    if (before.view.data == nullptr || after.view.data == nullptr) return;
    scene::TextRunInfo run;
    run.text = std::move(acceptedText);
    run.font = fontGetCurrent();
    run.colorAndFlags = static_cast<unsigned int>(color);
    run.x = x;
    run.y = y;
    run.width = width;
    run.cellWidth = cellWidth;
    run.lineHeight = lineHeight;
    run.characterX.reserve(characterOffsets.size());
    for (int offset : characterOffsets) run.characterX.push_back(x + offset);
    run.characterCellX.reserve(cellOffsets.size());
    for (int offset : cellOffsets) run.characterCellX.push_back(x + offset);
    layer().capture(range->id, range->orphan, run, clipped, before.view, after.view);
}

void textCaptureCopy(const unsigned char* source, int width, int height, int sourcePitch,
    unsigned char* destination, int destinationPitch, bool transparent, TextPixelCopyProc* copyProc)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (copyProc == nullptr) return;
    int sourceX = 0;
    int sourceY = 0;
    int destinationX = 0;
    int destinationY = 0;
    BufferRange* sourceRange = rangeForCopy(source, sourcePitch, width, height, sourceX, sourceY);
    BufferRange* destinationRange = rangeForCopy(destination, destinationPitch, width, height, destinationX, destinationY);
    if (sourceRange == nullptr || destinationRange == nullptr) {
        copyProc(destination, destinationPitch, source, sourcePitch, width, height);
        return;
    }

    bool hasSourceText = false;
    for (const auto& record : layer().records()) {
        if (record.buffer != sourceRange->id) continue;
        const std::int64_t right = static_cast<std::int64_t>(sourceX) + width;
        const std::int64_t bottom = static_cast<std::int64_t>(sourceY) + height;
        if (record.bounds.x < right && sourceX < static_cast<std::int64_t>(record.bounds.x) + record.bounds.width
            && record.bounds.y < bottom && sourceY < static_cast<std::int64_t>(record.bounds.y) + record.bounds.height) {
            hasSourceText = true;
            break;
        }
    }
    if (!hasSourceText) {
        copyProc(destination, destinationPitch, source, sourcePitch, width, height);
        return;
    }

    const scene::TextRect sourceRect { sourceX, sourceY, width, height };
    const scene::TextRect destinationRect { destinationX, destinationY, width, height };
    PixelSnapshot sourceSnapshot = snapshot(*sourceRange, sourceRect);
    PixelSnapshot destinationBefore = snapshot(*destinationRange, destinationRect);
    copyProc(destination, destinationPitch, source, sourcePitch, width, height);
    PixelSnapshot destinationAfter = snapshot(*destinationRange, destinationRect);
    layer().copy(sourceRange->id, destinationRange->id, destinationRange->orphan,
        sourceRect, destinationX, destinationY, transparent, 0,
        sourceSnapshot.view, destinationBefore.view, destinationAfter.view);
}

namespace scene {

const std::vector<TextRecord> textCaptureRecords(TextBufferId buffer)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    std::vector<TextRecord> result;
    for (const auto& record : layer().records()) {
        if (record.buffer == buffer && !record.orphan) result.push_back(record);
    }
    return result;
}

std::vector<TextRestorePixel> textCaptureRestoration(TextBufferId buffer)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    for (const auto& item : ranges()) {
        const BufferRange& range = item.second;
        if (range.id != buffer || range.orphan || range.pitch <= 0) continue;
        const auto* pixels = reinterpret_cast<const unsigned char*>(range.base);
        return layer().restorationPixels(buffer, { pixels, range.width, range.height, range.pitch });
    }
    return {};
}

} // namespace scene
} // namespace fallout
