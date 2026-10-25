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

} // namespace scene
} // namespace fallout
