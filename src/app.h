#pragma once

#include <windows.h>
#include <cstdint>

namespace f2native {

// Win32 window creation and the message pump. The window proc forwards WM_SIZE
// through onResize so the GPU layer can resize its swapchain.
class App {
public:
    // Registered before window creation; called with the new client size.
    using ResizeCallback = void (*)(uint32_t width, uint32_t height);

    HWND createWindow(const wchar_t* title, uint32_t width, uint32_t height,
                      ResizeCallback onResize);
    // Returns false when WM_QUIT was received.
    bool pumpMessages();
    void shutdown();
    uint32_t clientWidth() const { return width_; }
    uint32_t clientHeight() const { return height_; }

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    HWND hwnd_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    ResizeCallback onResize_ = nullptr;
};

} // namespace f2native
