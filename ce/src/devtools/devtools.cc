#include "devtools.h"
#include "preview_policy.h"

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
#include "../dialog.h"
#include "../draw.h"
#include "../input.h"
#include "../kb.h"
#include "../map.h"
#include "../map_defs.h"
#include "../game.h"
#include "../game_mouse.h"
#include "../object.h"
#include "../mouse.h"
#include "../movie.h"
#include "../platform/git_version.h"
#include "../platform_compat.h"
#include "../random.h"
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
char gStartupMap[16] = {};
int gStartupElevation = 0;
bool gStartupApplied = false;
bool gStartupFloorPreviewRequested = false;
bool gStartupWallPreviewRequested = false;
bool gStartupHeroPreviewRequested = false;
bool gStartupWalkPreviewRequested = false;
bool gStartupFloorPreviewAttempted = false;
int gStartupFloorPreviewReturnAfter = 0;
int gStartupFloorPreviewFramesRemaining = 0;
bool gSnapshotOncePending = false;
bool gStillSnapshot = false;
int gViewTile = -1;
bool gViewTileApplied = false;
char gSnapshotRun[33] = {};
// --dev-pick-probe: the mouse is put on the scene's critters one by one and
// what the game finds there is logged (devprobe.log); snapshots show the hex
// cursor, then the hover icon over a critter. 0: off, then frames counted.
int gPickProbeStage = 0;
int gPickProbeClick[2] = { -1, -1 }; // a left click the game loop is to take
char gToolsCommit[32] = "-";

bool gPanelOpen = false;
bool gStaticFloorPreview = false;
bool gStaticWallPreview = false;
bool gStaticHeroPreview = false;
bool gHeroUnavailableLogged = false;
unsigned int gObjectCardsSkipped = 0; // live objects whose frame could not be drawn
int gHeroStack = 0; // standing sprites the game paints on the hero's hex before the hero
// The objects of the scene's last frame, by pick id (scene_renderer::pickStaticObjects),
// with the hex they were on: a pick names one only while it is still there.
struct PickEntry {
    Object* object;
    int tile;
};
std::vector<PickEntry> gPickObjects;
int gHeroPickId = -1;
PreviewBlockReason gLastPreviewBlockReason = PreviewBlockReason::None;
bool gPreviewPolicyLogged = false;

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

PreviewRuntimeState previewRuntimeState()
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

const char* previewBlockReasonName(PreviewBlockReason reason)
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

