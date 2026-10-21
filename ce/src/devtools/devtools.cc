#include "devtools.h"
#include "snapshot_rows.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cctype>
#include <cmath>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "../art.h"
#include "../animation.h"
#include "../color.h"
#include "../combat.h"
#include "../critter.h"
#include "../cycle.h"
#include "../db.h"
#include "../debug.h"
#include "../dialog.h"
#include "../draw.h"
#include "../input.h"
#include "../light.h"
#include "../kb.h"
#include "../dinput.h"
#include "../map.h"
#include "../map_edge.h"
#include "../map_defs.h"
#include "../game.h"
#include "../game_mouse.h"
#include "../object.h"
#include "../preferences.h"
#include "../mouse.h"
#include "../movie.h"
#include "../platform/git_version.h"
#include "../proto.h"
#include "../platform_compat.h"
#include "../random.h"
#include "../scene/scene_lights.h"
#include "../scripts.h"
#include "../scene/scene_objects.h"
#include "../scene/scene_settings.h"
#include "../scene/scene_view.h"
#include "../scene_renderer/game_view.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/context.h"
#include "../scene_renderer/live_objects.h"
#include "../scene_renderer/picking.h"
#include "../scene_renderer/scene_draw.h"
#include "../scene_renderer/static_scene.h"
#include "../settings.h"
#include "../svga.h"
#include "../text_font.h"
#include "../tile.h"
#include "../window_manager.h"
#include "../worldmap.h"

#include <SDL.h>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace fallout {
namespace devtools {

namespace {

// --- state -----------------------------------------------------------------

bool gEnabled = false;
bool gListMaps = false;
FixedScreenOptions gFixedScreenOptions;
char gStartupMap[16] = {};
int gStartupElevation = 0;
bool gStartupApplied = false;
bool gStartupTargetSettled = true;
bool gStartupTargetAwaitingFinal = false;
bool gStartupFloorPreviewRequested = false;
bool gStartupWallPreviewRequested = false;
bool gStartupHeroPreviewRequested = false;
bool gStartupWalkPreviewRequested = false;
bool gStartupFloorPreviewAttempted = false;
int gStartupFloorPreviewReturnAfter = 0;
int gStartupFloorPreviewFramesRemaining = 0;
bool gSnapshotOncePending = false;
bool gPreferencesSnapshotPending = false;
bool gPreferencesSnapshotActive = false;
// --dev-map-tour: the map elevations to visit after the startup one; the
// step counts snapshots taken (-1: no tour).
struct TourStop {
    std::string map;
    int elevation;
    int viewTile; // -1: where the game puts the view
    // Batch views of one thing (the shapes eval): only what stands within
    // `solo` hexes of the view tile, from the fly camera `turn` degrees round
    // and `distance` units away. solo -1: everything; turn NaN: no flight.
    int solo = -1;
    double turn = std::nan("");
    double distance = -1.0;
};
// --dev-classic: the same startup and snapshots without the 3D scene.
bool gClassic = false;
std::vector<TourStop> gTour;
int gTourStep = -1;
int gTourFrames = 0;
bool gStillSnapshot = false;
bool gBackgroundGlow = false;
double gDevZoom = -1.0; // --dev-zoom=D: the background run's 3D zoom (default 1)
int gDevAmbient = -1; // --dev-ambient: the map's ambient light held there (night views)
bool gDevFly = false; // --dev-fly: the fly camera takes off at the first 3D frame (snapshots)
// --dev-fly-turn=D, --dev-fly-pitch=D, --dev-fly-distance=U: then it circles
// the point it looks at, D degrees round, D degrees above the floor, U units
// from it (a side view of a thing).
double gDevFlyTurn = 0.0;
int gDevCameraTurn = 0; // --dev-camera-turn=N: the 3D camera N quarter turns round before the snapshot
double gDevFlyPitch = -1.0;
double gDevFlyDistance = -1.0;
int gViewTile = -1;
bool gViewTileApplied = false;
int gHeroTile = -1; // --dev-hero-tile: the hero is put there before the scene starts
char gSnapshotRun[33] = {};
// --dev-pick-probe: the mouse is put on the scene's critters one by one and
// what the game finds there is logged (devprobe.log); snapshots show the hex
// cursor, then the hover icon over a critter. 0: off, then frames counted.
int gPickProbeStage = 0;
int gPickProbeClick[2] = { -1, -1 }; // a left click the game loop is to take
int gPickProbeStartTile = -1;
int gPickProbeEdgeTarget = -1;
int gPickProbeOutsideX = -1;
int gPickProbeOutsideY = -1;
int gPickProbeOutsideHeroTile = -1;
bool gPickProbeOutsideClick = false;
bool gPickProbeExitPending = false;
int gPickProbeExitWaitFrames = 0;
char gToolsCommit[32] = "-";

bool gPanelOpen = false;
std::vector<std::string> gMaps; // file names, e.g. "kladwtwn.map"
std::vector<int> gFiltered;     // indices into gMaps
char gFilter[17] = {};
int gSelected = 0;          // index into gFiltered
int gSelectedElevation = 0; // panel choice (the engine's gElevation is the truth)
char gStatus[128] = "dev tools ready";

// Pending elevation correction: after a cross-map transition the engine
// places the dude on the map's entrance elevation; if the requested
// elevation differs (and exists on that map), a follow-up same-map
// transition is issued once the load has settled.
bool gPendingElevation = false;
int gPendingMapId = -1;
int gPendingElevationValue = 0;

// Panel layout (logical framebuffer coordinates).
constexpr int kPanelX = 396;
constexpr int kPanelY = 46;
constexpr int kPanelW = 240;
constexpr int kPanelH = 214;
constexpr int kListRows = 7;

// Panel-relative click zones (computed by draw, reused by handleClick).
struct Rect2 {
    int x, y, w, h;
};
Rect2 gListRect;
Rect2 gElevRect;
Rect2 gPrevRect;
Rect2 gNextRect;
Rect2 gGoRect;
Rect2 gSnapRect;
int gListRowHeight = 0; // drawn row height (font 101), reused by clicks
bool gZonesValid = false;

#define DT_TEXT_COLOR COLOR_LIGHT_GREY
#define DT_HIGHLIGHT_COLOR COLOR_WHITE
#define DT_TITLE_COLOR COLOR_YELLOW_2
#define DT_ACTIVE_COLOR COLOR_GREEN
#define DT_ERROR_COLOR COLOR_RED

// --- sha256 (public-domain style compact implementation) --------------------

struct Sha256 {
    unsigned int state[8];
    unsigned char data[64];
    unsigned int datalen;
    unsigned long long totalBits;

    static unsigned int rotr(unsigned int x, int n)
    {
        return (x >> n) | (x << (32 - n));
    }

