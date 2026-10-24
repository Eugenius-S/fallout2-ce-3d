#include "text_capture.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <string>
#include <utility>

#include "../text_font.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

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
    int ownerWindow = -1;
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

struct TextOwnerCopySnapshot {
    scene::TextBufferId buffer = 0;
    PixelSnapshot snapshot;
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

TextCaptureTag& activeTextCaptureTag()
{
    static thread_local TextCaptureTag value = TextCaptureTag::None;
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

bool hasTrackedCoverage(scene::TextBufferId buffer, scene::TextRect rect)
{
    const std::int64_t right = static_cast<std::int64_t>(rect.x) + rect.width;
    const std::int64_t bottom = static_cast<std::int64_t>(rect.y) + rect.height;
    if (rect.width <= 0 || rect.height <= 0) return false;
    for (const auto& record : layer().records()) {
        if (record.buffer != buffer) continue;
        for (const auto& pixel : record.coverage) {
            if (pixel.x >= rect.x && pixel.y >= rect.y && pixel.x < right && pixel.y < bottom) return true;
        }
    }
    return false;
}

bool snapshotBlitSource(const unsigned char* source, int sourcePitch, int width, int height,
    const BufferRange* allocation, PixelSnapshot& result)
{
    if (source == nullptr || allocation == nullptr || width <= 0 || height <= 0 || sourcePitch < width) return false;
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(source);
    if (address < allocation->base) return false;
    const std::uint64_t offset = address - allocation->base;
    const std::uint64_t rows = static_cast<std::uint64_t>(height - 1);
    const std::uint64_t pitch = static_cast<std::uint64_t>(sourcePitch);
    const std::uint64_t rowWidth = static_cast<std::uint64_t>(width);
    const std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
    if (rows != 0 && pitch > (max - rowWidth) / rows) return false;
    const std::uint64_t footprint = rows * pitch + rowWidth;
    if (offset > allocation->size || footprint > allocation->size - offset) return false;
    if (rowWidth > max / static_cast<std::uint64_t>(height)) return false;
    const std::uint64_t pixelCount = rowWidth * static_cast<std::uint64_t>(height);
    if (pixelCount > std::numeric_limits<std::size_t>::max()) return false;

    result.pixels.resize(static_cast<std::size_t>(pixelCount));
    for (int y = 0; y < height; y++) {
        std::memcpy(result.pixels.data() + static_cast<std::size_t>(y) * width,
            source + static_cast<std::size_t>(y) * sourcePitch, static_cast<std::size_t>(width));
    }
    result.view = { result.pixels.data(), width, height, width, 0, 0 };
    return true;
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

bool textTraceEnabled()
{
    static const bool enabled = []() {
        const char* value = std::getenv("F2CE_TEXT_TRACE");
        return value != nullptr && value[0] == '1' && value[1] == '\0';
    }();
    return enabled;
}

bool isTextTraceTarget(const std::string& text)
{
    static constexpr unsigned char target[] = {
        0xCD, 0xEE, 0xF0, 0xEC, 0xE0, 0xEB, 0xFC, 0xED, 0xEE,
    };
    return text.size() == sizeof(target)
        && std::equal(text.begin(), text.end(), reinterpret_cast<const char*>(target));
}

void traceUntrackedCopySource(const BufferRange* sourceRange, const BufferRange* destinationRange,
    const unsigned char* source, int sourcePitch, int width, int height,
    int destinationX, int destinationY, bool transparent)
{
    static std::size_t eventCount = 0;
    if (!textTraceEnabled() || sourceRange != nullptr || destinationRange == nullptr || eventCount >= 1000) return;
    const std::uintptr_t sourceAddress = reinterpret_cast<std::uintptr_t>(source);
    const bool sourceHasRange = findRange(source) != nullptr;
    const BufferRange* candidate = findRange(source);
    const std::uintptr_t sourceOffset = candidate != nullptr && sourceAddress >= candidate->base
        ? sourceAddress - candidate->base : 0;
    const std::int64_t destinationRight = static_cast<std::int64_t>(destinationX) + width;
    const std::int64_t destinationBottom = static_cast<std::int64_t>(destinationY) + height;
    for (const auto& record : layer().records()) {
        if (record.buffer != destinationRange->id || !isTextTraceTarget(record.run.text)
            || record.bounds.x >= destinationRight || destinationX >= static_cast<std::int64_t>(record.bounds.x) + record.bounds.width
            || record.bounds.y >= destinationBottom || destinationY >= static_cast<std::int64_t>(record.bounds.y) + record.bounds.height) continue;
        eventCount++;
        std::fprintf(stderr,
            "F2CE_TEXT_TRACE stage=adapter_source_range_miss key=%llu:%d:%d:CD-EE-F0-EC-E0-EB-FC-ED-EE source=%p source_range=%d source_base=0x%llx source_size=%zu source_offset=%llu source_pitch=%d source_range_pitch=%d source_range_width=%d source_range_height=%d rect=%d,%d destination=%d,%d transparent=%d dest_buffer=%llu owner=%d generation=%llu\n",
            static_cast<unsigned long long>(record.buffer), record.run.x, record.run.y, static_cast<const void*>(source), sourceHasRange ? 1 : 0,
            static_cast<unsigned long long>(candidate != nullptr ? candidate->base : 0),
            candidate != nullptr ? candidate->size : 0, static_cast<unsigned long long>(sourceOffset), sourcePitch,
            candidate != nullptr ? candidate->pitch : 0, candidate != nullptr ? candidate->width : 0,
            candidate != nullptr ? candidate->height : 0, width, height, destinationX, destinationY,
            transparent ? 1 : 0, static_cast<unsigned long long>(destinationRange->id), record.ownerWindow,
            static_cast<unsigned long long>(record.ownerGeneration));
        if (eventCount >= 1000) break;
    }
}

} // namespace

ScopedTextCaptureTag::ScopedTextCaptureTag(TextCaptureTag tag)
    : previous_(activeTextCaptureTag())
{
    activeTextCaptureTag() = tag;
}

ScopedTextCaptureTag::~ScopedTextCaptureTag()
{
    activeTextCaptureTag() = previous_;
}

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
        int ownerWindow = -1;
        const auto old = ranges().find(reinterpret_cast<std::uintptr_t>(oldBuffer));
        if (old != ranges().end()) ownerWindow = old->second.ownerWindow;
        textCaptureAllocationDestroyed(oldBuffer);
        textCaptureAllocationCreated(newBuffer, newSize);
        const auto replacement = ranges().find(reinterpret_cast<std::uintptr_t>(newBuffer));
        if (replacement != ranges().end()) replacement->second.ownerWindow = ownerWindow;
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

void textCaptureMarkWindowBuffer(void* buffer, int window)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (BufferRange* range = findRange(buffer)) {
        range->orphan = false;
        range->ownerWindow = window;
    }
}

void textCaptureForgetWindow(int window, void* buffer)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    if (BufferRange* range = findRange(buffer)) {
        layer().eraseOwner(window, range->id);
        range->ownerWindow = -1;
    }
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
    run.sharpEligible = activeTextCaptureTag() == TextCaptureTag::Conversation
        && run.font == 101 && static_cast<Color>(static_cast<unsigned int>(color) & 0xFF) == COLOR_GREEN;
    run.x = x;
    run.y = y;
    run.width = width;
    run.cellWidth = cellWidth;
    run.lineHeight = lineHeight;
    run.characterX.reserve(characterOffsets.size());
    for (int offset : characterOffsets) run.characterX.push_back(x + offset);
    run.characterCellX.reserve(cellOffsets.size());
    for (int offset : cellOffsets) run.characterCellX.push_back(x + offset);
    layer().capture(range->id, range->orphan, run, clipped, before.view, after.view,
        range->ownerWindow, range->ownerWindow >= 0 ? range->id : 0);
}

