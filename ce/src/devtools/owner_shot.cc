#include "owner_shot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

#include <algorithm>
#include <cstdlib>
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

#include "../color.h"
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
#include "../text_font.h"
#include "../tile.h"
#include "../version.h"
#include "../platform/git_version.h"
#include "../scene/scene_view.h"
#include "../scene/menu_text.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/context.h"
#include "../scene_renderer/game_view.h"
#include "../scene_renderer/perf.h"
#include "../scene_renderer/scene_draw.h"
#include "preferences_probe.h"
#include "snapshot_rows.h"

namespace fallout {
namespace owner_shot {

namespace {

constexpr int kMaxRegionTiles = 256;
constexpr int kMaxRegionObjects = 128;
constexpr int kMaxRegionHexProbe = 20000;

// The pending --dev-owner-shot-once shot, how test runs capture acceptance
// shots without a key press. "=N" delays it N seconds past the ready view,
// so a run settles into steady frames before the capture (183's frame-time
// bar reads off the window that contains the shot). A failed attempt retries
// on the next frames and gives up after about ten seconds of presents.
bool gShotPending = false;
int gShotAttempts = 0;
Uint32 gShotReadyAt = 0;
int gShotDelaySeconds = 0;

// The automated mark (how test runs capture the acceptance shots without
// input): enter the mark mode after the view settles, drop the fixed
// rectangles, save. "--dev-owner-mark-once" drops one, "twice" two.
bool gMarkPending = false;
int gMarkPendingRects = 0;
int gMarkAttempts = 0;
Uint32 gMarkReadyAt = 0;
int gMarkDelaySeconds = 0;

// 218: the drag bench (--dev-owner-mark-drag=N). The 217 bench pushed its
// rectangles straight into the list, so the real input path - where the
// owner's drag actually broke - was never exercised. This one replays a drag
// through the SDL queue and asserts the rectangle registered and the green
// outline reached the presented pixels.
bool gMarkDragPending = false;
int gMarkDragDelaySeconds = 0;
Uint32 gMarkDragReadyAt = 0;
int gMarkDragAttempts = 0;
int gMarkDragState = 0;
int gMarkDragFrame = 0;
int gMarkDragGreenBaseline = -1;
bool gMarkDragPixelVerified = false;

// The region mark (step 2): the game holds on the frame the entry captured,
// the owner drags rectangles, Enter or a second press saves the clean PNG,
// the marked PNG and the JSON with the regions, Esc cancels.
bool gMarkMode = false;
std::vector<unsigned char> gMarkClean; // top-down RGBA of the frozen frame
int gMarkWidth = 0;
int gMarkHeight = 0;
char gMarkStem[96] = {}; // file name stem reserved at entry
struct MarkRect {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
};
std::vector<MarkRect> gMarkRects;
MarkRect gMarkDrag;
MarkRect gMarkDragPlanned; // 218: the drag bench's rectangle, before the drag
bool gMarkDragging = false;
int gMarkMouseX = 0;
int gMarkMouseY = 0;

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

// 217: the game's strings arrive in the game's own encoding (cp1251 on a
// Russian install); the JSON is UTF-8, so convert instead of keeping raw
// bytes that read as mojibake ("╓хэЄЁ уюЁюфр" for "Центр города").
void cp1251ToUtf8(const char* in, std::string& out)
{
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(in); *p != 0; p++) {
        unsigned int code = *p;
        if (code >= 0xC0 && code <= 0xFF) {
            code = 0x410 + (code - 0xC0); // А-я
        } else if (code == 0xA8) {
            code = 0x401; // Ё
        } else if (code == 0xB8) {
            code = 0x451; // ё
        } else if (code == 0x96) {
            code = 0x2013; // –
        } else if (code == 0x97) {
            code = 0x2014; // —
        }
        if (code < 0x80) {
            out += static_cast<char>(code);
        } else if (code < 0x800) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }
}

std::string protoNameOf(const Object* object)
{
    std::string name;
    char* text = protoGetName(object);
    if (text != nullptr && text[0] != '\0') {
        std::string utf8;
        cp1251ToUtf8(text, utf8);
        jsonEscape(utf8.c_str(), name);
    } else {
        name = "unnamed";
    }
    return name;
}

std::string jsonText(const char* text)
{
    std::string out;
    if (text != nullptr && text[0] != '\0') {
        std::string utf8;
        cp1251ToUtf8(text, utf8);
        jsonEscape(utf8.c_str(), out);
        return out;
    }
    return "unknown";
}

// The readback stays on the frame's thread: the GL context belongs to it.
// Returns the top-down RGBA rows of the frame as the game presents it (the
// 3D scene with the game's picture over it), or an indexed 2D frame with
// its palette.
bool readFrame(std::vector<unsigned char>& pixels, std::vector<unsigned char>& indexed,
    std::vector<unsigned char>& palette, int* width, int* height, std::string* failure)
{
    const bool sceneLive = scene_renderer::isActive() && scene::shouldPresent()
        && scene::activeView() == scene::View::Full;
    if (sceneLive) {
        return scene_renderer::captureStaticPreviewBuffer(true, true, true, pixels, width, height, failure);
    }
    if (scene_renderer::isActive()) {
        SDL_BlitSurface(gSdlSurface, nullptr, gSdlTextureSurface, nullptr);
        return scene_renderer::captureFrameBuffer(static_cast<const unsigned char*>(gSdlTextureSurface->pixels),
            gSdlTextureSurface->w, gSdlTextureSurface->h, gSdlTextureSurface->pitch,
            pixels, width, height, failure);
    }
    if (gSdlSurface == nullptr || gSdlSurface->pixels == nullptr) {
        if (failure != nullptr) *failure = "no framebuffer";
        return false;
    }
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
    *width = gSdlSurface->w;
    *height = gSdlSurface->h;
    return true;
}

// The JSON fields a plain shot and a region mark share, read on the frame
// that takes them.
struct ShotContext {
    std::string fpsJson;
    std::string mapJson;
    std::string mapFileJson;
    std::string cityJson;
    int elevation = 0;
    std::string playerJson;
    std::string cameraJson;
    std::string cursorJson;
    std::string resolutionJson;
    std::string renderJson;
    std::string versionJson;
};

void assembleContext(ShotContext& out)
{
    // The frame rate over the last few seconds, always on (183).
    double fps = 0.0;
    double averageMs = 0.0;
    double low1Ms = 0.0;
    if (scene_renderer::perfFrameStats(&fps, &averageMs, &low1Ms)) {
        char piece[192];
        snprintf(piece, sizeof(piece),
            "{\"fps\":%.1f,\"frame_avg_ms\":%.2f,\"frame_low1_ms\":%.2f,\"window_frames\":%d}",
            fps, averageMs, low1Ms, 300);
        out.fpsJson = piece;
    } else {
        out.fpsJson = "{\"window_frames\":300}";
    }

    out.elevation = gDude != nullptr ? static_cast<int>(gDude->elevation) : 0;
    out.mapFileJson = gMapHeader.name[0] != '\0' ? jsonText(gMapHeader.name) : std::string("none");
    out.mapJson = "unknown";
    out.cityJson = "unknown";
    const Map currentMap = mapGetCurrentMap();
    if (currentMap >= 0) {
        out.mapJson = jsonText(mapGetName(currentMap, static_cast<MapElevation>(out.elevation)));
        out.cityJson = jsonText(mapGetCityName(currentMap));
    }

    // The player: tile, hex and the world position under it.
    out.playerJson = "null";
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
                tile, tile % 200, tile / 200, worldX, worldZ, out.elevation, static_cast<int>(gDude->rotation));
        } else {
            snprintf(piece, sizeof(piece),
                "{\"tile\":%d,\"hex\":[%d,%d],\"world\":null,\"elevation\":%d,\"rotation\":%d}",
                tile, tile % 200, tile / 200, out.elevation, static_cast<int>(gDude->rotation));
        }
        out.playerJson = piece;
    }

    // The camera as the view looks from, and the game's own turn and zoom.
    {
        scene_renderer::StaticFlyCamera fly;
        char piece[512];
        if (scene_renderer::staticGameViewCamera(&fly)) {
            snprintf(piece, sizeof(piece),
                "{\"eye\":[%.3f,%.3f,%.3f],\"yaw_deg\":%.2f,\"fov_y_deg\":%.2f,\"camera_turn\":%.4f,\"zoom\":%.4f,\"center_tile\":%d}",
                fly.eye[0], fly.eye[1], fly.eye[2],
                static_cast<double>(fly.yaw) * 180.0 / 3.14159265358979323846,
                static_cast<double>(fly.fovY), scene::cameraTurnAngle(), settings.scene3d.zoom, gCenterTile);
        } else {
            snprintf(piece, sizeof(piece),
                "{\"camera_turn\":%.4f,\"zoom\":%.4f,\"center_tile\":%d}",
                scene::cameraTurnAngle(), settings.scene3d.zoom, gCenterTile);
        }
        out.cameraJson = piece;
    }

    // The object under the cursor, as the game's own pick reads the hex.
    out.cursorJson = "null";
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
                out.cursorJson = std::string("{\"tile\":") + std::to_string(tile) + ",\"object\":" + piece + "}";
                break;
            }
        }
    }

    int capturedWidth = gSdlSurface != nullptr ? gSdlSurface->w : 0;
    int capturedHeight = gSdlSurface != nullptr ? gSdlSurface->h : 0;
    {
        char piece[256];
        // 217: name the actual window mode - the old boolean read "false"
        // for borderless, which hid the owner's fullscreen reports.
        const char* windowMode = "windowed";
        if (settings.screen.windowed == WindowMode::Fullscreen) windowMode = "fullscreen";
        else if (settings.screen.windowed == WindowMode::WindowedFullscreen) windowMode = "borderless";
        snprintf(piece, sizeof(piece),
            "\"resolution\":\"%dx%d\",\"logical_resolution\":\"%dx%d\",\"scale\":%d,\"scale_factor\":%.1f,\"window_mode\":\"%s\"",
            capturedWidth, capturedHeight, screenGetWidth(), screenGetHeight(), settings.screen.scale,
            settings.screen.scale_factor, windowMode);
        out.resolutionJson = piece;
    }

    {
        const Scene3dSettings& scene3d = settings.scene3d;
        char piece[1024];
        snprintf(piece, sizeof(piece),
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
        out.renderJson = piece;
    }

    char version[VERSION_MAX] = {};
    versionGetVersion(version, sizeof(version));
    out.versionJson = jsonText(version);
}

