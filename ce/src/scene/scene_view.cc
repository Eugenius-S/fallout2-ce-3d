#include "scene_view.h"

#include <ctype.h>
#include <cmath>
#include <filesystem>
#include <vector>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "preview_policy.h"
#include "scene_settings.h"
#include "scene_objects.h"
#include "../art.h"
#include "../devtools/devtools.h"
#include "../dialog.h"
#include "../object.h"
#include "../scene_renderer/game_view.h"
#include "../scene_renderer/picking.h"
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
bool gFreeScroll = false; // tileSetFreeScroll as set for the full view
// The map elevation whose scene the renderer holds, and the last one that
// could not be loaded (not tried again until the game leaves it).
std::string gLoadedMap;
int gLoadedElevation = -1;
View gLoadedView = View::Off;
std::string gFailedMap;
int gFailedElevation = -1;
std::string gFailure;
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

// The current map's name as the exporter writes it (kladwtwn), and elevation.
std::string currentMapName()
{
    char base[64];
    stripMapExtension(gMapHeader.name, base, sizeof(base));
    std::string name = base;
    for (char& ch : name) ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    return name;
}

// Loads what `view` needs of the current map's scene.
bool loadScene(View view, const std::string& map, int elevation, std::string* message)
{
    // F2CE_SCENE_CACHE (dev tools only): a worktree's own scene cache, so a test
    // run of a new exporter leaves the cache of a game running alongside alone.
    const char* devCache = devtools::isEnabled() ? SDL_getenv("F2CE_SCENE_CACHE") : nullptr;
    const std::filesystem::path scenes = devCache != nullptr && devCache[0] != '\0'
        ? std::filesystem::path(devCache)
        : gameRootPath() / "mod-cache" / "scene";
    const std::filesystem::path cacheDirectory = scenes / map / std::to_string(elevation);
    std::string error;
    if (!scene_renderer::loadStaticFloor(cacheDirectory, &error)) {
        *message = "floor preview unavailable: " + error;
        return false;
    }
    if (view != View::Floor && !scene_renderer::loadStaticWalls(cacheDirectory, &error)) {
        *message = "wall preview unavailable: " + error;
        return false;
    }
    if (view == View::Full && !scene_renderer::loadStaticProjection(cacheDirectory, &error)) {
        *message = "hero projection unavailable: " + error;
        return false;
    }
    return true;
}

// Roofs as the game shows them now: hidden over the building the hero is in
// (tile_fill_roof), all of them off with the roof toggle; and the hero's
// see-through circle over them (the egg, as tileRenderRoof uses it).
void updateRoofsFromGame()
{
    static unsigned char visible[SQUARE_GRID_SIZE];
    const bool roofsOn = tileRoofIsVisible() != 0;
    const TileData* squares = _square[gElevation];
    for (int index = 0; index < SQUARE_GRID_SIZE; index++) {
        // The upper 16 bits: the roof's 12-bit frame and 4 flag bits.
        const unsigned int roof = static_cast<unsigned int>(squares->tileFid[index]) >> 16;
        const unsigned int flags = (roof >> 12) & 0xF;
        visible[index] = roofsOn && (flags & 0x3 /* TemporarilyHidden | AlwaysHidden */) == 0 ? 1 : 0;
    }
    scene_renderer::setStaticRoofVisibility(visible);

    static std::vector<unsigned char> eggMask;
    static int eggWidth = 0, eggHeight = 0, eggX = 0, eggY = 0;
    if (eggMask.empty() && gEgg != nullptr) {
        FrmImage image;
        if (image.lock(FrmId(gEgg)) && image.getData() != nullptr) {
            eggWidth = image.getWidth();
            eggHeight = image.getHeight();
            eggX = image.getXOffset();
            eggY = image.getYOffset();
            eggMask.assign(image.getData(), image.getData() + eggWidth * eggHeight);
        }
    }
    double pixel[2], x, y;
    if (eggMask.empty() || gEgg == nullptr || !hexGridTileIsValid(gEgg->tile)) {
        scene_renderer::clearStaticEgg();
        return;
    }
    scene_renderer::gameViewTilePixel(gEgg->tile, pixel);
    if (!scene_renderer::staticScreenFromGamePixel(pixel[0] + eggX + gEgg->x, pixel[1] + eggY + gEgg->y, &x, &y)) {
        scene_renderer::clearStaticEgg();
        return;
    }
    scene_renderer::setStaticEgg(eggMask.data(), eggWidth, eggHeight,
        static_cast<int>(std::lround(x)) - eggWidth / 2, static_cast<int>(std::lround(y)) - eggHeight + 1);
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
    return !gLoadedMap.empty() && gLoadedMap == currentMapName() && gLoadedElevation == static_cast<int>(gElevation);
}