    void transform(const unsigned char* chunk)
    {
        static const unsigned int k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };
        unsigned int m[64];
        for (int i = 0, j = 0; i < 16; i++, j += 4) {
            m[i] = (chunk[j] << 24) | (chunk[j + 1] << 16) | (chunk[j + 2] << 8) | chunk[j + 3];
        }
        for (int i = 16; i < 64; i++) {
            unsigned int s0 = rotr(m[i - 15], 7) ^ rotr(m[i - 15], 18) ^ (m[i - 15] >> 3);
            unsigned int s1 = rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10);
            m[i] = m[i - 16] + s0 + m[i - 7] + s1;
        }
        unsigned int a = state[0], b = state[1], c = state[2], d = state[3];
        unsigned int e = state[4], f = state[5], g = state[6], h = state[7];
        for (int i = 0; i < 64; i++) {
            unsigned int s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            unsigned int ch = (e & f) ^ (~e & g);
            unsigned int t1 = h + s1 + ch + k[i] + m[i];
            unsigned int s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
            unsigned int t2 = s0 + maj;
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    void init()
    {
        state[0] = 0x6a09e667;
        state[1] = 0xbb67ae85;
        state[2] = 0x3c6ef372;
        state[3] = 0xa54ff53a;
        state[4] = 0x510e527f;
        state[5] = 0x9b05688c;
        state[6] = 0x1f83d9ab;
        state[7] = 0x5be0cd19;
        datalen = 0;
        totalBits = 0;
    }

    void update(const unsigned char* p, size_t n)
    {
        totalBits += static_cast<unsigned long long>(n) * 8;
        for (size_t i = 0; i < n; i++) {
            data[datalen++] = p[i];
            if (datalen == 64) {
                transform(data);
                datalen = 0;
            }
        }
    }

    void final(unsigned char* hash)
    {
        unsigned long long bits = totalBits;
        bool shortPad = datalen < 56;
        data[datalen++] = 0x80;
        while (datalen < (shortPad ? 56 : 64)) {
            data[datalen++] = 0x00;
        }
        if (!shortPad) {
            transform(data);
            datalen = 56;
        }
        for (int i = 7; i >= 0; i--) {
            data[56 + (7 - i)] = static_cast<unsigned char>((bits >> (i * 8)) & 0xFF);
        }
        transform(data);
        for (int i = 0; i < 4; i++) {
            hash[i] = (state[0] >> (24 - i * 8)) & 0xFF;
            hash[i + 4] = (state[1] >> (24 - i * 8)) & 0xFF;
            hash[i + 8] = (state[2] >> (24 - i * 8)) & 0xFF;
            hash[i + 12] = (state[3] >> (24 - i * 8)) & 0xFF;
            hash[i + 16] = (state[4] >> (24 - i * 8)) & 0xFF;
            hash[i + 20] = (state[5] >> (24 - i * 8)) & 0xFF;
            hash[i + 24] = (state[6] >> (24 - i * 8)) & 0xFF;
            hash[i + 28] = (state[7] >> (24 - i * 8)) & 0xFF;
        }
    }
};

void sha256File(const char* path, char* outHex, size_t outSize)
{
    Sha256 sha;
    sha.init();
    FILE* f = fopen(path, "rb");
    if (f == nullptr) {
        snprintf(outHex, outSize, "<unavailable>");
        return;
    }
    unsigned char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        sha.update(buf, n);
    }
    fclose(f);
    unsigned char hash[32];
    sha.final(hash);
    for (int i = 0; i < 32; i++) {
        snprintf(outHex + i * 2, outSize - i * 2, "%02x", hash[i]);
    }
}

// --- helpers ----------------------------------------------------------------

const char* stristr(const char* haystack, const char* needle)
{
    size_t needleLen = strlen(needle);
    if (needleLen == 0) {
        return haystack;
    }
    for (const char* p = haystack; *p != '\0'; p++) {
        size_t i = 0;
        while (i < needleLen
            && tolower(static_cast<unsigned char>(p[i])) == tolower(static_cast<unsigned char>(needle[i]))) {
            i++;
        }
        if (i == needleLen) {
            return p;
        }
    }
    return nullptr;
}

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

// The map header bit means this elevation has no square data. _square_reset
// fills omitted elevations with Grid fids, so inspecting the tile array alone
// cannot establish whether the elevation exists.
bool elevationPresent(int elevation)
{
    if (elevation < 0 || elevation >= ELEVATION_COUNT) {
        return false;
    }
    static constexpr int kElevationFlags[ELEVATION_COUNT] = {
        MAP_HEADER_ELEVATION_0,
        MAP_HEADER_ELEVATION_1,
        MAP_HEADER_ELEVATION_2,
    };
    return (static_cast<int>(gMapHeader.flags) & kElevationFlags[elevation]) == 0;
}

// Reads only the authoritative map-header flags from an unloaded map. Square
// blocks follow global/local variables and are serialized only for elevations
// whose absence bit is clear; probing them here would need to reproduce the
// full map loader's variable and conditional-block handling.
int probeTargetElevation(const char* base, int elevation)
{
    if (elevation < 0 || elevation >= ELEVATION_COUNT) {
        return -1;
    }

    static constexpr int kElevationFlags[ELEVATION_COUNT] = {
        MAP_HEADER_ELEVATION_0,
        MAP_HEADER_ELEVATION_1,
        MAP_HEADER_ELEVATION_2,
    };

    for (int attempt = 0; attempt < 2; attempt++) {
        char fileName[16];
        snprintf(fileName, sizeof(fileName), "%s.%s", base, attempt == 0 ? "SAV" : "MAP");
        File* stream = fileOpen(mapBuildPath(fileName), "rb");
        if (stream == nullptr) {
            continue;
        }

        int version = 0;
        int dummy = 0;
        int flags = 0;
        bool ok = fileReadInt32(stream, &version) == 0;
        char mapName[17] = {};
        ok = ok && fileReadFixedLengthString(stream, mapName, sizeof(mapName) - 1) == 0;
        for (int i = 0; i < 5 && ok; i++) {
            ok = fileReadInt32(stream, &dummy) == 0; // enteringTile/elevation/rotation, localVariablesCount, scriptIndex
        }
        ok = ok && fileReadInt32(stream, &flags) == 0;

        fileClose(stream);
        if (!ok || (version != 19 && version != 20)) {
            return -1;
        }

        char headerBase[17];
        char expectedBase[17];
        stripMapExtension(mapName, headerBase, sizeof(headerBase));
        stripMapExtension(base, expectedBase, sizeof(expectedBase));
        if (compat_stricmp(headerBase, expectedBase) != 0) {
            return -1;
        }

        return (flags & kElevationFlags[elevation]) == 0 ? 0 : 1;
    }
    return -1;
}

void refreshFiltered()
{
    gFiltered.clear();
    for (size_t i = 0; i < gMaps.size(); i++) {
        if (gFilter[0] != '\0') {
            if (stristr(gMaps[i].c_str(), gFilter) == nullptr) {
                continue;
            }
        }
        gFiltered.push_back(static_cast<int>(i));
    }
    if (gSelected >= static_cast<int>(gFiltered.size())) {
        gSelected = static_cast<int>(gFiltered.size()) - 1;
    }
    if (gSelected < 0) {
        gSelected = 0;
    }
}

void rebuildMapList()
{
    gMaps.clear();
    char** fileList = nullptr;
    int count = fileNameListInit("maps\\*.map", &fileList);
    if (count > 0 && fileList != nullptr) {
        for (int i = 0; i < count; i++) {
            gMaps.push_back(fileList[i]);
        }
    }
    if (count > 0) {
        fileNameListFree(&fileList, count);
    }
    std::sort(gMaps.begin(), gMaps.end());
    refreshFiltered();
}

bool inBlockingGameState()
{
    if (isInCombat()) {
        return true;
    }
    if (_inDialog != 0) {
        return true;
    }
    if (_moviePlaying() != 0) {
        return true;
    }
    return false;
}

Rect2 textButton(int x, int y, int w, int h)
{
    Rect2 r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

bool hitTest(const Rect2& r, int x, int y)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void setStatus(const char* text)
{
    snprintf(gStatus, sizeof(gStatus), "%s", text);
}

bool disableStaticFloorPreview(const char* status)
{
    if (!scene::disable()) return false;
    gStartupFloorPreviewFramesRemaining = 0;
    setStatus(status);
    return true;
}

bool enableStaticPreview(bool withWalls, bool withHero = false)
{
    const scene::View view = withHero ? scene::View::Full : withWalls ? scene::View::Walls : scene::View::Floor;
    std::string message;
    if (!scene::enable(view, &message)) {
        setStatus(message.c_str());
        printf("devtools: static preview failed: %s\n", message.c_str());
        fflush(stdout);
        return false;
    }
    if (withHero) {
        setStatus("wall and hero preview on (Ctrl+F10 to return)");
        printf("devtools: static wall+hero preview started for %s elevation %d\n", gMapHeader.name,
            static_cast<int>(gElevation));
    } else if (withWalls) {
        unsigned int triangles = 0;
        unsigned int pages = 0;
        scene_renderer::staticWallStats(&triangles, &pages);
        setStatus("static wall preview on (Ctrl+F10 to return)");
        printf("devtools: static wall preview started for %s elevation %d (%u triangles, %u pages)\n",
            gMapHeader.name, static_cast<int>(gElevation), triangles, pages);
    } else {
        setStatus("static floor preview on (Ctrl+F10 to return)");
        printf("devtools: static floor preview started for %s elevation %d\n", gMapHeader.name,
            static_cast<int>(gElevation));
    }
    fflush(stdout);
    if (gSnapshotOncePending && !withHero) {
        gSnapshotOncePending = false;
        const bool captured = takeSnapshot();
        printf("devtools: one-shot preview snapshot %s: %s\n",
            captured ? "saved" : "failed", gStatus);
        fflush(stdout);
    }
    return true;
}

bool setSceneSettingEnabled(bool enabled)
{
    for (const SettingDescriptor& descriptor : settingsGetDescriptors()) {
        if (descriptor.section == "3d" && descriptor.key == "enabled") {
            std::string error;
            if (!settingsSetValue(descriptor, SettingValue(enabled), &error)) {
                setStatus(error.c_str());
                return false;
            }
            return true;
        }
    }
    setStatus("3D setting is unavailable");
    return false;
}

// One candidate check shared by the search and fallback. A Grid floor is the
// engine's blank floor frame; this only rejects blank cells and known blockers,
// and does not claim to prove path connectivity from the current tile.
bool elevationTileFree(int tile, int elevation)
{
    if (tile <= 0 || !hexGridTileIsValid(tile) || !elevationIsValid(elevation)) {
        return false;
    }

    const int squareIndex = squareTileFromTile(tile);
    if (!squareGridTileIsValid(squareIndex)) {
        return false;
    }
    const FloorTileFrmId floorTileFrmId(_square[elevation]->tileFid[squareIndex]);
    if (floorTileFrmId.frameId<TileFrameId>() == TileFrameId::Grid) {
        return false;
    }
    if (_obj_blocking_at(nullptr, tile, static_cast<MapElevation>(elevation)) != nullptr) {
        return false;
    }
    if (_obj_scroll_blocking_at(tile, static_cast<MapElevation>(elevation)) == 0) {
        return false;
    }
    return true;
}

// Checks complete hex-distance rings, bounded to a small square around center.
// Filtering with the engine distance helper gives each interior ring 6*d
// unique cells; at map edges, invalid coordinates are clipped.
int findFreeTileOnElevation(int center, int elevation, int maxDistance)
{
    if (!hexGridTileIsValid(center) || !elevationIsValid(elevation) || maxDistance < 0) {
        return -1;
    }

    const int centerCol = center % HEX_GRID_WIDTH;
    const int centerRow = center / HEX_GRID_WIDTH;
    for (int distance = 0; distance <= maxDistance; distance++) {
        for (int rowOffset = -distance; rowOffset <= distance; rowOffset++) {
            const int row = centerRow + rowOffset;
            if (row < 0 || row >= HEX_GRID_HEIGHT) {
                continue;
            }
            for (int colOffset = -distance; colOffset <= distance; colOffset++) {
                const int col = centerCol + colOffset;
                if (col < 0 || col >= HEX_GRID_WIDTH) {
                    continue;
                }
                const int candidate = row * HEX_GRID_WIDTH + col;
                if (tileDistanceBetween(center, candidate) == distance
                    && elevationTileFree(candidate, elevation)) {
                    return candidate;
                }
            }
        }
    }
    return -1;
}

// Applies a same-map elevation change: the engine skips dude placement when
// the transition tile is -1, so the map's entering tile is passed explicitly.
static bool applyElevationChange(int elevation)
{
    if (elevation < 0 || elevation >= ELEVATION_COUNT) {
        snprintf(gStatus, sizeof(gStatus), "no such elevation: %d", elevation);
        return false;
    }
    if (!elevationPresent(elevation)) {
        snprintf(gStatus, sizeof(gStatus), "elevation %d absent on this map", elevation);
        return false;
    }

    // Bounded: one search pass over rings up to distance 4. A map entrance is
    // a last resort only when it passes the exact same candidate checks.
    int targetTile = findFreeTileOnElevation(gDude->tile, elevation, 4);
    if (targetTile == -1 && elevationTileFree(gMapHeader.enteringTile, elevation)) {
        targetTile = gMapHeader.enteringTile;
    }
    if (targetTile == -1) {
        snprintf(gStatus, sizeof(gStatus), "no free tile on elevation %d", elevation);
        return false;
    }

    MapTransition transition;
    transition.map = gMapHeader.index;
    transition.elevation = static_cast<MapElevation>(elevation);
    transition.tile = targetTile;
    transition.rotation = static_cast<Rotation>(-1);

    if (mapSetTransition(&transition) == -1) {
        setStatus("transition rejected");
        return false;
    }
    return true;
}

} // namespace

// --- public API -------------------------------------------------------------

bool parseOption(const char* arg)
{
    if (strcmp(arg, "--dev-tools") == 0) {
        gEnabled = true;
        return true;
    }
    if (strcmp(arg, "--dev-floor-preview") == 0) {
        gStartupFloorPreviewRequested = true;
        return true;
    }
    if (strcmp(arg, "--dev-wall-preview") == 0) {
        gStartupWallPreviewRequested = true;
        return true;
    }
    if (strcmp(arg, "--dev-hero-preview") == 0) {
        gStartupHeroPreviewRequested = true;
        return true;
    }
    if (strcmp(arg, "--dev-walk-preview") == 0) {
        gStartupWalkPreviewRequested = true;
        gStartupHeroPreviewRequested = true;
        return true;
    }
    const char* returnAfterPrefix = "--dev-floor-preview-return-after=";
    if (strncmp(arg, returnAfterPrefix, strlen(returnAfterPrefix)) == 0) {
        char* end = nullptr;
        const long frames = strtol(arg + strlen(returnAfterPrefix), &end, 10);
        if (end != arg + strlen(returnAfterPrefix) && *end == '\0' && frames >= 1 && frames <= 10000) {
            gStartupFloorPreviewReturnAfter = static_cast<int>(frames);
        } else {
            gStartupFloorPreviewReturnAfter = 0;
            fprintf(stderr, "devtools: invalid --dev-floor-preview-return-after value (expected 1..10000)\n");
            fflush(stderr);
        }
        return true;
    }
    if (strcmp(arg, "--dev-snapshot-once") == 0) {
        gSnapshotOncePending = true;
        return true;
    }
    if (strcmp(arg, "--dev-preferences-snapshot") == 0) {
        gPreferencesSnapshotPending = true;
        return true;
    }
    if (strcmp(arg, "--dev-still-snapshot") == 0) {
        gStillSnapshot = true;
        return true;
    }
    // --dev-glow: keep the glow on in a background run (a snapshot of the
    // glowing view); off is the default there, so captures stay byte-stable.
    if (strcmp(arg, "--dev-glow") == 0) {
        gBackgroundGlow = true;
        debugPrint("DEVTOOLS: the glow stays on in this background run\n");
        return true;
    }
    if (strcmp(arg, "--dev-fly") == 0) {
        gDevFly = true;
        return true;
    }
    const char* flyTurnPrefix = "--dev-fly-turn=";
    if (strncmp(arg, flyTurnPrefix, strlen(flyTurnPrefix)) == 0) {
        gDevFly = true;
        gDevFlyTurn = strtod(arg + strlen(flyTurnPrefix), nullptr);
        return true;
    }
    const char* cameraTurnPrefix = "--dev-camera-turn=";
    if (strncmp(arg, cameraTurnPrefix, strlen(cameraTurnPrefix)) == 0) {
        gDevCameraTurn = static_cast<int>(strtol(arg + strlen(cameraTurnPrefix), nullptr, 10)) % 4;
        return true;
    }
    const char* flyDistancePrefix = "--dev-fly-distance=";
    if (strncmp(arg, flyDistancePrefix, strlen(flyDistancePrefix)) == 0) {
        gDevFly = true;
        gDevFlyDistance = std::clamp(strtod(arg + strlen(flyDistancePrefix), nullptr), 1.0, 200.0);
        return true;
    }
    const char* flyPitchPrefix = "--dev-fly-pitch=";
    if (strncmp(arg, flyPitchPrefix, strlen(flyPitchPrefix)) == 0) {
        gDevFly = true;
        gDevFlyPitch = std::clamp(strtod(arg + strlen(flyPitchPrefix), nullptr), 1.0, 85.0);
        return true;
    }
    const char* ambientPrefix = "--dev-ambient=";
    if (strncmp(arg, ambientPrefix, strlen(ambientPrefix)) == 0) {
        gDevAmbient = static_cast<int>(std::clamp(strtol(arg + strlen(ambientPrefix), nullptr, 10), 0L, 65536L));
        return true;
    }
    const char* zoomPrefix = "--dev-zoom=";
    if (strncmp(arg, zoomPrefix, strlen(zoomPrefix)) == 0) {
        gDevZoom = std::clamp(strtod(arg + strlen(zoomPrefix), nullptr), 0.25, 4.0);
        return true;
    }
    const char* heroPrefix = "--dev-hero-tile=";
    if (strncmp(arg, heroPrefix, strlen(heroPrefix)) == 0) {
        const long tile = strtol(arg + strlen(heroPrefix), nullptr, 10);
        if (tile >= 0 && tile < HEX_GRID_SIZE) gHeroTile = static_cast<int>(tile);
        return true;
    }
    const char* viewPrefix = "--dev-view-tile=";
    if (strncmp(arg, viewPrefix, strlen(viewPrefix)) == 0) {
        const char* value = arg + strlen(viewPrefix);
        char* end = nullptr;
        const long tile = strtol(value, &end, 10);
        if (end != value && *end == '\0' && tile >= 0 && tile < HEX_GRID_SIZE) {
            gViewTile = static_cast<int>(tile);
        } else {
            fprintf(stderr, "devtools: invalid --dev-view-tile (expected 0..39999)\n");
        }
        return true;
    }
    const char* runPrefix = "--dev-snapshot-run=";
    if (strncmp(arg, runPrefix, strlen(runPrefix)) == 0) {
        const char* value = arg + strlen(runPrefix);
        if (strlen(value) == 32 && std::all_of(value, value + 32,
                [](unsigned char ch) { return std::isxdigit(ch) != 0; })) {
            snprintf(gSnapshotRun, sizeof(gSnapshotRun), "%s", value);
        }
        return true;
    }
    // --dev-solo=TILE,R: only the things within R hexes of TILE are drawn (no
    // walls, no roofs), to look at one thing from every side.
    const char* soloPrefix = "--dev-solo=";
    if (strncmp(arg, soloPrefix, strlen(soloPrefix)) == 0) {
        int tile = -1;
        int radius = -1;
        if (sscanf(arg + strlen(soloPrefix), "%d,%d", &tile, &radius) == 2 && tile >= 0 && tile < HEX_GRID_SIZE
            && radius >= 0) {
            scene_renderer::setStaticSolo(tile, radius);
        } else {
            fprintf(stderr, "devtools: invalid --dev-solo (expected TILE,R)\n");
        }
        return true;
    }
    if (strcmp(arg, "--dev-parallel") == 0) {
        scene_renderer::setStaticViewDistance(9000.0);
        return true;
    }
    if (strcmp(arg, "--dev-classic") == 0) {
        gClassic = true;
        return true;
    }
    if (strcmp(arg, "--dev-pick-probe") == 0) {
        gPickProbeStage = 1;
        return true;
    }
    const char* tourPrefix = "--dev-map-tour=";
    if (strncmp(arg, tourPrefix, strlen(tourPrefix)) == 0) {
        // map:elevation[:view hex[:solo hexes:turn degrees:distance]],...
        std::string list = arg + strlen(tourPrefix);
        size_t start = 0;
        while (start <= list.size()) {
            const size_t comma = std::min(list.find(',', start), list.size());
            const std::string entry = list.substr(start, comma - start);
            const size_t colon = entry.find(':');
            const int elevation = colon == std::string::npos ? 0 : atoi(entry.c_str() + colon + 1);
            const size_t second = colon == std::string::npos ? std::string::npos : entry.find(':', colon + 1);
            const int viewTile = second == std::string::npos ? -1 : atoi(entry.c_str() + second + 1);
            if (!entry.empty()) {
                TourStop stop = { entry.substr(0, colon), std::clamp(elevation, 0, static_cast<int>(ELEVATION_COUNT) - 1),
                    viewTile >= 0 && viewTile < HEX_GRID_SIZE ? viewTile : -1 };
                const size_t third = second == std::string::npos ? std::string::npos : entry.find(':', second + 1);
                if (third != std::string::npos) {
                    int solo = -1;
                    double turn = 0.0;
                    double distance = -1.0;
                    if (sscanf(entry.c_str() + third + 1, "%d:%lf:%lf", &solo, &turn, &distance) == 3) {
                        stop.solo = solo;
                        stop.turn = turn;
                        stop.distance = distance;
                    }
                }
                gTour.push_back(stop);
            }
            start = comma + 1;
        }
        gTourStep = 0;
        return true;
    }
    if (strncmp(arg, "--dev-map=", 10) == 0) {
        stripMapExtension(arg + 10, gStartupMap, sizeof(gStartupMap));
        return true;
    }
    if (strncmp(arg, "--dev-elevation=", 16) == 0) {
        gStartupElevation = atoi(arg + 16);
        if (gStartupElevation < 0) {
            gStartupElevation = 0;
        }
        if (gStartupElevation > ELEVATION_COUNT - 1) {
            gStartupElevation = ELEVATION_COUNT - 1;
        }
        return true;
    }
    if (strncmp(arg, "--dev-tools-commit=", 19) == 0) {
        snprintf(gToolsCommit, sizeof(gToolsCommit), "%s", arg + 19);
        return true;
    }
    if (strcmp(arg, "--dev-list-maps") == 0) {
        gListMaps = true;
        return true;
    }
    return false;
}

bool initializeFixedScreenOptions(int argc, char** argv)
{
    const char* error = nullptr;
    if (!parseFixedScreenOptions(argc, argv, &gFixedScreenOptions, &error)) {
        fprintf(stderr, "devtools: %s\n", error != nullptr ? error : "invalid fixed screen options");
        fflush(stderr);
        return false;
    }
    return true;
}

bool fixedScreenOptionsEnabled()
{
    return gFixedScreenOptions.enabled;
}

bool backgroundCaptureActive()
{
    return fixedScreenOptionsEnabled() && SDL_getenv("F2CE_DEV_BACKGROUND") != nullptr;
}

void applyFixedScreenOptions()
{
    if (!gFixedScreenOptions.enabled) return;
    settings.screen.resolution_x = gFixedScreenOptions.width;
    settings.screen.resolution_y = gFixedScreenOptions.height;
    settings.screen.scale = gFixedScreenOptions.scale;
    settings.screen.windowed = WindowMode::Windowed;
    // A background test run shares game/fallout2.cfg with the owner's game:
    // the owner's zoom and camera rotation must not move its views.
    if (backgroundCaptureActive()) {
        settings.scene3d.zoom = 1.0;
        settings.scene3d.camera_rotation = false;
        settings.scene3d.glow = gBackgroundGlow;
    }
    debugPrint("DEVTOOLS: fixed screen %dx%d scale %d, windowed, glow %d%s\n",
        gFixedScreenOptions.width, gFixedScreenOptions.height, gFixedScreenOptions.scale,
        settings.scene3d.glow ? 1 : 0, gBackgroundGlow ? " (kept on)" : "");
}

void applyBackgroundGlow()
{
    if (backgroundCaptureActive()) settings.scene3d.glow = gBackgroundGlow;
}

// Runs after the command line is parsed (applyFixedScreenOptions is earlier),
// so --dev-zoom reaches the scene: a background run is pinned to zoom 1
// unless the dev argument names another one.
void applyBackgroundZoom()
{
    if (backgroundCaptureActive() && gDevZoom > 0.0) settings.scene3d.zoom = gDevZoom;
}

bool isEnabled()
{
    return gEnabled;
}

bool wantsListMaps()
{
    return gListMaps;
}

bool stillSnapshotRequested()
{
    return gEnabled && gStillSnapshot;
}

void prepareStillSnapshot()
{
    if (!stillSnapshotRequested()) return;
    // Keep map-enter scripts synchronous, but stop their background timers,
    // critter AI, fidgets and animation tickers before any map is loaded.
    randomSeedPrerandom(1);
    colorCycleDisable();
    tickersDisable();
    scene::setCameraGlide(false);
}

namespace {

// Ctrl+F12: a free camera flies over the 3D scene, for looking at a map up
// close and recording videos. It owns the keyboard and the mouse while it
// flies; the game keeps running.
bool gFlying = false;
scene_renderer::StaticFlyCamera gFly = {};
Uint64 gFlyLast = 0;
int gFlyCenterTile = -1; // the game's view, put back when the flight ends
char gFlyMap[16] = {}; // on this map and elevation only
int gFlyElevation = -1;
bool gFlyRestorePending = false; // landed inside a frame: the view goes back at the next update

constexpr float kFlySpeed = 6.0f; // units a second
constexpr float kFlyTurn = 0.0025f; // radians a mouse count
constexpr float kFlyPitchLimit = 1.5f;

// The game's view goes back where it was before the flight, on the same map.
void restoreFlightView()
{
    gFlyRestorePending = false;
    const bool sameMap = gFlyElevation == gElevation && strncmp(gFlyMap, gMapHeader.name, sizeof(gFlyMap)) == 0;
    if (sameMap && gFlyCenterTile >= 0 && gCenterTile != gFlyCenterTile) {
        tileSetCenter(gFlyCenterTile, TILE_SET_CENTER_REFRESH_WINDOW);
    }
}

// `restoreView`: the view goes back now; otherwise (a frame is being
// presented, and redrawing the map inside it is not safe) at the next update.
void stopFlying(const char* status, bool restoreView)
{
    if (!gFlying) return;
    gFlying = false;
    scene_renderer::setStaticFlyCamera(nullptr);
    mouseDeviceSetCaptured(false);
    if (restoreView) restoreFlightView();
    else gFlyRestorePending = true;
    setStatus(status);
}

// `capture`: the camera owns the mouse (a background test run leaves it alone).
void startFlying(bool capture)
{
    if (scene::activeView() != scene::View::Full || !scene::shouldPresent()
        || !scene_renderer::staticGameViewPose(&gFly)) {
        setStatus("the fly camera needs the 3D scene");
        return;
    }
    gFlying = true;
    gFlyRestorePending = false;
    gFlyLast = SDL_GetPerformanceCounter();
    gFlyCenterTile = gCenterTile;
    strncpy(gFlyMap, gMapHeader.name, sizeof(gFlyMap));
    gFlyElevation = gElevation;
    mouseDeviceSetCaptured(capture);
    scene_renderer::setStaticFlyCamera(&gFly);
    setStatus("fly camera: WASD, mouse, Q/E down/up, Shift fast, Ctrl slow; Ctrl+F12 or Esc to land");
}

// The fly camera circles the floor point it looks at: `turn` degrees round,
// `pitch` degrees above the floor and `distance` units from it, each when
// above 0.
void circleFlight(double turn, double pitch, double distance)
{
    float forward[3], right[3];
    scene_renderer::staticFlyCameraAxes(gFly, forward, right);
    if (!(forward[1] < -0.01f)) return;
    const float reach = -gFly.eye[1] / forward[1];
    const float target[3] = { gFly.eye[0] + forward[0] * reach, 0.0f, gFly.eye[2] + forward[2] * reach };
    constexpr double kRadians = 3.14159265358979323846 / 180.0;
    gFly.yaw += static_cast<float>(turn * kRadians);
    if (pitch > 0.0) gFly.pitch = static_cast<float>(-pitch * kRadians);
    scene_renderer::staticFlyCameraAxes(gFly, forward, right);
    const float away = distance > 0.0 ? static_cast<float>(distance) : reach;
    for (int i = 0; i < 3; i++) gFly.eye[i] = target[i] - forward[i] * away;
    scene_renderer::setStaticFlyCamera(&gFly);
}

// One step of the flight: the mouse turns, the keys move, by the time since
// the last step.
void fly()
{
    if (gFlyRestorePending) restoreFlightView();
    if (!gFlying) return;
    if (scene::activeView() != scene::View::Full || !scene::shouldPresent()) {
        stopFlying("fly camera landed: the 3D scene is not shown", true);
        return;
    }
    const Uint64 now = SDL_GetPerformanceCounter();
    const float seconds = std::min(static_cast<float>(now - gFlyLast) / SDL_GetPerformanceFrequency(), 0.1f);
    gFlyLast = now;

    int dx = 0, dy = 0;
    SDL_GetRelativeMouseState(&dx, &dy);
    gFly.yaw -= dx * kFlyTurn;
    gFly.pitch = std::clamp(gFly.pitch - dy * kFlyTurn, -kFlyPitchLimit, kFlyPitchLimit);

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float forward[3], right[3];
    scene_renderer::staticFlyCameraAxes(gFly, forward, right);
    float move[3] = { 0.0f, 0.0f, 0.0f };
    const float ahead = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);
    const float aside = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);
    const float rise = (keys[SDL_SCANCODE_E] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_Q] ? 1.0f : 0.0f);
    for (int i = 0; i < 3; i++) move[i] = forward[i] * ahead + right[i] * aside;
    move[1] += rise;
    float speed = kFlySpeed;
    if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) speed *= 4.0f;
    if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) speed *= 0.25f;
    for (int i = 0; i < 3; i++) gFly.eye[i] += move[i] * speed * seconds;
    scene_renderer::setStaticFlyCamera(&gFly);
}

} // namespace

