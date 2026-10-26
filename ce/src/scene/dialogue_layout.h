#ifndef FALLOUT_SCENE_DIALOGUE_LAYOUT_H
#define FALLOUT_SCENE_DIALOGUE_LAYOUT_H

#include <cstddef>
#include <functional>
#include <vector>

namespace fallout {
namespace scene {

struct DialogueOptionLayoutProbe {
    bool accepted = false;
    int estimatedBottom = 0;
    int drawnBottom = 0;
    int top = 0;
};

struct DialogueOptionPlacement {
    bool accepted = false;
    int top = 0;
};

struct DialogueLayout {
    std::vector<DialogueOptionPlacement> options;
    int groupGap = 0;
};

// Returns a uniform modern-only gap that preserves every option accepted by
// the baseline layout. Bottom is exclusive, matching Rect and draw bounds.
int dialogueOptionGroupGap(const std::vector<DialogueOptionLayoutProbe>& options,
    int bottom, bool enabled);

// Runs the baseline fit simulation, then allocates a modern-only gap while
// preserving the exact set of options accepted by the baseline layout.
DialogueLayout layoutDialogueOptions(std::size_t optionCount, int top, int bottom,
    bool enabled,
    const std::function<int(std::size_t, int)>& estimateBottom,
    const std::function<int(std::size_t, int)>& drawBottom);

} // namespace scene
} // namespace fallout

#endif // FALLOUT_SCENE_DIALOGUE_LAYOUT_H
