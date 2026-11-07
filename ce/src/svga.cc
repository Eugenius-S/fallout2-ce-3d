#include "svga.h"

#include <limits.h>
#include <cmath>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include <vector>

#include "color.h"
#include "config.h"
#include "dinput.h"
#include "draw.h"
#include "db.h"
#include "fps_counter.h"
#include "game.h"
#include "interface.h"
#include "memory.h"
#include "mouse.h"
#include "movie.h"
#include "scan_unimplemented.h"
#include "settings.h"
#include "scene/text_capture.h"
#include "devtools/devtools.h"
#include "devtools/owner_shot.h"
#include "scene/scene_overlay.h"
#include "scene/scene_view.h"
#include "scene_renderer/camera.h"
#include "scene_renderer/context.h"
#include "scene_renderer/live_objects.h"
#include "scene_renderer/picking.h"
#include "scene_renderer/post.h"
#include "scene_renderer/perf.h"
#include "scene_renderer/scene_draw.h"
#include "scene_renderer/static_scene.h"
#include "scene_renderer/text_present.h"
#include "text_font.h"
#include "tile.h"
#include "win32.h"
#include "window_manager_private.h"

namespace fallout {

static bool createRenderer(int width, int height);
static void destroyRenderer();

// screen rect
Rect _scr_size;

// 0x6ACA18 scr_blit
void (*_scr_blit)(unsigned char* src, int src_pitch, int unused, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y) = _GNW95_ShowRect;

// 0x6ACA1C zero_mem
void (*_zero_mem)() = nullptr;

SDL_Window* gSdlWindow = nullptr;
SDL_Surface* gSdlSurface = nullptr;
SDL_Renderer* gSdlRenderer = nullptr;
SDL_Texture* gSdlTexture = nullptr;
SDL_Surface* gSdlTextureSurface = nullptr;

static std::vector<unsigned char> gRgbaBackground;
static std::vector<unsigned char> gRgbaBackgroundMask;
static int gRgbaBackgroundWidth = 0;
static int gRgbaBackgroundHeight = 0;

void renderSetRgbaBackground(const unsigned char* rgba, const unsigned char* mask, int width, int height)
{
    if (rgba == nullptr || mask == nullptr || width <= 0 || height <= 0) {
        renderClearRgbaBackground();
        return;
    }

    size_t pixelCount = static_cast<size_t>(width) * height;
    gRgbaBackground.assign(rgba, rgba + pixelCount * 4);
    gRgbaBackgroundMask.assign(mask, mask + pixelCount);
    gRgbaBackgroundWidth = width;
    gRgbaBackgroundHeight = height;
}

void renderClearRgbaBackground()
{
    gRgbaBackground.clear();
    gRgbaBackgroundMask.clear();
    gRgbaBackgroundWidth = 0;
    gRgbaBackgroundHeight = 0;
}

static void renderApplyRgbaBackground()
{
    if (gSdlTextureSurface == nullptr || gRgbaBackground.empty()
        || gSdlTextureSurface->w != gRgbaBackgroundWidth || gSdlTextureSurface->h != gRgbaBackgroundHeight) {
        return;
    }

    for (int y = 0; y < gRgbaBackgroundHeight; y++) {
        unsigned char* dst = static_cast<unsigned char*>(gSdlTextureSurface->pixels) + y * gSdlTextureSurface->pitch;
        const unsigned char* src = gRgbaBackground.data() + static_cast<size_t>(y) * gRgbaBackgroundWidth * 4;
        const unsigned char* mask = gRgbaBackgroundMask.data() + static_cast<size_t>(y) * gRgbaBackgroundWidth;
        for (int x = 0; x < gRgbaBackgroundWidth; x++) {
            if (mask[x] != 0) {
                memcpy(dst + x * 4, src + x * 4, 4);
            }
        }
    }
}

// TODO: Remove once migration to update-render cycle is completed.
FpsLimiter sharedFpsLimiter;

// 0x4CAD08 init_mode_320_200
int _init_mode_320_200()
{
    return _GNW95_init_mode_ex(320, 200, 8);
}

// 0x4CAD40 init_mode_320_400
int _init_mode_320_400()
{
    return _GNW95_init_mode_ex(320, 400, 8);
}

// 0x4CAD5C init_mode_640_480_16
int _init_mode_640_480_16()
{
    return -1;
}

// 0x4CAD64 init_mode_640_480
int _init_mode_640_480()
{
    return _init_vesa_mode(640, 480);
}

// 0x4CAD94 init_mode_640_400
int _init_mode_640_400()
{
    return _init_vesa_mode(640, 400);
}

// 0x4CADA8 init_mode_800_600
int _init_mode_800_600()
{
    return _init_vesa_mode(800, 600);
}

// 0x4CADBC init_mode_1024_768
int _init_mode_1024_768()
{
    return _init_vesa_mode(1024, 768);
}

// 0x4CADD0 init_mode_1280_1024
int _init_mode_1280_1024()
{
    return _init_vesa_mode(1280, 1024);
}

// 0x4CADF8
void _get_start_mode_()
{
}

// 0x4CADFC zero_vid_mem
void _zero_vid_mem()
{
    if (_zero_mem) {
        _zero_mem();
    }
}

// 0x4CAE1C GNW95_init_mode_ex
int _GNW95_init_mode_ex(int width, int height, int bpp)
{
    int outputWidth = settings.screen.resolution_x;
    int outputHeight = settings.screen.resolution_y;
    if (settings.screen.resolution_mode > 0) {
        outputWidth = settings.screen.resolution_mode / 10000;
        outputHeight = settings.screen.resolution_mode % 10000;
    }
    double factor = settings.screen.scale_factor;
    width = static_cast<int>(std::lround(outputWidth / factor));
    height = static_cast<int>(std::lround(outputHeight / factor));

    // Preserve the minimum game surface size even for a stale configuration.
    if (width < 640 || height < 480) {
        width = outputWidth;
        height = outputHeight;
    }

    if (_GNW95_init_window(width, height, settings.screen.windowed, outputWidth, outputHeight) == -1) {
        return -1;
    }

    if (directDrawInit(width, height, bpp) == -1) {
        return -1;
    }

    // macOS seems to require dequeuing NSApp events in order for window to
    // become visible. There is no concrete number of calls required to make
    // it happen. Sadly there is no particular event to watch for because SDL
    // marks window as shown immediately after creation (see
    // `SDL_FinishWindowCreation`).
    for (int i = 0; i < 10; i++) {
        SDL_PumpEvents();
    }

    _scr_size.left = 0;
    _scr_size.top = 0;
    _scr_size.right = width - 1;
    _scr_size.bottom = height - 1;

    _mouse_blit_trans = nullptr;
    _scr_blit = _GNW95_ShowRect;
    _zero_mem = _GNW95_zero_vid_mem;
    _mouse_blit = _GNW95_ShowRect;

    return 0;
}

// 0x4CAECC init_vesa_mode
int _init_vesa_mode(int width, int height)
{
    return _GNW95_init_mode_ex(width, height, 8);
}

// 0x4CAEDC GNW95_init_window
int _GNW95_init_window(int width, int height, WindowMode mode, int outputWidth, int outputHeight)
{
    if (gSdlWindow == nullptr) {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

        Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
        const bool backgroundRun = SDL_getenv("F2CE_DEV_BACKGROUND") != nullptr;
        if (backgroundRun) {
            SDL_SetHint(SDL_HINT_WINDOW_NO_ACTIVATION_WHEN_SHOWN, "1");
        }

        if (mode == WindowMode::Fullscreen) {
            windowFlags |= SDL_WINDOW_FULLSCREEN;
        } else if (mode == WindowMode::WindowedFullscreen) {
            windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }

        // Dev test runs stay out of the way of the user's active game.
        int windowX = SDL_WINDOWPOS_UNDEFINED;
        int windowY = SDL_WINDOWPOS_UNDEFINED;
        if (backgroundRun && mode == WindowMode::Windowed) {
            windowFlags |= SDL_WINDOW_SKIP_TASKBAR;
            windowX = -20000;
            windowY = -20000;
        }

        gSdlWindow = SDL_CreateWindow(gProgramWindowTitle, windowX, windowY, outputWidth, outputHeight, windowFlags);
        if (gSdlWindow == nullptr) {
            return -1;
        }

#ifdef _WIN32
        if (mode != WindowMode::Fullscreen) {
            // Windows 11 draws a light one-pixel border around every window;
            // against the dark menu art it reads as a stray white strip at
            // the screen edges, so the game window opts out of it.
            SDL_SysWMinfo wmInfo;
            SDL_zero(wmInfo);
            SDL_VERSION(&wmInfo.version);
            if (SDL_GetWindowWMInfo(gSdlWindow, &wmInfo) && wmInfo.subsystem == SDL_SYSWM_WINDOWS) {
                HWND hwnd = wmInfo.info.win.window;

                if (mode == WindowMode::Windowed) {
                    // SDL strips the minimize box from non-resizable windows,
                    // which would leave the caption without any way to
                    // minimize the game.
                    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
                    SetWindowLongPtrW(hwnd, GWL_STYLE, style | WS_MINIMIZEBOX);
                }

                // Loaded by name: DWMWA_BORDER_COLOR is Windows 11 only, and
                // the call is silently skipped where it does not exist.
                using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, const void*, DWORD);
                HMODULE dwm = GetModuleHandleW(L"dwmapi.dll");
                if (dwm == nullptr) {
                    dwm = LoadLibraryW(L"dwmapi.dll");
                }
                if (dwm != nullptr) {
                    auto* setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
                    if (setAttribute != nullptr) {
                        COLORREF noBorder = 0xFFFFFFFE; // DWMWA_COLOR_NONE
                        setAttribute(hwnd, 34 /* DWMWA_BORDER_COLOR */, &noBorder, sizeof(noBorder));
                    }
                }
            }
        }
#endif

        // Sprite-lighting mod (P0): take over presentation with our own
        // OpenGL context; the vanilla SDL_Renderer stays as the fallback
        // path if that is unavailable.
        if (scene_renderer::init(gSdlWindow, width, height)) {
            gSdlTextureSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
            if (gSdlTextureSurface == nullptr) {
                scene_renderer::shutdown();

                SDL_DestroyWindow(gSdlWindow);
                gSdlWindow = nullptr;

                return -1;
            }
        } else if (!createRenderer(width, height)) {
            destroyRenderer();

            SDL_DestroyWindow(gSdlWindow);
            gSdlWindow = nullptr;

            return -1;
        }
    }

