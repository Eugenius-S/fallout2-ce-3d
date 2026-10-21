#include "static_scene.h"

#include <algorithm>
#include <cmath>
#include <stdio.h>

#include <lodepng.h>

#include "gl_api.h"
#include "scene_internal.h"
#include "../tile.h"

namespace fallout {
namespace scene_renderer {

namespace internal {

std::vector<WallBatch> gWallBatches;
bool gWallsLoaded = false;
unsigned int gWallSeeThroughTexture = 0;
bool gWallSeeThroughAny = false;

} // namespace internal

using namespace internal;

namespace {

struct BakedObject {
    // Skip: walls.bin draws it; Flat: the donor laid it on the floor; Plane: a
    // door, on its wall's plane under the fly camera.
    // FlyFlat: a low card, laid flat under the fly camera; FlySkip: a card a
    // solid made by name stands for in flight only.
    enum Mode { Skip, Lift, Flat, Plane, FlyFlat, FlySkip } mode;
    float offset[3]; // GL x, y, z when lifted; a plane's point x, its direction, z
};

unsigned int gWallTriangleCount = 0;
std::unordered_map<unsigned long long, BakedObject> gBakedObjects;
int gSoloTile = -1;
int gSoloRadius = 0;
std::filesystem::path gWallsDirectory; // what loadStaticWalls last loaded, for a new solo

int soloHexDistance(int tile1, int tile2)
{
    // Even-q cube coordinates, as scene_lights.cc counts hexes.
    const auto cube = [](int tile, int* x, int* z) {
        const int column = tile % 200;
        *x = column;
        *z = tile / 200 - (column + (column & 1)) / 2;
    };
    int x1, z1, x2, z2;
    cube(tile1, &x1, &z1);
    cube(tile2, &x2, &z2);
    const int dx = x1 - x2;
    const int dz = z1 - z2;
    return std::max({ std::abs(dx), std::abs(dz), std::abs(dx + dz) });
}

// walls.bin's object table: (tile, fid) -> row, and per row whether the hero
// stands behind it (this frame and as uploaded).
constexpr int kSeeThroughRowWidth = 1024;
std::unordered_map<unsigned long long, unsigned int> gWallObjectRows;
std::vector<unsigned char> gWallSeeThrough;
// Rows the engine paints at full light, whatever their hex (OBJECT_TRANS_WALL).
std::vector<unsigned char> gWallFullLight;
std::vector<unsigned char> gWallObjectTable; // as uploaded: R, G per row

struct WallVertex {
    DepthVertex vertex;
    float object; // row in walls.bin's object table, -1: none
};

unsigned long long bakedObjectKey(int tile, int fid)
{
    return (static_cast<unsigned long long>(static_cast<unsigned int>(tile)) << 32) | static_cast<unsigned int>(fid);
}

} // namespace

namespace internal {

void destroyStaticWalls()
{
    if (gContext != nullptr) {
        for (WallBatch& batch : gWallBatches) {
            if (batch.texture != 0 && batch.ownsTexture) glDeleteTextures(1, &batch.texture);
            if (batch.vbo != 0) glDeleteBuffers(1, &batch.vbo);
            if (batch.vao != 0) glDeleteVertexArrays(1, &batch.vao);
        }
    }
    gWallBatches.clear();
    gWallTriangleCount = 0;
    gWallsLoaded = false;
    gBakedObjects.clear();
    if (gContext != nullptr && gWallSeeThroughTexture != 0) glDeleteTextures(1, &gWallSeeThroughTexture);
    gWallSeeThroughTexture = 0;
    gWallSeeThroughAny = false;
    gWallObjectRows.clear();
    gWallSeeThrough.clear();
    gWallFullLight.clear();
    gWallObjectTable.clear();
}

} // namespace internal

void staticWallStats(unsigned int* triangles, unsigned int* pages)
{
    if (triangles != nullptr) *triangles = gWallTriangleCount;
    if (pages != nullptr) *pages = static_cast<unsigned int>(gWallBatches.size());
}

bool staticObjectBaked(int tile, int fid)
{
    const auto found = gBakedObjects.find(bakedObjectKey(tile, fid));
    if (found == gBakedObjects.end()) return false;
    double rightX, rightZ;
    return found->second.mode == BakedObject::Skip
        || (found->second.mode == BakedObject::FlySkip && flyCameraRight(&rightX, &rightZ));
}

void setStaticSolo(int tile, int radius)
{
    gSoloTile = tile;
    gSoloRadius = radius;
}

bool resoloStaticWalls(int tile, int radius)
{
    setStaticSolo(tile, radius);
    if (gWallsDirectory.empty()) return false;
    const std::filesystem::path directory = gWallsDirectory;
    destroyStaticWalls();
    std::string error;
    return loadStaticWalls(directory, &error);
}

bool staticSoloKeeps(int tile)
{
    return gSoloTile < 0 || (tile >= 0 && soloHexDistance(tile, gSoloTile) <= gSoloRadius);
}

bool staticObjectPlane(int tile, int fid, float plane[3])
{
    const auto found = gBakedObjects.find(bakedObjectKey(tile, fid));
    if (found == gBakedObjects.end() || found->second.mode != BakedObject::Plane) return false;
    for (int index = 0; index < 3; index++) plane[index] = found->second.offset[index];
    return true;
}

bool staticObjectFlyFlat(int tile, int fid)
{
    const auto found = gBakedObjects.find(bakedObjectKey(tile, fid));
    return found != gBakedObjects.end() && found->second.mode == BakedObject::FlyFlat;
}

bool staticObjectFlat(int tile, int fid)
{
    const auto found = gBakedObjects.find(bakedObjectKey(tile, fid));
    return found != gBakedObjects.end() && found->second.mode == BakedObject::Flat;
}

bool staticObjectLift(int tile, int fid, float lift[3])
{
    const auto found = gBakedObjects.find(bakedObjectKey(tile, fid));
    if (found == gBakedObjects.end() || found->second.mode != BakedObject::Lift) return false;
    for (int axis = 0; axis < 3; axis++) lift[axis] = found->second.offset[axis];
    return true;
}

bool loadStaticWalls(const std::filesystem::path& cacheDirectory, std::string* error)
{
    if (gWallsLoaded) return true;
    destroyStaticWalls();
    auto fail = [&](const std::string& message) {
        destroyStaticWalls();
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gFloorLoaded) return fail("static floor renderer is unavailable");

    constexpr size_t kHeaderSize = 24;
    constexpr size_t kBatchSizeV3 = 16;
    constexpr size_t kBatchSizeV4 = 20; // and flags
    constexpr unsigned int kFlyOnly = 1;
    constexpr unsigned int kGameOnly = 2; // version 5
    constexpr size_t kVertexSize = 36;
    constexpr size_t kObjectSize = 8;
    constexpr unsigned int kMaxObjects = 1000000;
    constexpr size_t kMaxSidecarBytes = 64 * 1024 * 1024;
    constexpr unsigned int kMaxPages = 16;
    constexpr unsigned int kMaxTriangles = 1000000;
    constexpr unsigned int kMaxImageDimension = 8192;
    std::vector<unsigned char> sidecar;
    if (!readBinary(cacheDirectory / "walls.bin", sidecar) || sidecar.size() > kMaxSidecarBytes) {
        return fail("walls.bin is missing, unreadable, or exceeds 64 MB");
    }
    if (sidecar.size() < kHeaderSize
        || sidecar[0] != 'F' || sidecar[1] != '2' || sidecar[2] != 'W' || sidecar[3] != 'L') {
        return fail("walls.bin has an invalid header");
    }
    const unsigned short version = readU16(sidecar.data() + 4);
    const unsigned short headerSize = readU16(sidecar.data() + 6);
    const unsigned int pageCount = readU32(sidecar.data() + 8);
    const unsigned int batchCount = readU32(sidecar.data() + 12);
    const unsigned int totalTriangles = readU32(sidecar.data() + 16);
    const unsigned int objectCount = readU32(sidecar.data() + 20);
    // A map elevation without walls has no batches and no triangles.
    // Version 4 adds batch flags: a page has up to two batches, the second
    // for the fly camera only.
    // Version 5 adds a third, for the game's camera only.
    if (version < 3 || version > 5 || headerSize != kHeaderSize || objectCount > kMaxObjects || pageCount == 0
        || pageCount > kMaxPages || batchCount > pageCount * (version - 2) || totalTriangles > kMaxTriangles
        || (batchCount == 0) != (totalTriangles == 0)) {
        return fail("walls.bin header counts or version are unsupported");
    }
    const size_t kBatchSize = version >= 4 ? kBatchSizeV4 : kBatchSizeV3;

    struct WallBatchData {
        unsigned int page;
        bool flyOnly;
        bool gameOnly;
        unsigned int triangleCount;
        unsigned int width;
        unsigned int height;
        std::vector<WallVertex> vertices;
        std::vector<unsigned char> rgba;
    };
    std::vector<WallBatchData> batches;
    batches.reserve(batchCount);
    std::vector<bool> seenPages(static_cast<size_t>(pageCount) * 3, false); // by page and flags
    std::vector<bool> decodedPages(pageCount, false);
    size_t offset = kHeaderSize;
    unsigned long long observedTriangles = 0;
    unsigned long long totalDecodedBytes = 0;
    for (unsigned int batchIndex = 0; batchIndex < batchCount; batchIndex++) {
        if (offset > sidecar.size() || sidecar.size() - offset < kBatchSize) {
            return fail("walls.bin has a truncated batch header");
        }
        const unsigned int page = readU32(sidecar.data() + offset);
        const unsigned int width = readU32(sidecar.data() + offset + 4);
        const unsigned int height = readU32(sidecar.data() + offset + 8);
        const unsigned int triangleCount = readU32(sidecar.data() + offset + 12);
        const unsigned int flags = version >= 4 ? readU32(sidecar.data() + offset + 16) : 0;
        offset += kBatchSize;
        if (page >= pageCount || flags > (version == 5 ? kGameOnly : kFlyOnly) || seenPages[page * 3 + flags] || width == 0 || height == 0
            || width > kMaxImageDimension || height > kMaxImageDimension
            || triangleCount == 0 || triangleCount > kMaxTriangles) {
            return fail("walls.bin contains an invalid page batch");
        }
        seenPages[page * 3 + flags] = true;
        const unsigned long long vertexCount = static_cast<unsigned long long>(triangleCount) * 3;
        const unsigned long long vertexBytes = vertexCount * kVertexSize;
        if (vertexBytes > sidecar.size() - offset) {
            return fail("walls.bin has truncated triangle data");
        }
        observedTriangles += triangleCount;
        if (observedTriangles > totalTriangles || observedTriangles > kMaxTriangles) {
            return fail("walls.bin triangle counts overflow the header");
        }
        WallBatchData batch;
        batch.page = page;
        batch.flyOnly = flags == kFlyOnly;
        batch.gameOnly = flags == kGameOnly;
        batch.triangleCount = triangleCount;
        batch.width = width;
        batch.height = height;
        batch.vertices.resize(static_cast<size_t>(vertexCount));
        for (size_t vertexIndex = 0; vertexIndex < batch.vertices.size(); vertexIndex++) {
            const unsigned char* vertexData = sidecar.data() + offset + vertexIndex * kVertexSize;
            DepthVertex& vertex = batch.vertices[vertexIndex].vertex;
            const float object = readF32(vertexData + 32);
            if (object != -1.0f && !(object >= 0.0f && object < static_cast<float>(objectCount)
                    && object == std::floor(object))) {
                return fail("walls.bin names an object outside its table");
            }
            batch.vertices[vertexIndex].object = object;
            vertex.x = readF32(vertexData);
            vertex.y = readF32(vertexData + 4);
            vertex.z = readF32(vertexData + 8);
            vertex.u = readF32(vertexData + 12);
            vertex.v = readF32(vertexData + 16);
            vertex.depthX = readF32(vertexData + 20);
            vertex.depthY = readF32(vertexData + 24);
            vertex.depthZ = readF32(vertexData + 28);
            if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z)
                || !std::isfinite(vertex.u) || !std::isfinite(vertex.v) || !std::isfinite(vertex.depthX)
                || !std::isfinite(vertex.depthY) || !std::isfinite(vertex.depthZ)
                || vertex.u < 0.0f || vertex.u > 1.0f || vertex.v < 0.0f || vertex.v > 1.0f) {
                return fail("walls.bin contains non-finite positions or out-of-range UVs");
            }
        }
        offset += static_cast<size_t>(vertexBytes);