void finishPickProbe();

void listMaps()
{
    rebuildMapList();
    printf("Available maps (%d):\n", static_cast<int>(gMaps.size()));
    for (const std::string& map : gMaps) {
        char base[64];
        stripMapExtension(map.c_str(), base, sizeof(base));
        printf("  %s\n", base);
    }
    printf("Use --dev-map=<name> --dev-elevation=<0..2> to travel.\n");
    fflush(stdout);
}

// Called every frame from the main loop after mapHandleTransition: applies the pending
// elevation correction once the requested map has finished loading.
void update()
{
    if (!gEnabled) {
        return;
    }
    if (gPreferencesSnapshotPending && sceneStartupReady() && !inBlockingGameState()) {
        gPreferencesSnapshotPending = false;
        gPreferencesSnapshotActive = true;
        doPreferences(false);
        gPreferencesSnapshotActive = false;
    }
    fly();
    // Map scripts set the light by the hour; a dev view holds its own.
    if (gDevAmbient >= 0 && gMapHeader.name[0] != '\0' && lightGetAmbientIntensity() != gDevAmbient) {
        lightSetAmbientIntensity(gDevAmbient, true);
    }
    if (gPickProbeClick[0] >= 0) {
        const bool edgeClick = gPickProbeOutsideClick;
        const int heroTileBefore = gDude->tile;
        const bool busyBefore = animationIsBusy(gDude) == -1;
        _gmouse_handle_event(gPickProbeClick[0], gPickProbeClick[1], MOUSE_EVENT_LEFT_BUTTON_UP);
        if (gPickProbeExitPending) {
            if (FILE* log = fopen("devprobe.log", "a")) {
                fprintf(log, "exit click dispatched: map=%s hero=%d->%d busy=%d->%d at (%d,%d)\n",
                    gMapHeader.name, heroTileBefore, gDude->tile, busyBefore, animationIsBusy(gDude) == -1,
                    gPickProbeClick[0], gPickProbeClick[1]);
                fclose(log);
            }
        }
        if (edgeClick) {
            if (FILE* log = fopen("devprobe.log", "a")) {
                fprintf(log, "outside click: fresh=%d cursor=%d hero=%d->%d busy=%d->%d\n",
                    tileUnderMouse(gPickProbeClick[0], gPickProbeClick[1]),
                    gGameMouseHexCursor != nullptr ? gGameMouseHexCursor->tile : -1, heroTileBefore, gDude->tile,
                    busyBefore, animationIsBusy(gDude) == -1);
                fclose(log);
            }
            gPickProbeOutsideClick = false;
        }
        gPickProbeClick[0] = gPickProbeClick[1] = -1;
        if (FILE* log = fopen("devprobe.log", "a")) {
            fprintf(log, "probe click: hero busy=%d tile=%d\n", animationIsBusy(gDude) == -1 ? 1 : 0, gDude->tile);
            fclose(log);
        }
    }
    if (gPickProbeExitPending) {
        gPickProbeExitWaitFrames++;
        if (strncmp(gMapHeader.name, "KLADWTWN.MAP", sizeof(gMapHeader.name)) == 0
            && gMapHeader.index == 9 && gDude->tile == 25142
            && gDude->elevation == ELEVATION_FIRST) {
            if (FILE* log = fopen("devprobe.log", "a")) {
                fprintf(log, "exit transition passed: map=%s index=%d elevation=%d tile=%d after=%d frames\n",
                    gMapHeader.name, gMapHeader.index, gDude->elevation, gDude->tile, gPickProbeExitWaitFrames);
                fclose(log);
            }
            finishPickProbe();
        } else if (strcmp(gMapHeader.name, "KLADWTWN.MAP") == 0) {
            if (FILE* log = fopen("devprobe.log", "a")) {
                fprintf(log, "outside probe failed: wrong exit arrival map=%s index=%d elevation=%d tile=%d\n",
                    gMapHeader.name, gMapHeader.index, gDude->elevation, gDude->tile);
                fclose(log);
            }
            finishPickProbe();
        } else if (gPickProbeExitWaitFrames % 60 == 0) {
            if (FILE* log = fopen("devprobe.log", "a")) {
                fprintf(log, "exit transition waiting: map=%s elevation=%d hero=%d busy=%d frames=%d\n",
                    gMapHeader.name, gDude->elevation, gDude->tile, animationIsBusy(gDude) == -1,
                    gPickProbeExitWaitFrames);
                fclose(log);
            }
        }
        if (gPickProbeExitPending && gPickProbeExitWaitFrames >= 1800) {
            if (FILE* log = fopen("devprobe.log", "a")) {
                fprintf(log, "outside probe failed: exit transition timed out at map=%s elevation=%d hero=%d\n",
                    gMapHeader.name, gDude->elevation, gDude->tile);
                fclose(log);
            }
            finishPickProbe();
        }
    }
    if (scene::requestedView() != scene::View::Off && gStartupFloorPreviewFramesRemaining > 0) {
        gStartupFloorPreviewFramesRemaining--;
        if (gStartupFloorPreviewFramesRemaining == 0) {
            disableStaticFloorPreview("static preview returned to vanilla");
            printf("devtools: startup preview returned to vanilla after %d frames\n", gStartupFloorPreviewReturnAfter);
            const bool captured = takeSnapshot();
            printf("devtools: vanilla return snapshot %s: %s\n", captured ? "saved" : "failed", gStatus);
            fflush(stdout);
        }
    }
    if (gPendingElevation) {
        if (inBlockingGameState()) {
            return;
        }
        if (static_cast<int>(gMapHeader.index) != gPendingMapId) {
            return; // the requested map has not finished loading yet
        }

        gPendingElevation = false;
        if (gPendingElevationValue != static_cast<int>(gElevation)) {
            if (!elevationPresent(gPendingElevationValue)) {
                snprintf(gStatus, sizeof(gStatus), "elevation %d absent on this map",
                    gPendingElevationValue);
            } else if (applyElevationChange(gPendingElevationValue)) {
                gStartupTargetAwaitingFinal = true;
                snprintf(gStatus, sizeof(gStatus), "moving to elevation %d", gPendingElevationValue);
                return; // let the scheduled transition settle before preview activation
            }
        }
        if (!gStartupTargetAwaitingFinal) gStartupTargetSettled = true;
    }
    if (gStartupTargetAwaitingFinal) {
        if (static_cast<int>(gMapHeader.index) != gPendingMapId
            || static_cast<int>(gElevation) != gPendingElevationValue) {
            return;
        }
        gStartupTargetAwaitingFinal = false;
        gStartupTargetSettled = true;
    }

    if ((gStartupFloorPreviewRequested || gStartupWallPreviewRequested || gStartupHeroPreviewRequested || gStartupWalkPreviewRequested
            || gClassic || gSnapshotOncePending || gTourStep >= 0)
        && !gStartupFloorPreviewAttempted
        && !gPendingElevation && gMapHeader.name[0] != '\0' && !inBlockingGameState()) {
        if (gHeroTile >= 0) {
            // The engine's own move: roofs over the hero hide as when walking in.
            objectSetLocation(gDude, gHeroTile, gElevation, nullptr);
            // The move is instant: the egg carries no leftover scroll shift,
            // its mask stays glued to the hero's hex (as after a settled
            // scroll; the shift would displace it by the old scroll delta).
            gEgg->x = 0;
            gEgg->y = 0;
            gHeroTile = -1;
        }
        if (gViewTile >= 0 && !gViewTileApplied) {
            tileSetCenter(gViewTile, TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS
                | TILE_SET_CENTER_FLAG_IGNORE_BOUNDS | TILE_SET_CENTER_REFRESH_WINDOW);
            // The renderer initializes at the new center instead of gliding
            // from an entrance or an intermediate map-load presentation.
            scene_renderer::setStaticCameraCenterTile(-1);
            gViewTileApplied = true;
        }
        if (stillSnapshotRequested()) colorCycleDisable();
        gStartupFloorPreviewAttempted = true;
        const bool withWalls = gStartupWallPreviewRequested || gStartupHeroPreviewRequested || gStartupWalkPreviewRequested;
        const bool withHero = gStartupHeroPreviewRequested || gStartupWalkPreviewRequested;
        const bool explicitPreview = gStartupFloorPreviewRequested || gStartupWallPreviewRequested
            || gStartupHeroPreviewRequested || gStartupWalkPreviewRequested;
        const bool started = gClassic || !explicitPreview || enableStaticPreview(withWalls, withHero);
        if (started) {
            gStartupFloorPreviewFramesRemaining = gStartupFloorPreviewReturnAfter;
        }
        if (!started && gSnapshotOncePending && scene::enabled()) {
            gSnapshotOncePending = false;
            printf("devtools: one-shot preview snapshot skipped: preview did not start\n");
            fflush(stdout);
        }
    }

    // --dev-map-tour: a snapshot where the startup landed, then one on each
    // listed map elevation in turn, all in this process.
    if (gTourStep >= 0 && gStartupFloorPreviewAttempted && !gPendingElevation && !inBlockingGameState()) {
        bool arrived = gTourStep == 0;
        if (!arrived) {
            char base[64];
            stripMapExtension(gMapHeader.name, base, sizeof(base));
            const TourStop& stop = gTour[gTourStep - 1];
            arrived = compat_stricmp(base, stop.map.c_str()) == 0 && static_cast<int>(gElevation) == stop.elevation;
        }
        // A stop the game cannot reach (no free hex on an unused elevation) is
        // snapshotted where the game is, and the tour goes on.
        gTourFrames++;
        if (arrived && gTourFrames == 1 && gTourStep > 0 && gTour[gTourStep - 1].viewTile >= 0) {
            const TourStop& stop = gTour[gTourStep - 1];
            if (gFlying) stopFlying("", true);
            tileSetCenter(stop.viewTile, TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS
                | TILE_SET_CENTER_FLAG_IGNORE_BOUNDS | TILE_SET_CENTER_REFRESH_WINDOW);
            scene_renderer::setStaticCameraCenterTile(-1);
            if (stop.solo >= 0) scene_renderer::resoloStaticWalls(stop.viewTile, stop.solo);
        }
        // A batch view: the fly camera takes off once the view has settled.
        if (arrived && gTourFrames == 15 && gTourStep > 0 && !std::isnan(gTour[gTourStep - 1].turn)) {
            const TourStop& stop = gTour[gTourStep - 1];
            startFlying(false);
            if (gFlying) circleFlight(stop.turn == 0.0 ? 0.01 : stop.turn, -1.0, stop.distance);
        }
        const bool ready = arrived && gTourFrames >= 30;
        const bool stuck = !arrived && gTourFrames >= 600;
        if (ready || stuck) {
            gTourFrames = 0;
            if (stuck) {
                const TourStop& stop = gTour[gTourStep - 1];
                printf("devtools: map tour did not reach %s e%d: %s\n", stop.map.c_str(), stop.elevation, gStatus);
                fflush(stdout);
            }
            takeSnapshot();
            if (gTourStep < static_cast<int>(gTour.size())) {
                const TourStop& next = gTour[gTourStep++];
                char here[64];
                stripMapExtension(gMapHeader.name, here, sizeof(here));
                // Another view of this map elevation needs no transition.
                const bool sameMap = compat_stricmp(here, next.map.c_str()) == 0 && static_cast<int>(gElevation) == next.elevation;
                if (!sameMap && !requestMapChange(next.map.c_str(), next.elevation)) {
                    printf("devtools: map tour stopped at %s e%d: %s\n", next.map.c_str(), next.elevation, gStatus);
                    fflush(stdout);
                    gTourStep = -1;
                }
            } else {
                gTourStep = -1;
            }
        }
    }
}