    return 0;
}

// 0x4CAF9C GNW95_init_DirectDraw
int directDrawInit(int width, int height, int bpp)
{
    if (gSdlSurface != nullptr) {
        unsigned char* palette = directDrawGetPalette();
        directDrawFree();

        if (directDrawInit(width, height, bpp) == -1) {
            return -1;
        }

        directDrawSetPalette(palette);

        return 0;
    }

    gSdlSurface = SDL_CreateRGBSurface(0, width, height, bpp, 0, 0, 0, 0);
    if (gSdlSurface == nullptr || gSdlSurface->format->palette == nullptr) {
        directDrawFree();
        return -1;
    }
    textCaptureRegisterExternalBuffer(gSdlSurface->pixels,
        static_cast<size_t>(gSdlSurface->pitch) * gSdlSurface->h,
        gSdlSurface->w, gSdlSurface->h, gSdlSurface->pitch, false);

    SDL_Color colors[256];
    for (int index = 0; index < 256; index++) {
        colors[index].r = index;
        colors[index].g = index;
        colors[index].b = index;
        colors[index].a = 255;
    }

    SDL_SetPaletteColors(gSdlSurface->format->palette, colors, 0, 256);

    return 0;
}

// 0x4CB1B0 GNW95_reset_mode
void directDrawFree()
{
    if (gSdlSurface != nullptr) {
        textCaptureUnregisterExternalBuffer(gSdlSurface->pixels);
        SDL_FreeSurface(gSdlSurface);
        gSdlSurface = nullptr;
    }
}