void syncToMap()
{
    const std::string map = currentMapName();
    const int elevation = static_cast<int>(gElevation);
    if (!gLoadedMap.empty() && (gLoadedMap != map || gLoadedElevation != elevation)) {
        scene_renderer::unloadStaticScene();
        gLoadedMap.clear();
        gLoadedElevation = -1;
    }
    if (gFailedMap != map || gFailedElevation != elevation) {
        gFailedMap.clear();
        gFailedElevation = -1;
    }
    // Floor < Walls < Full: a view needs what the smaller ones load, and more.
    const bool loaded = !gLoadedMap.empty() && static_cast<int>(gLoadedView) >= static_cast<int>(gView);
    if (gView != View::Off && !map.empty() && gFailedMap.empty() && !loaded) {
        std::string message;
        if (loadScene(gView, map, elevation, &message)) {
            gLoadedMap = map;
            gLoadedElevation = elevation;
            gLoadedView = gView;
            gFailure.clear();
        } else {
            scene_renderer::unloadStaticScene();
            gLoadedMap.clear();
            gLoadedElevation = -1;
            gFailedMap = map;
            gFailedElevation = elevation;
            gFailure = message;
            printf("scene: %s elevation %d stays 2D: %s\n", map.c_str(), elevation, message.c_str());
            fflush(stdout);
            // A file too: the Windows build has no console.
            if (FILE* log = fopen("scene.log", "a")) {
                fprintf(log, "scene: %s elevation %d stays 2D: %s\n", map.c_str(), elevation, message.c_str());
                fclose(log);
            }
        }
    }
    const bool active = activeView() == View::Full;
    if (active != gFreeScroll) {
        tileSetFreeScroll(active); // the 3D view scrolls past map edges
        gFreeScroll = active;
        resetHeroLog();
    }
}

void syncSettings(bool suppressInitialEnable)
{
    // Wait until gameplay has a real map; settings must not activate a scene
    // from the main menu or while a map transition is still being installed.
    if (gMapHeader.name[0] == '\0' || gameGetState() != GameState::Normal) return;

    static SettingsLifecycle lifecycle;
    const SettingsAction action = lifecycle.update(settings.scene3d.enabled,
        settingsGetThreeDRevision(), suppressInitialEnable);
    if (action == SettingsAction::EnableFull) {
        std::string error;
        enable(View::Full, &error);
    } else if (action == SettingsAction::Disable) {
        disable();
    }
}

bool enable(View view, std::string* message)
{
    if (view == View::Off) {
        disable();
        return true;
    }
    if (!settings.scene3d.enabled) {
        *message = "3D scene is disabled in settings";
        return false;
    }
    gView = view;
    gFailedMap.clear();
    gFailedElevation = -1;
    syncToMap();
    if (!showsCurrentMap()) {
        *message = gFailure.empty() ? "no map is loaded" : gFailure;
        return false;
    }
    message->clear();
    return true;
}

bool disable()
{
    if (gView == View::Off) return false;
    gView = View::Off;
    if (gFreeScroll) {
        tileSetFreeScroll(false);
        gFreeScroll = false;
    }
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
    return settings.scene3d.enabled && gView != View::Off && showsCurrentMap() ? gView : View::Off;
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
    updateRoofsFromGame();
    const double seconds = static_cast<double>(SDL_GetPerformanceCounter() - start) / SDL_GetPerformanceFrequency();
    return { heroUpdated, seconds };
}

} // namespace scene
} // namespace fallout
