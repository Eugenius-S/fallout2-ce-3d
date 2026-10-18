#include "devtools.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cctype>

#include <algorithm>
#include <string>
#include <vector>

#include "../art.h"
#include "../color.h"
#include "../combat.h"
#include "../db.h"
#include "../dialog.h"
#include "../draw.h"
#include "../input.h"
#include "../kb.h"
#include "../map.h"
#include "../map_defs.h"
#include "../movie.h"
#include "../platform/git_version.h"
#include "../platform_compat.h"
#include "../scene_renderer/scene_gl.h"
#include "../settings.h"
#include "../svga.h"
#include "../text_font.h"
#include "../tile.h"
#include "../worldmap.h"

#include <SDL.h>

#if defined(_WIN32)
#include <windows.h>
#include <filesystem>
#endif

namespace fallout {
namespace devtools {

namespace {

// --- state -----------------------------------------------------------------

bool gEnabled = false;
bool gListMaps = false;
char gStartupMap[16] = {};
int gStartupElevation = 0;
bool gStartupApplied = false;
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
    if (len > 4 && compat_stricmp(name + len - 4, ".map") == 0) {
        len -= 4;
    }
    if (len >= size) {
        len = size - 1;
    }
    memcpy(out, name, len);
    out[len] = '\0';
}

// True when the CURRENT map has tile data on this elevation. CE's map
// header marks an elevation with a SET bit when it has NO data: the loader
// reads an elevation's tiles only while (flags & bit) == 0.
bool elevationPresent(int elevation)
{
    if (elevation < 0 || elevation >= ELEVATION_COUNT) {
        return false;
    }
    static constexpr int kElevFlags[ELEVATION_COUNT] = {
        MAP_HEADER_ELEVATION_0,
        MAP_HEADER_ELEVATION_1,
        MAP_HEADER_ELEVATION_2,
    };
    return (gMapHeader.flags & kElevFlags[elevation]) == 0;
}

// Presence of an elevation on a NOT-loaded map, read from the map header in
// the game resources (SAV preferred, like mapLoadByName). 0 = present,
// 1 = absent, -1 = unknown (file missing or too short).
int probeTargetElevation(const char* base, int elevation)
{
    if (elevation < 0 || elevation >= ELEVATION_COUNT) {
        return -1;
    }
    static constexpr int kElevFlags[ELEVATION_COUNT] = {
        MAP_HEADER_ELEVATION_0,
        MAP_HEADER_ELEVATION_1,
        MAP_HEADER_ELEVATION_2,
    };

    for (int attempt = 0; attempt < 2; attempt++) {
        // mapLoadByName prefers the saved version of a visited map.
        char fileName[16];
        snprintf(fileName, sizeof(fileName), "%s.%s", base, attempt == 0 ? "SAV" : "MAP");
        const char* path = mapBuildPath(fileName);
        File* stream = fileOpen(path, "rb");
        if (stream == nullptr) {
            continue;
        }

        // MapHeader prefix: version, name[16], enteringTile,
        // enteringElevation, enteringRotation, localVariablesCount,
        // scriptIndex, flags.
        int version = 0;
        int nameInts[4] = {};
        int enteringTile = 0;
        int enteringElevation = 0;
        int enteringRotation = 0;
        int localVariablesCount = 0;
        int scriptIndex = 0;
        int flags = 0;
        bool ok = fileReadInt32(stream, &version) == 0
            && fileReadInt32(stream, &nameInts[0]) == 0
            && fileReadInt32(stream, &nameInts[1]) == 0
            && fileReadInt32(stream, &nameInts[2]) == 0
            && fileReadInt32(stream, &nameInts[3]) == 0
            && fileReadInt32(stream, &enteringTile) == 0
            && fileReadInt32(stream, &enteringElevation) == 0
            && fileReadInt32(stream, &enteringRotation) == 0
            && fileReadInt32(stream, &localVariablesCount) == 0
            && fileReadInt32(stream, &scriptIndex) == 0
            && fileReadInt32(stream, &flags) == 0;
        fileClose(stream);
        if (!ok) {
            return -1;
        }
        (void)enteringTile;
        (void)enteringElevation;
        (void)enteringRotation;
        (void)localVariablesCount;
        (void)scriptIndex;
        return (flags & kElevFlags[elevation]) == 0 ? 0 : 1;
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

    // The dude's current tile may be a wall or a void on the target
    // elevation - pick a non-blocked hex there, spiraling out from the
    // dude's position. Falls back to the map's entering tile (whose stored
    // value can be 0/-1, which the engine treats as "skip placement").
    int targetTile = -1;
    if (gDude->tile != -1 && _obj_blocking_at(nullptr, gDude->tile, static_cast<MapElevation>(elevation)) == nullptr) {
        targetTile = gDude->tile;
    } else {
        for (int distance = 1; distance <= 4 && targetTile == -1; distance++) {
            for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
                int candidate = tileGetTileInDirection(gDude->tile, static_cast<Rotation>(rotation), distance);
                if (candidate != -1
                    && _obj_blocking_at(nullptr, candidate, static_cast<MapElevation>(elevation)) == nullptr) {
                    targetTile = candidate;
                    break;
                }
            }
        }
    }
    if (targetTile == -1) {
        targetTile = gMapHeader.enteringTile;
    }
    if (targetTile <= 0) {
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

bool isEnabled()
{
    return gEnabled;
}

bool wantsListMaps()
{
    return gListMaps;
}

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

// Called every frame from the presentation hook: applies the pending
// elevation correction once the requested map has finished loading.
void update()
{
    if (!gEnabled || !gPendingElevation) {
        return;
    }
    if (inBlockingGameState()) {
        return;
    }
    if (static_cast<int>(gMapHeader.index) != gPendingMapId) {
        return; // the requested map has not finished loading yet
    }

    gPendingElevation = false;
    if (gPendingElevationValue == static_cast<int>(gElevation)) {
        return; // already there
    }
    if (!elevationPresent(gPendingElevationValue)) {
        snprintf(gStatus, sizeof(gStatus), "elevation %d absent on this map",
            gPendingElevationValue);
        return;
    }
    if (applyElevationChange(gPendingElevationValue)) {
        snprintf(gStatus, sizeof(gStatus), "moving to elevation %d", gPendingElevationValue);
    }
}

void applyStartupTarget()
{
    if (gStartupApplied || !gEnabled) {
        return;
    }
    gStartupApplied = true;
    if (gStartupMap[0] != '\0') {
        requestMapChange(gStartupMap, gStartupElevation);
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

bool handleKey(int key)
{
    if (!gEnabled) {
        return false;
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

void takeSnapshot()
{
    if (gSdlSurface == nullptr || gSdlSurface->pixels == nullptr) {
        setStatus("snapshot: no framebuffer");
        return;
    }

    unsigned char* palette = directDrawGetPalette();
    screenshotHandlerPngImpl(gSdlSurface->w, gSdlSurface->h,
        static_cast<unsigned char*>(gSdlSurface->pixels), palette);

    // The handler names files scrNNNNN.png in the current directory; pair
    // the newest one with the metadata sidecar.
    std::string imageFile;
    try {
        namespace fs = std::filesystem;
        auto newest = fs::file_time_type::min();
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            if (!entry.is_regular_file()) {
                continue;
            }
            std::string name = entry.path().filename().string();
            if (name.rfind("scr", 0) == 0 && name.rfind("screenshot", 0) != 0
                && name.size() > 4 && name.substr(name.size() - 4) == ".png") {
                auto t = fs::last_write_time(entry);
                if (imageFile.empty() || t > newest) {
                    newest = t;
                    imageFile = name;
                }
            }
        }
    } catch (...) {
        // directory scan failed - write metadata without the image name
    }

    char exePath[MAX_PATH + 1] = {};
#if defined(_WIN32)
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
#endif
    char exeSha[80];
    sha256File(exePath, exeSha, sizeof(exeSha));

    char mapName[32];
    currentMapName(mapName, sizeof(mapName));

    char stamp[32];
    {
        std::time_t t = time(nullptr);
        struct tm* tmv = localtime(&t);
        strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", tmv);
    }

    char sidecar[128];
    snprintf(sidecar, sizeof(sidecar), "devshot-%s.json", stamp);

    int drawableW = 0;
    int drawableH = 0;
    if (gSdlWindow != nullptr) {
        SDL_GL_GetDrawableSize(gSdlWindow, &drawableW, &drawableH);
    }

    FILE* f = fopen(sidecar, "w");
    if (f != nullptr) {
        fprintf(f,
            "{\n"
            "  \"image\": \"%s\",\n"
            "  \"map\": \"%s\",\n"
            "  \"elevation\": %d,\n"
            "  \"camera\": \"center tile %d\",\n"
            "  \"renderer\": \"%s\",\n"
            "  \"light\": \"off\",\n"
            "  \"resolution\": \"%dx%d\",\n"
            "  \"drawable\": \"%dx%d\",\n"
            "  \"ce_commit\": \"%s\",\n"
            "  \"tools_commit\": \"%s\",\n"
            "  \"exe_sha256\": \"%s\",\n"
            "  \"taken\": \"%s\"\n"
            "}\n",
            imageFile.c_str(),
            mapName,
            static_cast<int>(gElevation),
            gCenterTile,
            activeRendererName(),
            gSdlSurface->w,
            gSdlSurface->h,
            drawableW,
            drawableH,
            _BUILD_HASH,
            gToolsCommit,
            exeSha,
            stamp);
        fclose(f);
    }

    snprintf(gStatus, sizeof(gStatus), "snapshot: %s", sidecar);
}

} // namespace devtools
} // namespace fallout
