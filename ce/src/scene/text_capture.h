#ifndef FALLOUT_SCENE_TEXT_CAPTURE_H
#define FALLOUT_SCENE_TEXT_CAPTURE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../color.h"
#include "text_layer.h"

namespace fallout {

using TextPixelCopyProc = void(unsigned char* destination, int destinationPitch,
    const unsigned char* source, int sourcePitch, int width, int height);

enum class TextCaptureTag {
    None,
    Conversation,
};

class ScopedTextCaptureTag {
public:
    explicit ScopedTextCaptureTag(TextCaptureTag tag);
    ~ScopedTextCaptureTag();

    ScopedTextCaptureTag(const ScopedTextCaptureTag&) = delete;
    ScopedTextCaptureTag& operator=(const ScopedTextCaptureTag&) = delete;

private:
    TextCaptureTag previous_;
};

struct TextWindowSnapshot {
    int id = -1;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int pitch = 0;
    bool hidden = false;
    bool transparent = false;
    const unsigned char* pixels = nullptr;
    scene::TextBufferId generation = 0;
};

struct TextCursorSnapshot {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int pitch = 0;
    bool visible = false;
    unsigned char transparentIndex = 0;
    const unsigned char* pixels = nullptr;
};

bool textCaptureGetWindowSnapshot(int index, TextWindowSnapshot* snapshot, int* count);
bool textCaptureGetCursorSnapshot(TextCursorSnapshot* snapshot);

void textCaptureAllocationCreated(void* buffer, std::size_t size);
void textCaptureAllocationDestroyed(void* buffer);
void textCaptureAllocationResized(void* oldBuffer, void* newBuffer, std::size_t newSize);
void textCaptureRegisterExternalBuffer(void* buffer, std::size_t size, int width, int height, int pitch, bool orphan);
void textCaptureUnregisterExternalBuffer(void* buffer);
void textCaptureMarkBufferOwned(void* buffer);
void textCaptureMarkWindowBuffer(void* buffer, int window);
void textCaptureForgetWindow(int window, void* buffer);
scene::TextBufferId textCaptureBufferId(const void* buffer);
void textCaptureDrawText(unsigned char* buffer, const char* string, int length, int pitch, ColorWithFlags color);
void textCaptureCopy(const unsigned char* source, int width, int height, int sourcePitch,
    unsigned char* destination, int destinationPitch, bool transparent, TextPixelCopyProc* copyProc);

namespace scene {
const std::vector<TextRecord> textCaptureRecords(TextBufferId buffer);
std::vector<TextRestorePixel> textCaptureRestoration(TextBufferId buffer);
const std::vector<TextRecord> textCaptureSharpRecords(TextBufferId buffer);
std::vector<TextRestorePixel> textCaptureSharpRestoration(TextBufferId buffer);
bool textCaptureVisibilityMask(const TextRecord& record, int width, int height, TextVisibilityMask* mask);
}

} // namespace fallout

#endif // FALLOUT_SCENE_TEXT_CAPTURE_H
