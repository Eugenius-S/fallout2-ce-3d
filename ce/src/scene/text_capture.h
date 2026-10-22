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

void textCaptureAllocationCreated(void* buffer, std::size_t size);
void textCaptureAllocationDestroyed(void* buffer);
void textCaptureAllocationResized(void* oldBuffer, void* newBuffer, std::size_t newSize);
void textCaptureRegisterExternalBuffer(void* buffer, std::size_t size, int width, int height, int pitch, bool orphan);
void textCaptureUnregisterExternalBuffer(void* buffer);
void textCaptureMarkBufferOwned(void* buffer);
scene::TextBufferId textCaptureBufferId(const void* buffer);
void textCaptureDrawText(unsigned char* buffer, const char* string, int length, int pitch, ColorWithFlags color);
void textCaptureCopy(const unsigned char* source, int width, int height, int sourcePitch,
    unsigned char* destination, int destinationPitch, bool transparent, TextPixelCopyProc* copyProc);

namespace scene {
const std::vector<TextRecord> textCaptureRecords(TextBufferId buffer);
std::vector<TextRestorePixel> textCaptureRestoration(TextBufferId buffer);
}

} // namespace fallout

#endif // FALLOUT_SCENE_TEXT_CAPTURE_H