        // A page's second batch draws with the first one's texture.
        if (decodedPages[page]) {
            for (const WallBatchData& other : batches) {
                if (other.page == page && (other.width != width || other.height != height)) {
                    return fail("walls.bin batches of one page disagree on its size");
                }
            }
            batches.push_back(std::move(batch));
            continue;
        }
        decodedPages[page] = true;

        char textureName[32];
        snprintf(textureName, sizeof(textureName), "sprites%u.png", page);
        const std::filesystem::path texturePath = cacheDirectory / textureName;
        std::vector<unsigned char> png;
        if (!readBinary(texturePath, png)) {
            return fail(std::string(textureName) + " is missing, unreadable, or exceeds 64 MB");
        }
        if (png.empty()) {
            return fail(std::string(textureName) + " is empty");
        }
        lodepng::State pngState;
        unsigned pngWidth = 0;
        unsigned pngHeight = 0;
        unsigned pngError = lodepng_inspect(&pngWidth, &pngHeight, &pngState, png.data(), png.size());
        if (pngError != 0) {
            return fail(std::string(textureName) + " PNG header is invalid: " + lodepng_error_text(pngError));
        }
        if (pngWidth == 0 || pngHeight == 0 || pngWidth > kMaxImageDimension || pngHeight > kMaxImageDimension
            || pngWidth != width || pngHeight != height) {
            return fail(std::string(textureName) + " dimensions do not match walls.bin batch");
        }
        const unsigned long long decodedBytes = static_cast<unsigned long long>(pngWidth) * pngHeight * 4;
        totalDecodedBytes += decodedBytes;
        if (decodedBytes > 256ULL * 1024 * 1024 || totalDecodedBytes > 512ULL * 1024 * 1024) {
            return fail(std::string(textureName) + " decoded size is too large");
        }
        pngError = lodepng::decode(batch.rgba, pngWidth, pngHeight, pngState, png);
        if (pngError != 0) {
            return fail(std::string(textureName) + " decode failed: " + lodepng_error_text(pngError));
        }
        if (batch.rgba.size() != decodedBytes) {
            return fail(std::string(textureName) + " decoded byte count is inconsistent");
        }
        for (size_t alphaOffset = 3; alphaOffset < batch.rgba.size(); alphaOffset += 4) {
            // The exporter marks the engine's animated colours 254 (they glow)
            // or 252 (they only glow: tiles and walls); the shader reads it.
            const unsigned char alpha = batch.rgba[alphaOffset];
            if (alpha != 0 && alpha != 255 && alpha < 252) {
                return fail(std::string(textureName) + " alpha is not binary cutout data");
            }
        }
        batches.push_back(std::move(batch));
    }
    if (sidecar.size() - offset < static_cast<size_t>(objectCount) * kObjectSize) {
        return fail("walls.bin has a truncated object table");
    }
    std::unordered_map<unsigned long long, unsigned int> objectRows;
    std::vector<bool> soloRows(objectCount, false);
    for (unsigned int row = 0; row < objectCount; row++) {
        const unsigned char* entry = sidecar.data() + offset + row * kObjectSize;
        const int tile = static_cast<int>(readU32(entry));
        const int fid = static_cast<int>(readU32(entry + 4));
        objectRows[bakedObjectKey(tile, fid)] = row;
        soloRows[row] = ((fid >> 24) & 0xF) != 3 && staticSoloKeeps(tile); // 3: walls
    }
    if (gSoloTile >= 0) {
        for (WallBatchData& batch : batches) {
            std::vector<WallVertex> kept;
            for (size_t index = 0; index + 2 < batch.vertices.size(); index += 3) {
                const float object = batch.vertices[index].object;
                if (object < 0.0f || !soloRows[static_cast<size_t>(object)]) continue;
                kept.insert(kept.end(), batch.vertices.begin() + index, batch.vertices.begin() + index + 3);
            }
            batch.triangleCount = static_cast<unsigned int>(kept.size() / 3);
            batch.vertices = std::move(kept);
        }
    }
    offset += static_cast<size_t>(objectCount) * kObjectSize;
    if (offset != sidecar.size() || observedTriangles != totalTriangles) {
        return fail("walls.bin has trailing data or a mismatched triangle count");
    }