bool preferencesSnapshotRequested()
{
    return gPreferencesSnapshotActive;
}

bool suppressInitialSceneEnable()
{
    return gClassic || gStartupFloorPreviewRequested || gStartupWallPreviewRequested
        || gStartupHeroPreviewRequested || gStartupWalkPreviewRequested;
}

bool sceneStartupReady()
{
    return !gEnabled || (gStartupTargetSettled && !gPendingElevation && !gStartupTargetAwaitingFinal);
}

void applyStartupTarget()
{
    if (gStartupApplied || !gEnabled) {
        return;
    }
    gStartupApplied = true;
    if (gStartupMap[0] != '\0') {
        gStartupTargetSettled = false;
        if (requestMapChange(gStartupMap, gStartupElevation)) {
            gStartupTargetAwaitingFinal = true;
        } else {
            gStartupTargetSettled = true;
        }
    }
}

bool requestMapChange(const char* mapFileName, int elevation)
{
    if (inBlockingGameState()) {
        setStatus("refused: combat/dialog/movie");
        return false;
    }

    char base[64];
    stripMapExtension(mapFileName, base, sizeof(base));

    // Map id -> file name via the worldmap registry; ids are 1-based.
    int mapId = -1;
    for (int id = 1; id <= 999; id++) {
        char name[64];
        if (wmMapIdxToName(static_cast<Map>(id), name, sizeof(name)) == 0) {
            char stripped[64];
            stripMapExtension(name, stripped, sizeof(stripped));
            if (compat_stricmp(stripped, base) == 0) {
                mapId = id;
                break;
            }
        }
    }

    if (mapId == -1) {
        snprintf(gStatus, sizeof(gStatus), "no such map: %s", base);
        return false;
    }

    if (elevation < 0 || elevation >= ELEVATION_COUNT) {
        snprintf(gStatus, sizeof(gStatus), "no such elevation: %d", elevation);
        return false;
    }

    bool sameMap = static_cast<int>(gMapHeader.index) == mapId;
    gPendingElevation = false;

    if (sameMap) {
        // Elevation change within one map: presence is known from the
        // current header - refuse up front when absent.
        if (elevation != static_cast<int>(gElevation) && !elevationPresent(elevation)) {
            snprintf(gStatus, sizeof(gStatus), "elevation %d absent on %s", elevation, base);
            return false;
        }
        if (!applyElevationChange(elevation)) {
            return false;
        }
        gPendingMapId = mapId;
        gPendingElevationValue = elevation;
        if (elevation == static_cast<int>(gElevation)) {
            setStatus("already on this elevation");
        } else {
            snprintf(gStatus, sizeof(gStatus), "moving to elevation %d", elevation);
        }
        return true;
    }

    // Different map: verify the requested elevation exists on the TARGET
    // map before leaving the current one (reading the map header from the
    // game resources). The follow-up correction below stays as a backstop.
    int probe = probeTargetElevation(base, elevation);
    if (probe == 1) {
        snprintf(gStatus, sizeof(gStatus), "elevation %d absent on %s", elevation, base);
        return false;
    }
    if (probe == -1) {
        // Unknown - stay on the current map instead of guessing.
        snprintf(gStatus, sizeof(gStatus), "cannot read map header: %s", base);
        return false;
    }

    // Load through the entrance (tile = -1).
    MapTransition transition;
    transition.map = static_cast<Map>(mapId);
    transition.elevation = static_cast<MapElevation>(elevation);
    transition.tile = -1;
    transition.rotation = static_cast<Rotation>(-1);

    if (mapSetTransition(&transition) == -1) {
        setStatus("transition rejected");
        return false;
    }

    gPendingElevation = true;
    gPendingMapId = mapId;
    gPendingElevationValue = elevation;
    snprintf(gStatus, sizeof(gStatus), "traveling to %s e%d", base, elevation);
    return true;
}

