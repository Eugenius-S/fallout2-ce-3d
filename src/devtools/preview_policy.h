#ifndef FALLOUT_DEVTOOLS_PREVIEW_POLICY_H
#define FALLOUT_DEVTOOLS_PREVIEW_POLICY_H

namespace fallout {
namespace devtools {

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
bool walkPreviewMayCaptureInput(bool walkPreviewActive, bool sceneCurrentlyPresented);
bool walkObserverMayAdvance(const PreviewRuntimeState& state, bool requestPending);

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_PREVIEW_POLICY_H */