namespace {

std::vector<TextOwnerCopySnapshot> textCaptureCopyOwnerPixels(scene::TextBufferId source)
{
    struct RequiredOwner {
        int window;
        scene::TextBufferId generation;
        int left;
        int top;
        int right;
        int bottom;
    };
    std::vector<RequiredOwner> requiredOwners;
    for (const scene::TextRecord& record : layer().records()) {
        if (record.buffer != source) continue;
        const int window = record.ownerWitnessGeneration != 0 ? record.ownerWitnessWindow : record.ownerWindow;
        const scene::TextBufferId generation = record.ownerWitnessGeneration != 0
            ? record.ownerWitnessGeneration : record.ownerGeneration;
        if (window < 0 || generation == 0) continue;
        auto required = std::find_if(requiredOwners.begin(), requiredOwners.end(), [window, generation](const RequiredOwner& owner) {
            return owner.window == window && owner.generation == generation;
        });
        auto addPoint = [&](int x, int y) {
            if (required == requiredOwners.end()) {
                requiredOwners.push_back({ window, generation, x, y, x + 1, y + 1 });
                required = std::prev(requiredOwners.end());
            } else {
                required->left = std::min(required->left, x);
                required->top = std::min(required->top, y);
                required->right = std::max(required->right, x + 1);
                required->bottom = std::max(required->bottom, y + 1);
            }
        };
        if (!record.ownerWitnesses.empty()) {
            for (const scene::TextOwnerWitness& witness : record.ownerWitnesses) addPoint(witness.x, witness.y);
        } else {
            for (const scene::TextPixelRecord& pixel : record.pixels) {
                const std::int64_t x = static_cast<std::int64_t>(pixel.x) - record.ownerOffsetX;
                const std::int64_t y = static_cast<std::int64_t>(pixel.y) - record.ownerOffsetY;
                if (x >= std::numeric_limits<int>::min() && x <= std::numeric_limits<int>::max()
                    && y >= std::numeric_limits<int>::min() && y <= std::numeric_limits<int>::max()) {
                    addPoint(static_cast<int>(x), static_cast<int>(y));
                }
            }
        }
    }
    if (requiredOwners.empty()) return {};

    std::vector<TextOwnerCopySnapshot> ownerPixels;
    for (int index = 0; ; ++index) {
        TextWindowSnapshot snapshot;
        if (!textCaptureGetWindowSnapshot(index, &snapshot, nullptr)) break;
        const auto required = std::find_if(requiredOwners.begin(), requiredOwners.end(),
            [&snapshot](const RequiredOwner& owner) {
                return owner.window == snapshot.id && owner.generation == snapshot.generation;
            });
        if (required == requiredOwners.end()) continue;
        BufferRange* owner = findRange(snapshot.pixels);
        if (owner == nullptr || owner->id != snapshot.generation || owner->orphan || owner->pitch <= 0
            || owner->width <= 0 || owner->height <= 0
            || static_cast<std::size_t>(owner->width) > std::numeric_limits<std::size_t>::max()
                / static_cast<std::size_t>(owner->height)) continue;
        const int left = std::max(0, required->left);
        const int top = std::max(0, required->top);
        const int right = std::min(owner->width, required->right);
        const int bottom = std::min(owner->height, required->bottom);
        if (right <= left || bottom <= top) continue;
        TextOwnerCopySnapshot copy;
        copy.buffer = owner->id;
        const int width = right - left;
        const int height = bottom - top;
        copy.snapshot.pixels.resize(static_cast<std::size_t>(width) * height);
        const auto* base = reinterpret_cast<const unsigned char*>(owner->base);
        for (int y = 0; y < height; ++y) {
            std::memcpy(copy.snapshot.pixels.data() + static_cast<std::size_t>(y) * width,
                base + static_cast<std::size_t>(top + y) * owner->pitch + left, static_cast<std::size_t>(width));
        }
        copy.snapshot.view = { copy.snapshot.pixels.data(), width, height, width, left, top };
        ownerPixels.push_back(std::move(copy));
    }
    return ownerPixels;
}

} // namespace

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
        traceUntrackedCopySource(sourceRange, destinationRange, source, sourcePitch, width, height,
            destinationX, destinationY, transparent);
        PixelSnapshot sourceSnapshot;
        const scene::TextRect destinationRect { destinationX, destinationY, width, height };
        const bool trackTransparentBlit = sourceRange == nullptr && destinationRange != nullptr && transparent
            && hasTrackedCoverage(destinationRange->id, destinationRect)
            && snapshotBlitSource(source, sourcePitch, width, height, findRange(source), sourceSnapshot);
        copyProc(destination, destinationPitch, source, sourcePitch, width, height);
        if (destinationRange != nullptr && !transparent) {
            layer().overwrite(destinationRange->id, destinationRect);
        } else if (trackTransparentBlit) {
            layer().overwrite(destinationRange->id, destinationRect, sourceSnapshot.view, true, 0);
        }
        return;
    }

    bool hasSourceText = false;
    bool hasDestinationText = false;
    for (const auto& record : layer().records()) {
        const std::int64_t sourceRight = static_cast<std::int64_t>(sourceX) + width;
        const std::int64_t sourceBottom = static_cast<std::int64_t>(sourceY) + height;
        const std::int64_t destinationRight = static_cast<std::int64_t>(destinationX) + width;
        const std::int64_t destinationBottom = static_cast<std::int64_t>(destinationY) + height;
        if (record.buffer == sourceRange->id
            && record.bounds.x < sourceRight && sourceX < static_cast<std::int64_t>(record.bounds.x) + record.bounds.width
            && record.bounds.y < sourceBottom && sourceY < static_cast<std::int64_t>(record.bounds.y) + record.bounds.height) {
            hasSourceText = true;
        }
        if (record.buffer == destinationRange->id
            && record.bounds.x < destinationRight && destinationX < static_cast<std::int64_t>(record.bounds.x) + record.bounds.width
            && record.bounds.y < destinationBottom && destinationY < static_cast<std::int64_t>(record.bounds.y) + record.bounds.height) {
            hasDestinationText = true;
        }
        if (hasSourceText && hasDestinationText) break;
    }
    if (!hasSourceText && !hasDestinationText) {
        copyProc(destination, destinationPitch, source, sourcePitch, width, height);
        return;
    }

    const scene::TextRect sourceRect { sourceX, sourceY, width, height };
    const scene::TextRect destinationRect { destinationX, destinationY, width, height };
    PixelSnapshot sourceSnapshot = snapshot(*sourceRange, sourceRect);
    if (!hasSourceText) {
        copyProc(destination, destinationPitch, source, sourcePitch, width, height);
        layer().overwrite(destinationRange->id, destinationRect, sourceSnapshot.view, transparent, 0);
        return;
    }
    PixelSnapshot destinationBefore = snapshot(*destinationRange, destinationRect);
    const auto ownerSnapshots = textCaptureCopyOwnerPixels(sourceRange->id);
    std::vector<scene::TextValidationPixels> ownerPixels;
    ownerPixels.reserve(ownerSnapshots.size());
    for (const TextOwnerCopySnapshot& owner : ownerSnapshots) {
        ownerPixels.push_back({ owner.buffer, owner.snapshot.view });
    }
    copyProc(destination, destinationPitch, source, sourcePitch, width, height);
    PixelSnapshot destinationAfter = snapshot(*destinationRange, destinationRect);
    layer().copy(sourceRange->id, destinationRange->id, destinationRange->orphan,
        sourceRect, destinationX, destinationY, transparent, 0,
        sourceSnapshot.view, destinationBefore.view, destinationAfter.view,
        destinationRange->ownerWindow,
        destinationRange->ownerWindow >= 0 ? destinationRange->id : 0,
        ownerPixels);
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