bool panelOpen()
{
    return gEnabled && gPanelOpen;
}

bool staticWalkPreviewActive()
{
    return gEnabled && gStartupWalkPreviewRequested && scene::activeView() == scene::View::Full;
}

void finishPickProbe()
{
    if (FILE* log = fopen("devprobe.log", "a")) {
        fprintf(log, "probe done: hero tile=%d busy=%d scene=%s\n", gDude->tile, animationIsBusy(gDude) == -1 ? 1 : 0,
            scene::availability());
        fclose(log);
    }
    gPickProbeExitPending = false;
    gPickProbeStage = 0;
}

// --dev-pick-probe, one step per presented frame of the scene.
void runPickProbe()
{
    static int probeX = -1;
    static int probeY = -1;
    static int probeDistance = 1 << 30;
    const int stage = gPickProbeStage++;
    FILE* log = nullptr;
    auto note = [&](const char* format, auto... values) {
        if (log == nullptr) log = fopen("devprobe.log", "a");
        if (log != nullptr) fprintf(log, format, values...);
    };
    if (stage == 30) {
        gPickProbeStartTile = gDude->tile;
        // The pointer a little above each critter's hex, as the scene shows it.
        int hits = 0, hidden = 0, misses = 0;
        const std::vector<scene::PickEntry> entries = scene::pickEntries();
        for (const scene::PickEntry& entry : entries) {
            Object* object = entry.object;
            if (object == gDude || FrmId(object).objectType() != OBJ_TYPE_CRITTER) continue;
            double pixel[2], x, y;
            scene_renderer::gameViewTilePixel(entry.tile, pixel);
            if (!scene_renderer::staticScreenFromGamePixel(pixel[0] + object->x, pixel[1] + object->y - 25.0, &x, &y)) continue;
            const int mx = static_cast<int>(x), my = static_cast<int>(y);
            if (mx < 0 || my < 0 || mx >= screenGetWidth() || my >= screenGetHeight() - 99) continue;
            Object* found = nullptr;
            scene::pickObject(mx, my, OBJ_TYPE_INVALID, true, &found);
            const char* verdict = found == object ? "hit" : found != nullptr ? "hidden" : "miss";
            (found == object ? hits : found != nullptr ? hidden : misses)++;
            note("probe %s at (%d,%d) tile=%d: %s -> %s\n", objectGetName(object), mx, my, entry.tile, verdict,
                found != nullptr ? objectGetName(found) : "-");
            // The critter nearest the hero is the one clicked.
            const int distance = tileDistanceBetween(gDude->tile, entry.tile);
            if (found == object && distance < probeDistance) {
                probeX = mx;
                probeY = my;
                probeDistance = distance;
            }
        }
        note("probe critters: hit=%d hidden=%d miss=%d\n", hits, hidden, misses);
        // The hex cursor first: the pointer on the floor beside the hero.
        double pixel[2], x, y;
        scene_renderer::gameViewTilePixel(gDude->tile, pixel);
        if (scene_renderer::staticScreenFromGamePixel(pixel[0] + 96.0, pixel[1] + 24.0, &x, &y)) {
            _mouse_set_position(static_cast<int>(x), static_cast<int>(y));
            const int tile = tileUnderMouse(static_cast<int>(x), static_cast<int>(y));
            note("probe floor at (%d,%d): tile=%d\n", static_cast<int>(x), static_cast<int>(y), tile);
        }
    } else if (stage == 60) {
        takeSnapshot();
        if (probeX >= 0) {
            gameMouseSetMode(GAME_MOUSE_MODE_ARROW);
            _mouse_set_position(probeX, probeY);
        }
    } else if (stage == 110) {
        Object* found = nullptr;
        if (probeX >= 0) scene::pickObject(probeX, probeY, OBJ_TYPE_INVALID, true, &found);
        note("probe hover at (%d,%d): %s\n", probeX, probeY, found != nullptr ? objectGetName(found) : "-");
        takeSnapshot();
        // Then a walk: the pointer on a floor hex a few steps from the hero.
        gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
        double pixel[2], x, y;
        const int target = tileGetTileInDirection(gDude->tile, ROTATION_SE, 4);
        scene_renderer::gameViewTilePixel(target, pixel);
        if (scene_renderer::staticScreenFromGamePixel(pixel[0], pixel[1], &x, &y)) {
            probeX = static_cast<int>(x);
            probeY = static_cast<int>(y);
            _mouse_set_position(probeX, probeY);
        }
        note("probe walk target tile=%d at (%d,%d)\n", target, probeX, probeY);
    } else if (stage == 115) {
        // A left click there, once the game has moved its hex cursor; the
        // game loop takes it (update()), as it takes the player's clicks.
        note("probe click: hex cursor tile=%d\n", gGameMouseHexCursor != nullptr ? gGameMouseHexCursor->tile : -1);
        gPickProbeClick[0] = probeX;
        gPickProbeClick[1] = probeY;
    } else if (stage == 200) {
        note("probe after click (frame %d): hero busy=%d tile=%d\n", stage, animationIsBusy(gDude) == -1 ? 1 : 0, gDude->tile);
    } else if (stage == 400) {
        note("probe after click (frame %d): hero busy=%d tile=%d\n", stage, animationIsBusy(gDude) == -1 ? 1 : 0, gDude->tile);
        if (animationIsBusy(gDude) == -1) {
            gPickProbeStage = 400;
            if (log != nullptr) fclose(log);
            return;
        }
        if (gDude->tile == gPickProbeStartTile) {
            note("outside probe failed: inside move did not move hero\n");
            finishPickProbe();
            if (log != nullptr) fclose(log);
            return;
        }
        note("inside movement control passed: hero=%d->%d\n", gPickProbeStartTile, gDude->tile);
        gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
        for (int direction = ROTATION_FIRST; direction <= ROTATION_LAST; direction++) {
            const int target = tileGetTileInDirection(gDude->tile, static_cast<Rotation>(direction), 1);
            if (target < 0 || _make_path(gDude, gDude->tile, target, nullptr, 1) <= 0) continue;
            double pixel[2], x, y;
            scene_renderer::gameViewTilePixel(target, pixel);
            if (!scene_renderer::staticScreenFromGamePixel(pixel[0], pixel[1], &x, &y)) continue;
            probeX = static_cast<int>(x);
            probeY = static_cast<int>(y);
            if (tileUnderMouse(probeX, probeY) == target) {
                gPickProbeEdgeTarget = target;
                break;
            }
        }
        _mouse_set_position(probeX, probeY);
        note("outside probe target=%d hero=%d\n", gPickProbeEdgeTarget, gDude->tile);
        if (gPickProbeEdgeTarget < 0) {
            finishPickProbe();
            if (log != nullptr) fclose(log);
            return;
        }
    } else if (stage == 401) {
        if (tileUnderMouse(probeX, probeY) != gPickProbeEdgeTarget || gGameMouseHexCursor == nullptr
            || gGameMouseHexCursor->tile != gPickProbeEdgeTarget) {
            note("outside probe failed: valid target not primed\n");
            finishPickProbe();
        } else {
            Rect area = {}, viewport = {};
            const int hasArea = mapEdgeComputeVisibleArea(gElevation, &area) && windowGetRect(gIsoWindow, &viewport) == 0;
            if (hasArea) {
                const int minX = viewport.left + 2, maxX = viewport.right - 3;
                const int minY = viewport.top + 2, maxY = viewport.bottom - 3;
                const int mx = std::clamp((area.left + area.right) / 2, minX, maxX);
                const int my = std::clamp((area.top + area.bottom) / 2, minY, maxY);
                const int xs[] = { std::clamp(area.left - 2, minX, maxX), std::clamp(area.right + 2, minX, maxX), mx, mx };
                const int ys[] = { my, my, std::clamp(area.top - 2, minY, maxY), std::clamp(area.bottom + 2, minY, maxY) };
                for (int i = 0; i < 4 && gPickProbeOutsideX < 0; i++) {
                    if (windowGetAtPoint(xs[i], ys[i]) == gIsoWindow && tileUnderMouse(xs[i], ys[i]) < 0) {
                        gPickProbeOutsideX = xs[i];
                        gPickProbeOutsideY = ys[i];
                    }
                }
            }
            if (gPickProbeOutsideX < 0) {
                note("outside probe failed: no candidate pick=-1\n");
                finishPickProbe();
            } else {
                gPickProbeOutsideHeroTile = gDude->tile;
                _mouse_set_position(gPickProbeOutsideX, gPickProbeOutsideY);
            }
        }
    } else if (stage == 402) {
        const int freshTile = tileUnderMouse(gPickProbeOutsideX, gPickProbeOutsideY);
        const int cursorTile = gGameMouseHexCursor != nullptr ? gGameMouseHexCursor->tile : -1;
        note("outside candidate pick=%d stale=%d hero=%d at (%d,%d)\n", freshTile, cursorTile, gDude->tile,
            gPickProbeOutsideX, gPickProbeOutsideY);
        if (freshTile < 0 && cursorTile == gPickProbeEdgeTarget && gDude->tile == gPickProbeOutsideHeroTile) {
            gPickProbeClick[0] = gPickProbeOutsideX;
            gPickProbeClick[1] = gPickProbeOutsideY;
            gPickProbeOutsideClick = true;
        } else {
            note("outside probe failed: stale target state changed\n");
            finishPickProbe();
        }
    } else if (stage == 430) {
        note("outside probe result: hero=%d expected=%d busy=%d\n", gDude->tile, gPickProbeOutsideHeroTile,
            animationIsBusy(gDude) == -1 ? 1 : 0);
        if (gDude->tile == gPickProbeOutsideHeroTile && animationIsBusy(gDude) != -1) {
            note("outside movement rejected\n");
        } else {
            note("outside probe failed: movement state changed\n");
            finishPickProbe();
        }
    } else if (stage == 500) {
        if (strcmp(gMapHeader.name, "KLAMALL.MAP") == 0) {
            constexpr int exitTile = 28078;
            Object* exitGrid = nullptr;
            for (Object* object = objectFindFirstAtLocation(gElevation, exitTile); object != nullptr;
                 object = objectFindNextAtLocation()) {
                if (isExitGridProtoId(ProtoId(object->pid))) {
                    exitGrid = object;
                    break;
                }
            }
            double pixel[2], x, y;
            const bool reachable = _make_path(gDude, gDude->tile, exitTile, nullptr, 1) > 0;
            scene_renderer::gameViewTilePixel(exitTile, pixel);
            const bool projected = scene_renderer::staticScreenFromGamePixel(pixel[0], pixel[1], &x, &y);
            const int mx = projected ? static_cast<int>(x) : -1;
            const int my = projected ? static_cast<int>(y) : -1;
            const bool picked = projected && windowGetAtPoint(mx, my) == gIsoWindow
                && tileUnderMouse(mx, my) == exitTile;
            if (exitGrid == nullptr || exitGrid->data.misc.map != 9 || exitGrid->data.misc.tile != 25142
                || exitGrid->data.misc.elevation != ELEVATION_FIRST || !reachable || !picked) {
                note("outside probe failed: exit not ready hero=%d target=%d grid=%d dest=%d/%d reachable=%d picked=%d\n",
                    gDude->tile, exitTile, exitGrid != nullptr, exitGrid != nullptr ? exitGrid->data.misc.map : -1,
                    exitGrid != nullptr ? exitGrid->data.misc.tile : -1, reachable, picked);
                finishPickProbe();
                if (log != nullptr) fclose(log);
                return;
            }
            gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
            _mouse_set_position(mx, my);
            gPickProbeClick[0] = mx;
            gPickProbeClick[1] = my;
            gPickProbeExitPending = true;
            gPickProbeExitWaitFrames = 0;
            note("exit probe queued: map=%s elevation=%d hero=%d exit=%d dest=%d/%d/%d reachable=%d picked=%d at (%d,%d)\n",
                gMapHeader.name, gElevation, gDude->tile, exitTile, exitGrid->data.misc.map,
                exitGrid->data.misc.tile, exitGrid->data.misc.elevation, reachable, picked, mx, my);
            if (log != nullptr) fclose(log);
            return;
        }
        // Then talk: the pointer on the nearest critter the hero can talk to.
        Object* talker = nullptr;
        int talkerX = -1, talkerY = -1, nearest = 1 << 30;
        for (const scene::PickEntry& entry : scene::pickEntries()) {
            Object* object = entry.object;
            if (object == gDude || FrmId(object).objectType() != OBJ_TYPE_CRITTER || !_obj_action_can_talk_to(object)) continue;
            const KillType kind = critterGetKillType(object);
            if (kind != KILL_TYPE_MAN && kind != KILL_TYPE_WOMAN) continue; // brahmin only moo
            double pixel[2], x, y;
            scene_renderer::gameViewTilePixel(entry.tile, pixel);
            if (!scene_renderer::staticScreenFromGamePixel(pixel[0] + object->x, pixel[1] + object->y - 25.0, &x, &y)) continue;
            Object* found = nullptr;
            scene::pickObject(static_cast<int>(x), static_cast<int>(y), OBJ_TYPE_INVALID, true, &found);
            const int distance = tileDistanceBetween(gDude->tile, entry.tile);
            if (found == object && distance < nearest) {
                talker = object;
                talkerX = static_cast<int>(x);
                talkerY = static_cast<int>(y);
                nearest = distance;
            }
        }
        note("probe talk target: %s at (%d,%d)\n", talker != nullptr ? objectGetName(talker) : "-", talkerX, talkerY);
        if (talker != nullptr) {
            gameMouseSetMode(GAME_MOUSE_MODE_ARROW);
            _mouse_set_position(talkerX, talkerY);
            gPickProbeClick[0] = talkerX;
            gPickProbeClick[1] = talkerY;
        }
    } else if (stage >= 1200 && !gPickProbeExitPending) {
        finishPickProbe(); // the click led nowhere the scene leaves for
    }
    if (log != nullptr) fclose(log);
}