    // Objects walls.bin draws; live sprite cards skip them.
    constexpr size_t kBakedHeaderSize = 12;
    constexpr size_t kBakedEntrySize = 24;
    constexpr unsigned int kMaxBakedEntries = 1000000;
    std::vector<unsigned char> baked;
    if (!readBinary(cacheDirectory / "baked.bin", baked)) {
        return fail("baked.bin is missing or unreadable; re-run the scene exporter");
    }
    if (baked.size() < kBakedHeaderSize || baked[0] != 'F' || baked[1] != '2' || baked[2] != 'B' || baked[3] != 'K'
        || readU16(baked.data() + 4) < 2 || readU16(baked.data() + 4) > 5
        || readU16(baked.data() + 6) != kBakedHeaderSize) {
        return fail("baked.bin has an invalid header");
    }
    const unsigned int bakedVersion = readU16(baked.data() + 4); // 3 adds Flat, 4 Plane and FlyFlat, 5 FlySkip
    const unsigned int bakedCount = readU32(baked.data() + 8);
    if (bakedCount > kMaxBakedEntries || baked.size() != kBakedHeaderSize + bakedCount * kBakedEntrySize) {
        return fail("baked.bin has a bad length");
    }
    std::unordered_map<unsigned long long, BakedObject> bakedObjects;
    for (unsigned int index = 0; index < bakedCount; index++) {
        const unsigned char* entry = baked.data() + kBakedHeaderSize + index * kBakedEntrySize;
        const int tile = static_cast<int>(readU32(entry));
        const unsigned int mode = readU32(entry + 8);
        const float du = readF32(entry + 12);
        const float dv = readF32(entry + 16);
        const float dz = readF32(entry + 20);
        if (tile < 0 || tile >= HEX_GRID_SIZE) return fail("baked.bin names a tile outside the hex grid");
        const float reach = mode == 3 ? 1000.0f : 10.0f; // a plane's point is on the map, a lift is small
        if (mode > (bakedVersion >= 4 ? bakedVersion : bakedVersion - 1) || !std::isfinite(du) || !std::isfinite(dv) || !std::isfinite(dz)
            || std::abs(du) > reach || std::abs(dv) > reach || std::abs(dz) > 10.0f) {
            return fail("baked.bin has an unknown mode or a bad lift");
        }
        // Lifts are donor (u, v, z); GL is (u, z, v).
        bakedObjects[bakedObjectKey(tile, static_cast<int>(readU32(entry + 4)))]
            = { static_cast<BakedObject::Mode>(mode), { du, dz, dv } };
    }