// 0x4CB310 GNW95_SetPaletteEntries
void directDrawSetPaletteInRange(unsigned char* palette, int start, int count)
{
    if (gSdlSurface != nullptr && gSdlSurface->format->palette != nullptr) {
        SDL_Color colors[256];

        if (count != 0) {
            for (int index = 0; index < count; index++) {
                colors[index].r = palette[index * 3] << 2;
                colors[index].g = palette[index * 3 + 1] << 2;
                colors[index].b = palette[index * 3 + 2] << 2;
                colors[index].a = 255;
            }
        }

        SDL_SetPaletteColors(gSdlSurface->format->palette, colors, start, count);
        SDL_BlitSurface(gSdlSurface, nullptr, gSdlTextureSurface, nullptr);
    }
}

// 0x4CB568 GNW95_SetPalette
void directDrawSetPalette(unsigned char* palette)
{
    if (gSdlSurface != nullptr && gSdlSurface->format->palette != nullptr) {
        SDL_Color colors[256];

        for (int index = 0; index < 256; index++) {
            colors[index].r = palette[index * 3] << 2;
            colors[index].g = palette[index * 3 + 1] << 2;
            colors[index].b = palette[index * 3 + 2] << 2;
            colors[index].a = 255;
        }

        SDL_SetPaletteColors(gSdlSurface->format->palette, colors, 0, 256);
        SDL_BlitSurface(gSdlSurface, nullptr, gSdlTextureSurface, nullptr);
    }
}

