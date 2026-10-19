#ifndef FALLOUT_DEVTOOLS_SNAPSHOT_ROWS_H
#define FALLOUT_DEVTOOLS_SNAPSHOT_ROWS_H

#include <cstddef>

namespace fallout {
namespace devtools {

// Copies an 8-bit framebuffer with SDL row padding into tightly packed rows
// expected by the PNG screenshot handler.
bool copySnapshotRowsTightly(const unsigned char* source, int width, int height, int pitch,
    unsigned char* destination, size_t destinationSize);

} // namespace devtools
} // namespace fallout

#endif
