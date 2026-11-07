#include "preferences_probe.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

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
std::ifstream gInputScript;
bool gInputSceneMode = false; // --dev-scene-input: pumped from the main loop, no options screen
bool gInputActive = false;
bool gInputSeen = false;
bool gInputStarted = false;
bool gInputSnapshot = false;
bool gInputExternal = false; // the mark-drag bench drives the mouse without a script
int gInputX = 0, gInputY = 0, gInputWait = 0;
int gInputStep = 0;
Uint32 gInputWaitUntil = 0; // a waitms deadline, in SDL ticks
unsigned char gInputLeft = 0, gInputRight = 0;
}

bool parseScriptedInputOption(const char* arg)
{
    const char* scenePrefix = "--dev-scene-input=";
    const char* prefix = "--dev-input=";
    if (strncmp(arg, scenePrefix, strlen(scenePrefix)) == 0) {
        gInputSceneMode = true;
        gInputScript.open(arg + strlen(scenePrefix));
    } else if (strncmp(arg, prefix, strlen(prefix)) == 0) {
        gInputScript.open(arg + strlen(prefix));
    } else {
        return false;
    }
    gInputActive = gInputScript.is_open();
    if (!gInputActive) fprintf(stderr, "DEV_INPUT FAIL: cannot open script\n");
    return true;
}

bool scriptedInputSceneMode() { return gInputSceneMode; }

bool scriptedInputActive() { return gInputActive; }
void scriptedInputBegin() { gInputStarted = true; }
bool scriptedInputStarted() { return gInputStarted; }
bool scriptedInputTakeSnapshotRequest() { bool requested = gInputSnapshot; gInputSnapshot = false; return requested; }

void scriptedInputPump()
{
    if (!gInputActive || gInputWait-- > 0) return;
    if (gInputWaitUntil != 0) {
        if (SDL_GetTicks() < gInputWaitUntil) return;
        gInputWaitUntil = 0;
    }
    std::string line;
    while (std::getline(gInputScript, line)) {
        std::istringstream input(line);
        std::string action;
        if (!(input >> action) || action[0] == '#') continue;
        printf("DEV_INPUT STEP %d action=%s\n", ++gInputStep, action.c_str());
        fflush(stdout);
        if (action == "wait") {
            if (!(input >> gInputWait) || gInputWait < 0) { fprintf(stderr, "DEV_INPUT FAIL: invalid wait\n"); gInputActive = false; }
            return;
        }
        if (action == "waitms") {
            // A real-time wait: samples a motion strip evenly whatever the
            // run's frame pace is.
            int ms;
            if (!(input >> ms) || ms < 0) { fprintf(stderr, "DEV_INPUT FAIL: invalid waitms\n"); gInputActive = false; return; }
            gInputWaitUntil = SDL_GetTicks() + static_cast<Uint32>(ms);
            return;
        }
        if (action == "snap") { gInputSnapshot = true; printf("DEV_INPUT snap\n"); fflush(stdout); return; }
        if (action == "key") {
            // One press: a down and an up pushed in the same batch, so a held
            // key's repeat never fires. The value is an SDL_SCANCODE_* number.
            int scancode;
            if (!(input >> scancode) || scancode <= 0 || scancode >= SDL_NUM_SCANCODES) { fprintf(stderr, "DEV_INPUT FAIL: invalid key\n"); gInputActive = false; return; }
            SDL_Event down = {};
            down.type = SDL_KEYDOWN;
            down.key.keysym.scancode = static_cast<SDL_Scancode>(scancode);
            down.key.state = SDL_PRESSED;
            down.key.repeat = 0;
            SDL_Event up = {};
            up.type = SDL_KEYUP;
            up.key.keysym.scancode = down.key.keysym.scancode;
            up.key.state = SDL_RELEASED;
            up.key.repeat = 0;
            if (SDL_PushEvent(&down) != 1 || SDL_PushEvent(&up) != 1) fprintf(stderr, "DEV_INPUT FAIL: %s\n", SDL_GetError());
            else printf("DEV_INPUT pushed key %d\n", scancode);
            fflush(stdout);
            return;
        }
        SDL_Event event = {};
        if (action == "wheel") {
            event.type = SDL_MOUSEWHEEL;
            if (!(input >> event.wheel.y)) { fprintf(stderr, "DEV_INPUT FAIL: invalid wheel\n"); gInputActive = false; return; }
        } else if (action == "move" || action == "down" || action == "up") {
            int x, y;
            if (!(input >> x >> y)) { fprintf(stderr, "DEV_INPUT FAIL: invalid coordinates\n"); gInputActive = false; return; }
            event.type = action == "move" ? SDL_MOUSEMOTION : action == "down" ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
            if (event.type == SDL_MOUSEMOTION) { event.motion.which = kScriptMouse; event.motion.x = x; event.motion.y = y; }
            else { event.button.which = kScriptMouse; event.button.button = SDL_BUTTON_LEFT; event.button.x = x; event.button.y = y; event.button.state = action == "down" ? SDL_PRESSED : SDL_RELEASED; }
        } else { fprintf(stderr, "DEV_INPUT FAIL: unknown action %s\n", action.c_str()); gInputActive = false; return; }
        if (SDL_PushEvent(&event) != 1) fprintf(stderr, "DEV_INPUT FAIL: %s\n", SDL_GetError());
        else printf("DEV_INPUT pushed %s\n", action.c_str());
        fflush(stdout);
        return;
    }
    gInputActive = false;
    printf("DEV_INPUT PASS script complete\n");
    fflush(stdout);
}

