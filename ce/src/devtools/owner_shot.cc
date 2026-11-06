#include "owner_shot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <lodepng.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

#include "../combat.h"
#include "../dev_output.h"
#include "../devtools/devtools.h"
#include "../platform_compat.h"
#include "../display_monitor.h"
#include "../dialog.h"
#include "../game_dialog.h"
#include "../game_movie.h"
#include "../kb.h"
#include "../map.h"
#include "../map_defs.h"
#include "../mouse.h"
#include "../worldmap.h"
#include "../object.h"
#include "../proto.h"
#include "../settings.h"
#include "../svga.h"
#include "../tile.h"
#include "../version.h"
#include "../platform/git_version.h"
#include "../scene/scene_view.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/context.h"
#include "../scene_renderer/game_view.h"
#include "../scene_renderer/perf.h"
#include "../scene_renderer/scene_draw.h"
#include "snapshot_rows.h"

namespace fallout {
namespace owner_shot {

namespace {

// The pending --dev-owner-shot-once shot, how test runs capture acceptance
// shots without a key press. "=N" delays it N seconds past the ready view,
// so a run settles into steady frames before the capture (183's frame-time
// bar reads off the window that contains the shot). A failed attempt retries
// on the next frames and gives up after about ten seconds of presents.
bool gShotPending = false;
int gShotAttempts = 0;
Uint32 gShotReadyAt = 0;
int gShotDelaySeconds = 0;

// The map file name lowercased without its extension, for the file name stem:
// a batch of shots sorts by time and the name tells the location (183).
void baseMapName(const char* name, char* out, size_t size)
{
    size_t used = 0;
    for (; used + 1 < size && name[used] != '\0' && name[used] != '.'; used++) {
        char ch = name[used];
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        out[used] = ch;
    }
    out[used] = '\0';
    if (used == 0) snprintf(out, size, "map");
}

void jsonEscape(const char* in, std::string& out)
{
    for (const char* p = in; *p != '\0'; p++) {
        const unsigned char ch = static_cast<unsigned char>(*p);
        if (ch == '"' || ch == '\\') {
            out += '\\';
            out += static_cast<char>(ch);
        } else if (ch < 0x20) {
            char piece[8];
            snprintf(piece, sizeof(piece), "\\u%04x", ch);
            out += piece;
        } else {
            out += static_cast<char>(ch);
        }
    }
}

std::string protoNameOf(const Object* object)
{
    std::string name;
    char* text = protoGetName(object);
    if (text != nullptr && text[0] != '\0') {
        jsonEscape(text, name);
    } else {
        name = "unnamed";
    }
    return name;
}

// The game's strings stay in the game's own encoding (cp1251 on a Russian
// install); the JSON keeps the raw bytes so nothing is lost in translation.
std::string jsonText(const char* text)
{
    std::string out;
    if (text != nullptr && text[0] != '\0') {
        jsonEscape(text, out);
        return out;
    }
    return "unknown";
}

// Fails quiet: the caller decides what the owner reads (the key press posts
// to the message monitor; the pump retries and never spams).
bool takeShot(std::string* failure)
{
    namespace fs = std::filesystem;

    if (gSdlSurface == nullptr || gSdlSurface->pixels == nullptr) {
        if (failure != nullptr) *failure = "no framebuffer";
        return false;
    }

    char stamp[32] = {};
    {
        std::time_t t = time(nullptr);
        struct tm* tmv = localtime(&t);
        if (tmv == nullptr || strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", tmv) == 0) {
            if (failure != nullptr) *failure = "clock unavailable";
            return false;
        }
    }

    char dateStamp[11];
    snprintf(dateStamp, sizeof(dateStamp), "%.4s-%.2s-%.2s", stamp, stamp + 4, stamp + 6);

    // The files land next to the dev shots: shots/dev/<date>/.
    fs::path directory;
    std::string folderError;
    try {
        directory = dev_output::outputDirectory("shots/dev", dateStamp);
    } catch (const std::exception& exception) {
        folderError = exception.what();
    }
    if (folderError.empty() && directory.empty()) folderError = "no output folder";
    if (!folderError.empty()) {
        if (failure != nullptr) *failure = folderError;
        return false;
    }

    char mapBase[32] = "map";
    if (gMapHeader.name[0] != '\0') {
        baseMapName(gMapHeader.name, mapBase, sizeof(mapBase));
    }
    int elevation = gDude != nullptr ? static_cast<int>(gDude->elevation) : 0;

    fs::path imagePath;
    fs::path sidecarPath;
    std::string imageFile;
    for (int suffix = 0; suffix < 1000 && imagePath.empty(); suffix++) {
        char stem[96];
        if (suffix == 0) {
            snprintf(stem, sizeof(stem), "%s-%s-e%d", stamp, mapBase, elevation);
        } else {
            snprintf(stem, sizeof(stem), "%s-%s-e%d-%03d", stamp, mapBase, elevation, suffix);
        }
        fs::path candidateImage = directory / (std::string(stem) + ".png");
        fs::path candidateSidecar = directory / (std::string(stem) + ".json");
        std::error_code ec;
        if (!fs::exists(candidateImage, ec) && !fs::exists(candidateSidecar, ec)) {
            imagePath = candidateImage;
            sidecarPath = candidateSidecar;
            imageFile = std::string(stem) + ".png";
        }
    }
    if (imagePath.empty()) {
        if (failure != nullptr) *failure = "no free file name";
        return false;
    }

    // The readback stays on this thread: the GL context belongs to it. Only
    // the PNG encode and the two file writes go to the worker below (183).
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels; // 3D: top-down RGBA rows
    std::vector<unsigned char> indexed; // 2D: tightly packed palette indices
    std::vector<unsigned char> palette; // 2D: the 768 palette bytes
    std::string captureError;
    const bool sceneLive = scene_renderer::isActive() && scene::shouldPresent()
        && scene::activeView() == scene::View::Full;
    if (sceneLive) {
        // What the owner sees: the 3D scene with the game's picture over it.
        if (!scene_renderer::captureStaticPreviewBuffer(true, true, true, pixels, &width, &height, &captureError)) {
            return false; // the view is not ready yet; the pump retries
        }
    } else if (scene_renderer::isActive()) {
        SDL_BlitSurface(gSdlSurface, nullptr, gSdlTextureSurface, nullptr);
        if (!scene_renderer::captureFrameBuffer(static_cast<const unsigned char*>(gSdlTextureSurface->pixels),
                gSdlTextureSurface->w, gSdlTextureSurface->h, gSdlTextureSurface->pitch,
                pixels, &width, &height, &captureError)) {
            if (failure != nullptr) *failure = captureError;
            return false;
        }
    } else {
        indexed.resize(static_cast<size_t>(gSdlSurface->w) * static_cast<size_t>(gSdlSurface->h));
        if (!devtools::copySnapshotRowsTightly(static_cast<const unsigned char*>(gSdlSurface->pixels),
                gSdlSurface->w, gSdlSurface->h, gSdlSurface->pitch, indexed.data(), indexed.size())) {
            if (failure != nullptr) *failure = "invalid framebuffer row pitch";
            return false;
        }
        unsigned char* pal = directDrawGetPalette();
        if (pal == nullptr) {
            if (failure != nullptr) *failure = "no palette";
            return false;
        }
        palette.assign(pal, pal + 768);
        width = gSdlSurface->w;
        height = gSdlSurface->h;
    }

    // The frame rate over the last few seconds, always on (183).
    double fps = 0.0;
    double averageMs = 0.0;
    double low1Ms = 0.0;
    const bool haveFps = scene_renderer::perfFrameStats(&fps, &averageMs, &low1Ms);

    // The player: tile, hex and the world position under it.
    std::string playerJson = "null";
    if (gDude != nullptr) {
        const int tile = gDude->tile;
        double worldX = 0.0;
        double worldZ = 0.0;
        bool haveWorld = false;
        scene_renderer::GameView view;
        if (scene_renderer::gameViewAtCenterTile(tile, &view)
            && scene_renderer::gameViewCenter(view, &worldX, &worldZ)) {
            haveWorld = true;
        }
        char piece[256];
        if (haveWorld) {
            snprintf(piece, sizeof(piece),
                "{\"tile\":%d,\"hex\":[%d,%d],\"world\":[%.3f,%.3f],\"elevation\":%d,\"rotation\":%d}",
                tile, tile % 200, tile / 200, worldX, worldZ, elevation, static_cast<int>(gDude->rotation));
        } else {
            snprintf(piece, sizeof(piece),
                "{\"tile\":%d,\"hex\":[%d,%d],\"world\":null,\"elevation\":%d,\"rotation\":%d}",
                tile, tile % 200, tile / 200, elevation, static_cast<int>(gDude->rotation));
        }
        playerJson = piece;
    }

    // The camera as the view looks from, and the game's own turn and zoom.
    std::string cameraJson = "null";
    {
        scene_renderer::StaticFlyCamera fly;
        char piece[512];
        if (scene_renderer::staticGameViewCamera(&fly)) {
            snprintf(piece, sizeof(piece),
                "{\"eye\":[%.3f,%.3f,%.3f],\"yaw_deg\":%.2f,\"fov_y_deg\":%.2f,\"camera_turn\":%.4f,\"zoom\":%.4f,\"center_tile\":%d}",
                fly.eye[0], fly.eye[1], fly.eye[2],
                static_cast<double>(fly.yaw) * 180.0 / 3.14159265358979323846,
                static_cast<double>(fly.fovY), scene::cameraTurnAngle(), settings.scene3d.zoom, gCenterTile);
            cameraJson = piece;
        } else {
            snprintf(piece, sizeof(piece),
                "{\"camera_turn\":%.4f,\"zoom\":%.4f,\"center_tile\":%d}",
                scene::cameraTurnAngle(), settings.scene3d.zoom, gCenterTile);
            cameraJson = piece;
        }
    }

    // The object under the cursor, as the game's own pick reads the hex.
    std::string cursorJson = "null";
    {
        int mouseX = 0;
        int mouseY = 0;
        mouseGetPosition(&mouseX, &mouseY);
        const int tile = tileFromScreenXY(mouseX, mouseY);
        if (tile >= 0 && gDude != nullptr) {
            for (Object* object = objectFindFirstAtLocation(gDude->elevation, tile);
                object != nullptr; object = objectFindNextAtLocation()) {
                if (((object->fid >> 24) & 0xF) == OBJ_TYPE_TILE) continue; // flat ground
                if (object->pid == -1) continue; // no prototype: not a reportable thing
                char piece[512];
                snprintf(piece, sizeof(piece),
                    "{\"pid\":%d,\"name\":\"%s\",\"fid\":%d,\"tile\":%d}",
                    object->pid, protoNameOf(object).c_str(), object->fid, object->tile);
                cursorJson = std::string("{\"tile\":") + std::to_string(tile) + ",\"object\":" + piece + "}";
                break;
            }
        }
    }

    // The map as the game names it, plus the city or area from maps.txt.
    const std::string mapFileJson = gMapHeader.name[0] != '\0' ? jsonText(gMapHeader.name) : std::string("none");
    std::string mapJson = "unknown";
    std::string cityJson = "unknown";
    {
        const Map currentMap = mapGetCurrentMap();
        if (currentMap >= 0) {
            mapJson = jsonText(mapGetName(currentMap, static_cast<MapElevation>(elevation)));
            cityJson = jsonText(mapGetCityName(currentMap));
        }
    }

    char fpsJson[192];
    if (haveFps) {
        snprintf(fpsJson, sizeof(fpsJson),
            "{\"fps\":%.1f,\"frame_avg_ms\":%.2f,\"frame_low1_ms\":%.2f,\"window_frames\":%d}",
            fps, averageMs, low1Ms, 300);
    } else {
        snprintf(fpsJson, sizeof(fpsJson), "{\"window_frames\":%d}", 300);
    }

    char renderJson[1024];
    const Scene3dSettings& scene3d = settings.scene3d;
    snprintf(renderJson, sizeof(renderJson),
        "{\"scene3d\":{\"enabled\":%s,\"sharp_text\":%s,\"lights\":%s,\"sun\":%s,\"shadows\":%s,"
        "\"cloud_shadows\":%s,\"shadow_pixel_size\":%.2f,\"shadow_density\":%.2f,\"sun_swing\":%.1f,"
        "\"glow\":%s,\"glow_strength\":%.2f,\"camera_rotation\":%s,\"camera_rotation_unlimited\":%s,"
        "\"sprite_sides\":%s,\"zoom\":%.4f,\"clean_edges\":%s},\"renderer\":\"%s\"}",
        scene3d.enabled ? "true" : "false",
        scene3d.sharp_text ? "true" : "false",
        scene3d.lights ? "true" : "false",
        scene3d.sun ? "true" : "false",
        scene3d.shadows ? "true" : "false",
        scene3d.cloud_shadows ? "true" : "false",
        scene3d.shadow_pixel_size, scene3d.shadow_density, scene3d.sun_swing,
        scene3d.glow ? "true" : "false", scene3d.glow_strength,
        scene3d.camera_rotation ? "true" : "false",
        scene3d.camera_rotation_unlimited ? "true" : "false",
        scene3d.sprite_sides ? "true" : "false",
        scene3d.zoom, scene3d.clean_edges ? "true" : "false",
        scene_renderer::isActive() ? "scene-gl" : "sdl-renderer");

    char resolutionJson[256];
    snprintf(resolutionJson, sizeof(resolutionJson),
        "\"resolution\":\"%dx%d\",\"logical_resolution\":\"%dx%d\",\"scale\":%d,\"scale_factor\":%.1f,\"windowed\":%s",
        width, height, screenGetWidth(), screenGetHeight(), settings.screen.scale, settings.screen.scale_factor,
        settings.screen.windowed == WindowMode::Windowed ? "true" : "false");

    char version[VERSION_MAX] = {};
    versionGetVersion(version, sizeof(version));
    const std::string versionJson = jsonText(version);

    std::string json;
    json.reserve(3072);
    json += "{\n";
    json += "  \"image\": \"" + imageFile + "\",\n";
    json += "  \"fps\": " + std::string(fpsJson) + ",\n";
    json += "  \"map\": \"" + mapJson + "\",\n";
    json += "  \"map_file\": \"" + mapFileJson + "\",\n";
    json += "  \"city\": \"" + cityJson + "\",\n";
    json += "  \"elevation\": " + std::to_string(elevation) + ",\n";
    json += "  \"player\": " + playerJson + ",\n";
    json += "  \"camera\": " + cameraJson + ",\n";
    json += "  \"cursor\": " + cursorJson + ",\n";
    json += "  " + std::string(resolutionJson) + ",\n";
    json += "  \"render\": " + std::string(renderJson) + ",\n";
    json += "  \"build\": {\"commit\":\"" + std::string(_BUILD_HASH) + "\",\"version\":\"" + versionJson + "\",\"taken\":\"" + stamp + "\"}\n";
    json += "}\n";

    // Everything the worker touches is owned by the lambda: the pixel
    // buffers moved in, the paths and the JSON copied by value.
    std::thread worker([imagePath = imagePath.string(), sidecarPath = sidecarPath.string(),
        pixels = std::move(pixels), indexed = std::move(indexed), palette = std::move(palette),
        width, height, json = std::move(json)]() {
        unsigned encodeError = 0;
        if (!indexed.empty()) {
            // The 2D frame: palette-expand into RGB rows, then encode.
            std::vector<unsigned char> rgb(static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
            for (size_t i = 0; i < indexed.size(); i++) {
                const unsigned char* entry = palette.data() + static_cast<size_t>(indexed[i]) * 3;
                rgb[i * 3 + 0] = entry[0];
                rgb[i * 3 + 1] = entry[1];
                rgb[i * 3 + 2] = entry[2];
            }
            encodeError = lodepng::encode(imagePath, rgb, static_cast<unsigned>(width),
                static_cast<unsigned>(height), LCT_RGB, 8);
        } else {
            encodeError = lodepng::encode(imagePath, pixels, static_cast<unsigned>(width),
                static_cast<unsigned>(height), LCT_RGBA, 8);
        }
        if (encodeError != 0) {
            printf("owner shot: PNG encode failed: %s\n", lodepng_error_text(encodeError));
            fflush(stdout);
            return;
        }
        FILE* f = fopen(sidecarPath.c_str(), "wb");
        if (f == nullptr) {
            printf("owner shot: JSON write failed: %s\n", sidecarPath.c_str());
            fflush(stdout);
            return;
        }
        fwrite(json.data(), 1, json.size(), f);
        fclose(f);
    });
    worker.detach();

    char message[128];
    snprintf(message, sizeof(message), "shot saved: %s", imageFile.c_str());
    displayMonitorAddMessage(message);
    return true;
}

} // namespace

bool handleKey(int key)
{
    // The owner's shot key. The dev tools' handler runs first: with them on,
    // F9 stays the dev snapshot and never reaches here.
    if (key != KEY_F9) return false;
    std::string failure;
    if (!takeShot(&failure)) {
        char message[160];
        snprintf(message, sizeof(message), "shot failed: %s", failure.c_str());
        displayMonitorAddMessage(message);
        return false;
    }
    return true;
}

void parseOption(const char* arg)
{
    if (strcmp(arg, "--dev-owner-shot-once") == 0) {
        gShotPending = true;
        return;
    }
    if (strncmp(arg, "--dev-owner-shot-once=", 22) == 0) {
        gShotPending = true;
        gShotDelaySeconds = atoi(arg + 22);
        if (gShotDelaySeconds < 0) gShotDelaySeconds = 0;
        if (gShotDelaySeconds > 3600) gShotDelaySeconds = 3600;
    }
}

void pump()
{
    pumpTeleport();
    if (!gShotPending) return;
    // The plain shot waits for the game itself: a map loaded and the hero on
    // it. In the 3D view it waits for the scene's own present too, so the PNG
    // shows the world and not the loading screen.
    if (gDude == nullptr || gMapHeader.name[0] == '\0') return;
    if (scene_renderer::isActive()
        && (!scene::shouldPresent() || scene::activeView() != scene::View::Full)) return;
    if (gShotReadyAt == 0) gShotReadyAt = SDL_GetTicks() + gShotDelaySeconds * 1000;
    if (SDL_GetTicks() < gShotReadyAt) return;
    const Uint64 captureStart = SDL_GetPerformanceCounter();
    if (takeShot(nullptr)) {
        // The readback runs on this thread: its wall time is the cost the key
        // press frame pays (183's bar: under 50 ms). The devperf line puts
        // the capture into the same timeline as the frame windows.
        const double captureMs = static_cast<double>(SDL_GetPerformanceCounter() - captureStart)
            / static_cast<double>(SDL_GetPerformanceFrequency()) * 1000.0;
        char line[128];
        snprintf(line, sizeof(line), "perf owner_shot capture frame_ms=%.2f\n", captureMs);
        scene_renderer::perfLog(line);
        printf("owner shot: capture frame wall_ms=%.2f\n", captureMs);
        fflush(stdout);
        gShotPending = false;
        gShotAttempts = 0;
        return;
    }
    if (++gShotAttempts > 600) {
        gShotPending = false;
        gShotAttempts = 0;
        printf("owner shot: gave up waiting for a ready view\n");
        fflush(stdout);
    }
}

// --- the teleport (step 3) --------------------------------------------------

namespace {

// One jump: travel to the map and elevation, then stand on the named hex or
// on the safe pick. The file lives next to the exe: the owner's game folder.
struct TeleportRequest {
    bool active = false;
    int mapId = -1;
    int elevation = 0;
    int tile = -1; // -1: the safe pick around where the travel lands
    int framesWaited = 0;
};

TeleportRequest gTeleport;
int gTeleportPollCounter = 0;
bool gTeleportPathLogged = false;
bool gTeleportTransientRefused = false;

std::filesystem::path teleportFilePath()
{
    char exePath[COMPAT_MAX_PATH + 1] = {};
#if defined(_WIN32)
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
#else
    if (realpath(".", exePath) == nullptr) exePath[0] = '\0';
#endif
    std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
    if (dir.empty()) dir = std::filesystem::path(".");
    return dir / "dev-teleport.txt";
}

void consumeTeleportFile(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

bool inBlockingState()
{
    if (isInCombat()) return true;
    if (_inDialog != 0) return true;
    if (gameMovieIsPlaying()) return true;
    return false;
}

int resolveMapId(const char* base)
{
    for (int id = 1; id <= 999; id++) {
        char name[64];
        if (wmMapIdxToName(static_cast<Map>(id), name, sizeof(name)) == 0) {
            const char* dot = strchr(name, '.');
            size_t len = dot != nullptr ? static_cast<size_t>(dot - name) : strlen(name);
            if (len >= sizeof(name)) len = sizeof(name) - 1;
            char stripped[64];
            strncpy(stripped, name, len);
            stripped[len] = '\0';
            if (_stricmp(stripped, base) == 0) return id;
        }
    }
    return -1;
}

// The line reads map.elev (the safe pick decides the hex) or map.elev.hex
// (the owner's named hex stands as it is). '#' starts a comment.
bool parseTeleportLine(const char* line)
{
    char work[128] = {};
    size_t used = 0;
    for (const char* p = line; *p != '\0' && used + 1 < sizeof(work); p++) {
        if (*p == '#') break;
        work[used++] = *p == ',' ? '.' : static_cast<char>(*p);
    }
    work[used] = '\0';

    char* parts[3] = {};
    int count = 0;
    char* token = strtok(work, ". \t\r\n");
    while (token != nullptr && count < 3) {
        parts[count++] = token;
        token = strtok(nullptr, ". \t\r\n");
    }
    if (count < 2) return false;

    const int elevation = atoi(parts[1]);
    int tile = -1;
    if (count >= 3) {
        tile = atoi(parts[2]);
        if (tile < 0 || tile >= HEX_GRID_WIDTH * HEX_GRID_HEIGHT) {
            printf("owner teleport: bad hex %s\n", parts[2]);
            fflush(stdout);
            return false;
        }
    }
    return requestTeleport(parts[0], elevation, tile);
}

} // namespace

bool requestTeleport(const char* mapName, int elevation, int tile)
{
    if (gDude == nullptr) {
        printf("owner teleport: refused, no hero\n");
        fflush(stdout);
        return false;
    }
    if (gTeleport.active) {
        printf("owner teleport: held, one is already in flight\n");
        fflush(stdout);
        gTeleportTransientRefused = true;
        return false;
    }
    if (inBlockingState()) {
        // Transient: the caller keeps the file for the next polls instead of
        // eating the owner's word mid-dialog.
        printf("owner teleport: held, combat=%d dialog=%d movie=%d\n", isInCombat() ? 1 : 0,
            _inDialog != 0 ? 1 : 0, gameMovieIsPlaying() ? 1 : 0);
        fflush(stdout);
        gTeleportTransientRefused = true;
        return false;
    }
    const int mapId = resolveMapId(mapName);
    if (mapId == -1) {
        printf("owner teleport: no such map %s\n", mapName);
        fflush(stdout);
        return false;
    }
    if (elevation < 0 || elevation >= ELEVATION_COUNT) {
        printf("owner teleport: bad elevation %d\n", elevation);
        fflush(stdout);
        return false;
    }

    // The travel goes through the dev tools' safe transition (it refuses
    // absent elevations and drives the engine's own map change); this module
    // tracks the arrival itself, outside the dev tools' gated tick.
    if (!devtools::requestMapChange(mapName, elevation)) {
        printf("owner teleport: travel refused for %s e%d\n", mapName, elevation);
        fflush(stdout);
        return false;
    }
    gTeleport.active = true;
    gTeleport.mapId = mapId;
    gTeleport.elevation = elevation;
    gTeleport.tile = tile;
    gTeleport.framesWaited = 0;
    printf("owner teleport: traveling to %s e%d hex %d (map id %d, now on id %d)\n", mapName, elevation, tile,
        mapId, static_cast<int>(gMapHeader.index));
    fflush(stdout);
    return true;
}

void pumpTeleport()
{
    // The command file: polled every half second, one line per file.
    if (!gTeleport.active && ++gTeleportPollCounter >= 30) {
        gTeleportPollCounter = 0;
        const std::filesystem::path path = teleportFilePath();
        std::error_code ec;
        if (std::filesystem::exists(path, ec) && !ec) {
            FILE* f = fopen(path.string().c_str(), "rb");
            if (f != nullptr) {
                char line[256] = {};
                bool consumed = true;
                while (fgets(line, sizeof(line), f) != nullptr) {
                    if (line[0] != '\0' && line[0] != '#' && line[0] != '\r' && line[0] != '\n') {
                        gTeleportTransientRefused = false;
                        const bool accepted = parseTeleportLine(line);
                        // A transient refusal keeps the file: the owner's word
                        // survives until the game can take it.
                        consumed = accepted || !gTeleportTransientRefused;
                        if (!accepted && !gTeleportTransientRefused) {
                            printf("owner teleport: the file held no usable line\n");
                            fflush(stdout);
                        }
                        break;
                    }
                }
                fclose(f);
                if (consumed) consumeTeleportFile(path);
            }
        }
    }

    if (!gTeleport.active) return;

    // Wait for the travel to land: the header carries the target map and the
    // elevation the request asked for.
    if (static_cast<int>(gMapHeader.index) != gTeleport.mapId
        || static_cast<int>(gElevation) != gTeleport.elevation) {
        if (++gTeleport.framesWaited > 3600) {
            printf("owner teleport: gave up waiting for %d e%d (at %s e%d)\n",
                gTeleport.mapId, gTeleport.elevation, gMapHeader.name, static_cast<int>(gElevation));
            fflush(stdout);
            gTeleport.active = false;
        }
        return;
    }
    if (gMapHeader.name[0] == '\0' || gDude == nullptr) return;
    if (inBlockingState()) {
        if (++gTeleport.framesWaited > 3600) {
            printf("owner teleport: gave up, the arrival never left a blocking state\n");
            fflush(stdout);
            gTeleport.active = false;
        }
        return;
    }

    // The hex: the owner's name stands as it is; otherwise the safe pick
    // around where the travel landed, then the tile the engine entered on.
    int tile = gTeleport.tile;
    if (tile < 0) {
        // The travel landed the hero through the engine's entrance pick; the
        // same walk check the floor change uses decides whether to step the
        // hero to a walkable neighbor.
        tile = devtools::findWalkableTileOnElevation(gDude->tile, gTeleport.elevation, 4);
        if (tile == -1 && devtools::elevationTileFree(gDude->tile, gTeleport.elevation)) {
            tile = gDude->tile;
        }
    }
    if (tile >= 0 && tile != gDude->tile) {
        objectSetLocation(gDude, tile, static_cast<MapElevation>(gTeleport.elevation), nullptr);
        // The move is instant: the egg keeps no leftover scroll shift.
        gEgg->x = 0;
        gEgg->y = 0;
        tileSetCenter(gDude->tile, TILE_SET_CENTER_REFRESH_WINDOW);
    }
    // A map change drops the 3D view (task 173's known gap): put the
    // owner's view back, the teleport must not strand the game in 2D.
    if (settings.scene3d.enabled && scene::activeView() == scene::View::Off) {
        std::string viewError;
        scene::enable(scene::View::Full, &viewError);
    }
    char message[128];
    snprintf(message, sizeof(message), "teleported: %s e%d hex %d", gMapHeader.name,
        gTeleport.elevation, gDude->tile);
    displayMonitorAddMessage(message);
    printf("owner teleport: arrived, %s\n", message);
    fflush(stdout);
    gTeleport.active = false;
}

} // namespace owner_shot
} // namespace fallout
