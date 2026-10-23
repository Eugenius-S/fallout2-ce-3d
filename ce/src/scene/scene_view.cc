#include "scene_view.h"

#include "../dev_output.h"

#include <algorithm>
#include <ctype.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "preview_policy.h"
#include "scene_settings.h"
#include "scene_lights.h"
#include "scene_objects.h"
#include "../art.h"
#include "../devtools/devtools.h"
#include "../dialog.h"
#include "../map_edge.h"
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
#include "../scene_renderer/sprite_card.h"
#include "../settings.h"
#include "../tile.h"

namespace fallout {
namespace scene {

namespace {

View gView = View::Off;
bool gCameraGlide = true;
bool gFreeScroll = false; // tileSetFreeScroll as set for the full view
// The wheel sets a target zoom (settings.scene3d.zoom) and the shown zoom
// eases there, so the picture glides instead of jumping a step at a time.
constexpr double kZoomGlideSeconds = 0.2;
double gZoomShown = 0.0; // the zoom applied last frame (0: nothing applied yet)
bool gZoomGliding = false;
double gZoomGlideFrom = 1.0;
Uint64 gZoomGlideStarted = 0;
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
std::uint64_t gCleanEdgesRevision = static_cast<std::uint64_t>(-1);
std::string gClipMap;
int gClipElevation = -1;
std::string gClipFailureMap;
int gClipFailureElevation = -1;
std::uint64_t gClipFailureRevision = static_cast<std::uint64_t>(-1);

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

scene_renderer::MapClip authoredMapClip()
{
    using namespace scene_renderer;
    const MapClip floorClip = staticFloorMapClip();
    if (!mapEdgeIsLoaded()) return floorClip;
    const MapElevation elevation = static_cast<MapElevation>(gElevation);

    if (mapEdgeHasSquareRect(elevation)) {
        Rect square;
        mapEdgeGetSquareRect(elevation, &square);
        if (square.left < 0 || square.left > 99 || square.right < 0 || square.right > 99
            || square.top < 0 || square.top > 99 || square.bottom < 0 || square.bottom > 99) return floorClip;
        const double left = std::min(square.left, square.right) - 50.0;
        const double right = std::max(square.left, square.right) + 1.0 - 50.0;
        const double top = std::min(square.top, square.bottom) - 50.0;
        const double bottom = std::max(square.top, square.bottom) + 1.0 - 50.0;
        return makeMapClipRect(left, top, right, bottom, MapClipSource::Authored);
    }

    const EdgeElevationData& data = mapEdgeGetElevationData(elevation);
    if (data.zones.empty()) return floorClip;
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    bool haveZone = false;
    for (const EdgeZone& zone : data.zones) {
        const int left = std::min(zone.pixelRect.left, zone.pixelRect.right);
        const int right = std::max(zone.pixelRect.left, zone.pixelRect.right);
        const int top = std::min(zone.pixelRect.top, zone.pixelRect.bottom);
        const int bottom = std::max(zone.pixelRect.top, zone.pixelRect.bottom);
        if (left == right || top == bottom) continue;
        if (!haveZone) {
            minX = left; maxX = right; minY = top; maxY = bottom;
            haveZone = true;
        } else {
            minX = std::min(minX, left); maxX = std::max(maxX, right);
            minY = std::min(minY, top); maxY = std::max(maxY, bottom);
        }
    }
    if (!haveZone) return floorClip;

    // EDG v1 offsets are a different map coordinate origin and have opposite X
    // handedness from game pixels. Anchor at the same known hex, then apply the
    // game's own projection inverse to ground. Envelope zones before conversion
    // so walking across a zone never changes the cut.
    int anchorEdgeX = 0, anchorEdgeY = 0;
    tileToPixelOffset(0, anchorEdgeX, anchorEdgeY);
    double anchorGame[2];
    gameViewTilePixel(0, anchorGame);
    const double edgeCorners[4][2] = {
        { static_cast<double>(minX), static_cast<double>(minY) },
        { static_cast<double>(maxX), static_cast<double>(minY) },
        { static_cast<double>(maxX), static_cast<double>(maxY) },
        { static_cast<double>(minX), static_cast<double>(maxY) },
    };
    SpriteCardProjection projection;
    if (!scene_renderer::currentGameProjection(&projection)) return floorClip;
    MapClipPoint points[4];
    for (int index = 0; index < 4; index++) {
        double gameX = 0.0, gameY = 0.0;
        edgeOffsetToGamePixel(edgeCorners[index][0], edgeCorners[index][1], anchorEdgeX, anchorEdgeY,
            anchorGame[0], anchorGame[1], &gameX, &gameY);
        if (!spritePixelToGround(projection, gameX, gameY, &points[index].x, &points[index].z)) return floorClip;
    }
    const MapClip authored = makeMapClip(points, MapClipSource::Authored);
    return authored.enabled ? authored : floorClip;
}

void updateMapClip()
{
    if (gLoadedMap.empty() || gLoadedMap != currentMapName() || gLoadedElevation != static_cast<int>(gElevation)) {
        scene_renderer::setStaticMapClip({});
        gClipMap.clear();
        gClipElevation = -1;
        return;
    }
    const std::uint64_t revision = cleanEdgesRevision();
    if (gClipMap == gLoadedMap && gClipElevation == gLoadedElevation && gCleanEdgesRevision == revision) return;
    scene_renderer::MapClip clip;
    if (cleanEdges()) {
        const char* requested = devtools::isEnabled() ? SDL_getenv("F2CE_DEV_CLEAN_EDGES") : nullptr;
        if (requested != nullptr && compat_stricmp(requested, "floor") == 0) {
            clip = scene_renderer::staticFloorMapClip();
        } else {
            clip = authoredMapClip();
        }
    }
    scene_renderer::setStaticMapClip(clip);
    if (cleanEdges() && !clip.enabled
        && (gClipFailureMap != gLoadedMap || gClipFailureElevation != gLoadedElevation
            || gClipFailureRevision != revision)) {
        const char* reason = "no valid authored edge rectangle or floor envelope";
        printf("scene: %s elevation %d clean map edges disabled: %s\n",
            gLoadedMap.c_str(), gLoadedElevation, reason);
        fflush(stdout);
        if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
            fprintf(log, "scene: %s elevation %d clean map edges disabled: %s\n",
                gLoadedMap.c_str(), gLoadedElevation, reason);
            fclose(log);
        }
        gClipFailureMap = gLoadedMap;
        gClipFailureElevation = gLoadedElevation;
        gClipFailureRevision = revision;
    }
    gClipMap = gLoadedMap;
    gClipElevation = gLoadedElevation;
    gCleanEdgesRevision = revision;
}

// scene.json's `"daylight": true`: the map elevation is under the sky. An
// export without the key (or without the file) has no sun.
bool readDaylight(const std::filesystem::path& cacheDirectory)
{
    std::ifstream file(cacheDirectory / "scene.json", std::ios::binary);
    if (!file) return false;
    std::ostringstream text;
    text << file.rdbuf();
    const std::string json = text.str();
    const size_t key = json.find("\"daylight\"");
    if (key == std::string::npos) return false;
    size_t value = json.find(':', key);
    if (value == std::string::npos) return false;
    value = json.find_first_not_of(" \t\r\n", value + 1);
    return value != std::string::npos && json.compare(value, 4, "true") == 0;
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
    printf("scene: cache %s\n", cacheDirectory.string().c_str());
    fflush(stdout);
    std::string error;
    if (!scene_renderer::loadStaticFloor(cacheDirectory, &error)) {
        *message = "floor preview unavailable: " + error;
        return false;
    }
    setSceneDaylight(readDaylight(cacheDirectory));
    if (view != View::Floor && !scene_renderer::loadStaticWalls(cacheDirectory, &error)) {
        *message = "wall preview unavailable: " + error;
        return false;
    }
    if (view == View::Full && !scene_renderer::loadStaticProjection(cacheDirectory, &error)) {
        *message = "hero projection unavailable: " + error;
        return false;
    }
    if (view == View::Full) {
        scene_renderer::SpriteCardProjection projection;
        if (scene_renderer::currentGameProjection(&projection)) {
            scene_renderer::setStaticFloorMapProjection(projection);
        }
    }
    gClipMap.clear(); // Full loads add the projection needed to convert legacy EDG pixels.
    gClipElevation = -1;
    gCleanEdgesRevision = static_cast<std::uint64_t>(-1);
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
            if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
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
    updateMapClip();
}

void syncSettings(bool suppressInitialEnable)
{
    // Wait until gameplay has a real map; settings must not activate a scene
    // from the main menu or while a map transition is still being installed.
    if (gMapHeader.name[0] == '\0' || gameGetState() != GameState::Normal) return;

    static SettingsLifecycle lifecycle;
    const SettingsAction action = lifecycle.update(enabled(), settingsRevision(), suppressInitialEnable);
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

namespace {

double gTurnAt = 0.0; // degrees round, where the camera is
double gTurnTo = 0.0; // and where it goes
bool gTurnOwnsCamera = false;
Uint64 gTurnLast = 0;
scene_renderer::StaticFlyCamera gTurnPose = {};
constexpr double kTurnSpeed = 360.0; // degrees a second: a quarter turn in a quarter of a second

void landCameraTurn()
{
    gTurnAt = gTurnTo = 0.0;
    if (gTurnOwnsCamera) scene_renderer::setStaticFlyCamera(nullptr);
    gTurnOwnsCamera = false;
}

} // namespace

bool turnCamera(int steps)
{
    if (!settings.scene3d.camera_rotation || steps == 0) return false;
    if (activeView() != View::Full || !shouldPresent()) return false;
    if (scene_renderer::staticFlyCameraActive() && !gTurnOwnsCamera) return false; // the dev fly camera
    gTurnTo += 90.0 * steps;
    return true;
}

void setCameraTurnNow(int steps)
{
    gTurnAt = gTurnTo = 90.0 * steps;
    gTurnLast = 0;
}

bool cameraTurned()
{
    return gTurnOwnsCamera;
}

void updateCameraTurn()
{
    if (!gTurnOwnsCamera && gTurnAt == 0.0 && gTurnTo == 0.0) return;
    if (activeView() != View::Full || !shouldPresent()
        || (scene_renderer::staticFlyCameraActive() && !gTurnOwnsCamera)) {
        landCameraTurn();
        return;
    }
    const Uint64 now = SDL_GetPerformanceCounter();
    const double seconds = gTurnLast == 0 ? 0.0
        : static_cast<double>(now - gTurnLast) / static_cast<double>(SDL_GetPerformanceFrequency());
    gTurnLast = now;
    const double step = std::min(kTurnSpeed * seconds, std::abs(gTurnTo - gTurnAt));
    gTurnAt += gTurnTo > gTurnAt ? step : -step;
    if (gTurnAt == gTurnTo && std::fmod(gTurnTo, 360.0) == 0.0) {
        landCameraTurn();
        return;
    }
    // The game's view, circled round the floor point it looks at.
    scene_renderer::StaticFlyCamera pose;
    if (!scene_renderer::staticGameViewCamera(&pose)) return;
    float forward[3], right[3];
    scene_renderer::staticFlyCameraAxes(pose, forward, right);
    if (!(forward[1] < -0.01f)) return;
    const float reach = -pose.eye[1] / forward[1];
    const float target[3] = { pose.eye[0] + forward[0] * reach, 0.0f, pose.eye[2] + forward[2] * reach };
    pose.yaw += static_cast<float>(gTurnAt * 3.14159265358979323846 / 180.0);
    scene_renderer::staticFlyCameraAxes(pose, forward, right);
    for (int i = 0; i < 3; i++) pose.eye[i] = target[i] - forward[i] * reach;
    gTurnPose = pose;
    scene_renderer::setStaticFlyCamera(&gTurnPose);
    gTurnOwnsCamera = true;
}

void adjustZoom(int steps)
{
    if (steps == 0) return;
    if (activeView() != View::Full || !shouldPresent()) return;
    double low = settings.scene3d.zoom_min;
    double high = settings.scene3d.zoom_max;
    if (low > high) std::swap(low, high);
    double zoom = settings.scene3d.zoom;
    zoom *= std::pow(1.25, static_cast<double>(steps));
    if (zoom < low) zoom = low;
    if (zoom > high) zoom = high;
    settings.scene3d.zoom = zoom; // written back on save, kept between sessions
    // Ease from what is on the screen now: a step in mid-glide re-anchors and
    // keeps the motion continuous.
    if (gZoomShown <= 0.0) gZoomShown = zoom;
    gZoomGlideFrom = gZoomShown;
    gZoomGlideStarted = SDL_GetPerformanceCounter();
    gZoomGliding = true;
}

// The shown zoom for this frame: the stored one, or the glide toward it.
double shownZoom()
{
    double zoom = settings.scene3d.zoom;
    if (gZoomGliding) {
        const double seconds = static_cast<double>(SDL_GetPerformanceCounter() - gZoomGlideStarted)
            / static_cast<double>(SDL_GetPerformanceFrequency());
        if (seconds < kZoomGlideSeconds) {
            const double t = seconds / kZoomGlideSeconds;
            zoom = gZoomGlideFrom + (zoom - gZoomGlideFrom) * (1.0 - std::pow(1.0 - t, 3.0));
        } else {
            gZoomGliding = false;
        }
    }
    return zoom;
}

RefreshResult refreshForPresent()
{
    if (activeView() != View::Full) return { false, 0.0 };
    const Uint64 start = SDL_GetPerformanceCounter();
    // Follow the stored zoom every frame, so a changed setting applies live;
    // a wheel step glides there instead (see adjustZoom).
    gZoomShown = shownZoom();
    scene_renderer::setCameraZoom(gZoomShown);
    // The game scrolls (screen edges, arrow keys, following the hero); the 3D
    // view goes with it.
    if (!gCameraGlide) scene_renderer::setStaticCameraCenterTile(-1);
    scene_renderer::setStaticCameraCenterTile(gCenterTile);
    const bool heroUpdated = updateObjectsFromGame();
    updateRoofsFromGame();
    updateLightsFromGame();
    updateSunFromGame();
    const double seconds = static_cast<double>(SDL_GetPerformanceCounter() - start) / SDL_GetPerformanceFrequency();
    return { heroUpdated, seconds };
}

} // namespace scene
} // namespace fallout
