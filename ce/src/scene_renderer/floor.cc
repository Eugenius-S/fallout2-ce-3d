#include "static_scene.h"

#include <lodepng.h>

#include "gl_api.h"
#include "scene_internal.h"
#include "sprite_card.h"
#include "sky.h"

namespace fallout {
namespace scene_renderer {

namespace internal {

unsigned int gFloorVao = 0;
unsigned int gFloorTexture = 0;
int gFloorVertexCount = 0;
bool gFloorLoaded = false;

namespace {

unsigned int gFloorVbo = 0;
MapClip gFloorMapClip;
MapClip gActiveMapClip;
std::vector<unsigned char> gFloorOccupancy;

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
    gFloorLoaded = false;
    gFloorMapClip = {};
    gActiveMapClip = {};
    gFloorOccupancy.clear();
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
    if (!readBinary(cacheDirectory / "floor.bin", sidecar)) {
        return fail("floor.bin is missing or unreadable");
    }
    if (!readBinary(cacheDirectory / "tiles.png", png)) {
        return fail("tiles.png is missing or unreadable");
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
    // the wall's tile line, the first square along it and the number of squares,
    // then the rect of the run's strip in roof_fringe.png.
    // A version 3 list (10-byte entries, crossed sheets) is read for its size
    // only: its roofs stay flat squares.
    constexpr size_t kFringeEntryBytes = 16;
    const size_t entryBytes = version == 3 ? 10 : kFringeEntryBytes;
    const size_t layerBytes = version >= 2 ? 2 * kFloorBytes : kFloorBytes;
    const size_t cellsEnd = kHeaderSize + layerBytes + static_cast<size_t>(tileCount) * 4;
    const size_t listedCount = version >= 3 && sidecar.size() >= cellsEnd + 2 ? readU16(sidecar.data() + cellsEnd) : 0;
    const size_t fringeCount = version == 4 ? listedCount : 0;
    const size_t expectedSize = cellsEnd + (version >= 3 ? 2 + listedCount * entryBytes : 0);
    if ((version < 1 || version > 4) || gridWidth != 100 || gridHeight != 100 || tileCount == 0
        || cellSize != 88 || cellMargin != 6 || imageWidth == 0 || imageHeight == 0
        || imageWidth > 8192 || imageHeight > 8192
        || sidecar.size() != expectedSize) {
        return fail("floor.bin version, dimensions, or length is unsupported");
    }

    std::vector<unsigned char> rgba;
    unsigned pngWidth = 0;
    unsigned pngHeight = 0;
    const unsigned pngError = lodepng::decode(rgba, pngWidth, pngHeight, png);
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
    std::vector<unsigned char> fringeRgba;
    unsigned fringeWidth = 0, fringeHeight = 0;
    if (fringeCount > 0) {
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
        if (edge > 3 || plane > 100 || count == 0 || start + count > 100 || w == 0 || h == 0
            || x + w > static_cast<int>(fringeWidth) || y + h > static_cast<int>(fringeHeight)) {
            return fail("floor.bin contains a bad roof fringe entry");
        }
        const bool alongU = edge < 2;
        // The strip's columns run left to right on the game's screen: against
        // u (a larger u is further left), with v.
        const int fixed = alongU ? plane - (edge == 1 ? 1 : 0) : plane - (edge == 3 ? 1 : 0);
        if (fixed < 0 || fixed >= 100) return fail("floor.bin contains a bad roof fringe entry");
        for (int along = start; along < start + count; along++) {
            const float lowFraction = alongU ? static_cast<float>(start + count - (along + 1)) / count
                                             : static_cast<float>(along - start) / count;
            const float highFraction = lowFraction + 1.0f / count;
            RoofSquare::FringePiece piece;
            piece.edge = edge;
            piece.plane = static_cast<float>(plane) - kHalfMap;
            // Along u the low end of the wall is the strip's right side.
            const float atLow = alongU ? highFraction : lowFraction, atHigh = alongU ? lowFraction : highFraction;
            piece.uLow = (x + atLow * w) / fringeWidth;
            piece.uHigh = (x + atHigh * w) / fringeWidth;
            piece.vTop = static_cast<float>(y) / fringeHeight;
            piece.vBottom = static_cast<float>(y + h) / fringeHeight;
            const int index = alongU ? fixed * 100 + along : along * 100 + fixed;
            if (fringeOf[index].size() < 2) fringeOf[index].push_back(piece);
        }
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
        roofs.push_back(square);
    }

    std::string shaderError;
    if (!createSceneShader(&shaderError)) return fail(shaderError);

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
