#include "dialogue_layout.h"

#include <algorithm>
#include <limits>

namespace fallout {
namespace scene {

int dialogueOptionGroupGap(const std::vector<DialogueOptionLayoutProbe>& options,
    int bottom, bool enabled)
{
    if (!enabled) return 0;

    int acceptedCount = 0;
    int gap = 2;
    for (const DialogueOptionLayoutProbe& option : options) {
        if (!option.accepted) continue;

        if (acceptedCount > 0) {
            const long long estimateSlack = static_cast<long long>(bottom) - option.estimatedBottom - 1;
            const long long drawSlack = static_cast<long long>(bottom) - option.drawnBottom;
            const long long rowSlack = std::max(0LL, std::min(estimateSlack, drawSlack));
            const long long rowGap = rowSlack / acceptedCount;
            gap = std::min(gap, static_cast<int>(std::min<long long>(rowGap, std::numeric_limits<int>::max())));
        }

        acceptedCount++;
    }

    return acceptedCount > 1 ? std::max(0, gap) : 0;
}

DialogueLayout layoutDialogueOptions(std::size_t optionCount, int top, int bottom,
    bool enabled,
    const std::function<int(std::size_t, int)>& estimateBottom,
    const std::function<int(std::size_t, int)>& drawBottom)
{
    std::vector<DialogueOptionLayoutProbe> probes;
    probes.reserve(optionCount);

    int baselineTop = top;
    for (std::size_t index = 0; index < optionCount; index++) {
        const int estimatedBottom = estimateBottom(index, baselineTop);
        const bool accepted = estimatedBottom < bottom;
        const int drawnBottom = accepted ? drawBottom(index, baselineTop) : baselineTop;
        probes.push_back({ accepted, estimatedBottom, drawnBottom, baselineTop });
        if (accepted) {
            baselineTop = drawnBottom + 2;
        }
    }

    DialogueLayout layout;
    layout.options.resize(optionCount);
    layout.groupGap = dialogueOptionGroupGap(probes, bottom, enabled);

    int visibleOptionCount = 0;
    for (std::size_t index = 0; index < optionCount; index++) {
        const DialogueOptionLayoutProbe& probe = probes[index];
        if (!probe.accepted) continue;

        layout.options[index] = { true, probe.top + visibleOptionCount * layout.groupGap };
        visibleOptionCount++;
    }

    return layout;
}

} // namespace scene
} // namespace fallout
