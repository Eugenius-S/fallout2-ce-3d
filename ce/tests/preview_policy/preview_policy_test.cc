#include "preview_policy.h"

#include <cstdio>

using namespace fallout::scene;

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)
    const PreviewRuntimeState normal = { true, true, true, true };
    CHECK(previewMayPresent(normal));

    PreviewRuntimeState state = normal;
    state.gameNormal = false; // pending dialog/non-normal engine state
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::GameState);

    state = normal;
    state.gameModeShowsMap = false; // the world map
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::GameMode);

    state = normal;
    state.isoEnabled = false; // pause window
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::IsoDisabled);

    state = normal;
    state.noMovie = false;
    CHECK(!previewMayPresent(state));
    CHECK(previewBlockReason(state) == PreviewBlockReason::Movie);

    // Windows over the map, combat, the enemies' turn and dialogue keep the 3D view.
    CHECK(gameModeShowsMap(0x40 | 0x800)); // combat, the player's turn
    CHECK(gameModeShowsMap(0x1000)); // inventory
    CHECK(gameModeShowsMap(0x8)); // options
    CHECK(!gameModeShowsMap(0x1)); // the world map
}