// The PNG encode and the file writes: the worker thread's whole job (183).
void writeShotFiles(const std::string& imagePath, const std::string& sidecarPath,
    std::vector<unsigned char>&& pixels, std::vector<unsigned char>&& indexed,
    std::vector<unsigned char>&& palette, int width, int height, const std::string& json)
{
    std::thread worker([imagePath, sidecarPath, pixels = std::move(pixels),
        indexed = std::move(indexed), palette = std::move(palette), width, height,
        json = std::move(json)]() {
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
}

bool takeShot()
{
    namespace fs = std::filesystem;

    char stamp[32] = {};
    {
        std::time_t t = time(nullptr);
        struct tm* tmv = localtime(&t);
        if (tmv == nullptr || strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", tmv) == 0) {
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
    if (!folderError.empty()) return false;

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
    if (imagePath.empty()) return false;

    std::vector<unsigned char> pixels;
    std::vector<unsigned char> indexed;
    std::vector<unsigned char> palette;
    int width = 0;
    int height = 0;
    std::string captureError;
    if (!readFrame(pixels, indexed, palette, &width, &height, &captureError)) {
        return false;
    }

    ShotContext context;
    assembleContext(context);

    std::string json;
    json.reserve(3072);
    json += "{\n";
    json += "  \"image\": \"" + imageFile + "\",\n";
    json += "  \"fps\": " + context.fpsJson + ",\n";
    json += "  \"map\": \"" + context.mapJson + "\",\n";
    json += "  \"map_file\": \"" + context.mapFileJson + "\",\n";
    json += "  \"city\": \"" + context.cityJson + "\",\n";
    json += "  \"elevation\": " + std::to_string(context.elevation) + ",\n";
    json += "  \"player\": " + context.playerJson + ",\n";
    json += "  \"camera\": " + context.cameraJson + ",\n";
    json += "  \"cursor\": " + context.cursorJson + ",\n";
    json += "  " + context.resolutionJson + ",\n";
    json += "  \"render\": " + context.renderJson + ",\n";
    json += "  \"build\": {\"commit\":\"" + std::string(_BUILD_HASH) + "\",\"version\":\"" + context.versionJson
        + "\",\"taken\":\"" + stamp + "\"}\n";
    json += "}\n";

    writeShotFiles(imagePath.string(), sidecarPath.string(), std::move(pixels), std::move(indexed),
        std::move(palette), width, height, std::move(json));

    char message[128];
    snprintf(message, sizeof(message), "shot saved: %s", imageFile.c_str());
    displayMonitorAddMessage(scene::menuStatusText(message, settings.system.language).c_str());
    return true;
}

// The regions under the dragged rectangles, read off the frozen camera.
std::string buildRegionsJson()
{
    std::string out = "null";
    if (gMarkRects.empty() || gDude == nullptr) return out;

    scene_renderer::GameView view;
    if (!scene_renderer::gameViewAtCenterTile(gDude->tile, &view)) return out;

    // The hex grid in world units: centers at p00 + gx*du + gy*dv, probed
    // off the view itself (tiles 0, 1 and 200 step one hex along the axes).
    double p00[2] = { 0.0, 0.0 };
    double du[2] = { 0.0, 0.0 };
    double dv[2] = { 0.0, 0.0 };
    {
        double wx = 0.0;
        double wz = 0.0;
        scene_renderer::GameView probe;
        if (!scene_renderer::gameViewAtCenterTile(0, &probe)
            || !scene_renderer::gameViewCenter(probe, &wx, &wz)) return out;
        p00[0] = wx;
        p00[1] = wz;
        if (!scene_renderer::gameViewAtCenterTile(1, &probe)
            || !scene_renderer::gameViewCenter(probe, &wx, &wz)) return out;
        du[0] = wx - p00[0];
        du[1] = wz - p00[1];
        if (!scene_renderer::gameViewAtCenterTile(200, &probe)
            || !scene_renderer::gameViewCenter(probe, &wx, &wz)) return out;
        dv[0] = wx - p00[0];
        dv[1] = wz - p00[1];
    }
    const double determinant = du[0] * dv[1] - du[1] * dv[0];
    if (std::abs(determinant) < 1e-9) return out;
    const double scale = settings.screen.scale_factor > 0.0 ? settings.screen.scale_factor : 1.0;

    out = "[\n";
    bool firstRegion = true;
    for (const MarkRect& rect : gMarkRects) {
        const int rx0 = std::min(rect.x0, rect.x1);
        const int ry0 = std::min(rect.y0, rect.y1);
        const int rx1 = std::max(rect.x0, rect.x1);
        const int ry1 = std::max(rect.y0, rect.y1);
        if (rx1 <= rx0 || ry1 <= ry0) continue;

        // The world rect the two far corners see on the ground plane.
        double worldMin[2] = { 0.0, 0.0 };
        double worldMax[2] = { 0.0, 0.0 };
        bool haveWorld = true;
        for (int c = 0; c < 2 && haveWorld; c++) {
            const int px = c == 0 ? rx0 : rx1;
            const int py = c == 0 ? ry0 : ry1;
            double wx = 0.0;
            double wz = 0.0;
            if (!scene_renderer::gameViewPickGround(view, static_cast<double>(px) / scale,
                    static_cast<double>(py) / scale, &wx, &wz)) {
                haveWorld = false;
                break;
            }
            if (c == 0) {
                worldMin[0] = worldMax[0] = wx;
                worldMin[1] = worldMax[1] = wz;
            } else {
                worldMin[0] = std::min(worldMin[0], wx);
                worldMax[0] = std::max(worldMax[0], wx);
                worldMin[1] = std::min(worldMin[1], wz);
                worldMax[1] = std::max(worldMax[1], wz);
            }
        }
        if (!haveWorld) continue;

        // The hexes whose centers project into the rectangle.
        std::vector<int> tiles;
        std::vector<int> squares;
        std::vector<std::string> objectPieces;
        const auto hexAt = [&](double wx, double wz, int* gx, int* gy) {
            const double rx = wx - p00[0];
            const double rz = wz - p00[1];
            *gx = static_cast<int>((rx * dv[1] - rz * dv[0]) / determinant);
            *gy = static_cast<int>((rz * du[0] - rx * du[1]) / determinant);
        };
        int gx0 = 0;
        int gy0 = 0;
        int gx1 = 0;
        int gy1 = 0;
        hexAt(worldMin[0], worldMin[1], &gx0, &gy0);
        hexAt(worldMax[0], worldMax[1], &gx1, &gy1);
        if (gx0 > gx1) std::swap(gx0, gx1);
        if (gy0 > gy1) std::swap(gy0, gy1);
        int probes = 0;
        for (int gy = gy0 - 1; gy <= gy1 + 1 && probes < kMaxRegionHexProbe; gy++) {
            for (int gx = gx0 - 1; gx <= gx1 + 1 && probes < kMaxRegionHexProbe; gx++) {
                probes++;
                if (gx < 0 || gx >= 200 || gy < 0 || gy >= 200) continue;
                const double wx = p00[0] + gx * du[0] + gy * dv[0];
                const double wz = p00[1] + gx * du[1] + gy * dv[1];
                double lx = 0.0;
                double ly = 0.0;
                if (!scene_renderer::gameViewProject(view, wx, 0.0, wz, &lx, &ly)) continue;
                const int px = static_cast<int>(lx * scale);
                const int py = static_cast<int>(ly * scale);
                if (px < rx0 || px > rx1 || py < ry0 || py > ry1) continue;
                const int tile = gy * 200 + gx;
                if (std::find(tiles.begin(), tiles.end(), tile) == tiles.end()) {
                    if (tiles.size() < kMaxRegionTiles) tiles.push_back(tile);
                }
                const int square = squareTileFromTile(tile);
                if (square >= 0 && std::find(squares.begin(), squares.end(), square) == squares.end()
                    && squares.size() < kMaxRegionTiles) {
                    squares.push_back(square);
                }
                if (objectPieces.size() < kMaxRegionObjects) {
                    for (Object* object = objectFindFirstAtLocation(gDude->elevation, tile);
                        object != nullptr; object = objectFindNextAtLocation()) {
                        if (((object->fid >> 24) & 0xF) == OBJ_TYPE_TILE) continue;
                        if (object->pid == -1) continue;
                        char piece[384];
                        snprintf(piece, sizeof(piece),
                            "{\"pid\":%d,\"name\":\"%s\",\"type\":%d,\"tile\":%d}",
                            object->pid, protoNameOf(object).c_str(),
                            static_cast<int>((object->fid >> 24) & 0xF), object->tile);
                        std::string key = piece;
                        if (std::find(objectPieces.begin(), objectPieces.end(), key) == objectPieces.end()
                            && objectPieces.size() < kMaxRegionObjects) {
                            objectPieces.push_back(key);
                        }
                    }
                }
            }
        }

        if (!firstRegion) out += ",\n";
        firstRegion = false;
        out += "    {\"rect\":[" + std::to_string(rx0) + "," + std::to_string(ry0) + ","
            + std::to_string(rx1 - rx0) + "," + std::to_string(ry1 - ry0) + "],\n";
        out += "     \"tiles\":[";
        for (size_t i = 0; i < tiles.size(); i++) {
            out += std::to_string(tiles[i]);
            if (i + 1 < tiles.size()) out += ",";
        }
        out += tiles.size() >= kMaxRegionTiles ? "],\"tiles_truncated\":true,\n" : "],\n";
        out += "     \"squares\":[";
        for (size_t i = 0; i < squares.size(); i++) {
            out += std::to_string(squares[i]);
            if (i + 1 < squares.size()) out += ",";
        }
        out += "],\n";
        out += "     \"objects\":[";
        for (size_t i = 0; i < objectPieces.size(); i++) {
            out += objectPieces[i];
            if (i + 1 < objectPieces.size()) out += ",";
        }
        out += objectPieces.size() >= kMaxRegionObjects ? "],\"objects_truncated\":true}\n" : "]}\n";
    }
    out += "  ]";
    return out;
}

// The red outlines on the clean RGBA rows: what the marked PNG shows.
void drawMarkOnPixels(std::vector<unsigned char>& pixels, int width, int height)
{
    const auto setRed = [&](int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        unsigned char* pixel = pixels.data() + (static_cast<size_t>(y) * width + x) * 4;
        pixel[0] = 255;
        pixel[1] = 0;
        pixel[2] = 0;
        pixel[3] = 255;
    };
    for (const MarkRect& rect : gMarkRects) {
        const int x0 = std::max(0, std::min(rect.x0, rect.x1));
        const int y0 = std::max(0, std::min(rect.y0, rect.y1));
        const int x1 = std::min(width - 1, std::max(rect.x0, rect.x1));
        const int y1 = std::min(height - 1, std::max(rect.y0, rect.y1));
        if (x1 < x0 || y1 < y0) continue;
        for (int t = 0; t < 2; t++) {
            for (int x = x0 + t; x <= x1 - t; x++) {
                setRed(x, y0 + t);
                setRed(x, y1 - t);
            }
            for (int y = y0 + t; y <= y1 - t; y++) {
                setRed(x0 + t, y);
                setRed(x1 - t, y);
            }
        }
    }
}

bool saveMark()
{
    namespace fs = std::filesystem;

    if (gMarkClean.empty() || gMarkStem[0] == '\0') return false;

    char stamp[32] = {};
    {
        std::time_t t = time(nullptr);
        struct tm* tmv = localtime(&t);
        if (tmv == nullptr || strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", tmv) == 0) return false;
    }
    char dateStamp[11];
    snprintf(dateStamp, sizeof(dateStamp), "%.4s-%.2s-%.2s", stamp, stamp + 4, stamp + 6);
    fs::path directory;
    try {
        directory = dev_output::outputDirectory("shots/dev", dateStamp);
    } catch (const std::exception&) {
        return false;
    }
    if (directory.empty()) return false;

    fs::path cleanPath = directory / (std::string(gMarkStem) + ".png");
    fs::path markedPath = directory / (std::string(gMarkStem) + "-marked.png");
    fs::path sidecarPath = directory / (std::string(gMarkStem) + ".json");

    ShotContext context;
    assembleContext(context);
    const std::string regionsJson = buildRegionsJson();

    std::string json;
    json.reserve(4096);
    json += "{\n";
    json += "  \"image\": \"" + std::string(gMarkStem) + ".png\",\n";
    json += "  \"marked_image\": \"" + std::string(gMarkStem) + "-marked.png\",\n";
    json += "  \"fps\": " + context.fpsJson + ",\n";
    json += "  \"map\": \"" + context.mapJson + "\",\n";
    json += "  \"map_file\": \"" + context.mapFileJson + "\",\n";
    json += "  \"city\": \"" + context.cityJson + "\",\n";
    json += "  \"elevation\": " + std::to_string(context.elevation) + ",\n";
    json += "  \"player\": " + context.playerJson + ",\n";
    json += "  \"camera\": " + context.cameraJson + ",\n";
    json += "  \"cursor\": " + context.cursorJson + ",\n";
    json += "  " + context.resolutionJson + ",\n";
    json += "  \"render\": " + context.renderJson + ",\n";
    json += "  \"regions\": " + regionsJson + ",\n";
    json += "  \"build\": {\"commit\":\"" + std::string(_BUILD_HASH) + "\",\"version\":\"" + context.versionJson
        + "\",\"taken\":\"" + stamp + "\"}\n";
    json += "}\n";

    std::vector<unsigned char> marked = gMarkClean;
    drawMarkOnPixels(marked, gMarkWidth, gMarkHeight);

    std::vector<unsigned char> clean = gMarkClean;
    writeShotFiles(cleanPath.string(), sidecarPath.string(), std::move(clean), {}, {}, gMarkWidth,
        gMarkHeight, std::move(json));
    // The marked PNG has no sidecar of its own: encode it straight to file.
    std::thread markedWorker([markedPath = markedPath.string(), marked = std::move(marked),
        width = gMarkWidth, height = gMarkHeight]() {
        const unsigned encodeError = lodepng::encode(markedPath, marked, static_cast<unsigned>(width),
            static_cast<unsigned>(height), LCT_RGBA, 8);
        if (encodeError != 0) {
            printf("owner shot: marked PNG encode failed: %s\n", lodepng_error_text(encodeError));
            fflush(stdout);
        }
    });
    markedWorker.detach();

    char message[160];
    snprintf(message, sizeof(message), "mark saved: %s-marked.png", gMarkStem);
    displayMonitorAddMessage(scene::menuStatusText(message, settings.system.language).c_str());
    return true;
}

void enterMarkMode()
{
    if (gMarkMode) return;
    if (!(scene_renderer::isActive() && scene::shouldPresent()
            && scene::activeView() == scene::View::Full)) {
        displayMonitorAddMessage(scene::menuStatusText("mark failed: the 3D view is not up", settings.system.language).c_str());
        return;
    }

    char stamp[32] = {};
    {
        std::time_t t = time(nullptr);
        struct tm* tmv = localtime(&t);
        if (tmv == nullptr || strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", tmv) == 0) {
            displayMonitorAddMessage(scene::menuStatusText("mark failed: clock unavailable", settings.system.language).c_str());
            return;
        }
    }
    char dateStamp[11];
    snprintf(dateStamp, sizeof(dateStamp), "%.4s-%.2s-%.2s", stamp, stamp + 4, stamp + 6);
    std::filesystem::path directory;
    try {
        directory = dev_output::outputDirectory("shots/dev", dateStamp);
    } catch (const std::exception&) {
        directory.clear();
    }
    if (directory.empty()) {
        displayMonitorAddMessage(scene::menuStatusText("mark failed: no output folder", settings.system.language).c_str());
        return;
    }
    char mapBase[32] = "map";
    if (gMapHeader.name[0] != '\0') baseMapName(gMapHeader.name, mapBase, sizeof(mapBase));
    const int elevation = gDude != nullptr ? static_cast<int>(gDude->elevation) : 0;

    gMarkStem[0] = '\0';
    for (int suffix = 0; suffix < 1000 && gMarkStem[0] == '\0'; suffix++) {
        char stem[96];
        if (suffix == 0) snprintf(stem, sizeof(stem), "%s-%s-e%d", stamp, mapBase, elevation);
        else snprintf(stem, sizeof(stem), "%s-%s-e%d-%03d", stamp, mapBase, elevation, suffix);
        std::error_code ec;
        const std::string base = stem;
        if (!std::filesystem::exists(directory / (base + ".png"), ec)
            && !std::filesystem::exists(directory / (base + "-marked.png"), ec)
            && !std::filesystem::exists(directory / (base + ".json"), ec)) {
            snprintf(gMarkStem, sizeof(gMarkStem), "%s", stem);
        }
    }
    if (gMarkStem[0] == '\0') {
        displayMonitorAddMessage(scene::menuStatusText("mark failed: no free file name", settings.system.language).c_str());
        return;
    }

    std::vector<unsigned char> pixels;
    std::vector<unsigned char> indexed;
    std::vector<unsigned char> palette;
    std::string failure;
    if (!readFrame(pixels, indexed, palette, &gMarkWidth, &gMarkHeight, &failure)) {
        displayMonitorAddMessage(scene::menuStatusText("mark failed: capture unavailable", settings.system.language).c_str());
        return;
    }
    gMarkClean = std::move(pixels);
    gMarkRects.clear();
    gMarkDragging = false;
    // No monitor message here: the game floats the last monitor message over
    // the map view and the next frames' pictures, and the clean capture must
    // stay clean of it. The hint lives in draw().
    gMarkMode = true;
}

void exitMarkMode()
{
    gMarkMode = false;
    gMarkClean.clear();
    gMarkClean.shrink_to_fit();
    gMarkRects.clear();
    gMarkDragging = false;
}

// The scripted input's fake mouse device (preferences_probe.h): its events
// feed the game's mouse through the same device layer a real mouse uses,
// without touching the owner's cursor.
void pushScriptedMouse(Uint32 type, int x, int y)
{
    SDL_Event event = {};
    event.type = type;
    if (type == SDL_MOUSEMOTION) {
        event.motion.which = devtools::kScriptMouse;
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.which = devtools::kScriptMouse;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
        event.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
    }
    SDL_PushEvent(&event);
}

void pushScriptedReturnKey()
{
    SDL_Event down = {};
    down.type = SDL_KEYDOWN;
    down.key.keysym.scancode = SDL_SCANCODE_RETURN;
    down.key.state = SDL_PRESSED;
    SDL_Event up = down;
    up.type = SDL_KEYUP;
    up.key.state = SDL_RELEASED;
    SDL_PushEvent(&down);
    SDL_PushEvent(&up);
}

void handleMarkMouse()
{
    int x = 0;
    int y = 0;
    mouseGetPosition(&x, &y);
    gMarkMouseX = x;
    gMarkMouseY = y;
    const int mouseEvent = mouseGetEvent();
    if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0 && !gMarkDragging) {
        gMarkDragging = true;
        gMarkDrag.x0 = x;
        gMarkDrag.y0 = y;
        gMarkDrag.x1 = x;
        gMarkDrag.y1 = y;
    } else if (gMarkDragging) {
        gMarkDrag.x1 = x;
        gMarkDrag.y1 = y;
        if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
            gMarkDragging = false;
            // A click without a drag adds nothing.
            if (std::abs(gMarkDrag.x1 - gMarkDrag.x0) > 2 && std::abs(gMarkDrag.y1 - gMarkDrag.y0) > 2) {
                gMarkRects.push_back(gMarkDrag);
            }
        }
    }
}

} // namespace

bool handleKey(int key)
{
    // The owner's shot key. The dev tools' handler runs first: with them on,
    // F9 stays the dev snapshot and never reaches here.
    if (gMarkMode) {
        // The mark holds the whole input: the game gets nothing.
        //
        // 218: inputGetInput answers -2, not -1, on every frame a mouse
        // button goes down, repeats or comes up - it checks the same flags
        // itself. The mark only ever watched -1, so a real drag could not
        // register; the pump above pushed its rectangles past the input and
        // never noticed.
        if (key == -1 || key == -2) {
            handleMarkMouse();
            return true;
        }
        if (key == KEY_RETURN || key == KEY_F9 || key == KEY_CTRL_F9 || key == KEY_ALT_F9
            || key == KEY_F12 || key == KEY_CTRL_F12) {
            std::string failure;
            if (!saveMark()) {
                displayMonitorAddMessage(scene::menuStatusText("mark failed: save failed", settings.system.language).c_str());
            }
            exitMarkMode();
            return true;
        }
        if (key == KEY_ESCAPE) {
            exitMarkMode();
            displayMonitorAddMessage(scene::menuStatusText("mark cancelled", settings.system.language).c_str());
            return true;
        }
        return true;
    }
    if (key == KEY_CTRL_F9 || key == KEY_ALT_F9 || key == KEY_CTRL_F12) {
        enterMarkMode();
        return true;
    }
    if (key != KEY_F9 && key != KEY_F12) return false;
    std::string failure;
    if (!takeShot()) {
        displayMonitorAddMessage(scene::menuStatusText("shot failed", settings.system.language).c_str());
        return false;
    }
    return true;
}

void draw(unsigned char* surface, int width, int height, int pitch)
{
    if (!gMarkMode || surface == nullptr) return;
    // The rectangles the owner drew, the drag in progress, and the hint.
    const auto outline = [&](int x0, int y0, int x1, int y1) {
        x0 = std::max(0, std::min(x0, width - 1));
        x1 = std::max(0, std::min(x1, width - 1));
        y0 = std::max(0, std::min(y0, height - 1));
        y1 = std::max(0, std::min(y1, height - 1));
        if (x1 < x0 || y1 < y0) return;
        for (int x = x0; x <= x1; x++) {
            surface[pitch * y0 + x] = COLOR_GREEN;
            surface[pitch * y1 + x] = COLOR_GREEN;
        }
        for (int y = y0; y <= y1; y++) {
            surface[pitch * y + x0] = COLOR_GREEN;
            surface[pitch * y + x1] = COLOR_GREEN;
        }
    };
    for (const MarkRect& rect : gMarkRects) {
        outline(std::min(rect.x0, rect.x1), std::min(rect.y0, rect.y1),
            std::max(rect.x0, rect.x1), std::max(rect.y0, rect.y1));
    }
    if (gMarkDragging) {
        outline(std::min(gMarkDrag.x0, gMarkDrag.x1), std::min(gMarkDrag.y0, gMarkDrag.y1),
            std::max(gMarkDrag.x0, gMarkDrag.x1), std::max(gMarkDrag.y0, gMarkDrag.y1));
    }
    ScopedFont scopedFont(101);
    const int lineHeight = fontGetLineHeight();
    // Top-left, not the bottom corner: the game's own monitor messages share
    // the bottom-left corner and the two texts landed on top of each other.
    fontDrawText(surface + pitch * 8 + 8,
        scene::menuText("MARK: drag rectangles, Enter saves, Esc cancels", settings.system.language),
        width - 16, pitch, COLOR_GREEN);
}

// 217: the sharp-text present path rebuilds the screen from captured text
// runs only, so the raw outline pixels drawn into the indexed surface never
// reach a 3D frame - the owner dragged rectangles and saw nothing. This
// re-draws the same rectangles as RGBA patches straight over the presented
// scene, the way the fly-camera FPS readout does.
void drawSceneOverlay()
{
    if (!gMarkMode || gSdlSurface == nullptr) return;
    if (gMarkRects.empty() && !gMarkDragging) return;
    const int width = gSdlSurface->w;
    const int height = gSdlSurface->h;
    const auto outlinePatch = [&](const MarkRect& rect) {
        int x0 = std::max(0, std::min(std::min(rect.x0, rect.x1), width - 2));
        int x1 = std::max(0, std::min(std::max(rect.x0, rect.x1), width - 1));
        int y0 = std::max(0, std::min(std::min(rect.y0, rect.y1), height - 2));
        int y1 = std::max(0, std::min(std::max(rect.y0, rect.y1), height - 1));
        if (x1 < x0 || y1 < y0) return;
        const int w = x1 - x0 + 1;
        const int h = y1 - y0 + 1;
        std::vector<unsigned char> rgba(static_cast<size_t>(w) * h * 4, 0);
        const auto put = [&rgba, w, h](int x, int y) {
            unsigned char* pixel = rgba.data() + (static_cast<size_t>(y) * w + x) * 4;
            pixel[0] = 0;
            pixel[1] = 255;
            pixel[2] = 0;
            pixel[3] = 255;
        };
        for (int x = 0; x < w; x++) {
            put(x, 0);
            put(x, h - 1);
            if (h > 1) put(x, 1);
            if (h > 2) put(x, h - 2);
        }
        for (int y = 0; y < h; y++) {
            put(0, y);
            put(w - 1, y);
            if (w > 1) put(1, y);
            if (w > 2) put(w - 2, y);
        }
        scene_renderer::drawOverlay(rgba.data(), w, h, w * 4, x0, y0, w, h);
    };
    for (const MarkRect& rect : gMarkRects) {
        outlinePatch(rect);
    }
    if (gMarkDragging) {
        outlinePatch(gMarkDrag);
    }
}

bool markMode()
{
    return gMarkMode;
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
        return;
    }
    if (strncmp(arg, "--dev-owner-mark-once=", 22) == 0) {
        gMarkPending = true;
        gMarkPendingRects = 1;
        gMarkDelaySeconds = atoi(arg + 22);
        if (gMarkDelaySeconds < 0) gMarkDelaySeconds = 0;
        if (gMarkDelaySeconds > 3600) gMarkDelaySeconds = 3600;
        return;
    }
    if (strncmp(arg, "--dev-owner-mark-twice=", 23) == 0) {
        gMarkPending = true;
        gMarkPendingRects = 2;
        gMarkDelaySeconds = atoi(arg + 23);
        if (gMarkDelaySeconds < 0) gMarkDelaySeconds = 0;
        if (gMarkDelaySeconds > 3600) gMarkDelaySeconds = 3600;
        return;
    }
    if (strncmp(arg, "--dev-owner-mark-drag=", 22) == 0) {
        gMarkDragPending = true;
        gMarkDragDelaySeconds = atoi(arg + 22);
        if (gMarkDragDelaySeconds < 0) gMarkDragDelaySeconds = 0;
        if (gMarkDragDelaySeconds > 3600) gMarkDragDelaySeconds = 3600;
    }
}

void pumpMark()
{
    if (!gMarkPending) return;
    // The same readiness gates as the plain shot.
    if (gDude == nullptr || gMapHeader.name[0] == '\0') return;
    if (!(scene_renderer::isActive() && scene::shouldPresent()
            && scene::activeView() == scene::View::Full)) return;
    if (gMarkReadyAt == 0) gMarkReadyAt = SDL_GetTicks() + gMarkDelaySeconds * 1000;
    if (SDL_GetTicks() < gMarkReadyAt) return;

    enterMarkMode();
    if (!gMarkMode) {
        if (++gMarkAttempts > 600) {
            gMarkPending = false;
            gMarkAttempts = 0;
            printf("owner shot: mark gave up waiting for a ready view\n");
            fflush(stdout);
        }
        return;
    }
    // The rectangles stand where the view's own quarters are, so a run lands
    // them over live world pieces whatever the resolution is.
    const int w = gMarkWidth;
    const int h = gMarkHeight;
    MarkRect first;
    first.x0 = w / 3;
    first.y0 = h / 3;
    first.x1 = w * 2 / 3;
    first.y1 = h * 2 / 3;
    gMarkRects.push_back(first);
    if (gMarkPendingRects >= 2) {
        MarkRect second;
        second.x0 = w / 10;
        second.y0 = h * 3 / 5;
        second.x1 = w * 3 / 10;
        second.y1 = h * 9 / 10;
        gMarkRects.push_back(second);
    }
    const bool saved = saveMark();
    printf("owner shot: mark %s (%d rectangles, %s)\n", saved ? "saved" : "failed",
        static_cast<int>(gMarkRects.size()), gMarkStem);
    fflush(stdout);
    exitMarkMode();
    gMarkPending = false;
    gMarkAttempts = 0;
}

// 218: one drag replayed through the real input path - scripted mouse events
// in the SDL queue, so the flags cross inputGetInput exactly as the owner's
// drag does - then Enter to save. The printed verdict is the bench's
// assertion: the rectangle registered, and the green outline is in the
// presented pixels.
void pumpDrag()
{
    if (!gMarkDragPending) return;
    if (gMarkDragState == 0) {
        if (gDude == nullptr || gMapHeader.name[0] == '\0') return;
        if (!(scene_renderer::isActive() && scene::shouldPresent()
                && scene::activeView() == scene::View::Full)) return;
        if (gMarkDragReadyAt == 0) gMarkDragReadyAt = SDL_GetTicks() + gMarkDragDelaySeconds * 1000;
        if (SDL_GetTicks() < gMarkDragReadyAt) return;
        enterMarkMode();
        if (!gMarkMode) {
            if (++gMarkDragAttempts > 600) {
                gMarkDragPending = false;
                gMarkDragAttempts = 0;
                devtools::scriptedInputSetExternalMouse(false);
                printf("owner shot: mark drag gave up waiting for a ready view\n");
                fflush(stdout);
            }
            return;
        }
        // The rectangle spans the view's middle third, as the pump mark does.
        gMarkDragPlanned.x0 = gMarkWidth / 3;
        gMarkDragPlanned.y0 = gMarkHeight / 3;
        gMarkDragPlanned.x1 = gMarkWidth * 2 / 3;
        gMarkDragPlanned.y1 = gMarkHeight * 2 / 3;
        devtools::scriptedInputSetExternalMouse(true);
        gMarkDragState = 1;
        gMarkDragFrame = 0;
        return;
    }

    gMarkDragFrame++;
    switch (gMarkDragState) {
    case 1:
        if (gMarkDragFrame < 5) return;
        pushScriptedMouse(SDL_MOUSEMOTION, gMarkDragPlanned.x0, gMarkDragPlanned.y0);
        gMarkDragState = 2;
        break;
    case 2:
        pushScriptedMouse(SDL_MOUSEBUTTONDOWN, gMarkDragPlanned.x0, gMarkDragPlanned.y0);
        gMarkDragState = 3;
        gMarkDragFrame = 0;
        break;
    case 3:
        if (gMarkDragFrame <= 12) {
            const int x = gMarkDragPlanned.x0 + (gMarkDragPlanned.x1 - gMarkDragPlanned.x0) * gMarkDragFrame / 12;
            const int y = gMarkDragPlanned.y0 + (gMarkDragPlanned.y1 - gMarkDragPlanned.y0) * gMarkDragFrame / 12;
            pushScriptedMouse(SDL_MOUSEMOTION, x, y);
        } else {
            gMarkDragState = 4;
        }
        break;
    case 4:
        pushScriptedMouse(SDL_MOUSEBUTTONUP, gMarkDragPlanned.x1, gMarkDragPlanned.y1);
        gMarkDragState = 5;
        gMarkDragFrame = 0;
        break;
    case 5:
        if (gMarkDragFrame < 6) return;
        printf("owner shot: mark drag %s (%d rectangles)\n",
            gMarkRects.empty() ? "FAILED: drag never registered" : "rect registered",
            static_cast<int>(gMarkRects.size()));
        fflush(stdout);
        pushScriptedReturnKey();
        gMarkDragState = 6;
        gMarkDragFrame = 0;
        break;
    case 6:
        if (gMarkDragFrame < 30) return;
        printf("owner shot: mark drag done (overlay green %s)\n",
            gMarkDragPixelVerified ? "verified on screen" : "NOT verified");
        fflush(stdout);
        devtools::scriptedInputSetExternalMouse(false);
        exitMarkMode();
        gMarkDragPending = false;
        gMarkDragState = 0;
        break;
    default:
        gMarkDragState = 0;
        break;
    }
}

// The presented-frame readback for the drag bench: before the press it counts
// the strip's green as the scene's baseline, after it the overlay's outline
// must raise that count by most of the strip. Pure (0,255,0) is the overlay's
// own color, blended straight over the frame.
void verifySceneOverlayPresented()
{
    if (!gMarkDragPending || gMarkDragState < 1 || gMarkDragState > 5) return;
    int x0 = 0;
    int y0 = 0;
    if (!scene_renderer::overlayLogicalToWindow(gMarkDragPlanned.x0, gMarkDragPlanned.y0, &x0, &y0)) return;
    int x1 = 0;
    int y1 = 0;
    if (!scene_renderer::overlayLogicalToWindow(gMarkDragPlanned.x1, gMarkDragPlanned.y0, &x1, &y1)) return;
    const int stripWidth = x1 - x0 + 1;
    if (stripWidth <= 0) return;
    std::vector<unsigned char> pixels;
    if (!scene_renderer::readPresentPixels(x0, y0, stripWidth, 1, pixels)) return;
    int green = 0;
    for (int index = 0; index < stripWidth; index++) {
        const unsigned char* pixel = pixels.data() + static_cast<size_t>(index) * 4;
        if (pixel[0] < 32 && pixel[1] >= 250 && pixel[2] < 32) green++;
    }
    if (gMarkDragState <= 2) {
        // Before the press: whatever green the scene itself has is the baseline.
        gMarkDragGreenBaseline = green;
        return;
    }
    if (!gMarkDragPixelVerified && gMarkDragGreenBaseline >= 0
        && green - gMarkDragGreenBaseline >= stripWidth / 2) {
        gMarkDragPixelVerified = true;
        printf("owner shot: overlay pixels verified on screen (green %d, baseline %d, strip %d)\n",
            green, gMarkDragGreenBaseline, stripWidth);
        fflush(stdout);
    }
}


void pump()
{
    pumpTeleport();
    pumpMark();
    pumpDrag();
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
    if (takeShot()) {
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
    displayMonitorAddMessage(scene::menuStatusText(message, settings.system.language).c_str());
    printf("owner teleport: arrived, %s\n", message);
    fflush(stdout);
    gTeleport.active = false;
}

} // namespace owner_shot
} // namespace fallout