PreviewBlockReason currentPreviewBlockReason()
{
    const PreviewRuntimeState state = previewRuntimeState();
    const PreviewBlockReason reason = previewBlockReason(state);
    if (!gPreviewPolicyLogged || reason != gLastPreviewBlockReason) {
        printf("devtools: scene preview policy=%s gameState=%d gameMode=0x%x uiDisabled=%d isoDisabled=%d combat=%d dialog=%d movie=%d\n",
            previewBlockReasonName(reason), static_cast<int>(gameGetState()),
            static_cast<unsigned int>(GameMode::getCurrentGameMode()), gameUiIsDisabled() ? 1 : 0,
            isoIsDisabled() ? 1 : 0, isInCombat() ? 1 : 0, _inDialog != 0 ? 1 : 0,
            _moviePlaying() != 0 ? 1 : 0);
        fflush(stdout);
        gLastPreviewBlockReason = reason;
        gPreviewPolicyLogged = true;
    }
    return reason;
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

bool isStaticFloorTarget()
{
    char base[64];
    stripMapExtension(gMapHeader.name, base, sizeof(base));
    return compat_stricmp(base, "KLADWTWN") == 0 && static_cast<int>(gElevation) == 0;
}

bool updateStaticHeroFromGame()
{
    auto unavailable = [&](const char* reason) {
        scene_renderer::clearStaticHero();
        if (!gHeroUnavailableLogged) {
            setStatus("hero frame unavailable; wall scene remains active");
            printf("devtools: hero card skipped: %s\n", reason);
            fflush(stdout);
            gHeroUnavailableLogged = true;
        }
        return false;
    };
    if (gDude == nullptr) return unavailable("player object is missing");
    if (gDude->elevation != gElevation || !hexGridTileIsValid(gDude->tile)
        || (gDude->flags & OBJECT_HIDDEN) != OBJECT_NONE) {
        return unavailable("player is hidden, off-map, or on another elevation");
    }
    if (gDude->rotation < ROTATION_FIRST || gDude->rotation >= ROTATION_COUNT) {
        return unavailable("player rotation is invalid");
    }

    FrmImage image;
    const FrmId frmId(gDude);
    if (!image.lock(frmId, gDude->frame, gDude->rotation)) {
        return unavailable("current player FRM frame could not be locked");
    }
    const int width = image.getWidth();
    const int height = image.getHeight();
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048
        || gDude->x < -32768 || gDude->x > 32768 || gDude->y < -32768 || gDude->y > 32768) {
        return unavailable("current player FRM dimensions or movement are out of bounds");
    }
    const unsigned char* indices = image.getData();
    unsigned char* palette = directDrawGetPalette();
    if (indices == nullptr || palette == nullptr) return unavailable("player palette or pixels are unavailable");

    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    std::vector<unsigned char> rgba(pixelCount * 4);
    for (size_t index = 0; index < pixelCount; index++) {
        const unsigned char colorIndex = indices[index];
        rgba[index * 4] = static_cast<unsigned char>(palette[colorIndex * 3] << 2);
        rgba[index * 4 + 1] = static_cast<unsigned char>(palette[colorIndex * 3 + 1] << 2);
        rgba[index * 4 + 2] = static_cast<unsigned char>(palette[colorIndex * 3 + 2] << 2);
        rgba[index * 4 + 3] = colorIndex == 0 ? 0 : 255;
    }

    scene_renderer::StaticHeroFrame frame = {
        gDude->tile, static_cast<int>(gDude->elevation), gDude->fid, gDude->frame,
        static_cast<int>(gDude->rotation), gDude->x, gDude->y,
        image.getXOffset(), image.getYOffset(), width, height, rgba.data(), rgba.size(),
    };
    frame.stack = gHeroStack;
    frame.pickId = gHeroPickId;
    std::string error;
    if (!scene_renderer::updateStaticHero(frame, &error)) return unavailable(error.c_str());
    gHeroUnavailableLogged = false;
    return true;
}

struct ObjectCardPass {
    unsigned char* palette;
    std::vector<unsigned char> rgba;
    unsigned int skipped;
    int lastTile; // tile of the previous standing sprite, for the per-hex count
    int onTile; // standing sprites painted on lastTile so far
};

// How many standing sprites the game paints on this object's hex before it
// (the walk is in the game's own painting order).
int nextStackOnTile(ObjectCardPass& pass, Object* object)
{
    if ((object->flags & OBJECT_FLAT) != OBJECT_NONE) return 0;
    if (object->tile != pass.lastTile) {
        pass.lastTile = object->tile;
        pass.onTile = 0;
    }
    return pass.onTile++;
}

