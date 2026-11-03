#include "display_apply.h"

#include "display_dimensions.h"
#include "display_surface_swap.h"
#include "display_window_transaction.h"
#include "iso_view_swap.h"
#include "object_view_swap.h"
#include "screen_buffer_swap.h"
#include "tile_display_swap.h"

#include "../dinput.h"
#include "../interface.h"
#include "../map.h"
#include "../object.h"
#include "../scene_renderer/context.h"
#include "../settings.h"
#include "../svga.h"
#include "../text_object.h"
#include "../tile.h"
#include "../window_manager.h"

#include <algorithm>

namespace fallout::scene {
namespace {
struct DisplayConfig {
    int resolutionX = 0;
    int resolutionY = 0;
    int resolutionMode = 0;
    WindowMode windowMode = WindowMode::Fullscreen;
    double scale = 1.0;
};

DisplayConfig appliedConfig;
bool haveAppliedConfig = false;

DisplayConfig currentConfig()
{
    return { settings.screen.resolution_x, settings.screen.resolution_y,
        settings.screen.resolution_mode, settings.screen.windowed,
        settings.screen.scale_factor };
}

void setConfig(const DisplayConfig& config)
{
    settings.screen.resolution_x = config.resolutionX;
    settings.screen.resolution_y = config.resolutionY;
    settings.screen.resolution_mode = config.resolutionMode;
    settings.screen.windowed = config.windowMode;
    settings.screen.scale_factor = config.scale;
}

bool sameConfig(const DisplayConfig& a, const DisplayConfig& b)
{
    return a.resolutionX == b.resolutionX && a.resolutionY == b.resolutionY
        && a.resolutionMode == b.resolutionMode && a.windowMode == b.windowMode
        && a.scale == b.scale;
}
}

void rememberConfiguredDisplaySettings()
{
    appliedConfig = currentConfig();
    haveAppliedConfig = true;
}

bool applyConfiguredDisplaySettings()
{
    const DisplayConfig requested = currentConfig();
    if (!haveAppliedConfig) rememberConfiguredDisplaySettings();
    if (sameConfig(requested, appliedConfig)) return true;
    const auto reject = [&]() {
        setConfig(appliedConfig);
        settingsSave();
        return false;
    };

    int outputWidth = requested.resolutionMode > 0
        ? requested.resolutionMode / 10000 : requested.resolutionX;
    int outputHeight = requested.resolutionMode > 0
        ? requested.resolutionMode % 10000 : requested.resolutionY;
    if (requested.windowMode == WindowMode::WindowedFullscreen) {
        const int display = std::max(0, SDL_GetWindowDisplayIndex(gSdlWindow));
        SDL_DisplayMode desktop = {};
        if (display < 0 || SDL_GetDesktopDisplayMode(display, &desktop) != 0) return reject();
        outputWidth = desktop.w;
        outputHeight = desktop.h;
    }

    int logicalWidth = 0;
    int logicalHeight = 0;
    if (!displayLogicalDimensions(outputWidth, outputHeight, requested.scale,
            &logicalWidth, &logicalHeight)) return reject();

    const bool hasMap = gIsoWindow > 0 && windowGetWindow(gIsoWindow) != nullptr;
    const int visibleHeight = logicalHeight
        - (settings.ui.iface_bar_mode ? 0 : INTERFACE_BAR_HEIGHT);
    if (hasMap && visibleHeight < 1) return reject();

    DisplaySurfaceSwap surface;
    ScreenBufferSwap screen;
    WindowBufferSwap isoWindow;
    TileDisplaySwap tile;
    IsoViewSwap isoView;
    ObjectViewSwap objectView;
    TextViewSwap textView;
    const bool buffered = windowManagerIsBuffered();
    if (!surface.prepare(logicalWidth, logicalHeight, gSdlSurface, gSdlRenderer)
        || !screen.prepare(logicalWidth, logicalHeight, buffered)) return reject();

    Window* isoWindowObject = hasMap ? windowGetWindow(gIsoWindow) : nullptr;
    if (hasMap) {
        if (!isoWindow.prepare(logicalWidth, visibleHeight)) return reject();
    }

    DisplayWindowTransaction windowTransaction(gSdlWindow);
    if (windowTransaction.apply(requested.windowMode, outputWidth, outputHeight)
        != DisplayWindowTransaction::Result::Applied) return reject();

    bool surfaceExchanged = false;
    bool screenExchanged = false;
    bool isoExchanged = false;
    bool tileExchanged = false;
    bool isoViewExchanged = false;
    bool objectExchanged = false;
    bool textExchanged = false;

    // The map's tile and object helpers hold independent aliases into this
    // window buffer. Prepare their replacement views before publishing them.
    if (hasMap) {
        const int mapBufferWidth = logicalWidth;
        const int mapBufferHeight = visibleHeight;
        if (!tile.prepare(isoWindow.preparedBuffer(), mapBufferWidth, mapBufferHeight, mapBufferWidth)) {
            windowTransaction.rollback();
            return reject();
        }
        // Temporarily expose the proposed tile dimensions while object tables
        // are generated. No game frame runs inside this function.
        if (!tileExchangeDisplayBuffer(tile)) {
            windowTransaction.rollback();
            return reject();
        }
        const bool objectPrepared = objectView.prepare(isoWindow.preparedBuffer(),
            mapBufferWidth, mapBufferHeight, mapBufferWidth);
        const bool tileRestored = tileExchangeDisplayBuffer(tile);
        if (!objectPrepared || !tileRestored
            || !isoView.prepare(isoWindow.preparedBuffer(),
                Rect { 0, 0, mapBufferWidth - 1, mapBufferHeight - 1 })
            || !textView.prepare(isoWindow.preparedBuffer(), mapBufferWidth, mapBufferHeight)) {
            windowTransaction.rollback();
            return reject();
        }
    }

    auto rollback = [&]() {
        if (textExchanged) textObjectsExchangeView(textView);
        if (objectExchanged) objectExchangeView(objectView);
        if (isoViewExchanged) mapExchangeIsoView(isoView);
        if (tileExchanged) tileExchangeDisplayBuffer(tile);
        if (isoExchanged) isoWindow.exchange(isoWindowObject);
        if (screenExchanged) windowManagerExchangeScreenBuffer(screen);
        if (surfaceExchanged) surface.exchange(gSdlSurface, gSdlTextureSurface, gSdlTexture, gSdlRenderer);
        if (scene_renderer::isActive()) scene_renderer::setLogicalDimensions(screenGetWidth(), screenGetHeight());
        windowTransaction.rollback();
        mouseDeviceRefreshWindowMapping();
        setConfig(appliedConfig);
        settingsSave();
        return false;
    };

    if (!surface.exchange(gSdlSurface, gSdlTextureSurface, gSdlTexture, gSdlRenderer)) return rollback();
    surfaceExchanged = true;
    if (!windowManagerExchangeScreenBuffer(screen)) return rollback();
    screenExchanged = true;
    if (hasMap) {
        windowMove(gIsoWindow, 0, 0);
        if (!isoWindow.exchange(isoWindowObject)) return rollback();
        isoExchanged = true;
        if (!tileExchangeDisplayBuffer(tile)) return rollback();
        tileExchanged = true;
        if (!mapExchangeIsoView(isoView)) return rollback();
        isoViewExchanged = true;
        if (!objectExchangeView(objectView)) return rollback();
        objectExchanged = true;
        if (!textObjectsExchangeView(textView)) return rollback();
        textExchanged = true;
    }
    if (scene_renderer::isActive() && !scene_renderer::setLogicalDimensions(logicalWidth, logicalHeight)) return rollback();

    mouseDeviceRefreshWindowMapping();
    interfaceRepositionForDisplayChange();
    if (hasMap) tileSetCenter(gCenterTile, TILE_SET_CENTER_REFRESH_WINDOW);
    Rect fullScreen = { 0, 0, screenGetWidth() - 1, screenGetHeight() - 1 };
    windowRefreshAll(&fullScreen);
    windowTransaction.commit();

    settings.screen.resolution_x = outputWidth;
    settings.screen.resolution_y = outputHeight;
    appliedConfig = requested;
    appliedConfig.resolutionX = outputWidth;
    appliedConfig.resolutionY = outputHeight;
    settingsSave();
    return true;
}

} // namespace fallout::scene
