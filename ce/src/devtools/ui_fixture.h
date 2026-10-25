#ifndef FALLOUT_DEVTOOLS_UI_FIXTURE_H
#define FALLOUT_DEVTOOLS_UI_FIXTURE_H

namespace fallout {
namespace devtools {

bool parseUiFixtureOption(const char* arg);
bool startUiFixtureWhenReady(bool startupReady, bool blockingState);
bool uiFixtureUsesDialogBarter();
void uiFixtureDialogReady(const char* scriptName, int speakerId, int speakerTile, int eligibleRuns);
void uiFixtureVanillaPresented();

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_UI_FIXTURE_H */
