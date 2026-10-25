#ifndef FALLOUT_DEVTOOLS_UI_FIXTURE_H
#define FALLOUT_DEVTOOLS_UI_FIXTURE_H

namespace fallout {
namespace devtools {

bool parseUiFixtureOption(const char* arg);
bool parseUiFixtureFont101PreviewOption(const char* arg);
bool parseUiFixtureFont101WeightOption(const char* arg);
bool startUiFixtureWhenReady(bool startupReady, bool blockingState);
bool uiFixtureUsesDialogBarter();
void uiFixtureDialogReady(const char* scriptName, int speakerId, int speakerTile, int eligibleRuns);
void uiFixtureVanillaPresented();
void uiFixtureScenePresented();

} // namespace devtools
} // namespace fallout

#endif /* FALLOUT_DEVTOOLS_UI_FIXTURE_H */