const std::vector<TextRecord> textCaptureSharpRecords(TextBufferId buffer)
{
    std::vector<TextRecord> result = textCaptureRecords(buffer);
    result.erase(std::remove_if(result.begin(), result.end(), [](const TextRecord& record) {
        return !record.run.sharpEligible;
    }), result.end());
    return result;
}

namespace {

std::vector<TextRestorePixel> textCaptureRestorationFor(TextBufferId buffer, bool sharpEligibleOnly)
{
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    for (const auto& item : ranges()) {
        const BufferRange& range = item.second;
        if (range.id != buffer || range.orphan || range.pitch <= 0) continue;
        const auto* pixels = reinterpret_cast<const unsigned char*>(range.base);
        std::vector<std::pair<int, scene::TextBufferId>> requiredOwners;
        for (const scene::TextRecord& record : layer().records()) {
            if (record.buffer != buffer) continue;
            if (!record.ownerWitnesses.empty() && record.ownerWitnessWindow >= 0
                && record.ownerWitnessGeneration != 0 && record.ownerWitnessGeneration != buffer) {
                const auto witnessOwner = std::make_pair(record.ownerWitnessWindow, record.ownerWitnessGeneration);
                if (std::find(requiredOwners.begin(), requiredOwners.end(), witnessOwner) == requiredOwners.end()) {
                    requiredOwners.push_back(witnessOwner);
                }
            }
            if (record.ownerWindow >= 0 && record.ownerGeneration != 0 && record.ownerGeneration != buffer) {
                const auto owner = std::make_pair(record.ownerWindow, record.ownerGeneration);
                if (std::find(requiredOwners.begin(), requiredOwners.end(), owner) == requiredOwners.end()) {
                    requiredOwners.push_back(owner);
                }
            }
        }
        std::vector<scene::TextValidationPixels> ownerPixels;
        for (int index = 0; ; ++index) {
            TextWindowSnapshot snapshot;
            if (!textCaptureGetWindowSnapshot(index, &snapshot, nullptr)) break;
            const auto required = std::find(requiredOwners.begin(), requiredOwners.end(),
                std::make_pair(snapshot.id, snapshot.generation));
            if (required == requiredOwners.end()) continue;
            BufferRange* owner = findRange(snapshot.pixels);
            if (owner == nullptr || owner->id != snapshot.generation || owner->orphan || owner->pitch <= 0) continue;
            ownerPixels.push_back({ owner->id, { reinterpret_cast<const unsigned char*>(owner->base),
                owner->width, owner->height, owner->pitch } });
        }
        return layer().restorationPixels(buffer, { pixels, range.width, range.height, range.pitch },
            ownerPixels, false, sharpEligibleOnly);
    }
    return {};
}

} // namespace