void addObjectCard(Object* object, void* context)
{
    ObjectCardPass& pass = *static_cast<ObjectCardPass*>(context);
    // The mouse's cursor objects are drawn over the scene with the interface.
    if ((object->flags & OBJECT_HIDDEN) != OBJECT_NONE || !hexGridTileIsValid(object->tile)
        || object == gGameMouseHexCursor || object == gGameMouseBouncingCursor) {
        return;
    }
    const int pickId = static_cast<int>(gPickObjects.size());
    gPickObjects.push_back({ object, object->tile });
    // walls.bin draws it: its card is only picked, and the game's count of
    // sprites on its hex does not include it.
    const bool pickOnly = scene_renderer::staticObjectBaked(object->tile, object->fid);
    const int stack = pickOnly ? (object->tile == pass.lastTile ? pass.onTile : 0) : nextStackOnTile(pass, object);
    if (object == gDude) {
        gHeroStack = stack; // the hero card is drawn on its own
        gHeroPickId = pickId;
        return;
    }
    if (object->rotation < ROTATION_FIRST || object->rotation >= ROTATION_COUNT) {
        pass.skipped++;
        return;
    }
    FrmImage image;
    if (!image.lock(FrmId(object), object->frame, object->rotation)) {
        pass.skipped++;
        return;
    }
    const int width = image.getWidth();
    const int height = image.getHeight();
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048) {
        pass.skipped++;
        return;
    }
    const bool cached = scene_renderer::staticObjectArtCached(object->fid, object->frame, object->rotation);
    if (!cached) {
        const unsigned char* indices = image.getData();
        if (indices == nullptr) {
            pass.skipped++;
            return;
        }
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        pass.rgba.resize(pixelCount * 4);
        for (size_t index = 0; index < pixelCount; index++) {
            const unsigned char colorIndex = indices[index];
            pass.rgba[index * 4] = static_cast<unsigned char>(pass.palette[colorIndex * 3] << 2);
            pass.rgba[index * 4 + 1] = static_cast<unsigned char>(pass.palette[colorIndex * 3 + 1] << 2);
            pass.rgba[index * 4 + 2] = static_cast<unsigned char>(pass.palette[colorIndex * 3 + 2] << 2);
            pass.rgba[index * 4 + 3] = colorIndex == 0 ? 0 : 255;
        }
    }
    scene_renderer::StaticObjectFrame frame = {
        object->tile, object->fid, object->frame, static_cast<int>(object->rotation), object->x, object->y,
        image.getXOffset(), image.getYOffset(), width, height, (object->flags & OBJECT_FLAT) != OBJECT_NONE,
        cached ? nullptr : pass.rgba.data(), cached ? 0 : pass.rgba.size(),
    };
    frame.stack = stack;
    frame.pickId = pickId;
    frame.pickOnly = pickOnly;
    // The engine's translucent kinds and their tints (_obj_render_object).
    Color tint = COLOR_WHITE;
    bool translucent = true;
    switch (object->flags & OBJECT_FLAG_0xFC000) {
    case OBJECT_TRANS_RED:
        tint = COLOR_RED;
        break;
    case OBJECT_TRANS_WALL:
        tint = COLOR_PALE_BLUE;
        break;
    case OBJECT_TRANS_GLASS:
        tint = COLOR_CYAN;
        frame.glassGray = true;
        break;
    case OBJECT_TRANS_STEAM:
        tint = COLOR_WHITE;
        break;
    case OBJECT_TRANS_ENERGY:
        tint = COLOR_YELLOW_GREEN;
        break;
    default:
        translucent = false;
        break;
    }
    if (translucent) {
        const int rgb = Color2RGB(tint);
        frame.tint[0] = ((rgb >> 10) & 0x1F) / 31.0f;
        frame.tint[1] = ((rgb >> 5) & 0x1F) / 31.0f;
        frame.tint[2] = (rgb & 0x1F) / 31.0f;
    }
    std::string error;
    if (!scene_renderer::addStaticObject(frame, &error)) pass.skipped++;
}

// Every live object on the hero's elevation, in its current frame: critters
// with their animation, doors open or shut, items where they lie.
void updateStaticObjectsFromGame()
{
    scene_renderer::beginStaticObjects();
    gPickObjects.clear();
    gHeroPickId = -1;
    ObjectCardPass pass = { directDrawGetPalette(), {}, 0, -1, 0 };
    if (pass.palette != nullptr) objectForEachAtElevation(gElevation, addObjectCard, &pass);
    scene_renderer::endStaticObjects();
    gObjectCardsSkipped = pass.skipped;
}

