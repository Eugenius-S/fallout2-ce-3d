#include "preview_policy.h"

#include <cstdio>

using namespace fallout::devtools;

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)
    const PreviewRuntimeState normal = { true, true, true, true, true, true, true };
    CHECK(previewMayPresent(normal));
    // Mouse capture follows the visible preview, not MOVE mode or whether a
    // hero/click target is currently usable; that policy lives in the handler.
    CHECK(walkPreviewMayCaptureInput(true, previewMayPresent(normal)));
    CHECK(!walkPreviewMayCaptureInput(false, previewMayPresent(normal)));
    CHECK(walkObserverMayAdvance(normal, true));
    CHECK(!walkObserverMayAdvance(normal, false));

    PreviewRuntimeState state = normal;
    state.gameNormal = false; // pending dialog/non-normal engine state
    CHECK(!previewMayPresent(state));
    CHECK(!walkPreviewMayCaptureInput(true, previewMayPresent(state)));
    CHECK(previewBlockReason(state) == PreviewBlockReason::GameState);

    state = normal;
    state.gameModeClear = false; // options/preferences modal
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::GameMode);
    CHECK(!walkObserverMayAdvance(state, true));

    state = normal;
    state.isoEnabled = false; // pause window
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::IsoDisabled);

    state = normal;
    state.uiEnabled = false;
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::UiDisabled);

    state = normal;
    state.noDialog = false;
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::Dialog);

    state = normal;
    state.noMovie = false;
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::Movie);

    state = normal;
    state.notCombat = false;
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::Combat);
}
