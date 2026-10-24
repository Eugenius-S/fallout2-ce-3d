#include "ui_fixture.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../character_editor.h"
#include "../color.h"
#include "../config.h"
#include "../content_config.h"
#include "../display_monitor.h"
#include "../game.h"
#include "../game_dialog.h"
#include "../input.h"
#include "../interface.h"
#include "../inventory.h"
#include "../kb.h"
#include "../pipboy.h"
#include "../scene/text_capture.h"
#include "../scene_renderer/text_present.h"
#include "../scripts.h"
#include "../settings.h"
#include "../svga.h"
#include "../window_manager.h"
#include "devtools.h"

namespace fallout {
namespace devtools {

namespace {

enum class UiFixtureKind {
    None,
    Inventory,
    Pipboy,
    Character,
    Barter,
    Log,
};

enum class UiFixtureStage {
    Idle,
    WaitingForScreen,
    WaitingForDialog,
    WaitingForBarter,
    Capturing,
    Finished,
};

UiFixtureKind gKind = UiFixtureKind::None;
UiFixtureStage gStage = UiFixtureStage::Idle;
bool gInvalidUiFixture = false;
char gSpeakerScript[96] = {};
int gSpeakerId = -1;
int gSpeakerTile = -1;
int gInitialDateLogCount = 0;
int gPipboyDiagnosticFrames = 0;

const char* fixtureName()
{
    switch (gKind) {
    case UiFixtureKind::Inventory: return "inventory";
    case UiFixtureKind::Pipboy: return "pipboy";
    case UiFixtureKind::Character: return "character";
    case UiFixtureKind::Barter: return "barter";
    case UiFixtureKind::Log: return "log";
    default: return "none";
    }
}

void fixtureFail(const char* reason)
{
    if (gStage == UiFixtureStage::Finished || gKind == UiFixtureKind::None) return;
    printf("UI_FIXTURE FAIL screen=%s reason=%s\n", fixtureName(), reason);
    fflush(stdout);
    gStage = UiFixtureStage::Finished;
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_EXIT;
}

int expectedWindow()
{
    switch (gKind) {
    case UiFixtureKind::Inventory:
    case UiFixtureKind::Barter: return inventoryGetWindow();
    case UiFixtureKind::Pipboy: return pipboyGetWindow();
    case UiFixtureKind::Character: return characterEditorGetWindow();
    case UiFixtureKind::Log: return gInterfaceBarWindow;
    default: return -1;
    }
}

bool snapshotWindow(int windowId, TextWindowSnapshot* result)
{
    if (windowId < 0 || result == nullptr) return false;
    TextWindowSnapshot first = {};
    int count = 0;
    if (!textCaptureGetWindowSnapshot(0, &first, &count)) return false;
    for (int index = 0; index < count; index++) {
        TextWindowSnapshot snapshot = {};
        if (!textCaptureGetWindowSnapshot(index, &snapshot, nullptr)) continue;
        if (snapshot.id == windowId && !snapshot.hidden && snapshot.pixels != nullptr) {
            *result = snapshot;
            return true;
        }
    }
    return false;
}

std::vector<scene::TextRecord> eligibleRuns(const TextWindowSnapshot& window)
{
    const scene::TextBufferId buffer = textCaptureBufferId(window.pixels);
    if (buffer == 0) return {};
    std::vector<scene::TextRecord> result;
    for (const scene::TextRecord& record : scene::textCaptureSharpRecords(buffer)) {
        if (record.ownerWindow == window.id && record.run.font == 101 && record.run.sharpEligible) {
            result.push_back(record);
        }
    }
    return result;
}

bool isDateLogRun(const scene::TextRecord& record)
{
    if (record.run.text.find(':') == std::string::npos) return false;
    // The stock '/' key appends the current in-game date and time from editor.msg.
    // Its year is stable for the duration of this isolated new-game fixture.
    int yearValue = 0;
    gameTimeGetDate(nullptr, nullptr, &yearValue);
    const std::string year = std::to_string(yearValue);
    return record.run.text.find(year) != std::string::npos;
}

struct ScreenProof {
    std::vector<scene::TextRecord> records;
    std::vector<scene::TextRecord> visibleRecords;
    std::vector<scene::TextRecord> overlayRecords;
    unsigned int visibleCoverage = 0;
    unsigned int overlayPixels = 0;
};

unsigned int countOverlayInk(const scene::TextVisibilityMask& mask,
    const scene_renderer::PreparedSharpTextFrame& frame)
{
    if (mask.pixels.empty() || mask.bounds.width <= 0 || mask.bounds.height <= 0
        || frame.width <= 0 || frame.height <= 0 || frame.overlay.width <= 0 || frame.overlay.height <= 0
        || frame.overlay.pitch < frame.overlay.width * 4
        || frame.overlay.pixels.size() < static_cast<std::size_t>(frame.overlay.pitch) * frame.overlay.height) {
        return 0;
    }

    const double sx = static_cast<double>(frame.overlay.width) / frame.width;
    const double sy = static_cast<double>(frame.overlay.height) / frame.height;
    const int left = std::clamp(static_cast<int>(std::floor(mask.bounds.x * sx)), 0, frame.overlay.width);
    const int top = std::clamp(static_cast<int>(std::floor(mask.bounds.y * sy)), 0, frame.overlay.height);
    const int right = std::clamp(static_cast<int>(std::ceil((mask.bounds.x + mask.bounds.width) * sx)), 0, frame.overlay.width);
    const int bottom = std::clamp(static_cast<int>(std::ceil((mask.bounds.y + mask.bounds.height) * sy)), 0, frame.overlay.height);
    unsigned int count = 0;
    for (int y = top; y < bottom; y++) {
        const unsigned char* row = frame.overlay.pixels.data() + static_cast<std::size_t>(y) * frame.overlay.pitch;
        for (int x = left; x < right; x++) {
            const int logicalX = std::clamp(static_cast<int>(x / sx), 0, frame.width - 1);
            const int logicalY = std::clamp(static_cast<int>(y / sy), 0, frame.height - 1);
            if (mask.visibleAt(logicalX, logicalY) && row[static_cast<std::size_t>(x) * 4 + 3] != 0) count++;
        }
    }
    return count;
}

ScreenProof screenProof(const TextWindowSnapshot& window,
    const scene_renderer::PreparedSharpTextFrame& frame)
{
    ScreenProof proof;
    if (gSdlSurface == nullptr || gSdlSurface->pixels == nullptr) return proof;
    const int surfaceWidth = gSdlSurface->w;
    const int surfaceHeight = gSdlSurface->h;
    if (frame.active && (frame.width != surfaceWidth || frame.height != surfaceHeight)) return proof;
    const scene::TextBufferId buffer = textCaptureBufferId(gSdlSurface->pixels);
    if (buffer == 0) return proof;

    for (const scene::TextRecord& record : scene::textCaptureSharpRecords(buffer)) {
        if (record.ownerWindow != window.id || record.ownerGeneration != window.generation
            || record.run.font != 101 || !record.run.sharpEligible) continue;
        proof.records.push_back(record);

        scene::TextVisibilityMask mask;
        if (!scene::textCaptureVisibilityMask(record, surfaceWidth, surfaceHeight, &mask)) continue;
        const unsigned int visiblePixels = static_cast<unsigned int>(std::count_if(
            mask.pixels.begin(), mask.pixels.end(), [](unsigned char pixel) { return pixel != 0; }));
        if (visiblePixels == 0) continue;
        proof.visibleCoverage += visiblePixels;
        proof.visibleRecords.push_back(record);

        const unsigned int overlayPixels = frame.active ? countOverlayInk(mask, frame) : 0;
        if (overlayPixels > 0) {
            proof.overlayPixels += overlayPixels;
            proof.overlayRecords.push_back(record);
        }
    }
    return proof;
}

void captureWindow(const TextWindowSnapshot& window, const std::vector<scene::TextRecord>& runs)
{
    if (runs.empty()) return;

    const scene_renderer::PreparedSharpTextFrame& frame = scene_renderer::preparedSharpTextFrame();
    const ScreenProof proof = screenProof(window, frame);
    if (proof.visibleRecords.empty() || proof.visibleCoverage == 0) return;

    std::vector<scene::TextRecord> evidence = proof.visibleRecords;
    std::vector<scene::TextRecord> overlayEvidence = proof.overlayRecords;
    int whiteZeroCount = 0;
    int leftZeroX = -1;
    int rightZeroX = -1;
    const unsigned int colorMask = 0xFF;
    if (gKind == UiFixtureKind::Barter) {
        std::vector<scene::TextRecord> zeroTotals;
        std::vector<scene::TextRecord> visibleZeroTotals;
        std::vector<scene::TextRecord> overlaidZeroTotals;
        for (const scene::TextRecord& record : proof.records) {
            if (record.run.text == "$0"
                && static_cast<Color>(record.run.colorAndFlags & colorMask) == COLOR_WHITE) {
                zeroTotals.push_back(record);
            }
        }
        whiteZeroCount = static_cast<int>(zeroTotals.size());
        if (whiteZeroCount < 2) return;
        if (whiteZeroCount != 2 || zeroTotals[0].run.x == zeroTotals[1].run.x) {
            fixtureFail("barter-does-not-have-exactly-two-white-zero-totals");
            return;
        }
        for (const scene::TextRecord& record : zeroTotals) {
            if (std::any_of(proof.visibleRecords.begin(), proof.visibleRecords.end(), [&record](const scene::TextRecord& visible) {
                    return visible.run.x == record.run.x && visible.run.text == record.run.text;
                })) {
                visibleZeroTotals.push_back(record);
            }
            if (std::any_of(proof.overlayRecords.begin(), proof.overlayRecords.end(), [&record](const scene::TextRecord& overlaid) {
                    return overlaid.run.x == record.run.x && overlaid.run.text == record.run.text;
                })) {
                overlaidZeroTotals.push_back(record);
            }
        }
        if (visibleZeroTotals.size() != 2) return;
        if (settings.scene3d.sharp_text && overlaidZeroTotals.size() != 2) return;
        leftZeroX = std::min(zeroTotals[0].run.x, zeroTotals[1].run.x);
        rightZeroX = std::max(zeroTotals[0].run.x, zeroTotals[1].run.x);
        evidence = std::move(visibleZeroTotals);
        overlayEvidence = std::move(overlaidZeroTotals);
    } else if (gKind == UiFixtureKind::Log) {
        std::vector<scene::TextRecord> dateRuns;
        std::vector<scene::TextRecord> visibleDateRuns;
        std::vector<scene::TextRecord> overlaidDateRuns;
        for (const scene::TextRecord& record : proof.records) {
            if (isDateLogRun(record)) dateRuns.push_back(record);
        }
        if (dateRuns.size() <= static_cast<std::size_t>(gInitialDateLogCount)) return;
        for (const scene::TextRecord& record : dateRuns) {
            if (std::any_of(proof.visibleRecords.begin(), proof.visibleRecords.end(), [&record](const scene::TextRecord& visible) {
                    return visible.run.x == record.run.x && visible.run.text == record.run.text;
                })) {
                visibleDateRuns.push_back(record);
            }
            if (std::any_of(proof.overlayRecords.begin(), proof.overlayRecords.end(), [&record](const scene::TextRecord& overlaid) {
                    return overlaid.run.x == record.run.x && overlaid.run.text == record.run.text;
                })) {
                overlaidDateRuns.push_back(record);
            }
        }
        if (visibleDateRuns.empty()) return;
        if (settings.scene3d.sharp_text && overlaidDateRuns.empty()) return;
        evidence = std::move(visibleDateRuns);
        overlayEvidence = std::move(overlaidDateRuns);
    }

    const bool sharpActive = settings.scene3d.sharp_text && frame.active && frame.drawn;
    if (settings.scene3d.sharp_text && !sharpActive) return;
    if (settings.scene3d.sharp_text && overlayEvidence.empty()) return;

    int greenCount = 0;
    int whiteCount = 0;
    int yellowCount = 0;
    int redCount = 0;
    int otherCount = 0;
    for (const scene::TextRecord& record : proof.records) {
        const Color color = static_cast<Color>(record.run.colorAndFlags & colorMask);
        if (color == COLOR_GREEN) greenCount++;
        else if (color == COLOR_WHITE) whiteCount++;
        else if (color == COLOR_LIGHT_YELLOW) yellowCount++;
        else if (color == COLOR_RED) redCount++;
        else otherCount++;
    }

    gStage = UiFixtureStage::Capturing;
    if (!takeSnapshot()) {
        fixtureFail("snapshot-failed");
        return;
    }

    printf("UI_FIXTURE PASS screen=%s window=%d generation=%llu font=101 runs=%zu screen_runs=%zu visible_runs=%zu visible_coverage=%u overlay_runs=%zu overlay_pixels=%u colors=original green=%d white=%d yellow=%d red=%d other=%d sharp_setting=%d active=%d drawn=%d",
        fixtureName(), window.id, static_cast<unsigned long long>(window.generation), evidence.size(),
        proof.records.size(), proof.visibleRecords.size(), proof.visibleCoverage, overlayEvidence.size(), proof.overlayPixels,
        greenCount, whiteCount, yellowCount, redCount, otherCount,
        settings.scene3d.sharp_text ? 1 : 0, frame.active ? 1 : 0, frame.drawn ? 1 : 0);
    if (gKind == UiFixtureKind::Barter) {
        printf(" speaker_script=%s speaker=%d speaker_tile=%d white_zero_totals=%d left=$0@%d right=$0@%d",
            gSpeakerScript, gSpeakerId, gSpeakerTile, whiteZeroCount, leftZeroX, rightZeroX);
    } else if (gKind == UiFixtureKind::Log) {
        printf(" baseline_date_runs=%d matching_date_runs=%zu real_date_log=%s",
            gInitialDateLogCount, evidence.size(), evidence.front().run.text.c_str());
    } else {
        printf(" sample=%s", evidence.front().run.text.c_str());
    }
    printf("\n");
    fflush(stdout);
    gStage = UiFixtureStage::Finished;
}

} // namespace

bool parseUiFixtureOption(const char* arg)
{
    const char* prefix = "--dev-ui-fixture=";
    if (strncmp(arg, prefix, strlen(prefix)) != 0) return false;
    const char* value = arg + strlen(prefix);
    if (strcmp(value, "inventory") == 0) gKind = UiFixtureKind::Inventory;
    else if (strcmp(value, "pipboy") == 0) gKind = UiFixtureKind::Pipboy;
    else if (strcmp(value, "character") == 0) gKind = UiFixtureKind::Character;
    else if (strcmp(value, "barter") == 0) gKind = UiFixtureKind::Barter;
    else if (strcmp(value, "log") == 0) gKind = UiFixtureKind::Log;
    else {
        gInvalidUiFixture = true;
        fprintf(stderr, "devtools: invalid --dev-ui-fixture value (expected inventory, pipboy, character, barter, or log)\n");
        fflush(stderr);
    }
    return true;
}

bool startUiFixtureWhenReady(bool startupReady, bool blockingState)
{
    if (gInvalidUiFixture && gStage == UiFixtureStage::Idle && startupReady && !blockingState) {
        printf("UI_FIXTURE FAIL screen=unknown reason=invalid-mode\n");
        fflush(stdout);
        gStage = UiFixtureStage::Finished;
        _game_user_wants_to_quit = GAME_QUIT_REQUEST_EXIT;
        return false;
    }
    if (gKind == UiFixtureKind::None || gStage != UiFixtureStage::Idle
        || !startupReady || blockingState) return false;

    if (gKind == UiFixtureKind::Barter) {
        gStage = UiFixtureStage::WaitingForDialog;
        printf("UI_FIXTURE START screen=barter route=Klamath-Sulik-real-dialog-key-B\n");
        fflush(stdout);
        return true;
    }

    gStage = UiFixtureStage::WaitingForScreen;
    int key = -1;
    switch (gKind) {
    case UiFixtureKind::Inventory: key = KEY_LOWERCASE_I; break;
    case UiFixtureKind::Pipboy: key = KEY_LOWERCASE_P; break;
    case UiFixtureKind::Character: key = KEY_LOWERCASE_C; break;
    case UiFixtureKind::Log: key = KEY_SLASH; break;
    default: break;
    }
    if (gKind == UiFixtureKind::Pipboy) {
        // The isolated test map can start without a Pip-Boy. Use the engine's
        // existing start option in memory so the normal P-key path is available.
        if (!configSetInt(&gContentConfig, CONTENT_CONFIG_START_SECTION, "pipboy", 2)) {
            fixtureFail("could-not-enable-process-only-start-pipboy");
            return false;
        }
        pipboyReset();
        printf("UI_FIXTURE SETUP screen=pipboy start_pipboy=2 process_only=1\n");
        fflush(stdout);
    }
    if (key == -1) {
        fixtureFail("no-engine-open-key");
        return false;
    }
    if (gKind == UiFixtureKind::Log) {
        TextWindowSnapshot window = {};
        if (snapshotWindow(gInterfaceBarWindow, &window)) {
            for (const scene::TextRecord& record : eligibleRuns(window)) {
                if (isDateLogRun(record)) gInitialDateLogCount++;
            }
        }
    }
    printf("UI_FIXTURE START screen=%s engine_key=%d\n", fixtureName(), key);
    fflush(stdout);
    enqueueInputEvent(key);
    return false;
}

bool uiFixtureUsesDialogBarter()
{
    return gKind == UiFixtureKind::Barter;
}

void uiFixtureDialogReady(const char* scriptName, int speakerId, int speakerTile, int eligibleRunCount)
{
    if (gKind != UiFixtureKind::Barter || gStage != UiFixtureStage::WaitingForDialog) return;
    if (eligibleRunCount <= 0) return;
    snprintf(gSpeakerScript, sizeof(gSpeakerScript), "%s", scriptName != nullptr ? scriptName : "unknown");
    gSpeakerId = speakerId;
    gSpeakerTile = speakerTile;
    gStage = UiFixtureStage::WaitingForBarter;
    printf("UI_FIXTURE STEP screen=barter dialog_runs=%d script=%s speaker=%d speaker_tile=%d input=KEY_B\n",
        eligibleRunCount, gSpeakerScript, gSpeakerId, gSpeakerTile);
    fflush(stdout);
    enqueueInputEvent(KEY_LOWERCASE_B);
}

void uiFixtureVanillaPresented()
{
    if (gStage != UiFixtureStage::WaitingForScreen && gStage != UiFixtureStage::WaitingForBarter) return;
    if (gKind == UiFixtureKind::Barter && gStage != UiFixtureStage::WaitingForBarter) return;

    const int windowId = expectedWindow();
    TextWindowSnapshot window = {};
    if (!snapshotWindow(windowId, &window)) {
        if (gKind == UiFixtureKind::Pipboy && gPipboyDiagnosticFrames < 8) {
            TextWindowSnapshot first = {};
            int count = 0;
            textCaptureGetWindowSnapshot(0, &first, &count);
            printf("UI_FIXTURE WAIT screen=pipboy frame=%d target_window=%d snapshot_count=%d\n",
                gPipboyDiagnosticFrames++, windowId, count);
            fflush(stdout);
        }
        return;
    }

    std::vector<scene::TextRecord> runs = eligibleRuns(window);
    if (gKind == UiFixtureKind::Pipboy && gPipboyDiagnosticFrames < 8) {
        const ScreenProof proof = screenProof(window, scene_renderer::preparedSharpTextFrame());
        printf("UI_FIXTURE WAIT screen=pipboy frame=%d window=%d generation=%llu eligible=%zu screen_runs=%zu visible_runs=%zu coverage=%u overlay_runs=%zu\n",
            gPipboyDiagnosticFrames++, window.id, static_cast<unsigned long long>(window.generation), runs.size(),
            proof.records.size(), proof.visibleRecords.size(), proof.visibleCoverage, proof.overlayRecords.size());
        fflush(stdout);
    }
    if (runs.empty()) return;
    captureWindow(window, runs);
}

} // namespace devtools
} // namespace fallout