void scenePresented(const scene::RefreshResult& refreshed)
{
    if (!sceneStartupReady()) return;
    if (scene::activeView() != scene::View::Full) return;
    // Frame rate of the 3D view, logged every two seconds: presented frames
    // per second and the share of each frame spent rebuilding live objects.
    static Uint64 windowStart = 0;
    static unsigned int windowFrames = 0;
    static double windowUpdateSeconds = 0.0;
    const Uint64 now = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    if (windowStart == 0) windowStart = now;
    windowFrames++;
    windowUpdateSeconds += refreshed.seconds;
    const double elapsed = static_cast<double>(now - windowStart) / frequency;
    if (elapsed >= 2.0) {
        // A file: the Windows build has no console to print to.
        if (FILE* log = fopen("devperf.log", "a")) {
            fprintf(log, "3d fps=%.1f objects=%u update_ms=%.2f\n", windowFrames / elapsed,
                scene_renderer::staticObjectCount(), windowUpdateSeconds * 1000.0 / windowFrames);
            fclose(log);
        }
        windowStart = now;
        windowFrames = 0;
        windowUpdateSeconds = 0.0;
    }
    if (refreshed.heroUpdated && gPickProbeStage > 0) {
        runPickProbe();
        return;
    }
    if (refreshed.heroUpdated && gDevCameraTurn != 0) {
        scene::setCameraTurnNow(gDevCameraTurn);
        gDevCameraTurn = 0;
        return; // the snapshot comes with a later frame, turned
    }
    if (refreshed.heroUpdated && gDevFly && !gFlying) {
        gDevFly = false;
        startFlying(false);
        if (gFlying && (gDevFlyTurn != 0.0 || gDevFlyPitch > 0.0 || gDevFlyDistance > 0.0)) {
            circleFlight(gDevFlyTurn, gDevFlyPitch, gDevFlyDistance);
        }
        return; // the snapshot comes with the next frame, from the air
    }
    if (refreshed.heroUpdated && gSnapshotOncePending) {
        gSnapshotOncePending = false;
        const bool captured = takeSnapshot();
        printf("devtools: one-shot hero preview snapshot %s: %s\n",
            captured ? "saved" : "failed", gStatus);
        fflush(stdout);
    }
}

void vanillaPresented()
{
    if (!gSnapshotOncePending || !gEnabled || !sceneStartupReady() || !gStartupFloorPreviewAttempted
        || scene::activeView() != scene::View::Off) {
        return;
    }
    gSnapshotOncePending = false;
    const bool captured = takeSnapshot();
    printf("devtools: one-shot vanilla snapshot %s: %s\n", captured ? "saved" : "failed", gStatus);
    fflush(stdout);
}

void sceneOverlaySynced(bool on)
{
    // --dev-pick-probe: the click took the game out of the scene (a dialog).
    if (!on && gPickProbeStage > 115 && !gPickProbeExitPending) finishPickProbe();
}

// Ctrl+F11: the window between 16:9 and 21:9. The engine sizes its screen once
// at start, so tools/dev-launch.ps1 starts the game again in the other shape as
// soon as this one has quit.
void restartInOtherAspect()
{
#if defined(_WIN32)
    const bool wide = settings.screen.resolution_x * 9 > settings.screen.resolution_y * 17;
    const char* aspect = wide ? "16:9" : "21:9";
    const std::filesystem::path launcher = std::filesystem::current_path().parent_path() / "tools" / "dev-launch.ps1";
    std::error_code error;
    if (!std::filesystem::exists(launcher, error)) {
        setStatus("window shape switch needs tools/dev-launch.ps1");
        return;
    }
    std::string command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \""
        + launcher.string() + "\" -Aspect " + aspect + " -WaitPid " + std::to_string(GetCurrentProcessId());
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
            &startup, &process)) {
        setStatus("window shape switch could not start the launcher");
        return;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    printf("devtools: restarting in %s\n", aspect);
    fflush(stdout);
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_EXIT;
#else
    setStatus("window shape switch is Windows only");
#endif
}

void landFlight()
{
    stopFlying("fly camera landed: the 3D scene is not shown", false);
}

void gameReset()
{
    gFlyRestorePending = false;
}

bool handleKey(int key)
{
    if (!gEnabled) {
        return false;
    }

    if (key == KEY_CTRL_F12) {
        if (gFlying) stopFlying("fly camera landed", true);
        else startFlying(true);
        return true;
    }
    // A flying camera owns the keyboard: the game gets no key.
    if (gFlying) {
        if (key == KEY_ESCAPE) stopFlying("fly camera landed", true);
        if (key == KEY_F9) takeSnapshot();
        return true;
    }

    if (key == KEY_CTRL_F10) {
        if (!scene::enabled()) {
            if (!setSceneSettingEnabled(true)) return true;
            std::string message;
            if (!scene::enable(scene::View::Full, &message)) setStatus(message.c_str());
            else setStatus("3D scene on (Ctrl+F10 to return)");
            return true;
        }
        if (scene::requestedView() != scene::View::Off) {
            disableStaticFloorPreview("3D scene off");
            return true;
        }
        // Back to the whole 3D scene, as the game starts it.
        enableStaticPreview(true, true);
        return true;
    }

    if (key == KEY_CTRL_F11) {
        restartInOtherAspect();
        return true;
    }

    if (key == KEY_ALT_D) {
        if (!gPanelOpen) {
            rebuildMapList();
        }
        gPanelOpen = !gPanelOpen;
        setStatus(gPanelOpen ? "panel opened" : "panel closed");
        return true;
    }

    if (!gPanelOpen) {
        if (key == KEY_F9) {
            takeSnapshot();
            return true;
        }
        return false;
    }

    switch (key) {
    case KEY_ARROW_UP:
        if (gSelected > 0) {
            gSelected--;
        }
        return true;
    case KEY_ARROW_DOWN:
        if (gSelected < static_cast<int>(gFiltered.size()) - 1) {
            gSelected++;
        }
        return true;
    case KEY_ARROW_LEFT:
        if (gSelectedElevation > 0) {
            gSelectedElevation--;
        }
        return true;
    case KEY_ARROW_RIGHT:
        if (gSelectedElevation < ELEVATION_COUNT - 1) {
            gSelectedElevation++;
        }
        return true;
    case KEY_RETURN:
        if (!gFiltered.empty()) {
            requestMapChange(gMaps[gFiltered[gSelected]].c_str(), gSelectedElevation);
        } else {
            setStatus("no map selected");
        }
        return true;
    case KEY_ESCAPE:
        gPanelOpen = false;
        setStatus("panel closed");
        return true;
    case KEY_BACKSPACE:
        if (gFilter[0] != '\0') {
            gFilter[strlen(gFilter) - 1] = '\0';
            refreshFiltered();
        }
        return true;
    case KEY_TAB:
        return true; // keep TAB (inventory) from leaking while typing
    case KEY_F9:
        takeSnapshot();
        return true;
    case 0x31:
    case 0x32:
    case 0x33:
        // Digits pick the wanted elevation (arrows do not survive the
        // scroll layer in every input path).
        gSelectedElevation = key - 0x31;
        setStatus("elevation changed");
        return true;
    default:
        break;
    }

    int ch = key & 0xFF;
    if (key >= KEY_SPACE && key < 0x0100 && ch >= 0x20 && ch < 0x7F) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = ch - 'A' + 'a';
        }
        size_t len = strlen(gFilter);
        if (len + 1 < sizeof(gFilter)) {
            gFilter[len] = static_cast<char>(ch);
            gFilter[len + 1] = '\0';
            refreshFiltered();
        }
        return true;
    }

    return false;
}