std::vector<TextRestorePixel> textCaptureRestoration(TextBufferId buffer)
{
    return textCaptureRestorationFor(buffer, false);
}

std::vector<TextRestorePixel> textCaptureSharpRestoration(TextBufferId buffer)
{
    return textCaptureRestorationFor(buffer, true);
}

bool textCaptureVisibilityMask(const TextRecord& record, int width, int height, TextVisibilityMask* mask)
{
    if (mask == nullptr || width <= 0 || height <= 0) return false;
    mask->bounds = {};
    mask->pixels.clear();
    std::lock_guard<std::recursive_mutex> lock(captureMutex());
    BufferRange* recordRange = nullptr;
    for (auto& item : ranges()) {
        if (item.second.id == record.buffer) {
            recordRange = &item.second;
            break;
        }
    }
    if (recordRange == nullptr || recordRange->orphan || recordRange->pitch <= 0) return true;

    const std::int64_t left = std::max<std::int64_t>(0, record.bounds.x);
    const std::int64_t top = std::max<std::int64_t>(0, record.bounds.y);
    const std::int64_t right = std::min<std::int64_t>({ width, recordRange->width,
        static_cast<std::int64_t>(record.bounds.x) + record.bounds.width });
    const std::int64_t bottom = std::min<std::int64_t>({ height, recordRange->height,
        static_cast<std::int64_t>(record.bounds.y) + record.bounds.height });
    if (record.bounds.width <= 0 || record.bounds.height <= 0 || right <= left || bottom <= top) return true;
    mask->bounds = { static_cast<int>(left), static_cast<int>(top),
        static_cast<int>(right - left), static_cast<int>(bottom - top) };
    mask->pixels.assign(static_cast<std::size_t>(mask->bounds.width) * mask->bounds.height, 0);
    auto clearMask = [&]() { std::fill(mask->pixels.begin(), mask->pixels.end(), 0); };

    const unsigned char* current = reinterpret_cast<const unsigned char*>(recordRange->base);
    for (const TextPixelCoverage& pixel : record.coverage) {
        if (pixel.x < left || pixel.y < top || pixel.x >= right || pixel.y >= bottom) continue;
        if (current[static_cast<std::size_t>(pixel.y) * recordRange->pitch + pixel.x] == pixel.expected) {
            mask->pixels[static_cast<std::size_t>(pixel.y - mask->bounds.y) * mask->bounds.width
                + pixel.x - mask->bounds.x] = 1;
        }
    }
    if (record.ownerWindow >= 0) {
        int windowCount = 0;
        int ownerIndex = -1;
        TextWindowSnapshot owner;
        for (int index = 0; ; ++index) {
            TextWindowSnapshot candidate;
            if (!textCaptureGetWindowSnapshot(index, &candidate, &windowCount)) break;
            if (candidate.id == record.ownerWindow) {
                ownerIndex = index;
                owner = candidate;
                break;
            }
        }
        if (ownerIndex < 0 || owner.generation != record.ownerGeneration || owner.hidden) {
            clearMask();
            return true;
        }

        for (const TextPixelCoverage& pixel : record.coverage) {
            if (pixel.x < owner.x || pixel.x >= owner.x + owner.width
                || pixel.y < owner.y || pixel.y >= owner.y + owner.height) {
                if (pixel.x >= mask->bounds.x && pixel.y >= mask->bounds.y
                    && pixel.x < mask->bounds.x + mask->bounds.width
                    && pixel.y < mask->bounds.y + mask->bounds.height) {
                    mask->pixels[static_cast<std::size_t>(pixel.y - mask->bounds.y) * mask->bounds.width
                        + pixel.x - mask->bounds.x] = 0;
                }
            }
        }
        for (int index = ownerIndex + 1; index < windowCount; ++index) {
            TextWindowSnapshot above;
            if (!textCaptureGetWindowSnapshot(index, &above, nullptr) || above.hidden) continue;
            const int coverLeft = std::max(above.x, mask->bounds.x);
            const int coverTop = std::max(above.y, mask->bounds.y);
            const int coverRight = std::min(above.x + above.width - 1, mask->bounds.x + mask->bounds.width - 1);
            const int coverBottom = std::min(above.y + above.height - 1, mask->bounds.y + mask->bounds.height - 1);
            if (coverLeft > coverRight || coverTop > coverBottom) continue;
            BufferRange* aboveRange = findRange(above.pixels);
            const bool safeTransparent = above.transparent && aboveRange != nullptr
                && above.generation != 0 && aboveRange->id == above.generation
                && !aboveRange->orphan && aboveRange->pitch >= above.pitch && above.pitch >= above.width
                && above.width > 0 && above.height > 0;
            for (int y = coverTop; y <= coverBottom; ++y) {
                unsigned char* visible = mask->pixels.data()
                    + static_cast<std::size_t>(y - mask->bounds.y) * mask->bounds.width + coverLeft - mask->bounds.x;
                if (!above.transparent || !safeTransparent) {
                    std::fill(visible, visible + coverRight - coverLeft + 1, 0);
                    continue;
                }
                const unsigned char* pixels = above.pixels
                    + static_cast<std::size_t>(y - above.y) * above.pitch + coverLeft - above.x;
                for (int x = coverLeft; x <= coverRight; ++x, ++pixels, ++visible) {
                    if (*pixels != COLOR_FIRST) *visible = 0;
                }
            }
        }
    }

    TextCursorSnapshot cursor = {};
    if (!textCaptureGetCursorSnapshot(&cursor) || !cursor.visible || cursor.pixels == nullptr
        || cursor.width <= 0 || cursor.height <= 0 || cursor.pitch < cursor.width) return true;
    const int cursorLeft = std::max(cursor.x, mask->bounds.x);
    const int cursorTop = std::max(cursor.y, mask->bounds.y);
    const int cursorRight = std::min(cursor.x + cursor.width, mask->bounds.x + mask->bounds.width);
    const int cursorBottom = std::min(cursor.y + cursor.height, mask->bounds.y + mask->bounds.height);
    if (cursorLeft >= cursorRight || cursorTop >= cursorBottom) return true;
    for (int y = cursorTop; y < cursorBottom; ++y) {
        const unsigned char* source = cursor.pixels + static_cast<std::size_t>(y - cursor.y) * cursor.pitch
            + cursorLeft - cursor.x;
        unsigned char* visible = mask->pixels.data()
            + static_cast<std::size_t>(y - mask->bounds.y) * mask->bounds.width + cursorLeft - mask->bounds.x;
        for (int x = cursorLeft; x < cursorRight; ++x, ++source, ++visible) {
            if (*source != cursor.transparentIndex) *visible = 0;
        }
    }
    return true;
}

} // namespace scene
} // namespace fallout
