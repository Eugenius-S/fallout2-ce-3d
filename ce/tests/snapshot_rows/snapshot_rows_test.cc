#include "../../src/devtools/snapshot_rows.h"
#include "../../src/devtools/snapshot_palette.h"

#include <cstdio>
#include <vector>

using fallout::devtools::copySnapshotRowsTightly;

int main()
{
    const auto colors = fallout::devtools::snapshotCyclePalette();
    bool hasBlueMonitor = false;
    for (int index = 233; index <= 237; ++index) {
        const int offset = (index - 229) * 3;
        if (colors[offset + 2] > colors[offset] && colors[offset + 2] > 30) hasBlueMonitor = true;
    }
    if (!hasBlueMonitor || colors[1] == 0) {
        std::fprintf(stderr, "frozen cycling palette left monitors or slime black\n");
        return 4;
    }
    for (auto channel : colors) {
        if (channel > 63) return 5;
    }
    constexpr int width = 1155;
    constexpr int height = 4;
    constexpr int pitch = 1160;
    std::vector<unsigned char> source(pitch * height, 0xE7);
    std::vector<unsigned char> tightlyPacked(width * height, 0xCD);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            source[y * pitch + x] = static_cast<unsigned char>((x * 13 + y * 47) % 251);
        }
    }

    if (!copySnapshotRowsTightly(source.data(), width, height, pitch,
            tightlyPacked.data(), tightlyPacked.size())) {
        std::fprintf(stderr, "copySnapshotRowsTightly rejected a valid padded framebuffer\n");
        return 1;
    }
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (tightlyPacked[y * width + x] != source[y * pitch + x]) {
                std::fprintf(stderr, "row copy mismatch at (%d,%d)\n", x, y);
                return 2;
            }
        }
    }
    if (tightlyPacked[width] != source[pitch] || tightlyPacked[2 * width] != source[2 * pitch]
        || tightlyPacked[height * width - 1] == 0xE7) {
        std::fprintf(stderr, "row starts or final visible pixel do not match\n");
        return 3;
    }
    if (copySnapshotRowsTightly(source.data(), width, height, width - 1,
            tightlyPacked.data(), tightlyPacked.size())) {
        std::fprintf(stderr, "accepted a pitch smaller than row width\n");
        return 4;
    }
    return 0;
}