bool handleClick(int mouseX, int mouseY)
{
    if (!gEnabled || !gPanelOpen || !gZonesValid) {
        return false;
    }

    if (!hitTest({ kPanelX, kPanelY, kPanelW, kPanelH }, mouseX, mouseY)) {
        return false;
    }

    if (hitTest(gListRect, mouseX, mouseY)) {
        // The draw starts kListRows/2 rows above the selection - mirror it,
        // using the row height that was captured while drawing (font 101),
        // not whatever font is active at click time.
        if (gListRowHeight <= 0) {
            return true;
        }
        int row = (mouseY - gListRect.y) / gListRowHeight;
        int idx = gSelected - kListRows / 2 + row;
        if (idx >= 0 && idx < static_cast<int>(gFiltered.size())) {
            gSelected = idx;
        }
        return true;
    }

    if (hitTest(gElevRect, mouseX, mouseY)) {
        int e = (mouseX - gElevRect.x) / 24;
        if (e < 0) {
            e = 0;
        }
        if (e > ELEVATION_COUNT - 1) {
            e = ELEVATION_COUNT - 1;
        }
        gSelectedElevation = e;
        setStatus("elevation changed");
        return true;
    }

    if (hitTest(gPrevRect, mouseX, mouseY)) {
        if (!gFiltered.empty()) {
            gSelected = (gSelected - 1 + static_cast<int>(gFiltered.size())) % static_cast<int>(gFiltered.size());
        }
        return true;
    }

    if (hitTest(gNextRect, mouseX, mouseY)) {
        if (!gFiltered.empty()) {
            gSelected = (gSelected + 1) % static_cast<int>(gFiltered.size());
        }
        return true;
    }

    if (hitTest(gGoRect, mouseX, mouseY)) {
        if (!gFiltered.empty()) {
            requestMapChange(gMaps[gFiltered[gSelected]].c_str(), gSelectedElevation);
        } else {
            setStatus("no map selected");
        }
        return true;
    }

    if (hitTest(gSnapRect, mouseX, mouseY)) {
        takeSnapshot();
        return true;
    }

    return true; // inside panel: consumed regardless
}

void draw(unsigned char* surface, int width, int height, int pitch)
{
    if (!gEnabled || !gPanelOpen) {
        gZonesValid = false;
        return;
    }
    if (surface == nullptr || width < kPanelX + kPanelW || height < kPanelY + kPanelH) {
        gZonesValid = false;
        return;
    }

    ScopedFont scopedFont(101);
    int lineHeight = fontGetLineHeight();
    int rowHeight = lineHeight + 2;
    gListRowHeight = rowHeight;

    bufferFill(surface + pitch * kPanelY + kPanelX, kPanelW, kPanelH, pitch, COLOR_BLACK);
    for (int x = kPanelX; x < kPanelX + kPanelW; x++) {
        surface[pitch * kPanelY + x] = COLOR_GREY;
        surface[pitch * (kPanelY + kPanelH - 1) + x] = COLOR_GREY;
    }
    for (int y = kPanelY; y < kPanelY + kPanelH; y++) {
        surface[pitch * y + kPanelX] = COLOR_GREY;
        surface[pitch * y + kPanelX + kPanelW - 1] = COLOR_GREY;
    }

    char line[128];
    int y = kPanelY + 4;

    snprintf(line, sizeof(line), "DEV TOOLS  (Alt+D close)");
    fontDrawText(surface + pitch * y + kPanelX + 6, line, kPanelW - 12, pitch, DT_TITLE_COLOR);
    y += lineHeight + 1;

    char mapName[32];
    currentMapName(mapName, sizeof(mapName));
    snprintf(line, sizeof(line), "map: %s e%d (want e%d)", mapName,
        static_cast<int>(gElevation), gSelectedElevation);
    fontDrawText(surface + pitch * y + kPanelX + 6, line, kPanelW - 12, pitch, DT_TEXT_COLOR);
    y += lineHeight;

    snprintf(line, sizeof(line), "render: %s  light: off", activeRendererName());
    fontDrawText(surface + pitch * y + kPanelX + 6, line, kPanelW - 12, pitch, DT_ACTIVE_COLOR);
    y += lineHeight + 1;

    snprintf(line, sizeof(line), "filter: %s_", gFilter);
    fontDrawText(surface + pitch * y + kPanelX + 6, line, kPanelW - 12, pitch, DT_TEXT_COLOR);
    y += lineHeight + 1;

    gListRect = textButton(kPanelX + 4, y, kPanelW - 8, kListRows * rowHeight);
    for (int row = 0; row < kListRows; row++) {
        int idx = gSelected - kListRows / 2 + row;
        if (idx < 0 || idx >= static_cast<int>(gFiltered.size())) {
            continue;
        }
        ColorWithFlags color = idx == gSelected ? ColorWithFlags{ DT_HIGHLIGHT_COLOR } : ColorWithFlags{ DT_TEXT_COLOR };
        char base[64];
        stripMapExtension(gMaps[gFiltered[idx]].c_str(), base, sizeof(base));
        snprintf(line, sizeof(line), "%s%s", idx == gSelected ? "> " : "  ", base);
        fontDrawText(surface + pitch * (y + row * rowHeight) + kPanelX + 6, line, kPanelW - 12, pitch, color);
    }
    if (gFiltered.empty()) {
        fontDrawText(surface + pitch * y + kPanelX + 6, "(no maps match)", kPanelW - 12, pitch, DT_ERROR_COLOR);
    }
    y += kListRows * rowHeight + 1;

    // Elevation row: the click zone starts where the labels start.
    int elevX = kPanelX + 6 + 40;
    snprintf(line, sizeof(line), "elev:");
    fontDrawText(surface + pitch * y + kPanelX + 6, line, 40, pitch, DT_TEXT_COLOR);
    gElevRect = textButton(elevX, y, ELEVATION_COUNT * 24, lineHeight + 2);
    for (int e = 0; e < ELEVATION_COUNT; e++) {
        bool present = elevationPresent(e);
        snprintf(line, sizeof(line), "[%d]%s", e, present ? "" : "-");
        fontDrawText(surface + pitch * y + elevX + e * 24, line, 24, pitch,
            e == gSelectedElevation ? ColorWithFlags{ DT_HIGHLIGHT_COLOR }
                                    : (present ? ColorWithFlags{ DT_TEXT_COLOR } : ColorWithFlags{ COLOR_GREY }));
    }
    y += lineHeight + 2;

    int bw = (kPanelW - 16) / 4;
    gPrevRect = textButton(kPanelX + 4, y, bw, lineHeight + 2);
    gNextRect = textButton(kPanelX + 4 + bw, y, bw, lineHeight + 2);
    gGoRect = textButton(kPanelX + 4 + 2 * bw, y, bw, lineHeight + 2);
    gSnapRect = textButton(kPanelX + 4 + 3 * bw, y, bw, lineHeight + 2);
    fontDrawText(surface + pitch * y + gPrevRect.x + 2, "<prev", bw - 4, pitch, DT_TEXT_COLOR);
    fontDrawText(surface + pitch * y + gNextRect.x + 2, "next>", bw - 4, pitch, DT_TEXT_COLOR);
    fontDrawText(surface + pitch * y + gGoRect.x + 2, "GO", bw - 4, pitch, DT_ACTIVE_COLOR);
    fontDrawText(surface + pitch * y + gSnapRect.x + 2, "SNAP", bw - 4, pitch, DT_TEXT_COLOR);
    y += lineHeight + 2;

    snprintf(line, sizeof(line), "%s", gStatus);
    fontDrawText(surface + pitch * y + kPanelX + 6, line, kPanelW - 12, pitch, COLOR_LIGHT_GREY);
    gZonesValid = true;
}

void currentMapName(char* buffer, size_t size)
{
    if (gMapHeader.name[0] != '\0') {
        snprintf(buffer, size, "%s", gMapHeader.name);
    } else {
        snprintf(buffer, size, "(none)");
    }
}

const char* activeRendererName()
{
    if (scene_renderer::isActive()) {
        return "scene-gl";
    }
    return "sdl-renderer";
}