void scriptedInputObserveEvent(const SDL_Event& event)
{
    if (!gInputActive && !gInputExternal) return;
    if (event.type == SDL_MOUSEMOTION && event.motion.which == kScriptMouse) {
        gInputSeen = true; gInputX = event.motion.x; gInputY = event.motion.y;
        printf("DEV_INPUT motion %d %d\n", gInputX, gInputY); fflush(stdout);
    } else if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.which == kScriptMouse) {
        gInputSeen = true; gInputX = event.button.x; gInputY = event.button.y;
        gInputLeft = event.type == SDL_MOUSEBUTTONDOWN;
        printf("DEV_INPUT button %d at %d %d\n", gInputLeft, gInputX, gInputY); fflush(stdout);
    }
}

bool scriptedInputMouse(int* x, int* y, unsigned char* left, unsigned char* right)
{
    if ((!gInputActive && !gInputExternal) || !gInputSeen) return false;
    *x = gInputX; *y = gInputY; *left = gInputLeft; *right = gInputRight;
    return true;
}

void scriptedInputSetExternalMouse(bool enable)
{
    gInputExternal = enable;
}

namespace {

enum class ProbeMode {
    None,
    Save,
    SaveKeyboard,
    Cancel,
    CancelKeyboard,
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
int gMouseDragTargetX = 0;
int gMouseTargetY = 0;
int gWheelScrollBefore = 0;
int gScrollBeforeDrag = 0;
int gZoomRow = -1;
std::string gZoomBefore;
int gLegacyDifficultyBefore = -1;
int gStagedLegacyDifficulty = -1;
bool gVerifyWheelOnNext = false;
bool gWaitingForFooter = false;
int gExpectedFooterEvent = -1;
int gFooterInputPhase = 0;
int gFooterMouseX = 0;
int gFooterMouseY = 0;
int gFooterMouseButtons = 0;
bool gFooterOverrideLogged = false;
bool gFooterLoopLogged = false;
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

void scheduleFooterMouse(int eventCode)
{
    Window* window = windowGetWindow(gWindowId);
    if (window == nullptr) {
        failProbe("preferences window disappeared before footer click injection");
        return;
    }
    for (Button* button = window->buttonListHead; button != nullptr; button = button->next) {
        if (button->leftMouseUpEventCode != eventCode) continue;
        gFooterMouseX = window->rect.left + button->rect.left + (button->rect.right - button->rect.left) / 2;
        gFooterMouseY = window->rect.top + button->rect.top + (button->rect.bottom - button->rect.top) / 2;
        gFooterMouseButtons = 0;
        gFooterInputPhase = 1;
        gFooterOverrideLogged = false;
        gFooterLoopLogged = false;
        printf("PREFERENCES_PROBE injected button state event=%d at logical=%d,%d rect=%d,%d..%d,%d\n",
            eventCode, gFooterMouseX, gFooterMouseY, button->rect.left, button->rect.top, button->rect.right, button->rect.bottom);
        fflush(stdout);
        return;
    }
    failProbe("footer button event code was not found in the current window");
}

} // namespace

bool parsePreferencesProbeOption(const char* arg)
{
    const char* prefix = "--dev-preferences-probe=";
    if (strncmp(arg, prefix, strlen(prefix)) != 0) return false;
    const char* mode = arg + strlen(prefix);
    if (strcmp(mode, "save") == 0) gMode = ProbeMode::Save;
    else if (strcmp(mode, "save-key") == 0) gMode = ProbeMode::SaveKeyboard;
    else if (strcmp(mode, "cancel") == 0) gMode = ProbeMode::Cancel;
    else if (strcmp(mode, "cancel-key") == 0) gMode = ProbeMode::CancelKeyboard;
    else if (strcmp(mode, "escape") == 0) gMode = ProbeMode::Escape;
    else if (strcmp(mode, "defaults") == 0) gMode = ProbeMode::Defaults;
    else if (strcmp(mode, "verify") == 0) gMode = ProbeMode::Verify;
    else {
        printf("PREFERENCES_PROBE FAIL: expected save, save-key, cancel, cancel-key, escape, defaults, or verify\n");
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

bool preferencesProbeOverrideMouse(int* x, int* y, unsigned char* leftButton, unsigned char* rightButton)
{
    if (!preferencesProbeActive() || !gWaitingForFooter || gFooterInputPhase == 0
        || x == nullptr || y == nullptr || leftButton == nullptr || rightButton == nullptr) {
        return false;
    }
    *x = gFooterMouseX;
    *y = gFooterMouseY;
    *leftButton = (gFooterMouseButtons & MOUSE_STATE_LEFT_BUTTON_DOWN) != 0 ? 1 : 0;
    *rightButton = (gFooterMouseButtons & MOUSE_STATE_RIGHT_BUTTON_DOWN) != 0 ? 1 : 0;
    if (!gFooterOverrideLogged) {
        printf("PREFERENCES_PROBE mouse override read phase=%d logical=%d,%d buttons=%d\n",
            gFooterInputPhase, *x, *y, *leftButton);
        fflush(stdout);
        gFooterOverrideLogged = true;
    }
    return true;
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
    if (gMode == ProbeMode::Cancel || gMode == ProbeMode::CancelKeyboard || gMode == ProbeMode::Escape) {
        if (panel->settingValueForProbe("3d.zoom") != gZoomBefore
            || panel->settingValueForProbe("3d.enabled") != gEnabledBefore
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
    } else if (gMode == ProbeMode::Defaults) {
        if (panel->settingValueForProbe("3d.zoom") != "1.000000"
            || panel->settingValueForProbe("3d.enabled") != "true"
            || settings.scene3d.zoom != 1.0 || !settings.scene3d.enabled
            || stagedLegacyDifficulty != 1
            || static_cast<int>(settings.preferences.combat_difficulty) != 1) {
            failProbe("Defaults and Done did not apply the registered defaults");
            return;
        }
        printf("PREFERENCES_PROBE PASS actual Defaults and Done clicks committed\n");
    } else if (gMode == ProbeMode::Verify) {
        printf("PREFERENCES_PROBE PASS readback zoom=%s enabled=%s combat_difficulty=%d\n",
            panel->settingValueForProbe("3d.zoom").c_str(), panel->settingValueForProbe("3d.enabled").c_str(),
            static_cast<int>(settings.preferences.combat_difficulty));
    } else if (gMode == ProbeMode::Save || gMode == ProbeMode::SaveKeyboard) {
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
    } else {
        failProbe("unexpected preferences probe completion mode");
        return;
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
    if (gWaitingForFooter) {
        if (!gFooterLoopLogged) {
            int x = 0, y = 0;
            mouseGetPosition(&x, &y);
            printf("PREFERENCES_PROBE footer loop event=%d mouse_event=%d pos=%d,%d phase=%d\n",
                eventCode, mouseGetEvent(), x, y, gFooterInputPhase);
            fflush(stdout);
            gFooterLoopLogged = true;
        }
        if (gFooterInputPhase == 1 && eventCode == -1) {
            Window* window = windowGetWindow(gWindowId);
            if (window == nullptr || window->hoveredButton == nullptr
                || window->hoveredButton->leftMouseUpEventCode != gExpectedFooterEvent) {
                return -1;
            }
            gFooterMouseButtons = MOUSE_STATE_LEFT_BUTTON_DOWN;
            gFooterInputPhase = 2;
            printf("PREFERENCES_PROBE mouse hover reached the button; injecting down\n");
            return -1;
        }
        if (gFooterInputPhase == 2 && eventCode == -2) {
            gFooterMouseButtons = 0;
            gFooterInputPhase = 3;
            printf("PREFERENCES_PROBE mouse down reached the window manager; injecting up\n");
            return -1;
        }
        if (eventCode == -1 || eventCode == -2) return -1;
        bool expected = eventCode == gExpectedFooterEvent;
        if ((gMode == ProbeMode::Save || gMode == ProbeMode::SaveKeyboard) && eventCode == KEY_RETURN) expected = true;
        if ((gMode == ProbeMode::Cancel || gMode == ProbeMode::CancelKeyboard) && eventCode == KEY_ESCAPE) expected = true;
        if (!expected) {
            printf("PREFERENCES_PROBE ignored input=%d while waiting for footer=%d\n", eventCode, gExpectedFooterEvent);
            return -1;
        }
        gWaitingForFooter = false;
        gFooterInputPhase = 0;
        gFooterMouseButtons = 0;
        gAwaitingCompletion = eventCode == 504 || eventCode == 528 || eventCode == KEY_RETURN || eventCode == KEY_ESCAPE;
        printf("PREFERENCES_PROBE actual footer input event=%d\n", eventCode);
        fflush(stdout);
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
        if (Window* window = windowGetWindow(windowId)) {
            const int inset = panel->frameInset();
            printf("PREFERENCES_PROBE viewport logical=%dx%d frame_x=%d..%d scrollbar_x=%d\n",
                window->width, window->height, inset, 640 - inset - 1, panel->scrollbarX());
        }
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
        const int y = 170 + panel->legacyColumnOffset() - panel->scroll();
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
        const SettingDescriptor* zoomDescriptor = nullptr;
        for (const SettingDescriptor& descriptor : settingsGetDescriptors()) {
            if (descriptor.id == "3d.zoom" && descriptor.numericRange.has_value()) {
                zoomDescriptor = &descriptor;
                break;
            }
        }
        if (zoomDescriptor == nullptr) {
            failProbe("registered 3D zoom range was not found");
            return KEY_ESCAPE;
        }
        const double currentZoom = std::strtod(gZoomBefore.c_str(), nullptr);
        const double minZoom = zoomDescriptor->numericRange->minimum;
        const double maxZoom = zoomDescriptor->numericRange->maximum;
        const double targetRatio = std::abs(currentZoom - minZoom) > std::abs(maxZoom - currentZoom) ? 0.0 : 1.0;
        const int x = mouseXForWindow(window, pageOffsetX(window) + panel->sliderTrackTargetX(0.5));
        const int y = mouseYForWindow(window, panel->sliderTrackY(gZoomRow) - panel->scroll());
        if (y < window->rect.top || y > window->rect.bottom) {
            failProbe("registered 3D zoom slider is outside the viewport");
            return KEY_ESCAPE;
        }
        setMouseAt(x, y, MOUSE_STATE_LEFT_BUTTON_DOWN);
        gMouseTargetX = x;
        gMouseDragTargetX = mouseXForWindow(window, pageOffsetX(window) + panel->sliderTrackTargetX(targetRatio));
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
        panel->setScroll(panel->maxScroll());
        if (gMode == ProbeMode::Save || gMode == ProbeMode::SaveKeyboard) {
            gWaitingForFooter = true;
            gExpectedFooterEvent = 504;
            if (gMode == ProbeMode::SaveKeyboard) {
                SDL_Event key = {};
                key.type = SDL_KEYDOWN;
                key.key.state = SDL_PRESSED;
                key.key.keysym.scancode = SDL_SCANCODE_RETURN;
                key.key.keysym.sym = SDLK_RETURN;
                SDL_PushEvent(&key);
                printf("PREFERENCES_PROBE queued isolated Enter through SDL input\n");
            } else {
                scheduleFooterMouse(504);
                printf("PREFERENCES_PROBE queued isolated mouse click for Done\n");
            }
            return -1;
        }
        if (gMode == ProbeMode::Cancel || gMode == ProbeMode::CancelKeyboard) {
            gWaitingForFooter = true;
            gExpectedFooterEvent = 528;
            if (gMode == ProbeMode::CancelKeyboard) {
                SDL_Event key = {};
                key.type = SDL_KEYDOWN;
                key.key.state = SDL_PRESSED;
                key.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
                key.key.keysym.sym = SDLK_ESCAPE;
                SDL_PushEvent(&key);
                printf("PREFERENCES_PROBE queued isolated Escape through SDL input\n");
            } else {
                scheduleFooterMouse(528);
                printf("PREFERENCES_PROBE queued isolated mouse click for Cancel\n");
            }
            return -1;
        }
        if (gMode == ProbeMode::Escape) {
            printf("PREFERENCES_PROBE Escape rollback requested\n");
            gAwaitingCompletion = true;
            return KEY_ESCAPE;
        }
        gWaitingForFooter = true;
        gExpectedFooterEvent = 527;
        scheduleFooterMouse(527);
        printf("PREFERENCES_PROBE queued isolated mouse click for Default\n");
        return -1;
    case 12:
        if (panel->settingValueForProbe("3d.zoom") != "1.000000"
            || panel->settingValueForProbe("3d.enabled") != "true" || gStagedLegacyDifficulty != 1) {
            failProbe("Defaults did not restore the staged settings");
            return KEY_ESCAPE;
        }
        printf("PREFERENCES_PROBE PASS default values staged\n");
        gWaitingForFooter = true;
        gExpectedFooterEvent = 504;
        scheduleFooterMouse(504);
        printf("PREFERENCES_PROBE queued isolated mouse click for Done after Defaults\n");
        return -1;
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
        if (gDrag == DragProbe::Slider) x = gMouseDragTargetX;
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
