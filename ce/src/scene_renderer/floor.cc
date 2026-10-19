#include "static_scene.h"

#include <lodepng.h>

#include "gl_api.h"
#include "scene_internal.h"

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

} // namespace

void destroyStaticFloor()
{
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
    const size_t layerBytes = version == 2 ? 2 * kFloorBytes : kFloorBytes;
    const size_t expectedSize = kHeaderSize + layerBytes + static_cast<size_t>(tileCount) * 4;
    if ((version != 1 && version != 2) || gridWidth != 100 || gridHeight != 100 || tileCount == 0
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
    const unsigned char* roofData = version == 2 ? floorData + kFloorBytes : nullptr;
    const unsigned char* cellData = floorData + layerBytes;
    for (int row = 0; row < 100; row++) {
        for (int col = 0; col < 100; col++) {
            const unsigned short cellIndex = readU16(floorData + (row * 100 + col) * 2);
            if (cellIndex == 0) continue;
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
        roofs.push_back({ index, x0 - grow, z0 - grow, x0 + 1.0f + grow, z0 + 1.0f + grow,
            static_cast<float>(cellX) / imageWidth, static_cast<float>(cellY) / imageHeight,
            static_cast<float>(cellX + cellSize + 2 * cellMargin) / imageWidth,
            static_cast<float>(cellY + cellSize + 2 * cellMargin) / imageHeight });
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
            maxCol + 1.0 - 50.0, maxRow + 1.0 - 50.0, MapClipSource::Floor);
    }
    setRoofSquares(std::move(roofs));
    gFloorLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

MapClip staticFloorMapClip()
{
    return gFloorMapClip;
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