bool takeSnapshot()
{
    namespace fs = std::filesystem;
    const bool previewRequested = scene::activeView() != scene::View::Off;
    const bool preview = scene_renderer::isActive() && scene::shouldPresent();
    const bool heroPreview = preview && scene::activeView() == scene::View::Full;
    const bool wallPreview = preview && scene::activeView() == scene::View::Walls;
    const bool withWalls = wallPreview || heroPreview;
    if (!preview && (gSdlSurface == nullptr || gSdlSurface->pixels == nullptr)) {
        setStatus("snapshot: no framebuffer");
        return false;
    }

    char stamp[32] = {};
    {
        std::time_t t = time(nullptr);
        struct tm* tmv = localtime(&t);
        if (tmv == nullptr || strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", tmv) == 0) {
            setStatus("snapshot: clock unavailable");
            return false;
        }
    }

    fs::path imagePath;
    fs::path sidecarPath;
    std::string imageFile;
    int capturedWidth = 0;
    int capturedHeight = 0;
    try {
        const fs::path outputDirectory = fs::current_path();
        if (preview) {
            for (int suffix = 0; suffix < 1000; suffix++) {
                char stem[64];
                snprintf(stem, sizeof(stem), "%s-%s-%03d",
                    staticWalkPreviewActive() ? "devwalk" : heroPreview ? "devhero" : wallPreview ? "devwall" : "devfloor", stamp, suffix);
                fs::path candidateImage = outputDirectory / (std::string(stem) + ".png");
                fs::path candidateSidecar = outputDirectory / (std::string("devshot-") + stamp + "-" +
                    (suffix < 10 ? "00" : suffix < 100 ? "0" : "") + std::to_string(suffix) + ".json");
                if (!fs::exists(candidateImage) && !fs::exists(candidateSidecar)) {
                    imagePath = candidateImage;
                    sidecarPath = candidateSidecar;
                    imageFile = candidateImage.filename().string();
                    break;
                }
            }
            if (imagePath.empty()) {
                setStatus("snapshot: no unique filename available");
                return false;
            }
            std::string error;
            if (!scene_renderer::captureStaticPreviewPng(imagePath, withWalls, heroPreview,
                    &capturedWidth, &capturedHeight, &error)) {
                std::string message = "snapshot failed: " + error;
                setStatus(message.c_str());
                return false;
            }
        } else {
            // The vanilla snapshot handler uses the first free scrNNNNN.png.
            int index = 0;
            char name[16];
            for (; index < 100000; index++) {
                snprintf(name, sizeof(name), "scr%.5d.png", index);
                if (!fs::exists(outputDirectory / name)) break;
            }
            if (index == 100000) {
                setStatus("snapshot: no screenshot filename available");
                return false;
            }
            imagePath = outputDirectory / name;
            imageFile = name;
            if (scene_renderer::isActive()) {
                // As the window shows it, the same size as scene snapshots.
                SDL_BlitSurface(gSdlSurface, nullptr, gSdlTextureSurface, nullptr);
                std::string error;
                if (!scene_renderer::captureFramePng(imagePath, static_cast<const unsigned char*>(gSdlTextureSurface->pixels),
                        gSdlTextureSurface->w, gSdlTextureSurface->h, gSdlTextureSurface->pitch,
                        &capturedWidth, &capturedHeight, &error)) {
                    std::string message = "snapshot failed: " + error;
                    setStatus(message.c_str());
                    return false;
                }
            } else {
                unsigned char* palette = directDrawGetPalette();
                std::vector<unsigned char> tightlyPacked(static_cast<size_t>(gSdlSurface->w) * gSdlSurface->h);
                if (!copySnapshotRowsTightly(static_cast<const unsigned char*>(gSdlSurface->pixels),
                        gSdlSurface->w, gSdlSurface->h, gSdlSurface->pitch,
                        tightlyPacked.data(), tightlyPacked.size())) {
                    setStatus("snapshot: invalid framebuffer row pitch");
                    return false;
                }
                if (screenshotHandlerPngImpl(gSdlSurface->w, gSdlSurface->h,
                        tightlyPacked.data(), palette) != 0) {
                    setStatus("snapshot: vanilla PNG write failed");
                    return false;
                }
                capturedWidth = gSdlSurface->w;
                capturedHeight = gSdlSurface->h;
            }
            char sidecar[64];
            snprintf(sidecar, sizeof(sidecar), "devshot-%s.json", stamp);
            sidecarPath = outputDirectory / sidecar;
            for (int suffix = 1; fs::exists(sidecarPath) && suffix < 1000; suffix++) {
                snprintf(sidecar, sizeof(sidecar), "devshot-%s-%03d.json", stamp, suffix);
                sidecarPath = outputDirectory / sidecar;
            }
        }
    } catch (const std::exception& exception) {
        std::string message = std::string("snapshot failed: ") + exception.what();
        setStatus(message.c_str());
        return false;
    }

    char exePath[MAX_PATH + 1] = {};
#if defined(_WIN32)
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
#endif
    char exeSha[80];
    sha256File(exePath, exeSha, sizeof(exeSha));

    char mapName[32];
    currentMapName(mapName, sizeof(mapName));
    char camera[256];
    if (preview) {
        const scene_renderer::StaticPreviewCamera info = scene_renderer::staticPreviewCamera(withWalls);
        snprintf(camera, sizeof(camera), "fixed eye=(%.5f,%.5f,%.5f), target=(%.5f,%.5f,%.5f), fov_y=%.1f, near=%.1f, far=%.1f, screen_right=%s",
            info.eye[0], info.eye[1], info.eye[2], info.target[0], info.target[1], info.target[2],
            info.fovY, info.nearPlane, info.farPlane,
            info.donorScreenRight ? "-cross(forward,world_up)" : "cross(forward,world_up)");
    } else {
        snprintf(camera, sizeof(camera), "center tile %d", gCenterTile);
    }
    int drawableW = capturedWidth;
    int drawableH = capturedHeight;
    unsigned int wallTriangles = 0;
    unsigned int wallPages = 0;
    if (withWalls) scene_renderer::staticWallStats(&wallTriangles, &wallPages);
    scene_renderer::StaticHeroInfo heroInfo = {};
    const bool haveHeroInfo = heroPreview && scene_renderer::staticHeroInfo(&heroInfo);
    char heroMetadata[256];
    if (haveHeroInfo) {
        snprintf(heroMetadata, sizeof(heroMetadata),
            "{\"tile\":%d,\"fid\":%d,\"frame\":%d,\"rotation\":%d,\"world_anchor\":[%.9f,%.9f]}",
            heroInfo.tile, heroInfo.fid, heroInfo.frame, heroInfo.rotation, heroInfo.worldU, heroInfo.worldV);
    } else {
        snprintf(heroMetadata, sizeof(heroMetadata), "null");
    }
    int eggLeft = 0, eggTop = 0, eggWidth = 0, eggHeight = 0;
    double eggZoom = 1.0;
    const bool haveEgg = scene_renderer::staticEggInfo(&eggLeft, &eggTop, &eggWidth, &eggHeight, &eggZoom);
    int wallSeeThroughRows = 0;
    bool wallSeeThroughAny = false;
    scene_renderer::staticWallSeeThroughStats(&wallSeeThroughRows, &wallSeeThroughAny);
    char eggMetadata[256];
    if (haveEgg) {
        snprintf(eggMetadata, sizeof(eggMetadata),
            "{\"shown\":true,\"rect\":[%d,%d,%d,%d],\"zoom\":%.4f,\"wall_rows_marked\":%d,\"wall_rows_any\":%s}",
            eggLeft, eggTop, eggWidth, eggHeight, eggZoom, wallSeeThroughRows, wallSeeThroughAny ? "true" : "false");
    } else {
        snprintf(eggMetadata, sizeof(eggMetadata),
            "{\"shown\":false,\"wall_rows_marked\":%d,\"wall_rows_any\":%s}",
            wallSeeThroughRows, wallSeeThroughAny ? "true" : "false");
    }
    const scene_renderer::MapClip mapClip = scene_renderer::staticMapClip();
    const char* clipSource = mapClip.source == scene_renderer::MapClipSource::Authored ? "authored"
        : mapClip.source == scene_renderer::MapClipSource::Floor ? "floor"
        : mapClip.source == scene_renderer::MapClipSource::FloorGridFallback ? "floor-grid-fallback" : "disabled";
    const char* clipVariant = "off";
    if (scene::cleanEdges()) {
        const char* requested = isEnabled() ? SDL_getenv("F2CE_DEV_CLEAN_EDGES") : nullptr;
        clipVariant = requested != nullptr && strcmp(requested, "floor") == 0 ? "floor" : "authored";
    }
    char clipMetadata[768];
    if (mapClip.enabled) {
        snprintf(clipMetadata, sizeof(clipMetadata),
            "{\"enabled\":true,\"setting_enabled\":%s,\"variant\":\"%s\",\"source\":\"%s\","
            "\"world_bounds\":[%.6f,%.6f,%.6f,%.6f],\"planes\":[[%.7f,%.7f,%.7f],[%.7f,%.7f,%.7f],"
            "[%.7f,%.7f,%.7f],[%.7f,%.7f,%.7f]]}",
            scene::cleanEdges() ? "true" : "false", clipVariant, clipSource,
            mapClip.left, mapClip.top, mapClip.right, mapClip.bottom,
            mapClip.planes[0].x, mapClip.planes[0].z, mapClip.planes[0].offset,
            mapClip.planes[1].x, mapClip.planes[1].z, mapClip.planes[1].offset,
            mapClip.planes[2].x, mapClip.planes[2].z, mapClip.planes[2].offset,
            mapClip.planes[3].x, mapClip.planes[3].z, mapClip.planes[3].offset);
    } else {
        snprintf(clipMetadata, sizeof(clipMetadata),
            "{\"enabled\":false,\"setting_enabled\":%s,\"variant\":\"%s\",\"source\":\"disabled\"}",
            scene::cleanEdges() ? "true" : "false", clipVariant);
    }
    if (!preview && gSdlWindow != nullptr) {
        SDL_GL_GetDrawableSize(gSdlWindow, &drawableW, &drawableH);
    }
    // Preview snapshots come before the first frame that would update it.
    scene::updateSunFromGame();
    const scene::Sun& sun = scene::currentSun();
    char sunMetadata[384];
    snprintf(sunMetadata, sizeof(sunMetadata),
        "{\"setting\":%s,\"daylight\":%s,\"game_time\":%u,\"up\":%s,\"height\":%.3f,\"azimuth\":%.3f,"
        "\"direction\":[%.5f,%.5f,%.5f],\"color\":[%.4f,%.4f,%.4f],\"strength\":%.4f}",
        settings.scene3d.sun ? "true" : "false", scene::sceneDaylight() ? "true" : "false", gameTimeGetTime(),
        sun.up ? "true" : "false", sun.height, sun.azimuth, sun.direction[0], sun.direction[1], sun.direction[2],
        sun.color[0], sun.color[1], sun.color[2], sun.strength);

    FILE* f = fopen(sidecarPath.string().c_str(), "w");
    if (f == nullptr) {
        if (preview) {
            std::error_code ec;
            fs::remove(imagePath, ec);
        }
        setStatus("snapshot: metadata write failed");
        return false;
    }
    fprintf(f,
        "{\n"
        "  \"image\": \"%s\",\n"
        "  \"view\": \"%s\",\n"
        "  \"map\": \"%s\",\n"
        "  \"elevation\": %d,\n"
        "  \"still_snapshot\": %s,\n"
        "  \"snapshot_run\": \"%s\",\n"
        "  \"view_tile\": %d,\n"
        "  \"preview_requested\": %s,\n"
        "  \"scene_3d_enabled\": %s,\n"
        "  \"scene_view\": \"%s\",\n"
        "  \"map_clip\": %s,\n"
        "  \"camera\": \"%s\",\n"
        "  \"wall_triangles\": %u,\n"
        "  \"wall_pages\": %u,\n"
        "  \"hero\": %s,\n"
        "  \"egg\": %s,\n"
        "  \"objects\": {\"drawn\":%u,\"skipped\":%u},\n"
        "  \"preview_availability\": \"%s\",\n"
        "  \"renderer\": \"%s\",\n"
        "  \"light\": \"off\",\n"
        "  \"sun\": %s,\n"
        "  \"resolution\": \"%dx%d\",\n"
        "  \"logical_resolution\": \"%dx%d\",\n"
        "  \"scale\": %d,\n"
        "  \"windowed\": %s,\n"
        "  \"drawable\": \"%dx%d\",\n"
        "  \"ce_commit\": \"%s\",\n"
        "  \"tools_commit\": \"%s\",\n"
        "  \"exe_sha256\": \"%s\",\n"
        "  \"taken\": \"%s\"\n"
        "}\n",
        imageFile.c_str(),
        heroPreview ? "static-wall-plus-hero-preview" : wallPreview ? "static-wall-preview"
            : preview ? "static-floor-preview" : "vanilla-framebuffer",
        mapName,
        static_cast<int>(gElevation),
        stillSnapshotRequested() ? "true" : "false",
        gSnapshotRun,
        gCenterTile,
        previewRequested ? "true" : "false",
        scene::enabled() ? "true" : "false",
        scene::activeView() == scene::View::Full ? "full"
            : scene::activeView() == scene::View::Walls ? "walls"
            : scene::activeView() == scene::View::Floor ? "floor" : "off",
        clipMetadata,
        camera,
        wallTriangles,
        wallPages,
        heroMetadata,
        eggMetadata,
        heroPreview ? scene_renderer::staticObjectCount() : 0u,
        heroPreview ? scene::objectCardsSkipped() : 0u,
        previewRequested ? (scene_renderer::isActive() ? scene::availability() : "renderer-inactive") : "not-requested",
        activeRendererName(),
        sunMetadata,
        capturedWidth,
        capturedHeight,
        screenGetWidth(),
        screenGetHeight(),
        settings.screen.scale,
        settings.screen.windowed == WindowMode::Windowed ? "true" : "false",
        drawableW,
        drawableH,
        _BUILD_HASH,
        gToolsCommit,
        exeSha,
        stamp);
    const bool metadataOk = fclose(f) == 0;
    if (!metadataOk) {
        std::error_code ec;
        fs::remove(sidecarPath, ec);
        if (preview) fs::remove(imagePath, ec);
        setStatus("snapshot: metadata write failed");
        return false;
    }

    snprintf(gStatus, sizeof(gStatus), "snapshot: %s", sidecarPath.filename().string().c_str());
    return true;
}

} // namespace devtools
} // namespace fallout
