#ifndef FALLOUT_SCENE_PREVIEW_POLICY_H
#define FALLOUT_SCENE_PREVIEW_POLICY_H

namespace fallout {
namespace scene {

enum class PreviewBlockReason {
    None,
    GameState,
    GameMode,
    IsoDisabled,
    Dialog,
    Movie,
};

// What blocks the 3D view is only what hides the map in the original game
// too: a game state other than playing, the world map, the map not drawn
// (isoDisable: loading, fades), a movie. Combat, a disabled interface (the
// enemies' turn, scripted moments) and windows over the map keep the 3D view:
// a window covers the scene as it covers the 2D map (the overlay mask).
// Dialogue keeps its own block for now (066 draws its backdrop in 2D).
struct PreviewRuntimeState {
    bool gameNormal;
    bool gameModeShowsMap;
    bool isoEnabled;
    bool noDialog;
    bool noMovie;
};

// The game modes (GameMode flags) that show no map: the world map.
constexpr unsigned int kGameModesWithoutMap = 0x1;
inline bool gameModeShowsMap(unsigned int gameMode)
{
    return (gameMode & kGameModesWithoutMap) == 0;
}

PreviewBlockReason previewBlockReason(const PreviewRuntimeState& state);
bool previewMayPresent(const PreviewRuntimeState& state);

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_PREVIEW_POLICY_H */
