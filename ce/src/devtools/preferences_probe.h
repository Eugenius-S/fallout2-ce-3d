#ifndef FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H
#define FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H

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

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_PREFERENCES_PROBE_H */