    while (glGetError() != 0) {
    }
    glPixelStorei(kUnpackAlignment, 1);
    for (const WallBatchData& source : batches) {
        gWallBatches.emplace_back();
        WallBatch& batch = gWallBatches.back();
        batch.page = source.page;
        batch.flyOnly = source.flyOnly;
        batch.gameOnly = source.gameOnly;
        batch.triangleCount = source.triangleCount;
        glGenVertexArrays(1, &batch.vao);
        glBindVertexArray(batch.vao);
        glGenBuffers(1, &batch.vbo);
        glBindBuffer(kArrayBuffer, batch.vbo);
        glBufferData(kArrayBuffer,
            static_cast<long long>(source.vertices.size() * sizeof(WallVertex)), source.vertices.data(), kStaticDraw);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(WallVertex), reinterpret_cast<const void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(WallVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, 0x1406, 0, sizeof(WallVertex), reinterpret_cast<const void*>(5 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, 0x1406, 0, sizeof(WallVertex), reinterpret_cast<const void*>(8 * sizeof(float)));
        if (source.rgba.empty()) {
            for (const WallBatch& other : gWallBatches) {
                if (other.ownsTexture && other.page == source.page) batch.texture = other.texture;
            }
            if (batch.texture == 0) return fail("walls.bin page batch has no texture");
            continue;
        }
        glGenTextures(1, &batch.texture);
        batch.ownsTexture = true;
        glBindTexture(kTexture2D, batch.texture);
        glTexImage2D(kTexture2D, 0, kRgba8, source.width, source.height, 0, kRGBA, kUnsignedByte, source.rgba.data());
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        if (glGetError() != 0) {
            glPixelStorei(kUnpackAlignment, 4);
            return fail("OpenGL failed to upload a wall atlas batch");
        }
    }
    glPixelStorei(kUnpackAlignment, 4);
    glBindVertexArray(0);
    if (objectCount > 0) {
        const unsigned int rows = (objectCount + kSeeThroughRowWidth - 1) / kSeeThroughRowWidth;
        gWallSeeThrough.assign(static_cast<size_t>(rows) * kSeeThroughRowWidth, 0);
        gWallFullLight.assign(gWallSeeThrough.size(), 0);
        gWallObjectTable.assign(gWallSeeThrough.size() * 2, 255);
        glGenTextures(1, &gWallSeeThroughTexture);
        glBindTexture(kTexture2D, gWallSeeThroughTexture);
        glTexImage2D(kTexture2D, 0, kRG8, kSeeThroughRowWidth, rows, 0, kRG, kUnsignedByte,
            gWallObjectTable.data());
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        if (glGetError() != 0) return fail("OpenGL failed to upload the wall object table");
    }
    gWallObjectRows = std::move(objectRows);
    gWallTriangleCount = totalTriangles;
    gBakedObjects = std::move(bakedObjects);
    gWallsLoaded = true;
    if (error != nullptr) error->clear();
    gWallsDirectory = cacheDirectory;
    return true;
}

void beginStaticWallSeeThrough()
{
    std::fill(gWallSeeThrough.begin(), gWallSeeThrough.end(), 0);
    std::fill(gWallFullLight.begin(), gWallFullLight.end(), 0);
}

void markStaticWallFullLight(int tile, int fid)
{
    const auto found = gWallObjectRows.find(bakedObjectKey(tile, fid));
    if (found != gWallObjectRows.end()) gWallFullLight[found->second] = 1;
}

void markStaticWallSeeThrough(int tile, int fid)
{
    const auto found = gWallObjectRows.find(bakedObjectKey(tile, fid));
    if (found != gWallObjectRows.end()) gWallSeeThrough[found->second] = 255;
}

void endStaticWallSeeThrough()
{
    gWallSeeThroughAny = std::find(gWallSeeThrough.begin(), gWallSeeThrough.end(), 255) != gWallSeeThrough.end();
}

namespace internal {

void flushWallObjectTable()
{
    if (gWallSeeThroughTexture == 0) return;
    std::vector<unsigned char> table(gWallSeeThrough.size() * 2);
    for (size_t row = 0; row < gWallSeeThrough.size(); row++) {
        table[row * 2] = gWallSeeThrough[row];
        table[row * 2 + 1] = gWallFullLight[row] != 0 ? 255 : 0;
    }
    if (table == gWallObjectTable) return;
    gWallObjectTable = std::move(table);
    glBindTexture(kTexture2D, gWallSeeThroughTexture);
    glPixelStorei(kUnpackAlignment, 2);
    glTexSubImage2D(kTexture2D, 0, 0, 0, kSeeThroughRowWidth,
        static_cast<int>(gWallSeeThrough.size() / kSeeThroughRowWidth), kRG, kUnsignedByte, gWallObjectTable.data());
    glPixelStorei(kUnpackAlignment, 4);
}

} // namespace internal

} // namespace scene_renderer
} // namespace fallout
