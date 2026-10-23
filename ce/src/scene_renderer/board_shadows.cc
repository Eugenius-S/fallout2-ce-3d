#include "board_shadows.h"

#include <unordered_map>
#include <vector>

#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {

using namespace internal;

namespace {

struct BoardShadow {
    int width = 0;
    int height = 0;
    int artXOffset = 0;
    int artYOffset = 0;
    int frame = 0;
    int rotation = 0;
    std::vector<unsigned char> rgba;
};

std::vector<BoardShadow> gBoardShadows;
std::unordered_map<unsigned long long, size_t> gBoardShadowOfObject;

unsigned long long objectKey(int tile, unsigned int fid)
{
    return (static_cast<unsigned long long>(static_cast<unsigned int>(tile)) << 32)
        | static_cast<unsigned long long>(fid);
}

} // namespace

bool loadBoardShadows(const std::filesystem::path& cacheDirectory, std::string* error)
{
    auto fail = [&](const char* message) {
        unloadBoardShadows();
        if (error != nullptr) *error = message;
        return false;
    };
    unloadBoardShadows();
    if (!gInitialized) return fail("static scene renderer is unavailable");
    // The sidecar is optional: a cache from before it carries no shadow-free
    // sprites, and the boards keep their painted shadows.
    std::vector<unsigned char> sidecar;
    if (!readBinary(cacheDirectory / "shadows.bin", sidecar)) {
        if (error != nullptr) error->clear();
        return true;
    }
    constexpr size_t kHeaderSize = 10;
    constexpr size_t kRowSize = 21;
    constexpr unsigned int kMaxRows = 100000;
    constexpr size_t kMaxSidecarBytes = 256 * 1024 * 1024;
    if (sidecar.size() > kMaxSidecarBytes) return fail("shadows.bin exceeds 256 MB");
    if (sidecar.size() < kHeaderSize
        || sidecar[0] != 'F' || sidecar[1] != '2' || sidecar[2] != 'B' || sidecar[3] != 'S') {
        return fail("shadows.bin has an invalid header");
    }
    const unsigned short version = readU16(sidecar.data() + 4);
    const unsigned short headerSize = readU16(sidecar.data() + 6);
    const unsigned int rowCount = readU32(sidecar.data() + 8);
    if (version != 1 || headerSize < kHeaderSize) return fail("shadows.bin has an unsupported version");
    if (rowCount > kMaxRows || sidecar.size() < headerSize + static_cast<size_t>(rowCount) * kRowSize) {
        return fail("shadows.bin row count is out of bounds");
    }
    size_t offset = headerSize;
    for (unsigned int index = 0; index < rowCount; index++) {
        if (offset + kRowSize > sidecar.size()) return fail("shadows.bin row is truncated");
        const unsigned char* row = sidecar.data() + offset;
        BoardShadow shadow;
        const int tile = static_cast<int>(readU32(row));
        const unsigned int fid = readU32(row + 4);
        shadow.frame = static_cast<int>(readU32(row + 8));
        shadow.rotation = row[12];
        shadow.width = readU16(row + 13);
        shadow.height = readU16(row + 15);
        shadow.artXOffset = static_cast<short>(readU16(row + 17));
        shadow.artYOffset = static_cast<short>(readU16(row + 19));
        offset += kRowSize;
        const size_t pixelBytes = static_cast<size_t>(shadow.width) * static_cast<size_t>(shadow.height) * 4;
        if (shadow.width == 0 || shadow.height == 0 || pixelBytes > sidecar.size() - offset) {
            return fail("shadows.bin sprite size is out of bounds");
        }
        shadow.rgba.assign(sidecar.begin() + static_cast<long>(offset),
            sidecar.begin() + static_cast<long>(offset + pixelBytes));
        offset += pixelBytes;
        gBoardShadowOfObject.emplace(objectKey(tile, fid), gBoardShadows.size());
        gBoardShadows.push_back(std::move(shadow));
    }
    if (error != nullptr) error->clear();
    return true;
}

void unloadBoardShadows()
{
    gBoardShadows.clear();
    gBoardShadowOfObject.clear();
}

const unsigned char* staticObjectShadowFreeArt(int tile, unsigned int fid, int frame, int rotation,
    int* width, int* height, int* artXOffset, int* artYOffset)
{
    if (tile < 0 || gBoardShadows.empty()) return nullptr;
    const auto found = gBoardShadowOfObject.find(objectKey(tile, fid));
    if (found == gBoardShadowOfObject.end()) return nullptr;
    const BoardShadow& shadow = gBoardShadows[found->second];
    if (shadow.frame != frame || shadow.rotation != rotation) return nullptr;
    *width = shadow.width;
    *height = shadow.height;
    *artXOffset = shadow.artXOffset;
    *artYOffset = shadow.artYOffset;
    return shadow.rgba.data();
}

} // namespace scene_renderer
} // namespace fallout
