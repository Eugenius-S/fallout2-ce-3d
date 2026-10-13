#include "app.h"

#include <stdexcept>

namespace f2native {

namespace {
App* g_app = nullptr; // wndProc needs access to the stored resize callback
}

LRESULT CALLBACK App::wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_SIZE: {
        if (g_app && wparam != SIZE_MINIMIZED && g_app->onResize_) {
            const uint32_t w = LOWORD(lparam);
            const uint32_t h = HIWORD(lparam);
            if (w > 0 && h > 0) {
                g_app->width_ = w;
                g_app->height_ = h;
                g_app->onResize_(w, h);
            }
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

HWND App::createWindow(const wchar_t* title, uint32_t width, uint32_t height,
                       ResizeCallback onResize) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"F2NativeWindow";
    if (!RegisterClassExW(&wc)) throw std::runtime_error("RegisterClassExW failed");

    RECT rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                            CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
                            nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd_) throw std::runtime_error("CreateWindowExW failed");
    width_ = width;
    height_ = height;
    onResize_ = onResize;
    g_app = this;
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return hwnd_;
}

bool App::pumpMessages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

void App::shutdown() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    g_app = nullptr;
    UnregisterClassW(L"F2NativeWindow", GetModuleHandleW(nullptr));
}

} // namespace f2native
