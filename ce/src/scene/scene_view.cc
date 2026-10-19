#include "scene_view.h"

#include <filesystem>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "preview_policy.h"
#include "scene_objects.h"
#include "../dialog.h"
#include "../combat.h"
#include "../game.h"
#include "../map.h"
#include "../movie.h"
#include "../platform_compat.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/live_objects.h"
#include "../scene_renderer/static_scene.h"
#include "../settings.h"
#include "../tile.h"

namespace fallout {
namespace scene {

namespace {

View gView = View::Off;
bool gCameraGlide = true;
PreviewBlockReason gLastBlockReason = PreviewBlockReason::None;
bool gPolicyLogged = false;

void stripMapExtension(const char* name, char* out, size_t size)
{
    size_t len = strlen(name);
    if (len > 4
        && (compat_stricmp(name + len - 4, ".map") == 0
            || compat_stricmp(name + len - 4, ".sav") == 0)) {
        len -= 4;
    }
    if (len >= size) {
        len = size - 1;
    }
    memcpy(out, name, len);
    out[len] = '\0';
}

std::filesystem::path gameRootPath()
{
    if (settings.system.master_dat_path.empty()) {
        return std::filesystem::current_path();
    }
    std::filesystem::path datPath(settings.system.master_dat_path);
    if (!datPath.is_absolute()) {
        datPath = std::filesystem::current_path() / datPath;
    }
    std::filesystem::path root = datPath.parent_path();
    if (compat_stricmp(root.filename().string().c_str(), "data") == 0) {
        root = root.parent_path();
    }
    return root;
}

PreviewRuntimeState runtimeState()
{
    return {
        gameGetState() == GameState::Normal,
        GameMode::getCurrentGameMode() == GameMode::kNone,
        !gameUiIsDisabled(),
        !isoIsDisabled(),
        !isInCombat(),
        _inDialog == 0,
        _moviePlaying() == 0,
    };
}

const char* blockReasonName(PreviewBlockReason reason)
{
    switch (reason) {
    case PreviewBlockReason::None: return "available";
    case PreviewBlockReason::GameState: return "game-state";
    case PreviewBlockReason::GameMode: return "game-mode";
    case PreviewBlockReason::UiDisabled: return "ui-disabled";
    case PreviewBlockReason::IsoDisabled: return "iso-disabled";
    case PreviewBlockReason::Combat: return "combat";
    case PreviewBlockReason::Dialog: return "dialog";
    case PreviewBlockReason::Movie: return "movie";
    }
    return "unknown";
}

PreviewBlockReason currentBlockReason()
{
    const PreviewBlockReason reason = previewBlockReason(runtimeState());
    if (!gPolicyLogged || reason != gLastBlockReason) {
        printf("scene: preview policy=%s gameState=%d gameMode=0x%x uiDisabled=%d isoDisabled=%d combat=%d dialog=%d movie=%d\n",
            blockReasonName(reason), static_cast<int>(gameGetState()),
            static_cast<unsigned int>(GameMode::getCurrentGameMode()), gameUiIsDisabled() ? 1 : 0,
            isoIsDisabled() ? 1 : 0, isInCombat() ? 1 : 0, _inDialog != 0 ? 1 : 0,
            _moviePlaying() != 0 ? 1 : 0);
        fflush(stdout);
        gLastBlockReason = reason;
        gPolicyLogged = true;
    }
    return reason;
}

} // namespace

bool showsCurrentMap()
{
    char base[64];
    stripMapExtension(gMapHeader.name, base, sizeof(base));
    return compat_stricmp(base, "KLADWTWN") == 0 && static_cast<int>(gElevation) == 0;
}

bool enable(View view, std::string* message)
{
    if (view == View::Off) {
        disable();
        return true;
    }
    if (!showsCurrentMap()) {
        *message = "static floor preview requires KLADWTWN e0";
        return false;
    }
    const bool withWalls = view != View::Floor;
    const bool withHero = view == View::Full;
    const std::filesystem::path cacheDirectory = gameRootPath() / "mod-cache" / "scene" / "kladwtwn" / "0";
    std::string error;
    if (!scene_renderer::loadStaticFloor(cacheDirectory, &error)) {
        *message = "floor preview unavailable: " + error;
        return false;
    }
    if (withWalls && !scene_renderer::loadStaticWalls(cacheDirectory, &error)) {
        *message = "wall preview unavailable: " + error;
        return false;
    }
    if (withHero && !scene_renderer::loadStaticProjection(cacheDirectory, &error)) {
        *message = "hero projection unavailable: " + error;
        return false;
    }
    gView = view;
    tileSetFreeScroll(withHero); // the 3D view scrolls past map edges
    resetHeroLog();
    message->clear();
    return true;
}

bool disable()
{
    if (gView == View::Off) return false;
    gView = View::Off;
    tileSetFreeScroll(false);
    resetHeroLog();
    scene_renderer::clearStaticHero();
    scene_renderer::clearStaticObjects();
    scene_renderer::setStaticCameraCenterTile(-1);
    return true;
}

View requestedView()
{
    return gView;
}

View activeView()
{
    return gView != View::Off && showsCurrentMap() ? gView : View::Off;
}

bool shouldPresent()
{
    return activeView() != View::Off && currentBlockReason() == PreviewBlockReason::None;
}

const char* availability()
{
    if (activeView() == View::Off) return "not-requested";
    return blockReasonName(previewBlockReason(runtimeState()));
}

void setCameraGlide(bool glide)
{
    gCameraGlide = glide;
}

RefreshResult refreshForPresent()
{
    if (activeView() != View::Full) return { false, 0.0 };
    const Uint64 start = SDL_GetPerformanceCounter();
    // The game scrolls (screen edges, arrow keys, following the hero); the 3D
    // view goes with it.
    if (!gCameraGlide) scene_renderer::setStaticCameraCenterTile(-1);
    scene_renderer::setStaticCameraCenterTile(gCenterTile);
    const bool heroUpdated = updateObjectsFromGame();
    const double seconds = static_cast<double>(SDL_GetPerformanceCounter() - start) / SDL_GetPerformanceFrequency();
    return { heroUpdated, seconds };
}

} // namespace scene
} // namespace fallout
