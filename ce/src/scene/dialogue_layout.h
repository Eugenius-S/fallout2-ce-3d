#ifndef FALLOUT_SCENE_DIALOGUE_LAYOUT_H
#define FALLOUT_SCENE_DIALOGUE_LAYOUT_H

#include <vector>

namespace fallout {
namespace scene {

struct DialogueOptionLayoutProbe {
    bool accepted = false;
    int estimatedBottom = 0;
    int drawnBottom = 0;
};

// Returns a uniform modern-only gap that preserves every option accepted by
// the baseline layout. Bottom is exclusive, matching Rect and draw bounds.
int dialogueOptionGroupGap(const std::vector<DialogueOptionLayoutProbe>& options,
    int bottom, bool enabled);

} // namespace scene
} // namespace fallout

#endif // FALLOUT_SCENE_DIALOGUE_LAYOUT_H