// 0x4CB68C GNW95_GetPalette
unsigned char* directDrawGetPalette()
{
    // 0x6ACA24
    static unsigned char palette[768];

    if (gSdlSurface != nullptr && gSdlSurface->format->palette != nullptr) {
        SDL_Color* colors = gSdlSurface->format->palette->colors;

        for (int index = 0; index < 256; index++) {
            SDL_Color* color = &(colors[index]);
            palette[index * 3] = color->r >> 2;
            palette[index * 3 + 1] = color->g >> 2;
            palette[index * 3 + 2] = color->b >> 2;
        }
    }

    return palette;
}

// 0x4CB850 GNW95_ShowRect
void _GNW95_ShowRect(unsigned char* src, int srcPitch, int unused, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY)
{
    (void)unused;

    blitBufferToBuffer(src + srcPitch * srcY + srcX, srcWidth, srcHeight, srcPitch, (unsigned char*)gSdlSurface->pixels + gSdlSurface->pitch * destY + destX, gSdlSurface->pitch);

    SDL_Rect srcRect;
    srcRect.x = destX;
    srcRect.y = destY;
    srcRect.w = srcWidth;
    srcRect.h = srcHeight;

    SDL_Rect destRect;
    destRect.x = destX;
    destRect.y = destY;
    SDL_BlitSurface(gSdlSurface, &srcRect, gSdlTextureSurface, &destRect);
}

// Clears drawing surface.
//
// 0x4CBBC8 GNW95_zero_vid_mem
void _GNW95_zero_vid_mem()
{
    if (!gProgramIsActive) {
        return;
    }

    unsigned char* surface = (unsigned char*)gSdlSurface->pixels;
    for (int y = 0; y < gSdlSurface->h; y++) {
        memset(surface, 0, gSdlSurface->w);
        surface += gSdlSurface->pitch;
    }

    SDL_BlitSurface(gSdlSurface, nullptr, gSdlTextureSurface, nullptr);
}

int screenGetWidth()
{
    // TODO: Make it on par with _xres;
    return rectGetWidth(&_scr_size);
}

int screenGetHeight()
{
    // TODO: Make it on par with _yres.
    return rectGetHeight(&_scr_size);
}

