#ifndef FALLOUT_SCENE_CLEAN_EDGES_BOUNDARY_MASK_H
#define FALLOUT_SCENE_CLEAN_EDGES_BOUNDARY_MASK_H

#include <cctype>
#include <cstddef>

namespace fallout {
namespace scene {

// BLACK002 is a dedicated opaque border card in the scenery art list. Match its
// resolved art path rather than its fill color, which is also used by real art.
inline bool isCleanEdgesBoundaryMask(bool cleanEdges, bool clipEnabled, bool isSceneryType, const char* artPath)
{
    if (!cleanEdges || !clipEnabled || !isSceneryType || artPath == nullptr) return false;

    const char* basename = artPath;
    for (const char* character = artPath; *character != '\0'; character++) {
        if (*character == '/' || *character == '\\') basename = character + 1;
    }

    constexpr char kMaskName[] = "black002.frm";
    for (std::size_t index = 0; index < sizeof(kMaskName) - 1; index++) {
        if (basename[index] == '\0'
            || std::tolower(static_cast<unsigned char>(basename[index])) != kMaskName[index]) {
            return false;
        }
    }
    return basename[sizeof(kMaskName) - 1] == '\0';
}

} // namespace scene
} // namespace fallout

#endif // FALLOUT_SCENE_CLEAN_EDGES_BOUNDARY_MASK_H
