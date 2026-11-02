#include "../../src/scene/screen_buffer_swap.h"
#include "../../src/window_manager.h"

#include <cstdlib>

namespace fallout {
bool failAllocation = false;
void* internal_malloc(size_t size) { return failAllocation ? nullptr : std::malloc(size); }
void internal_free(void* ptr) { std::free(ptr); }
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
    return 0;
}