bool disableStaticFloorPreview(const char* status)
{
    if (!gStaticFloorPreview && !gStaticWallPreview && !gStaticHeroPreview) return false;
    gStaticFloorPreview = false;
    gStaticWallPreview = false;
    gStaticHeroPreview = false;
    tileSetFreeScroll(false);
    gHeroUnavailableLogged = false;
    scene_renderer::clearStaticHero();
    scene_renderer::clearStaticObjects();
    scene_renderer::setStaticCameraCenterTile(-1);
    gStartupFloorPreviewFramesRemaining = 0;
    setStatus(status);
    return true;
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

bool enableStaticPreview(bool withWalls, bool withHero = false)
{
    if (!isStaticFloorTarget()) {
        setStatus("static floor preview requires KLADWTWN e0");
        printf("devtools: static floor preview failed: requires KLADWTWN elevation 0\n");
        fflush(stdout);
        return false;
    }
    const std::filesystem::path cacheDirectory = gameRootPath() / "mod-cache" / "scene" / "kladwtwn" / "0";
    std::string error;
    if (!scene_renderer::loadStaticFloor(cacheDirectory, &error)) {
        std::string message = "floor preview unavailable: " + error;
        setStatus(message.c_str());
        printf("devtools: static floor preview failed: %s\n", error.c_str());
        fflush(stdout);
        return false;
    }
    if (withWalls && !scene_renderer::loadStaticWalls(cacheDirectory, &error)) {
        std::string message = "wall preview unavailable: " + error;
        setStatus(message.c_str());
        printf("devtools: static wall preview failed: %s\n", error.c_str());
        fflush(stdout);
        return false;
    }
    if (withHero && !scene_renderer::loadStaticProjection(cacheDirectory, &error)) {
        std::string message = "hero projection unavailable: " + error;
        setStatus(message.c_str());
        printf("devtools: static hero preview failed: %s\n", error.c_str());
        fflush(stdout);
        return false;
    }
    gStaticFloorPreview = !withWalls;
    gStaticWallPreview = withWalls && !withHero;
    gStaticHeroPreview = withHero;
    tileSetFreeScroll(withHero); // the 3D view scrolls past map edges
    gHeroUnavailableLogged = false;
    if (withHero) {
        setStatus("wall and hero preview on (Ctrl+F10 to return)");
        printf("devtools: static wall+hero preview started for KLADWTWN elevation 0\n");
    } else if (withWalls) {
        unsigned int triangles = 0;
        unsigned int pages = 0;
        scene_renderer::staticWallStats(&triangles, &pages);
        setStatus("static wall preview on (Ctrl+F10 to return)");
        printf("devtools: static wall preview started for KLADWTWN elevation 0 (%u triangles, %u pages)\n",
            triangles, pages);
    } else {
        setStatus("static floor preview on (Ctrl+F10 to return)");
        printf("devtools: static floor preview started for KLADWTWN elevation 0\n");
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
    if (strcmp(arg, "--dev-still-snapshot") == 0) {
        gStillSnapshot = true;
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
    if (strcmp(arg, "--dev-pick-probe") == 0) {
        gPickProbeStage = 1;
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

// Called every frame from the main loop after mapHandleTransition: applies the pending
// elevation correction once the requested map has finished loading.
void update()
{
    if (!gEnabled) {
        return;
    }
    if (gPickProbeClick[0] >= 0) {
        _gmouse_handle_event(gPickProbeClick[0], gPickProbeClick[1], MOUSE_EVENT_LEFT_BUTTON_UP);
        gPickProbeClick[0] = gPickProbeClick[1] = -1;
        if (FILE* log = fopen("devprobe.log", "a")) {
            fprintf(log, "probe click: hero busy=%d tile=%d\n", animationIsBusy(gDude) == -1 ? 1 : 0, gDude->tile);
            fclose(log);
        }
    }
    if ((gStaticFloorPreview || gStaticWallPreview || gStaticHeroPreview) && !isStaticFloorTarget()) {
        disableStaticFloorPreview("static floor preview off: map/elevation changed");
    }
    if ((gStaticFloorPreview || gStaticWallPreview || gStaticHeroPreview) && gStartupFloorPreviewFramesRemaining > 0) {
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
                snprintf(gStatus, sizeof(gStatus), "moving to elevation %d", gPendingElevationValue);
                return; // let the scheduled transition settle before preview activation
            }
        }
    }

    if ((gStartupFloorPreviewRequested || gStartupWallPreviewRequested || gStartupHeroPreviewRequested || gStartupWalkPreviewRequested)
        && !gStartupFloorPreviewAttempted
        && !gPendingElevation && gMapHeader.name[0] != '\0' && !inBlockingGameState()) {
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
        const bool started = enableStaticPreview(withWalls, withHero);
        if (started) {
            gStartupFloorPreviewFramesRemaining = gStartupFloorPreviewReturnAfter;
        }
        if (!started && gSnapshotOncePending) {
            gSnapshotOncePending = false;
            printf("devtools: one-shot preview snapshot skipped: preview did not start\n");
            fflush(stdout);
        }
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

bool staticFloorPreviewActive()
{
    return gEnabled && gStaticFloorPreview && !gStaticWallPreview && isStaticFloorTarget();
}

bool staticWallPreviewActive()
{
    return gEnabled && gStaticWallPreview && isStaticFloorTarget();
}

bool staticHeroPreviewActive()
{
    return gEnabled && gStaticHeroPreview && isStaticFloorTarget();
}

bool staticWalkPreviewActive()
{
    return gEnabled && gStartupWalkPreviewRequested && staticHeroPreviewActive();
}

bool staticScenePreviewShouldPresent()
{
    if (!(staticHeroPreviewActive() || staticWallPreviewActive() || staticFloorPreviewActive())) {
        return false;
    }
    return currentPreviewBlockReason() == PreviewBlockReason::None;
}

const char* staticScenePreviewAvailability()
{
    if (!(staticHeroPreviewActive() || staticWallPreviewActive() || staticFloorPreviewActive())) {
        return "not-requested";
    }
    return previewBlockReasonName(previewBlockReason(previewRuntimeState()));
}

void finishPickProbe()
{
    if (FILE* log = fopen("devprobe.log", "a")) {
        fprintf(log, "probe done: hero tile=%d busy=%d scene=%s\n", gDude->tile, animationIsBusy(gDude) == -1 ? 1 : 0,
            staticScenePreviewAvailability());
        fclose(log);
    }
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
        // The pointer a little above each critter's hex, as the scene shows it.
        int hits = 0, hidden = 0, misses = 0;
        const std::vector<PickEntry> entries = gPickObjects;
        for (const PickEntry& entry : entries) {
            Object* object = entry.object;
            if (object == gDude || FrmId(object).objectType() != OBJ_TYPE_CRITTER) continue;
            double pixel[2], x, y;
            scene_renderer::gameViewTilePixel(entry.tile, pixel);
            if (!scene_renderer::staticScreenFromGamePixel(pixel[0] + object->x, pixel[1] + object->y - 25.0, &x, &y)) continue;
            const int mx = static_cast<int>(x), my = static_cast<int>(y);
            if (mx < 0 || my < 0 || mx >= screenGetWidth() || my >= screenGetHeight() - 99) continue;
            Object* found = nullptr;
            scenePickObject(mx, my, OBJ_TYPE_INVALID, true, &found);
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
        if (probeX >= 0) scenePickObject(probeX, probeY, OBJ_TYPE_INVALID, true, &found);
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
        // Then talk: the pointer on the nearest critter the hero can talk to.
        Object* talker = nullptr;
        int talkerX = -1, talkerY = -1, nearest = 1 << 30;
        for (const PickEntry& entry : gPickObjects) {
            Object* object = entry.object;
            if (object == gDude || FrmId(object).objectType() != OBJ_TYPE_CRITTER || !_obj_action_can_talk_to(object)) continue;
            const KillType kind = critterGetKillType(object);
            if (kind != KILL_TYPE_MAN && kind != KILL_TYPE_WOMAN) continue; // brahmin only moo
            double pixel[2], x, y;
            scene_renderer::gameViewTilePixel(entry.tile, pixel);
            if (!scene_renderer::staticScreenFromGamePixel(pixel[0] + object->x, pixel[1] + object->y - 25.0, &x, &y)) continue;
            Object* found = nullptr;
            scenePickObject(static_cast<int>(x), static_cast<int>(y), OBJ_TYPE_INVALID, true, &found);
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
    } else if (stage >= 1200) {
        finishPickProbe(); // the click led nowhere the scene leaves for
    }
    if (log != nullptr) fclose(log);
}

void refreshStaticHeroForPresent()
{
    if (!staticHeroPreviewActive()) return;
    const Uint64 updateStart = SDL_GetPerformanceCounter();
    // The game scrolls (screen edges, arrow keys, following the hero); the 3D
    // view goes with it.
    if (stillSnapshotRequested()) scene_renderer::setStaticCameraCenterTile(-1);
    scene_renderer::setStaticCameraCenterTile(gCenterTile);
    updateStaticObjectsFromGame();
    const bool heroUpdated = updateStaticHeroFromGame();

    // Frame rate of the 3D view, logged every two seconds: presented frames
    // per second and the share of each frame spent rebuilding live objects.
    static Uint64 windowStart = 0;
    static unsigned int windowFrames = 0;
    static double windowUpdateSeconds = 0.0;
    const Uint64 now = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    if (windowStart == 0) windowStart = now;
    windowFrames++;
    windowUpdateSeconds += static_cast<double>(now - updateStart) / frequency;
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
    if (heroUpdated && gPickProbeStage > 0) {
        runPickProbe();
        return;
    }
    if (heroUpdated && gSnapshotOncePending) {
        gSnapshotOncePending = false;
        const bool captured = takeSnapshot();
        printf("devtools: one-shot hero preview snapshot %s: %s\n",
            captured ? "saved" : "failed", gStatus);
        fflush(stdout);
    }
}

namespace {

// Where the scene shows a hex, relative to where the tile window would draw it.
bool sceneTileShift(int tile, int* dx, int* dy)
{
    double pixel[2];
    double sceneX, sceneY;
    scene_renderer::gameViewTilePixel(tile, pixel);
    if (!scene_renderer::staticScreenFromGamePixel(pixel[0], pixel[1], &sceneX, &sceneY)) return false;
    int screenX, screenY;
    if (tileToScreenXY(tile, &screenX, &screenY) != 0) return false;
    *dx = static_cast<int>(std::lround(sceneX - (screenX + 16)));
    *dy = static_cast<int>(std::lround(sceneY - (screenY + 8)));
    return true;
}

// The hex the scene shows at a screen point.
int sceneTileAt(int x, int y)
{
    scene_renderer::GroundPickResult picked = {};
    return scene_renderer::pickStaticGroundTile(x, y, &picked) && hexGridTileIsValid(picked.tile) ? picked.tile : -1;
}

struct PickFilter {
    int objectType;
};

bool pickWanted(int pickId, void* context)
{
    const PickFilter& filter = *static_cast<const PickFilter*>(context);
    if (pickId < 0 || pickId >= static_cast<int>(gPickObjects.size())) return false;
    const PickEntry& entry = gPickObjects[pickId];
    if (!objectIsOnTile(entry.object, entry.tile)) return false;
    return filter.objectType == OBJ_TYPE_INVALID || FrmId(entry.object).objectType() == filter.objectType;
}

} // namespace

void syncSceneOverlay()
{
    const bool on = scene_renderer::isActive() && staticHeroPreviewActive() && staticScenePreviewShouldPresent();
    const bool was = tileSceneOverlayActive();
    if (on != was) tileSetSceneOverlay(on ? sceneTileShift : nullptr, sceneTileAt);
    // --dev-pick-probe: the click took the game out of the scene (a dialog).
    if (!on && gPickProbeStage > 115) finishPickProbe();
    // While on, every frame: floating text and the hex cursor follow the
    // scene's camera as it glides.
    if (on || was) tileWindowRefresh();
}

void maskSceneOverlay(const unsigned char* indices, int indexPitch, unsigned char* rgba, int rgbaPitch,
    int width, int height)
{
    static std::vector<unsigned char> shown;
    shown.resize(static_cast<size_t>(width) * height);
    if (!windowShownMask(gIsoWindow, shown.data(), width, height)) return;
    for (int y = 0; y < height; y++) {
        const unsigned char* index = indices + static_cast<size_t>(y) * indexPitch;
        const unsigned char* world = shown.data() + static_cast<size_t>(y) * width;
        unsigned char* pixel = rgba + static_cast<size_t>(y) * rgbaPitch;
        for (int x = 0; x < width; x++, pixel += 4) {
            if (world[x] != 0 && index[x] == COLOR_FIRST) {
                pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
            } else {
                pixel[3] = 255;
            }
        }
    }
}

bool scenePickObject(int mouseX, int mouseY, int objectType, bool includeDude, Object** found)
{
    if (!tileSceneOverlayActive()) return false;
    // gameMouseGetObjectUnderCursor on the scene's picture: from the nearest
    // down, the first opaque one that is not a dead or unconscious critter,
    // else the last one under the pointer.
    PickFilter filter = { objectType };
    int picked[8];
    const int count = scene_renderer::pickStaticObjects(mouseX, mouseY, pickWanted, &filter, picked, 8);
    *found = nullptr;
    for (int index = 0; index < count; index++) {
        Object* object = gPickObjects[picked[index]].object;
        if (!includeDude && object == gDude) continue;
        *found = object;
        const bool translucent = (object->flags & OBJECT_FLAG_0xFC000) != OBJECT_NONE
            && (object->flags & OBJECT_TRANS_NONE) == OBJECT_NONE;
        const bool down = FrmId(object).objectType() == OBJ_TYPE_CRITTER
            && (object->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD)) != 0;
        if (!translucent && !down) break;
    }
    return true;
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

bool handleKey(int key)
{
    if (!gEnabled) {
        return false;
    }

    if (key == KEY_CTRL_F10) {
        if (gStaticFloorPreview || gStaticWallPreview || gStaticHeroPreview) {
            disableStaticFloorPreview("static floor preview off");
            return true;
        }
        enableStaticPreview(false);
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
    const bool previewRequested = staticHeroPreviewActive() || staticWallPreviewActive() || staticFloorPreviewActive();
    const bool preview = scene_renderer::isActive() && staticScenePreviewShouldPresent();
    const bool heroPreview = preview && staticHeroPreviewActive();
    const bool wallPreview = preview && staticWallPreviewActive();
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
            unsigned char* palette = directDrawGetPalette();
            if (screenshotHandlerPngImpl(gSdlSurface->w, gSdlSurface->h,
                    static_cast<unsigned char*>(gSdlSurface->pixels), palette) != 0) {
                setStatus("snapshot: vanilla PNG write failed");
                return false;
            }
            capturedWidth = gSdlSurface->w;
            capturedHeight = gSdlSurface->h;
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
    if (!preview && gSdlWindow != nullptr) {
        SDL_GL_GetDrawableSize(gSdlWindow, &drawableW, &drawableH);
    }

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
        "  \"camera\": \"%s\",\n"
        "  \"wall_triangles\": %u,\n"
        "  \"wall_pages\": %u,\n"
        "  \"hero\": %s,\n"
        "  \"objects\": {\"drawn\":%u,\"skipped\":%u},\n"
        "  \"preview_availability\": \"%s\",\n"
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
        heroPreview ? "static-wall-plus-hero-preview" : wallPreview ? "static-wall-preview"
            : preview ? "static-floor-preview" : "vanilla-framebuffer",
        mapName,
        static_cast<int>(gElevation),
        stillSnapshotRequested() ? "true" : "false",
        gSnapshotRun,
        gCenterTile,
        previewRequested ? "true" : "false",
        camera,
        wallTriangles,
        wallPages,
        heroMetadata,
        heroPreview ? scene_renderer::staticObjectCount() : 0u,
        heroPreview ? gObjectCardsSkipped : 0u,
        previewRequested ? (scene_renderer::isActive() ? staticScenePreviewAvailability() : "renderer-inactive") : "not-requested",
        activeRendererName(),
        capturedWidth,
        capturedHeight,
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
