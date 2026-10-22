#include "preferences_probe.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>

#include "../input.h"
#include "../kb.h"
#include "../mouse.h"
#include "../scene/options_panel.h"
#include "../settings.h"
#include "../window_manager.h"

#include <SDL.h>

namespace fallout {
namespace devtools {

namespace {

enum class ProbeMode {
    None,
    Save,
    Cancel,
    Escape,
    Defaults,
    Verify,
};

enum class DragProbe {
    None,
    ScrollbarGroove,
    ScrollbarThumb,
    Slider,
};

ProbeMode gMode = ProbeMode::None;
DragProbe gDrag = DragProbe::None;
int gStage = 0;
int gPumpStep = 0;
int gPumpCount = 0;
bool gWaitingForWheel = false;
bool gFinished = false;
bool gFailed = false;
bool gAwaitingCompletion = false;
int gWindowId = -1;
int gMouseTargetX = 0;
int gMouseTargetY = 0;
int gWheelScrollBefore = 0;
int gScrollBeforeDrag = 0;
int gZoomRow = -1;
std::string gZoomBefore;
int gLegacyDifficultyBefore = -1;
int gStagedLegacyDifficulty = -1;
bool gVerifyWheelOnNext = false;
std::string gEnabledBefore;
std::chrono::steady_clock::time_point gDeadline;

void failProbe(const char* reason)
{
    if (gFailed) return;
    gFailed = true;
    gFinished = true;
    gDrag = DragProbe::None;
    _mouse_set_position(20, 20);
    _mouse_simulate_input(0, 0, 0);
    enqueueInputEvent(KEY_ESCAPE);
    printf("PREFERENCES_PROBE FAIL: %s\n", reason);
    fflush(stdout);
}

void setMouseAt(int x, int y, int buttons)
{
    _mouse_set_position(x, y);
    _mouse_simulate_input(0, 0, buttons);
}

int mouseXForWindow(const Window* window, int x)
{
    return window != nullptr ? window->rect.left + x : x;
}

int mouseYForWindow(const Window* window, int y)
{
    return window != nullptr ? window->rect.top + y : y;
}

int pageOffsetX(const Window* window)
{
    return window != nullptr ? (window->width - 640) / 2 : 0;
}

} // namespace

bool parsePreferencesProbeOption(const char* arg)
{
    const char* prefix = "--dev-preferences-probe=";
    if (strncmp(arg, prefix, strlen(prefix)) != 0) return false;
    const char* mode = arg + strlen(prefix);
    if (strcmp(mode, "save") == 0) gMode = ProbeMode::Save;
    else if (strcmp(mode, "cancel") == 0) gMode = ProbeMode::Cancel;
    else if (strcmp(mode, "escape") == 0) gMode = ProbeMode::Escape;
    else if (strcmp(mode, "defaults") == 0) gMode = ProbeMode::Defaults;
    else if (strcmp(mode, "verify") == 0) gMode = ProbeMode::Verify;
    else {
        printf("PREFERENCES_PROBE FAIL: expected save, cancel, escape, defaults, or verify\n");
        gMode = ProbeMode::Cancel;
        gFailed = true;
    }
    gStage = 0;
    gPumpStep = 0;
    gPumpCount = 0;
    gWaitingForWheel = false;
    gFinished = false;
    gAwaitingCompletion = false;
    gVerifyWheelOnNext = false;
    gDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    return true;
}

bool preferencesProbeRequested()
{
    return gMode != ProbeMode::None;
}

bool preferencesProbeActive()
{
    return gMode != ProbeMode::None && !gFinished;
}

bool preferencesProbeFinished()
{
    return gFinished;
}

bool preferencesProbeUsesSimulatedMouse()
{
    return preferencesProbeActive();
}

void preferencesProbeComplete(bool success, scene::OptionsPanel* panel, int stagedLegacyDifficulty)
{
    if (!preferencesProbeActive() || !gAwaitingCompletion) return;
    if (!success) {
        failProbe("Done or Cancel did not close the preferences window");
        return;
    }
    gAwaitingCompletion = false;
    if (panel == nullptr) {
        failProbe("preferences panel disappeared before result check");
        return;
    }
    if (gMode == ProbeMode::Cancel || gMode == ProbeMode::Escape) {
        if (panel->settingValueForProbe("3d.zoom") != gZoomBefore
            || panel->settingValueForProbe("3d.enabled") != gEnabledBefore
            || gStagedLegacyDifficulty == gLegacyDifficultyBefore
            || stagedLegacyDifficulty != gLegacyDifficultyBefore
            || static_cast<int>(settings.preferences.combat_difficulty) != gLegacyDifficultyBefore
            || std::abs(settings.scene3d.zoom - std::strtod(gZoomBefore.c_str(), nullptr)) > 0.00001
            || settings.scene3d.enabled != (gEnabledBefore == "true")) {
            failProbe("cancel or Escape did not restore staged and legacy values");
            return;
        }
        printf("PREFERENCES_PROBE PASS rollback zoom=%s enabled=%s combat_difficulty=%d staged_combat_difficulty=%d\n",
            panel->settingValueForProbe("3d.zoom").c_str(), panel->settingValueForProbe("3d.enabled").c_str(),
            static_cast<int>(settings.preferences.combat_difficulty), stagedLegacyDifficulty);
    } else if (gMode == ProbeMode::Verify) {
        printf("PREFERENCES_PROBE PASS readback zoom=%s enabled=%s combat_difficulty=%d\n",
            panel->settingValueForProbe("3d.zoom").c_str(), panel->settingValueForProbe("3d.enabled").c_str(),
            static_cast<int>(settings.preferences.combat_difficulty));
    } else if (gMode == ProbeMode::Defaults) {
        int checkedThreeDDefaults = 0;
        for (const SettingDescriptor& descriptor : settingsGetDescriptors()) {
            if (descriptor.section != "3d" || descriptor.readOnly || descriptor.commandLineOverride || !descriptor.applicable) continue;
            if (settingsGetConfiguredValue(descriptor) != descriptor.defaultValue) {
                failProbe("Defaults did not reset every editable registered 3D value");
                return;
            }
            checkedThreeDDefaults++;
        }
        if (panel->settingValueForProbe("3d.zoom") != "1.000000"
            || panel->settingValueForProbe("3d.enabled") != "true"
            || settings.scene3d.zoom != 1.0 || !settings.scene3d.enabled
            || gStagedLegacyDifficulty != 1) {
            failProbe("Defaults did not restore registered 3D defaults");
            return;
        }
        printf("PREFERENCES_PROBE PASS defaults staged zoom=%s enabled=%s checked_3d=%d\n",
            panel->settingValueForProbe("3d.zoom").c_str(), panel->settingValueForProbe("3d.enabled").c_str(), checkedThreeDDefaults);
    } else {
        const std::string committedZoom = panel->settingValueForProbe("3d.zoom");
        if (committedZoom == gZoomBefore || panel->settingValueForProbe("3d.enabled") == gEnabledBefore
            || gStagedLegacyDifficulty == gLegacyDifficultyBefore
            || std::abs(settings.scene3d.zoom - std::strtod(committedZoom.c_str(), nullptr)) > 0.00001
            || settings.scene3d.enabled != (panel->settingValueForProbe("3d.enabled") == "true")) {
            failProbe("Done did not commit the staged 3D edits");
            return;
        }
        printf("PREFERENCES_PROBE PASS committed zoom=%s enabled=%s combat_difficulty=%d\n",
            panel->settingValueForProbe("3d.zoom").c_str(), panel->settingValueForProbe("3d.enabled").c_str(),
            gStagedLegacyDifficulty);
    }
    gFinished = true;
    fflush(stdout);
}

int preferencesProbeNextEvent(int eventCode, scene::OptionsPanel* panel, int windowId, int stagedLegacyDifficulty)
{
    if (!preferencesProbeActive()) return eventCode;
    gStagedLegacyDifficulty = stagedLegacyDifficulty;
    if (gFailed) {
        gFinished = true;
        return KEY_ESCAPE;
    }
    if (std::chrono::steady_clock::now() >= gDeadline) {
        failProbe("the scripted preferences sequence timed out");
        return KEY_ESCAPE;
    }
    if (panel == nullptr || windowId < 0) {
        failProbe("preferences panel did not initialize");
        return KEY_ESCAPE;
    }
    if (gWaitingForWheel) {
        if ((mouseGetEvent() & MOUSE_EVENT_WHEEL) == 0) return eventCode;
        gWaitingForWheel = false;
        gVerifyWheelOnNext = true;
        return eventCode;
    }
    if (gVerifyWheelOnNext) {
        gVerifyWheelOnNext = false;
        if (panel->scroll() == gWheelScrollBefore) {
            failProbe("wheel event did not change panel scroll");
            return KEY_ESCAPE;
        }
        printf("PREFERENCES_PROBE PASS wheel scroll %d -> %d\n", gWheelScrollBefore, panel->scroll());
    }
    if (++gStage > 32) {
        failProbe("script exceeded the event limit");
        return KEY_ESCAPE;
    }
    gWindowId = windowId;

    switch (gStage) {
    case 1:
        gZoomRow = panel->findRowBySettingId("3d.zoom");
        if (gZoomRow < 0) {
            failProbe("registered 3D zoom setting was not found");
            return KEY_ESCAPE;
        }
        gZoomBefore = panel->settingValueForProbe("3d.zoom");
        gEnabledBefore = panel->settingValueForProbe("3d.enabled");
        gLegacyDifficultyBefore = static_cast<int>(settings.preferences.combat_difficulty);
        gStagedLegacyDifficulty = gLegacyDifficultyBefore;
        printf("PREFERENCES_PROBE rows=%d initialScroll=%d zoom=%s enabled=%s combat_difficulty=%d\n",
            panel->rowCount(), panel->scroll(), gZoomBefore.c_str(), gEnabledBefore.c_str(), gLegacyDifficultyBefore);
        if (gMode == ProbeMode::Verify) {
            printf("PREFERENCES_PROBE READBACK zoom=%s enabled=%s combat_difficulty=%d\n",
                gZoomBefore.c_str(), gEnabledBefore.c_str(), gLegacyDifficultyBefore);
            gAwaitingCompletion = true;
            return KEY_ESCAPE;
        }
        return KEY_SPACE;
    case 2:
        if (panel->settingValueForProbe("3d.enabled") == gEnabledBefore) {
            failProbe("Space did not change the selected 3D toggle");
            return KEY_ESCAPE;
        }
        printf("PREFERENCES_PROBE PASS keyboard toggle %s -> %s\n", gEnabledBefore.c_str(), panel->settingValueForProbe("3d.enabled").c_str());
        return KEY_END;
    case 3:
        if (panel->scroll() != panel->maxScroll()) {
            failProbe("End did not scroll to the final row");
            return KEY_ESCAPE;
        }
        return KEY_HOME;
    case 4:
        if (panel->scroll() != 0) {
            failProbe("Home did not return to the first row");
            return KEY_ESCAPE;
        }
        return KEY_PAGE_DOWN;
    case 5: {
        if (panel->scroll() == 0 || panel->scroll() == panel->maxScroll()) {
            failProbe("Page Down did not reach an intermediate scroll position");
            return KEY_ESCAPE;
        }
        gWheelScrollBefore = panel->scroll();
        SDL_Event event = {};
        event.type = SDL_MOUSEWHEEL;
        event.wheel.y = 1;
        if (SDL_PushEvent(&event) < 0) {
            failProbe("SDL wheel event could not be queued");
            return KEY_ESCAPE;
        }
        gWaitingForWheel = true;
        return eventCode;
    }
    case 6: {
        if (panel->scroll() == 0 || panel->scroll() == panel->maxScroll()) {
            failProbe("page navigation did not reach the expected intermediate scroll");
            return KEY_ESCAPE;
        }
        Window* window = windowGetWindow(windowId);
        const int x = panel->scrollbarX();
        const int y = mouseYForWindow(window, 80);
        setMouseAt(x, y, MOUSE_STATE_LEFT_BUTTON_DOWN);
        gMouseTargetX = x;
        gMouseTargetY = y;
        gDrag = DragProbe::ScrollbarGroove;
        gPumpStep = 0;
        gPumpCount = 0;
        gScrollBeforeDrag = panel->scroll();
        printf("PREFERENCES_PROBE scrollbar groove engine state armed\n");
        return 599;
    }
    case 7: {
        if (panel->scroll() == gScrollBeforeDrag) {
            failProbe("scrollbar groove did not change panel scroll");
            return KEY_ESCAPE;
        }
        Window* window = windowGetWindow(windowId);
        const int x = panel->scrollbarX();
        const int y = mouseYForWindow(window, panel->scrollbarThumbTop() + panel->scrollbarThumbHeight() / 2);
        setMouseAt(x + 5, y, MOUSE_STATE_LEFT_BUTTON_DOWN);
        gMouseTargetX = x + 5;
        gMouseTargetY = y;
        gDrag = DragProbe::ScrollbarThumb;
        gPumpStep = 0;
        gPumpCount = 0;
        gScrollBeforeDrag = panel->scroll();
        printf("PREFERENCES_PROBE scrollbar thumb engine state armed scroll=%d\n", panel->scroll());
        return 599;
    }
    case 8: {
        if (panel->scroll() == gScrollBeforeDrag) {
            failProbe("scrollbar thumb drag did not change panel scroll");
            return KEY_ESCAPE;
        }
        Window* window = windowGetWindow(windowId);
        const int y = 170 - panel->scroll();
        if (y < 0 || y >= window->height) {
            failProbe("scrolled legacy preference was outside the viewport");
            return KEY_ESCAPE;
        }
        setMouseAt(mouseXForWindow(window, pageOffsetX(window) + 99), mouseYForWindow(window, y), 0);
        gLegacyDifficultyBefore = static_cast<int>(settings.preferences.combat_difficulty);
        printf("PREFERENCES_PROBE legacy preference event 506 dispatched at scroll=%d localY=%d\n", panel->scroll(), y);
        return 506;
    }
    case 9:
        if (gStagedLegacyDifficulty == gLegacyDifficultyBefore) {
            failProbe("legacy preference event did not change the value after scroll translation");
            return KEY_ESCAPE;
        }
        printf("PREFERENCES_PROBE PASS legacy combat difficulty changed %d -> %d at scroll=%d\n",
            gLegacyDifficultyBefore, gStagedLegacyDifficulty, panel->scroll());
        return KEY_END;
    case 10: {
        Window* window = windowGetWindow(windowId);
        const int x = mouseXForWindow(window, pageOffsetX(window) + 400);
        const int y = mouseYForWindow(window, panel->sliderTrackY(gZoomRow) - panel->scroll());
        if (y < window->rect.top || y > window->rect.bottom) {
            failProbe("registered 3D zoom slider is outside the viewport");
            return KEY_ESCAPE;
        }
        setMouseAt(x, y, MOUSE_STATE_LEFT_BUTTON_DOWN);
        gMouseTargetX = x;
        gMouseTargetY = y;
        gDrag = DragProbe::Slider;
        gPumpStep = 0;
        gPumpCount = 0;
        printf("PREFERENCES_PROBE numeric slider engine state armed id=3d.zoom row=%d\n", gZoomRow);
        return 600 + gZoomRow;
    }
    case 11:
        if (panel->settingValueForProbe("3d.zoom") == gZoomBefore) {
            failProbe("production slider drag did not change 3d.zoom");
            return KEY_ESCAPE;
        }
        printf("PREFERENCES_PROBE PASS 3d.zoom %s -> %s\n", gZoomBefore.c_str(), panel->settingValueForProbe("3d.zoom").c_str());
        if (gMode == ProbeMode::Save) {
            gAwaitingCompletion = true;
            return 504;
        }
        if (gMode == ProbeMode::Cancel) {
            printf("PREFERENCES_PROBE Cancel button requested\n");
            gAwaitingCompletion = true;
            return 528;
        }
        if (gMode == ProbeMode::Escape) {
            printf("PREFERENCES_PROBE Escape rollback requested\n");
            gAwaitingCompletion = true;
            return KEY_ESCAPE;
        }
        printf("PREFERENCES_PROBE Defaults requested\n");
        return 527;
    case 12:
        if (panel->settingValueForProbe("3d.zoom") != "1.000000"
            || panel->settingValueForProbe("3d.enabled") != "true" || gStagedLegacyDifficulty != 1) {
            failProbe("Defaults did not restore the staged settings");
            return KEY_ESCAPE;
        }
        printf("PREFERENCES_PROBE PASS default values staged\n");
        gAwaitingCompletion = true;
        return 504;
    default:
        failProbe("unexpected script state");
        return KEY_ESCAPE;
    }
}

void preferencesProbePump()
{
    if (!preferencesProbeActive() || gDrag == DragProbe::None) return;
    if (++gPumpCount > 20) {
        failProbe("drag pump exceeded the iteration limit");
        return;
    }
    Window* window = windowGetWindow(gWindowId);
    if (window == nullptr) {
        failProbe("preferences window disappeared during drag");
        return;
    }
    if (gPumpStep == 0) {
        int x = gMouseTargetX;
        int y = gMouseTargetY;
        if (gDrag == DragProbe::Slider) x += 100;
        else y = std::min(window->rect.bottom - 1, y + 11);
        setMouseAt(x, y, MOUSE_STATE_LEFT_BUTTON_DOWN);
        gPumpStep++;
        return;
    }
    setMouseAt(window->rect.left + 20, window->rect.top + 20, 0);
    gDrag = DragProbe::None;
    gPumpStep = 0;
}

} // namespace devtools
} // namespace fallout
