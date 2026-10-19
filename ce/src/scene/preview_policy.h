#ifndef FALLOUT_SCENE_PREVIEW_POLICY_H
#define FALLOUT_SCENE_PREVIEW_POLICY_H

namespace fallout {
namespace scene {

enum class PreviewBlockReason {
    None,
    GameState,
    GameMode,
    UiDisabled,
    IsoDisabled,
    Combat,
    Dialog,
    Movie,
};

struct PreviewRuntimeState {
    bool gameNormal;
    bool gameModeClear;
    bool uiEnabled;
    bool isoEnabled;
    bool notCombat;
    bool noDialog;
    bool noMovie;
};

PreviewBlockReason previewBlockReason(const PreviewRuntimeState& state);
bool previewMayPresent(const PreviewRuntimeState& state);

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_PREVIEW_POLICY_H */
