#ifndef FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H
#define FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H

#include <SDL.h>

namespace fallout {

namespace scene {
class OptionsPanel;
}

namespace devtools {

bool parsePreferencesProbeOption(const char* arg);
bool preferencesProbeRequested();
bool preferencesProbeActive();
bool preferencesProbeFinished();
bool preferencesProbeUsesSimulatedMouse();
bool preferencesProbeOverrideMouse(int* x, int* y, unsigned char* leftButton, unsigned char* rightButton);
void preferencesProbeComplete(bool success, scene::OptionsPanel* panel, int stagedLegacyDifficulty);
int preferencesProbeNextEvent(int eventCode, scene::OptionsPanel* panel, int windowId, int stagedLegacyDifficulty);
void preferencesProbePump();
bool parseScriptedInputOption(const char* arg);
bool scriptedInputActive();
void scriptedInputBegin();
bool scriptedInputStarted();
bool scriptedInputTakeSnapshotRequest();
void scriptedInputPump();
void scriptedInputObserveEvent(const SDL_Event& event);
bool scriptedInputMouse(int* x, int* y, unsigned char* left, unsigned char* right);

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H */