int screenGetVisibleHeight()
{
    int windowBottomMargin = 0;

    if (!settings.ui.iface_bar_mode) {
        windowBottomMargin = INTERFACE_BAR_HEIGHT;
    }
    return screenGetHeight() - windowBottomMargin;
}

// returns true if the game is running in fullscreen mode, false otherwise (including windowed fullscreen mode)
bool screenIsExclusiveFullscreen()
{
    Uint32 flags = SDL_GetWindowFlags(gSdlWindow);
    return (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN;
}

static bool createRenderer(int width, int height)
{
    gSdlRenderer = SDL_CreateRenderer(gSdlWindow, -1, 0);
    if (gSdlRenderer == nullptr) {
        return false;
    }

    // SDL's default draw color is white, and the logical-size letterbox bars
    // are drawn with it; keep them black like the GL presentation's.
    SDL_SetRenderDrawColor(gSdlRenderer, 0, 0, 0, 255);

    if (SDL_RenderSetLogicalSize(gSdlRenderer, width, height) != 0) {
        return false;
    }

    gSdlTexture = SDL_CreateTexture(gSdlRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (gSdlTexture == nullptr) {
        return false;
    }

    Uint32 format;
    if (SDL_QueryTexture(gSdlTexture, &format, nullptr, nullptr, nullptr) != 0) {
        return false;
    }

    gSdlTextureSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(format), format);
    if (gSdlTextureSurface == nullptr) {
        return false;
    }

    return true;
}

static void destroyRenderer()
{
    scene_renderer::shutdown();

    if (gSdlTextureSurface != nullptr) {
        SDL_FreeSurface(gSdlTextureSurface);
        gSdlTextureSurface = nullptr;
    }

    if (gSdlTexture != nullptr) {
        SDL_DestroyTexture(gSdlTexture);
        gSdlTexture = nullptr;
    }

    if (gSdlRenderer != nullptr) {
        SDL_DestroyRenderer(gSdlRenderer);
        gSdlRenderer = nullptr;
    }
}

void handleWindowSizeChanged()
{
    if (scene_renderer::isActive()) {
        // The GL presentation letterboxes into the current window size on
        // every frame; there is nothing to recreate.
        mouseDeviceRefreshWindowMapping();
        return;
    }

    movieHandleRendererReset();
    destroyRenderer();
    createRenderer(screenGetWidth(), screenGetHeight());
    mouseDeviceRefreshWindowMapping();
}

// 209: the display-mode pick applies the moment the menu closes. The picture
// itself keeps its size and letterboxes, so the engine buffers stay intact;
// only the resolution and the interface scale still need a restart.
void applyWindowModeLive()
{
    if (gSdlWindow == nullptr || SDL_getenv("F2CE_DEV_BACKGROUND") != nullptr) {
        return;
    }

    int width = settings.screen.resolution_x;
    int height = settings.screen.resolution_y;
    if (settings.screen.resolution_mode > 0) {
        width = settings.screen.resolution_mode / 10000;
        height = settings.screen.resolution_mode % 10000;
    }

    const WindowMode mode = settings.screen.windowed;
    if (mode == WindowMode::WindowedFullscreen) {
        SDL_SetWindowFullscreen(gSdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else if (mode == WindowMode::Fullscreen) {
        SDL_DisplayMode request = {};
        request.w = width;
        request.h = height;
        SDL_SetWindowDisplayMode(gSdlWindow, &request);
        SDL_SetWindowFullscreen(gSdlWindow, SDL_WINDOW_FULLSCREEN);
    } else {
        SDL_SetWindowFullscreen(gSdlWindow, 0);
        SDL_SetWindowSize(gSdlWindow, width, height);
        SDL_SetWindowPosition(gSdlWindow, SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0));
    }

    handleWindowSizeChanged();
}

void renderFpsCounter()
{
    if (!settings.debug.show_fps || gSdlSurface == nullptr || gSdlTextureSurface == nullptr) {
        return;
    }

    static unsigned int sampleStartTicks = 0;
    static int sampleFrames = 0;
    static double fps = 0.0;

    unsigned int now = SDL_GetTicks();
    if (sampleStartTicks == 0) {
        sampleStartTicks = now;
    }

    sampleFrames++;

    unsigned int elapsed = now - sampleStartTicks;
    if (elapsed >= 500) {
        fps = sampleFrames * 1000.0 / elapsed;
        sampleFrames = 0;
        sampleStartTicks = now;
    }

    char text[32];
    snprintf(text, sizeof(text), "FPS: %.1f", fps);

    ScopedFont font(101);

    constexpr int kPadding = 2;
    int textWidth = fontGetStringWidth(text);
    int textHeight = fontGetLineHeight();
    int width = textWidth + kPadding * 2;
    int height = textHeight + kPadding * 2;

    if (width > gSdlSurface->w) {
        width = gSdlSurface->w;
    }

    if (height > gSdlSurface->h) {
        height = gSdlSurface->h;
    }

    bufferFill(static_cast<unsigned char*>(gSdlSurface->pixels), width, height, gSdlSurface->pitch, COLOR_BLACK);
    if (width > kPadding * 2 && height > kPadding * 2) {
        fontDrawText(static_cast<unsigned char*>(gSdlSurface->pixels) + gSdlSurface->pitch * kPadding + kPadding, text, width - kPadding * 2, gSdlSurface->pitch, COLOR_LIGHT_GREY);
    }

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    SDL_BlitSurface(gSdlSurface, &rect, gSdlTextureSurface, &rect);
}

static void renderPresentBody();

void renderPresent()
{
    // The file read progress handler (map.cc) presents the frame on the main
    // thread while a load reads its files. A present that starts inside a
    // present (the 127 readout loading font 101 through an art read in the
    // middle of one) must be a no-op: the nested pass tramples the renderer
    // and presentation state and corrupts the heap at map load.
    static bool inRenderPresent = false;
    if (inRenderPresent) return;
    inRenderPresent = true;
    renderPresentBody();
    inRenderPresent = false;
}

static void renderPresentBody()
{
    // The frame ring feeds the owner shot's fps block (183); the pump takes a
    // pending --dev-owner-shot-once at the first present that can hold it.
    scene_renderer::perfFrameTick();
    // 127: the frame-time log samples the same present cadence.
    fpsCounterOnFrame();
    owner_shot::pump();
    // The 3D scene shows the world: the tile window keeps only what the game
    // draws over it.
    if (scene_renderer::isActive()) {
        scene::syncToMap();
        devtools::sceneOverlaySynced(scene::syncOverlay());
    }
    // 127: the player's frame rate readout. It draws after the tile window's
    // own redraw (syncToMap above), which restores every captured text pixel
    // in its region: a readout drawn before that redraw was erased the same
    // frame and never reached the presented picture.
    static Uint32 fpsWindowStart = 0;
    static unsigned int fpsWindowFrames = 0;
    static unsigned int fpsValue = 0;
    static double frameTimeMs = 0.0;
    static bool fpsWasDrawn = false;
    static int fpsX = 0;
    static int fpsWidth = 0;
    const bool showFps = settings.scene3d.fps_counter && scene_renderer::isActive() && gSdlSurface != nullptr
        // A present driven by the file read progress handler belongs to a
        // loading screen: drawing here (a font load, a text draw) lands in
        // the middle of the load and corrupted the heap at map load.
        && !fileReadProgressHandlerActive();
    if (!showFps) {
        if (fpsWasDrawn && gSdlSurface != nullptr) {
            textCaptureEraseTextRuns(static_cast<unsigned char*>(gSdlSurface->pixels), "FPS ");
        }
        fpsWasDrawn = false;
        fpsWindowStart = 0;
        fpsWindowFrames = 0;
    } else {
        const Uint32 now = SDL_GetTicks();
        if (fpsWindowStart == 0) fpsWindowStart = now;
        ++fpsWindowFrames;
        const Uint32 elapsed = now - fpsWindowStart;
        if (elapsed >= 500 && fpsWindowFrames > 0) {
            fpsValue = static_cast<unsigned int>((static_cast<Uint64>(fpsWindowFrames) * 1000 + elapsed / 2) / elapsed);
            frameTimeMs = static_cast<double>(elapsed) / fpsWindowFrames;
            fpsWindowStart = now;
            fpsWindowFrames = 0;
        }
        textCaptureEraseTextRuns(static_cast<unsigned char*>(gSdlSurface->pixels), "FPS ");
        char fpsText[48];
        snprintf(fpsText, sizeof(fpsText), "FPS %3u  %5.1f ms", fpsValue, frameTimeMs);
        ScopedFont fpsFont(101);
        const int width = fontGetStringWidth(fpsText);
        const int x = gSdlSurface->w - width - 8;
        if (x >= 0) {
            fpsX = x;
            fpsWidth = width;
            fontDrawText(static_cast<unsigned char*>(gSdlSurface->pixels) + gSdlSurface->pitch * 8 + x,
                fpsText, width, gSdlSurface->pitch, COLOR_GREEN | DRAW_TEXT_FLAG_SHADOWED);
            fpsWasDrawn = true;
        }
    }
    // Sprite-lighting mod: the dev panel is drawn into the framebuffer after
    // the game (so it shows in both presentation paths).
    devtools::draw(static_cast<unsigned char*>(gSdlSurface->pixels), gSdlSurface->w, gSdlSurface->h, gSdlSurface->pitch);
    // The owner's region mark rectangles, over everything else (183).
    owner_shot::draw(static_cast<unsigned char*>(gSdlSurface->pixels), gSdlSurface->w, gSdlSurface->h, gSdlSurface->pitch);
    const bool uiVisible = scene_renderer::isActive() && !scene_renderer::staticFlyCameraActive()
        && (!scene::shouldPresent() || scene::activeView() == scene::View::Full);
    const bool sharpTextReady = scene_renderer::prepareSharpTextForPresent(gSdlSurface, gSdlTextureSurface, uiVisible);
    // A frame without the 3D scene (combat, dialog, ...) lands the fly camera
    // before the game takes input again.
    if (scene_renderer::staticFlyCameraActive()
        && (!scene_renderer::isActive() || !scene::shouldPresent() || scene::activeView() != scene::View::Full)) {
        devtools::landFlight();
    }
    if (scene_renderer::isActive()) {
        if (scene::shouldPresent()) {
            // Read the current hero frame at each scene presentation; nested
            // modal presents yield below and never advance movement observers.
            devtools::scenePresented(scene::refreshForPresent());
            const scene::View view = scene::activeView();
            if (view == scene::View::Full) {
                // With the glow on, the scene renders into its own
                // framebuffer, which comes back with its bleed added.
                scene_renderer::perfBegin();
                const bool glow = scene_renderer::beginSceneCapture();
                scene_renderer::drawStaticHeroPreview();
                scene_renderer::perfMark("scene");
                scene_renderer::perfLoadMark("first_frame");
                if (glow) scene_renderer::compositeGlow();
                scene_renderer::perfMark("glow");
                // A fly camera shows the world alone.
                if (scene_renderer::staticFlyCameraActive()) {
                    if (fpsWasDrawn && fpsWidth > 0 && gSdlSurface->format != nullptr
                        && gSdlSurface->format->palette != nullptr) {
                        const int overlayHeight = std::min(fontGetLineHeight(), gSdlSurface->h - 8);
                        const int overlayWidth = std::min(fpsWidth, gSdlSurface->w - fpsX);
                        const int rgbaPitch = overlayWidth * 4;
                        std::vector<unsigned char> rgba(static_cast<size_t>(rgbaPitch) * overlayHeight, 0);
                        SDL_Palette* palette = gSdlSurface->format->palette;
                        for (int y = 0; y < overlayHeight; ++y) {
                            for (int x = 0; x < overlayWidth; ++x) {
                                const unsigned char index = static_cast<const unsigned char*>(gSdlSurface->pixels)
                                    [static_cast<size_t>(y + 8) * gSdlSurface->pitch + fpsX + x];
                                if (index == COLOR_FIRST || index >= palette->ncolors) continue;
                                const SDL_Color& color = palette->colors[index];
                                unsigned char* pixel = rgba.data() + static_cast<size_t>(y) * rgbaPitch + x * 4;
                                pixel[0] = color.r;
                                pixel[1] = color.g;
                                pixel[2] = color.b;
                                pixel[3] = 255;
                            }
                        }
                        scene_renderer::drawOverlay(rgba.data(), overlayWidth, overlayHeight, rgbaPitch,
                            fpsX, 8, overlayWidth, overlayHeight);
                    }
                    owner_shot::drawSceneOverlay();
                    devtools::sceneLiveFrameReady();
                    scene_renderer::swap();
                    scene_renderer::perfEnd();
                    devtools::sceneOverlayPresented();
                    return;
                }
                if (!gRgbaBackground.empty()) {
                    // Fullscreen owner art replaces the old indexed background
                    // while preserving the screen's foreground (for example,
                    // death subtitles) through its mask.
                    SDL_BlitSurface(gSdlSurface, nullptr, gSdlTextureSurface, nullptr);
                    scene::maskOverlay(static_cast<const unsigned char*>(gSdlSurface->pixels), gSdlSurface->pitch,
                        static_cast<unsigned char*>(gSdlTextureSurface->pixels), gSdlTextureSurface->pitch,
                        gSdlSurface->w, gSdlSurface->h);
                    renderApplyRgbaBackground();
                    scene_renderer::drawSceneOverlay(
                        static_cast<const unsigned char*>(gSdlTextureSurface->pixels),
                        gSdlTextureSurface->w, gSdlTextureSurface->h, gSdlTextureSurface->pitch);
                    scene_renderer::swap();
                    scene_renderer::perfEnd();
                    devtools::sceneOverlayPresented();
                    return;
                }
                // The game's interface, mouse cursor and floating text over it.
                scene_renderer::drawSceneOverlayWithSharpText(gSdlSurface, gSdlTextureSurface, sharpTextReady);
                scene_renderer::perfMark("overlay");
                // 217: the mark rectangles ride the RGBA overlay pass - the
                // sharp-text rebuild above drops raw indexed pixels, so the
                // surface draw alone never reached the screen.
                owner_shot::drawSceneOverlay();
                // 218: the drag bench reads the presented pixels right here,
                // where the overlay has just landed.
                owner_shot::verifySceneOverlayPresented();
                devtools::sceneLiveFrameReady();
                scene_renderer::swap();
                scene_renderer::perfEnd();
                devtools::sceneOverlayPresented();
                return;
            }
            if (view == scene::View::Walls) {
                scene_renderer::drawStaticWallPreview();
                scene_renderer::swap();
                return;
            }
            if (view == scene::View::Floor) {
                scene_renderer::drawStaticFloor();
                scene_renderer::swap();
                return;
            }
        }
        // Palette -> RGBA conversion is the same SDL_BlitSurface the vanilla
        // path performs every frame, so palette fades and cycling behave
        // identically under the OpenGL presentation.
        scene_renderer::presentFrameWithSharpText(gSdlSurface, gSdlTextureSurface, sharpTextReady);
        renderApplyRgbaBackground();
        // render movie frame if present
        movieRenderDirectOverlay();
        scene_renderer::swap();
        devtools::vanillaPresented();
        return;
    }

    renderApplyRgbaBackground();
    SDL_UpdateTexture(gSdlTexture, nullptr, gSdlTextureSurface->pixels, gSdlTextureSurface->pitch);
    SDL_RenderClear(gSdlRenderer);
    SDL_RenderCopy(gSdlRenderer, gSdlTexture, nullptr, nullptr);
    // render movie SDL texture if present
    movieRenderDirectOverlay();
    SDL_RenderPresent(gSdlRenderer);
    devtools::vanillaPresented();
}

} // namespace fallout
