#include "static_scene.h"

#include <lodepng.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "band_span.h"
#include "gl_api.h"
#include "scene_internal.h"
#include "perf.h"
#include "sprite_card.h"
#include "sky.h"

namespace fallout {
namespace scene_renderer {

namespace internal {

unsigned int gFloorVao = 0;
unsigned int gFloorTexture = 0;
int gFloorVertexCount = 0;
bool gFloorLoaded = false;
int gFloorExtensionFirst = 0;
int gFloorExtensionCount = 0;
float gFloorSeaColor[3] = {0.04f, 0.04f, 0.045f};

namespace {

unsigned int gFloorVbo = 0;
MapClip gFloorMapClip;
MapClip gActiveMapClip;
std::vector<unsigned char> gFloorOccupancy;
// A tile-less elevation (171): every square is the nothing tile, the floor
// grid holds no drawn square. The void is the ground; the game's own
// walkability decides where the hero steps.
bool gFloorTileLess = false;

} // namespace

void destroyStaticFloor()
{
    destroySceneSky();
    if (gContext != nullptr) {
        if (gFloorTexture != 0) glDeleteTextures(1, &gFloorTexture);
        if (gFloorVbo != 0) glDeleteBuffers(1, &gFloorVbo);
        if (gFloorVao != 0) glDeleteVertexArrays(1, &gFloorVao);
    }
    destroySceneShader();
    destroyRoofs();
    destroyStaticLight();
    gFloorVao = 0;
    gFloorVbo = 0;
    gFloorTexture = 0;
    gFloorVertexCount = 0;
    gFloorExtensionFirst = 0;
    gFloorExtensionCount = 0;
    gFloorSeaColor[0] = gFloorSeaColor[1] = 0.04f;
    gFloorSeaColor[2] = 0.045f;
    gFloorLoaded = false;
    gFloorMapClip = {};
    gActiveMapClip = {};
    gFloorOccupancy.clear();
    gFloorTileLess = false;
}

} // namespace internal

using namespace internal;

void unloadStaticScene()
{
    destroyStaticObjects();
    destroyStaticHero();
    destroyStaticWalls();
    destroyStaticFloor();
    resetCamera();
}

bool loadStaticFloor(const std::filesystem::path& cacheDirectory, std::string* error)
{
    if (gFloorLoaded) return true;
    auto fail = [&](const std::string& message) {
        destroyStaticFloor();
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized) return fail("OpenGL renderer is unavailable");

    std::vector<unsigned char> sidecar;
    std::vector<unsigned char> png;
    {
        PerfScope stage("floor_read");
        if (!readBinary(cacheDirectory / "floor.bin", sidecar)) {
            return fail("floor.bin is missing or unreadable");
        }
        if (!readBinary(cacheDirectory / "tiles.png", png)) {
            return fail("tiles.png is missing or unreadable");
        }
    }
    constexpr size_t kHeaderSize = 20;
    constexpr size_t kFloorBytes = 100 * 100 * 2;
    if (sidecar.size() < kHeaderSize + kFloorBytes
        || sidecar[0] != 'F' || sidecar[1] != '2' || sidecar[2] != 'F' || sidecar[3] != 'L') {
        return fail("floor.bin has an invalid header");
    }
    const unsigned short version = readU16(sidecar.data() + 4);
    const unsigned short gridWidth = readU16(sidecar.data() + 6);
    const unsigned short gridHeight = readU16(sidecar.data() + 8);
    const unsigned short tileCount = readU16(sidecar.data() + 10);
    const unsigned short imageWidth = readU16(sidecar.data() + 12);
    const unsigned short imageHeight = readU16(sidecar.data() + 14);
    const unsigned short cellSize = readU16(sidecar.data() + 16);
    const unsigned short cellMargin = readU16(sidecar.data() + 18);
    // Version 2 has a roof layer after the floor layer; version 1 has none.
    // Version 3 adds the roof fringe list after the cells: a count, then per
    // entry; version 4's entries are runs of wall tops: the tile edge (0-3),
    // the roof edge's tile line, the first square along it and the number of
    // squares, then the rect of the run's strip in roof_fringe.png. Version 5
    // adds the wall the run stands over: the plane of its outer face (tile
    // units), its top height (GL) and the band's span along the line (tiles).
    // Version 6 adds, after the runs, the roof squares that have a cell of
    // their own in roof_fringe.png (a mixed tile: its roof, the wall-top part
    // cut out; the band stands up from that part): a count, then per entry the
    // square index and the cell's x and y in roof_fringe.png (8 bytes).
    // Version 7 adds, after the patches, a byte per square: 1 where the
    // square's floor tile is water art. A water edge continues past the map
    // as a sea plane (168); the renderer fills the void squares that reach
    // such an edge and stretches strips out of every boundary water square.
    // A version 3 or 4 list is read for its size only: its roofs stay flat
    // squares (version 3 drew crossed sheets, version 4 bands off the wall).
    constexpr size_t kFringeEntryBytes = 32;
    const size_t entryBytes = version == 3 ? 10 : version == 4 ? 16 : kFringeEntryBytes;
    const size_t layerBytes = version >= 2 ? 2 * kFloorBytes : kFloorBytes;
    const size_t cellsEnd = kHeaderSize + layerBytes + static_cast<size_t>(tileCount) * 4;
    const size_t listedCount = version >= 3 && sidecar.size() >= cellsEnd + 2 ? readU16(sidecar.data() + cellsEnd) : 0;
    const size_t fringeCount = version >= 5 ? listedCount : 0;
    constexpr size_t kPatchEntryBytes = 8;
    const size_t patchesAt = cellsEnd + (version >= 3 ? 2 + listedCount * entryBytes : 0);
    const size_t patchCount = version >= 6 && sidecar.size() >= patchesAt + 2 ? readU16(sidecar.data() + patchesAt) : 0;
    const size_t waterAt = patchesAt + (version >= 6 ? 2 + patchCount * kPatchEntryBytes : 0);
    constexpr size_t kGridSquares = 100 * 100;
    const size_t expectedSize = waterAt + (version >= 7 ? 3 + kGridSquares : 0);
    // tileCount 0 is a tile-less elevation (171): the map's squares are all
    // the nothing tile, the scene draws no tiles and the void carries it.
    if ((version < 1 || version > 7) || gridWidth != 100 || gridHeight != 100
        || cellSize != 88 || cellMargin != 6 || imageWidth == 0 || imageHeight == 0
        || imageWidth > 8192 || imageHeight > 8192
        || sidecar.size() != expectedSize) {
        return fail("floor.bin version, dimensions, or length is unsupported");
    }
    gFloorTileLess = tileCount == 0;

    std::vector<unsigned char> rgba;
    unsigned pngWidth = 0;
    unsigned pngHeight = 0;
    unsigned pngError = 0;
    {
        PerfScope stage("floor_decode");
        pngError = lodepng::decode(rgba, pngWidth, pngHeight, png);
    }
    if (pngError != 0) {
        return fail(std::string("tiles.png decode failed: ") + lodepng_error_text(pngError));
    }
    if (pngWidth != imageWidth || pngHeight != imageHeight) {
        return fail("tiles.png dimensions do not match floor.bin");
    }

    std::vector<FloorVertex> vertices;
    vertices.reserve(6 + 10000 * 6);
    std::vector<unsigned char> floorOccupancy(10000, 0);
    int minCol = 100, minRow = 100, maxCol = -1, maxRow = -1;
    auto quad = [&](float x0, float y, float z0, float x1, float z1,
                    float u0, float v0, float u1, float v1) {
        vertices.push_back({ x0, y, z0, u0, v0 });
        vertices.push_back({ x1, y, z0, u1, v0 });
        vertices.push_back({ x1, y, z1, u1, v1 });
        vertices.push_back({ x0, y, z0, u0, v0 });
        vertices.push_back({ x1, y, z1, u1, v1 });
        vertices.push_back({ x0, y, z1, u0, v1 });
    };
    constexpr float kHalfMap = 50.0f;
    quad(-kHalfMap, -0.08f, -kHalfMap, kHalfMap, kHalfMap, 0.0f, 0.0f, 1.0f, 1.0f);
    const unsigned char* floorData = sidecar.data() + kHeaderSize;
    const unsigned char* roofData = version >= 2 ? floorData + kFloorBytes : nullptr;
    const unsigned char* cellData = floorData + layerBytes;
    for (int row = 0; row < 100; row++) {
        for (int col = 0; col < 100; col++) {
            const unsigned short cellIndex = readU16(floorData + (row * 100 + col) * 2);
            if (cellIndex == 0) continue;
            floorOccupancy[row * 100 + col] = 1;
            if (col < minCol) minCol = col;
            if (col > maxCol) maxCol = col;
            if (row < minRow) minRow = row;
            if (row > maxRow) maxRow = row;
            if (cellIndex > tileCount) return fail("floor.bin contains an out-of-range cell index");
            const unsigned char* cell = cellData + (cellIndex - 1) * 4;
            const unsigned short cellX = readU16(cell);
            const unsigned short cellY = readU16(cell + 2);
            if (cellX + cellSize + 2 * cellMargin > imageWidth
                || cellY + cellSize + 2 * cellMargin > imageHeight) {
                return fail("floor.bin contains a tile cell outside the atlas");
            }
            // The whole cell, margin included: tile art overhangs its square,
            // and later squares paint over their neighbours' rim, as the engine
            // and the donor viewer draw them (cut to the square, the rim's
            // transparent texels showed a grid of seams).
            const float grow = static_cast<float>(cellMargin) / cellSize;
            const float u0 = static_cast<float>(cellX) / imageWidth;
            const float v0 = static_cast<float>(cellY) / imageHeight;
            const float u1 = static_cast<float>(cellX + cellSize + 2 * cellMargin) / imageWidth;
            const float v1 = static_cast<float>(cellY + cellSize + 2 * cellMargin) / imageHeight;
            const float x0 = static_cast<float>(col) - kHalfMap;
            const float z0 = static_cast<float>(row) - kHalfMap;
            quad(x0 - grow, 0.0f, z0 - grow, x0 + 1.0f + grow, z0 + 1.0f + grow, u0, v0, u1, v1);
        }
    }

    // The wall-top runs (roof_fringe.png) become pieces of the squares they cross.
    std::vector<std::vector<RoofSquare::FringePiece>> fringeOf(100 * 100);
    // Every square of an exported run, whether or not clipping leaves a band piece on it: the flat
    // atlas square is never drawn in flight for it (its wall-top art stands as a band or is cut away).
    std::vector<unsigned char> inRun(100 * 100, 0);
    std::vector<unsigned char> fringeRgba;
    unsigned fringeWidth = 0, fringeHeight = 0;
    if (fringeCount > 0 || patchCount > 0) {
        PerfScope stage("floor_fringe");
        std::vector<unsigned char> fringePng;
        if (!readBinary(cacheDirectory / "roof_fringe.png", fringePng)) {
            return fail("roof_fringe.png is missing or unreadable");
        }
        const unsigned fringeError = lodepng::decode(fringeRgba, fringeWidth, fringeHeight, fringePng);
        if (fringeError != 0) {
            return fail(std::string("roof_fringe.png decode failed: ") + lodepng_error_text(fringeError));
        }
    }
    for (size_t entry = 0; entry < fringeCount; entry++) {
        const unsigned char* data = sidecar.data() + cellsEnd + 2 + entry * kFringeEntryBytes;
        const int edge = readU16(data), plane = readU16(data + 2), start = readU16(data + 4), count = readU16(data + 6);
        const int x = readU16(data + 8), y = readU16(data + 10), w = readU16(data + 12), h = readU16(data + 14);
        float wall[4];
        std::memcpy(wall, data + 16, sizeof(wall));
        const float face = wall[0], top = wall[1], spanLow = wall[2], spanHigh = wall[3];
        if (!std::isfinite(face) || !std::isfinite(top) || !std::isfinite(spanLow) || !std::isfinite(spanHigh)
            || spanHigh <= spanLow || top <= 0.0f) {
            return fail("floor.bin contains a bad roof fringe entry");
        }
        if (edge > 3 || plane > 100 || count == 0 || start + count > 100 || w == 0 || h == 0
            || x + w > static_cast<int>(fringeWidth) || y + h > static_cast<int>(fringeHeight)) {
            return fail("floor.bin contains a bad roof fringe entry");
        }
        const bool alongU = edge < 2;
        // The strip's columns run left to right on the game's screen: against
        // u (a larger u is further left), with v.
        const int fixed = alongU ? plane - (edge == 1 ? 1 : 0) : plane - (edge == 3 ? 1 : 0);
        if (fixed < 0 || fixed >= 100) return fail("floor.bin contains a bad roof fringe entry");
        // The strip's column fraction at a place along the line (clamped: a
        // band end that runs past the squares repeats the strip's end column).
        auto stripAt = [&](float s) {
            const float fraction = alongU ? (start + count - s) / count : (s - start) / count;
            return (x + std::min(std::max(fraction, 0.0f), 1.0f) * w) / fringeWidth;
        };
        for (int along = start; along < start + count; along++) {
            RoofSquare::FringePiece piece;
            piece.edge = edge;
            piece.plane = face - kHalfMap;
            piece.bottom = top;
            // The band's own span: the run's squares, the ends following the wall's.
            const int member = alongU ? fixed * 100 + along : along * 100 + fixed;
            inRun[member] = 1;
            float low = 0, high = 0;
            if (!::fallout::scene::bandPieceSpan(along, along == start, along == start + count - 1, spanLow, spanHigh, &low, &high)) continue;
            piece.span0 = low - kHalfMap;
            piece.span1 = high - kHalfMap;
            piece.uLow = stripAt(low);
            piece.uHigh = stripAt(high);
            piece.vTop = static_cast<float>(y) / fringeHeight;
            piece.vBottom = static_cast<float>(y + h) / fringeHeight;
            piece.rows = static_cast<float>(h);
            const int index = alongU ? fixed * 100 + along : along * 100 + fixed;
            if (fringeOf[index].size() < 2) fringeOf[index].push_back(piece);
        }
    }
    // The squares drawn from a cell of roof_fringe.png under the free camera.
    std::vector<unsigned char> patchCell(100 * 100, 0);
    std::vector<int> patchX(100 * 100, 0), patchY(100 * 100, 0);
    for (size_t entry = 0; entry < patchCount; entry++) {
        const unsigned char* data = sidecar.data() + patchesAt + 2 + entry * kPatchEntryBytes;
        const int index = readU16(data), x = readU16(data + 2), y = readU16(data + 4);
        if (index >= 100 * 100 || fringeRgba.empty()
            || x + cellSize + 2 * cellMargin > static_cast<int>(fringeWidth)
            || y + cellSize + 2 * cellMargin > static_cast<int>(fringeHeight)) {
            return fail("floor.bin contains a bad roof patch entry");
        }
        patchCell[index] = 1;
        patchX[index] = x;
        patchY[index] = y;
    }
    std::vector<RoofSquare> roofs;
    for (int index = 0; roofData != nullptr && index < 100 * 100; index++) {
        const unsigned short cellIndex = readU16(roofData + index * 2);
        if (cellIndex == 0) continue;
        if (cellIndex > tileCount) return fail("floor.bin contains an out-of-range roof cell index");
        const unsigned char* cell = cellData + (cellIndex - 1) * 4;
        const unsigned short cellX = readU16(cell);
        const unsigned short cellY = readU16(cell + 2);
        if (cellX + cellSize + 2 * cellMargin > imageWidth
            || cellY + cellSize + 2 * cellMargin > imageHeight) {
            return fail("floor.bin contains a roof cell outside the atlas");
        }
        // As the floor: the whole cell, its rim over the neighbours.
        const float grow = static_cast<float>(cellMargin) / cellSize;
        const float x0 = static_cast<float>(index % 100) - kHalfMap;
        const float z0 = static_cast<float>(index / 100) - kHalfMap;
        RoofSquare square = { index, x0 - grow, z0 - grow, x0 + 1.0f + grow, z0 + 1.0f + grow,
            static_cast<float>(cellX) / imageWidth, static_cast<float>(cellY) / imageHeight,
            static_cast<float>(cellX + cellSize + 2 * cellMargin) / imageWidth,
            static_cast<float>(cellY + cellSize + 2 * cellMargin) / imageHeight };
        for (const RoofSquare::FringePiece& piece : fringeOf[index]) {
            square.pieces[square.pieceCount++] = piece;
            square.fringe = true;
            square.tileX = x0;
            square.tileZ = z0;
        }
        if (inRun[index] != 0) square.fringe = true;
        if (patchCell[index] != 0) {
            square.patch = true;
            square.patchU0 = static_cast<float>(patchX[index]) / fringeWidth;
            square.patchV0 = static_cast<float>(patchY[index]) / fringeHeight;
            square.patchU1 = static_cast<float>(patchX[index] + cellSize + 2 * cellMargin) / fringeWidth;
            square.patchV1 = static_cast<float>(patchY[index] + cellSize + 2 * cellMargin) / fringeHeight;
        }
        roofs.push_back(square);
    }

    // The sea past a map edge that is water (168). floor.bin 7 carries the
    // flat water colour (3 bytes) and a byte per square: 1 open water, 2 the
    // shore. The water squares themselves mostly draw nothing (the docks'
    // sea is the near-black grid000, whose value 1 the tile layer keeps
    // undrawn), so the renderer fills every flagged square and every void
    // square the water spills into, and stretches strips of the same flat
    // colour out of the grid's boundary squares, the steps doubling so the
    // last ones reach the far plane's horizon. All of it draws flat in the
    // water colour, a hair under the tiles. The draw keeps these vertices
    // apart: flight and turned views only, and never clipped to the map's
    // clean edges.
    const int tilesEnd = static_cast<int>(vertices.size());
    if (version >= 7 && sidecar.size() >= waterAt + 3 + kGridSquares) {
        const unsigned char* water = sidecar.data() + waterAt + 3;
        gFloorSeaColor[0] = sidecar.data()[waterAt] / 255.0f;
        gFloorSeaColor[1] = sidecar.data()[waterAt + 1] / 255.0f;
        gFloorSeaColor[2] = sidecar.data()[waterAt + 2] / 255.0f;
        auto flagged = [&](int r, int c) {
            return r >= 0 && r < 100 && c >= 0 && c < 100 && water[r * 100 + c] != 0;
        };
        auto landSquare = [&](int r, int c) {
            return r >= 0 && r < 100 && c >= 0 && c < 100
                && floorOccupancy[r * 100 + c] != 0 && water[r * 100 + c] == 0;
        };
        std::vector<unsigned char> filled(kGridSquares, 0);
        std::vector<unsigned char> blocked(kGridSquares, 0);
        std::vector<std::pair<int, int>> queue;
        // The water squares fill first (flagged void: the sea the map left
        // undrawn); a void square beside land on its sides stops the spill,
        // so the sea ends at the shore and never wraps around a land corner.
        static constexpr int kRing[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
        for (int r = 0; r < 100; r++) {
            for (int c = 0; c < 100; c++) {
                const int index = r * 100 + c;
                if (floorOccupancy[index] != 0) continue;
                if (water[index] != 0) {
                    filled[index] = 1;
                    queue.push_back({r, c});
                    continue;
                }
                for (const auto& ring : kRing) {
                    if ((ring[0] == 0 || ring[1] == 0) && landSquare(r + ring[0], c + ring[1])) {
                        blocked[index] = 1;
                        break;
                    }
                }
            }
        }
        size_t filledCount = queue.size();
        static constexpr int kSteps[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (size_t head = 0; head < queue.size() && filledCount < kGridSquares; head++) {
            const int r = queue[head].first;
            const int c = queue[head].second;
            for (const auto& step : kSteps) {
                const int rr = r + step[0], cc = c + step[1];
                if (rr < 0 || rr >= 100 || cc < 0 || cc >= 100) continue;
                const int index = rr * 100 + cc;
                if (filled[index] || blocked[index] || floorOccupancy[index] != 0 || water[index] != 0) continue;
                bool stop = false;
                for (const auto& around : kSteps) {
                    if (landSquare(rr + around[0], cc + around[1])) { stop = true; break; }
                }
                if (stop) {
                    blocked[index] = 1;
                    continue;
                }
                filled[index] = 1;
                queue.push_back({rr, cc});
                filledCount++;
            }
        }
        // The squares, then the strips past the grid's boundary: from 12 tiles
        // out the steps double, so the sea reaches the horizon with a few
        // quads. All flat: the water colour everywhere, UVs unused.
        constexpr float kSeaY = -0.03f;     // a hair under the tiles, whose rims overdraw the joint
        auto seaQuad = [&](float x0, float z0, float x1, float z1) {
            quad(x0, kSeaY, z0, x1, z1, 0.0f, 0.0f, 0.0f, 0.0f);
        };
        for (int r = 0; r < 100; r++) {
            for (int c = 0; c < 100; c++) {
                if (filled[r * 100 + c] == 0) continue;
                const float x0 = static_cast<float>(c) - kHalfMap;
                const float z0 = static_cast<float>(r) - kHalfMap;
                seaQuad(x0, z0, x0 + 1.0f, z0 + 1.0f);
            }
        }
        for (int r = 0; r < 100; r++) {
            for (int c = 0; c < 100; c++) {
                const int index = r * 100 + c;
                // A filled square, or a drawn water square at the boundary
                // (the abbey's shore tiles): either carries the sea outward.
                const bool waterSquare = filled[index] != 0
                    || (floorOccupancy[index] != 0 && water[index] != 0);
                if (!waterSquare) continue;
                if (r != 0 && r != 99 && c != 0 && c != 99) continue;
                const float x0 = static_cast<float>(c) - kHalfMap;
                const float z0 = static_cast<float>(r) - kHalfMap;
                for (int side = 0; side < 4; side++) {
                    const bool on = side == 0 ? r == 0 : side == 1 ? r == 99 : side == 2 ? c == 0 : c == 99;
                    if (!on) continue;
                    const int dr = side == 0 ? -1 : side == 1 ? 1 : 0;
                    const int dc = side == 2 ? -1 : side == 3 ? 1 : 0;
                    float from = 0.0f;
                    float step = 1.0f;
                    for (int k = 0; k < 24 && from < 2048.0f; k++) {
                        const float to = from + step;
                        if (dr != 0) {
                            const float zA = z0 + (dr < 0 ? -to : to);
                            const float zB = z0 + (dr < 0 ? -from : from);
                            seaQuad(x0, zA, x0 + 1.0f, zB);
                        } else {
                            const float xA = x0 + (dc < 0 ? -to : to);
                            const float xB = x0 + (dc < 0 ? -from : from);
                            seaQuad(xA, z0, xB, z0 + 1.0f);
                        }
                        from = to;
                        if (k >= 11) step *= 2.0f;
                    }
                }
            }
        }
        gFloorExtensionFirst = tilesEnd;
        gFloorExtensionCount = static_cast<int>(vertices.size()) - tilesEnd;
    }

    std::string shaderError;
    {
        PerfScope stage("floor_shader");
        if (!createSceneShader(&shaderError)) return fail(shaderError);
    }

    PerfScope stage("floor_upload");
    glGenVertexArrays(1, &gFloorVao);
    glBindVertexArray(gFloorVao);
    glGenBuffers(1, &gFloorVbo);
    glBindBuffer(kArrayBuffer, gFloorVbo);
    glBufferData(kArrayBuffer, static_cast<long long>(vertices.size() * sizeof(FloorVertex)),
        vertices.data(), kStaticDraw);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
    glGenTextures(1, &gFloorTexture);
    glBindTexture(kTexture2D, gFloorTexture);
    glPixelStorei(kUnpackRowLength, 0);
    glTexImage2D(kTexture2D, 0, kRgba8, pngWidth, pngHeight, 0, kRGBA, kUnsignedByte, rgba.data());
    glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    glBindVertexArray(0);

    gFloorVertexCount = static_cast<int>(vertices.size());
    if (maxCol >= minCol && maxRow >= minRow) {
        gFloorMapClip = makeMapClipRect(minCol - 50.0, minRow - 50.0,
            maxCol + 1.0 - 50.0, maxRow + 1.0 - 50.0, MapClipSource::FloorGridFallback);
    }
    gFloorOccupancy = std::move(floorOccupancy);
    setRoofSquares(std::move(roofs));
    if (!fringeRgba.empty()) {
        unsigned int fringeTexture = 0;
        glGenTextures(1, &fringeTexture);
        glBindTexture(kTexture2D, fringeTexture);
        glPixelStorei(kUnpackRowLength, 0);
        glTexImage2D(kTexture2D, 0, kRgba8, fringeWidth, fringeHeight, 0, kRGBA, kUnsignedByte, fringeRgba.data());
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        setRoofFringeTexture(fringeTexture);
    }
    gFloorLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

MapClip staticFloorMapClip()
{
    return gFloorMapClip;
}

bool staticFloorHasGround(double x, double z)
{
    if (gFloorOccupancy.size() != 10000) return false;
    // A tile-less elevation has no drawn square anywhere; the void box is
    // walkable wherever the game's own data says so (171).
    if (gFloorTileLess) return true;
    return floorGridContainsWorldPoint(gFloorOccupancy.data(), 100, 100, x, z);
}

bool setStaticFloorMapProjection(const SpriteCardProjection& projection)
{
    if (gFloorOccupancy.size() != 10000) {
        gFloorMapClip = {};
        return false;
    }
    gFloorMapClip = makeProjectedFloorMapClip(gFloorOccupancy.data(), 100, 100,
        projection.squ, projection.sqv, projection.origin);
    return gFloorMapClip.enabled;
}

void setStaticMapClip(const MapClip& clip)
{
    gActiveMapClip = clip;
}

MapClip staticMapClip()
{
    return gActiveMapClip;
}

void bindStaticMapClipUniforms()
{
    glUniform1i(gSceneClipEnabledLocation, gActiveMapClip.enabled ? 1 : 0);
    for (int index = 0; index < 4; index++) {
        const MapClipPlane& plane = gActiveMapClip.planes[index];
        glUniform3f(gSceneClipPlaneLocations[index], plane.x, plane.z, plane.offset);
    }
}

} // namespace scene_renderer
} // namespace fallout
