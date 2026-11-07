#ifndef FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H
#define FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H

#include <SDL.h>

namespace fallout {

namespace scene {
class OptionsPanel;
}

namespace devtools {

// The fake SDL mouse device the scripted input tags its events with: the dev
// probes and the mark-drag bench drive the mouse without the owner's real one.
inline constexpr Uint32 kScriptMouse = 0x154154u;

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
bool scriptedInputSceneMode();
bool scriptedInputActive();
void scriptedInputBegin();
bool scriptedInputStarted();
bool scriptedInputTakeSnapshotRequest();
void scriptedInputPump();
void scriptedInputObserveEvent(const SDL_Event& event);
bool scriptedInputMouse(int* x, int* y, unsigned char* left, unsigned char* right);
// The mark-drag bench has no script file: it pushes its own scripted events
// and flips this on so they drive the mouse device like a script's would.
void scriptedInputSetExternalMouse(bool enable);

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H */
