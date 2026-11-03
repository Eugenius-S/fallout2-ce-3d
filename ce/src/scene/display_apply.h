#ifndef FALLOUT_SCENE_DISPLAY_APPLY_H
#define FALLOUT_SCENE_DISPLAY_APPLY_H

namespace fallout::scene {

// Applies committed display settings after all preferences/options windows
// have closed. Failure restores both the live display and saved settings.
bool applyConfiguredDisplaySettings();
void rememberConfiguredDisplaySettings();

} // namespace fallout::scene

#endif
