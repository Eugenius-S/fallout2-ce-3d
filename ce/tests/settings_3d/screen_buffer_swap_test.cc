#include "../../src/scene/screen_buffer_swap.h"
#include "../../src/window_manager.h"

#include <cstdlib>

namespace fallout {
bool failAllocation = false;
void* internal_malloc(size_t size) { return failAllocation ? nullptr : std::malloc(size); }
void internal_free(void* ptr) { std::free(ptr); }
void* forgottenBuffer = nullptr;
int forgetCount = 0;
void textCaptureMarkWindowBuffer(void*, int) {}
void textCaptureForgetWindow(int, void* ptr) { forgottenBuffer = ptr; ++forgetCount; }
}

int main()
{
    using namespace fallout;
    Window root = {};
    Rect bounds = { 0, 0, 1370, 770 };
    root.rect = bounds;
    root.width = 1371;
    root.height = 771;
    auto* old = static_cast<unsigned char*>(internal_malloc(1371u * 771u));
    unsigned char* active = old;
    {
        scene::ScreenBufferSwap state;
        failAllocation = true;
        if (state.prepare(1280, 720, true) || active != old || root.width != 1371 || root.height != 771) return 7;
        failAllocation = false;
    }
    {
        scene::ScreenBufferSwap state;
        if (state.prepare(639, 480, true)) return 1;
        if (!state.prepare(1280, 720, true)) return 2;
        if (!state.exchange(active, bounds, &root) || active == old || root.width != 1280 || root.height != 720) return 3;
        if (!state.exchange(active, bounds, &root) || active != old || root.width != 1371 || root.height != 771) return 4;
    }
    internal_free(active);
    active = nullptr;
    {
        scene::ScreenBufferSwap state;
        if (!state.prepare(1280, 720, false) || !state.exchange(active, bounds, &root)) return 5;
        if (active != nullptr || root.width != 1280 || root.height != 720) return 6;
    }
    {
        Window iso = {};
        iso.id = 42;
        iso.rect = { 0, 0, 1370, 670 };
        iso.width = 1371;
        iso.height = 671;
        auto* original = static_cast<unsigned char*>(internal_malloc(1371u * 671u));
        iso.buffer = original;
        {
            scene::WindowBufferSwap state;
            if (!state.prepare(1280, 620) || !state.exchange(&iso)) return 8;
            if (iso.id != 42 || iso.buffer == original || iso.width != 1280 || iso.rect.bottom != 619) return 9;
            if (!state.exchange(&iso) || iso.buffer != original || iso.width != 1371 || iso.rect.bottom != 670) return 10;
            if (forgetCount != 0) return 11;
        }
        if (forgetCount != 1 || forgottenBuffer == original) return 12;
        internal_free(iso.buffer);
    }
    {
        Window iso = {};
        iso.id = 43;
        iso.rect = { 0, 0, 639, 479 };
        iso.width = 640;
        iso.height = 480;
        auto* original = static_cast<unsigned char*>(internal_malloc(640u * 480u));
        iso.buffer = original;
        const int before = forgetCount;
        {
            scene::WindowBufferSwap state;
            if (!state.prepare(800, 600) || !state.exchange(&iso)) return 13;
        }
        if (forgetCount != before + 1 || forgottenBuffer != original || iso.buffer == original) return 14;
        internal_free(iso.buffer);
    }
    return 0;
}
