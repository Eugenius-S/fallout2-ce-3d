#include "dialogue_backdrop.h"

#include "../debug.h"
#include "../svga.h"
#include "../window_manager.h"

namespace fallout {
namespace scene {
namespace {

int backdropWindow = -1;

void destroyBackdrop()
{
    if (backdropWindow != -1) {
        windowDestroy(backdropWindow);
        backdropWindow = -1;
    }
}

} // namespace

int createDialogueBackgroundWindow(int x, int y, int width, int height)
{
    // A normal opaque window hides the world in both presentation paths,
    // leaving all dialogue windows, barter and the cursor above it.
    destroyBackdrop();
    backdropWindow = windowCreate(0, 0, screenGetWidth(), screenGetHeight(),
        static_cast<ColorWithFlags>(0), WINDOW_DONT_MOVE_TOP);
    if (backdropWindow == -1) {
        debugPrint("Dialogue backdrop disabled: window creation failed.\n");
    }

    const int window = windowCreate(x, y, width, height,
        static_cast<ColorWithFlags>(256), WINDOW_DONT_MOVE_TOP | WINDOW_MODAL);
    if (window == -1) {
        destroyBackdrop();
    } else if (backdropWindow != -1) {
        windowRefresh(backdropWindow);
    }
    return window;
}

void destroyDialogueBackgroundWindow(int window)
{
    windowDestroy(window);
    // Destroying the full-screen window refreshes the exposed world now.
    destroyBackdrop();
}

} // namespace scene
} // namespace fallout
