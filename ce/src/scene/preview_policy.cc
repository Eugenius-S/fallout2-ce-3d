#include "preview_policy.h"

namespace fallout {
namespace scene {

PreviewBlockReason previewBlockReason(const PreviewRuntimeState& state)
{
    if (!state.gameNormal) return PreviewBlockReason::GameState;
    if (!state.gameModeClear) return PreviewBlockReason::GameMode;
    if (!state.uiEnabled) return PreviewBlockReason::UiDisabled;
    if (!state.isoEnabled) return PreviewBlockReason::IsoDisabled;
    if (!state.notCombat) return PreviewBlockReason::Combat;
    if (!state.noDialog) return PreviewBlockReason::Dialog;
    if (!state.noMovie) return PreviewBlockReason::Movie;
    return PreviewBlockReason::None;
}

bool previewMayPresent(const PreviewRuntimeState& state)
{
    return previewBlockReason(state) == PreviewBlockReason::None;
}

} // namespace scene
} // namespace fallout
