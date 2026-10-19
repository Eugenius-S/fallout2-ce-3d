#include "snapshot_rows.h"

#include <cstring>
#include <limits>

namespace fallout {
namespace devtools {

bool copySnapshotRowsTightly(const unsigned char* source, int width, int height, int pitch,
    unsigned char* destination, size_t destinationSize)
{
    if (source == nullptr || destination == nullptr || width <= 0 || height <= 0 || pitch < width) {
        return false;
    }

    const size_t rowSize = static_cast<size_t>(width);
    if (static_cast<size_t>(height) > std::numeric_limits<size_t>::max() / rowSize) {
        return false;
    }
    const size_t totalSize = rowSize * static_cast<size_t>(height);
    if (destinationSize < totalSize) {
        return false;
    }

    for (int y = 0; y < height; y++) {
        std::memcpy(destination + rowSize * static_cast<size_t>(y),
            source + static_cast<size_t>(pitch) * static_cast<size_t>(y), rowSize);
    }
    return true;
}

} // namespace devtools
} // namespace fallout
