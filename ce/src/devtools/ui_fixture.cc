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
#include "../text_object.h"
#include "../tile.h"
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
    Bark,
};

enum class UiFixtureStage {
    Idle,
    WaitingForScreen,
    WaitingForDialog,
    WaitingForBarter,
    WaitingForBark,
    WaitingForBarkReplacement,
    WaitingForBarkExpiration,
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
bool gPipboyStatusRequested = false;
int gLogDiagnosticFrames = 0;
bool gFont101PreviewRequested = false;
bool gFont101PreviewActivated = false;
double gFont101PreviewScale = 1.29;
bool gFont101PreviewWeightRequested = false;
bool gFont101PreviewBold = false;
int gBarkBaselineCount = 0;
int gBarkWaitFrames = 0;
bool gBarkFirstCaptured = false;
bool gBarkReplacementSeen = false;
// CP1251 Russian bark fixtures.
char gBarkFirstText[] = "\xD2\xFB \xEC\xE5\xED\xFF \xEF\xF3\xE3\xE0\xE5\xF8\xFC.";
char gBarkReplacementText[] = "\xD1\xF2\xEE\xE9!";

const char* fixtureName()
{
    switch (gKind) {
    case UiFixtureKind::Inventory: return "inventory";
    case UiFixtureKind::Pipboy: return "pipboy";
    case UiFixtureKind::Character: return "character";
    case UiFixtureKind::Barter: return "barter";
    case UiFixtureKind::Log: return "log";
    case UiFixtureKind::Bark: return "bark";
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

int outlinedTextCount(const char* text, std::size_t* ownedPixelCount = nullptr)
{
    int count = 0;
    std::size_t pixels = 0;
    int windowCount = 0;
    TextWindowSnapshot first = {};
    if (!textCaptureGetWindowSnapshot(0, &first, &windowCount)) return 0;
    for (int index = 0; index < windowCount; index++) {
        TextWindowSnapshot window = {};
        if (!textCaptureGetWindowSnapshot(index, &window, nullptr) || window.hidden || window.pixels == nullptr) continue;
        for (const scene::TextRecord& record : scene::textCaptureRecords(window.generation)) {
            if (record.run.font == 101 && record.run.hasOutline && record.run.text == text) {
                count++;
                pixels += record.pixels.size();
            }
        }
    }
    if (ownedPixelCount != nullptr) *ownedPixelCount = pixels;
    return count;
}

void runBarkFixtureFrame()
{
    if (gStage == UiFixtureStage::Idle || gStage == UiFixtureStage::Finished) return;
    gBarkWaitFrames++;
    const int stageFrameLimit = gStage == UiFixtureStage::WaitingForBarkExpiration ? 1200 : 300;
    if (gBarkWaitFrames > stageFrameLimit) {
        fixtureFail("bark-lifecycle-timeout");
        return;
    }
    if (gStage == UiFixtureStage::WaitingForBark) {
        std::size_t firstOwnedPixels = 0;
        if (outlinedTextCount(gBarkFirstText, &firstOwnedPixels) == 0) return;
        gStage = UiFixtureStage::Capturing;
        if (!takeSnapshot()) {
            fixtureFail("bark-first-snapshot-failed");
            return;
        }
        gBarkFirstCaptured = true;
        gStage = UiFixtureStage::WaitingForBark;
        printf("UI_FIXTURE STEP screen=bark stage=visible hasOutline=1 ownedPixels=%zu text=first-ru-line\n", firstOwnedPixels);
        fflush(stdout);
        Rect refreshRect;
        if (textObjectAdd(gDude, gBarkReplacementText, 101, COLOR_LIGHT_YELLOW, COLOR_BLACK, &refreshRect) == -1) {
            fixtureFail("bark-replacement-add-failed");
            return;
        }
        tileWindowRefreshRect(&refreshRect, gDude->elevation);
        gStage = UiFixtureStage::WaitingForBarkReplacement;
        gBarkWaitFrames = 0;
        int sceneOffsetX = 0;
        int sceneOffsetY = 0;
        tileSceneShift(gDude->tile, &sceneOffsetX, &sceneOffsetY);
        printf("UI_FIXTURE STEP screen=bark stage=replace old=long-ru new=short-ru tile=%d scene_shift=%d,%d objects=%d baseline=%d\n",
            gDude->tile, sceneOffsetX, sceneOffsetY, textObjectsGetCount(), gBarkBaselineCount);
        fflush(stdout);
        return;
    }
    if (gStage == UiFixtureStage::WaitingForBarkReplacement) {
        if (gBarkWaitFrames == 60) {
            int sceneOffsetX = 0;
            int sceneOffsetY = 0;
            tileSceneShift(gDude->tile, &sceneOffsetX, &sceneOffsetY);
            printf("UI_FIXTURE DIAG screen=bark stage=replacement-wait frames=%d tile=%d scene_shift=%d,%d old_runs=%d new_runs=%d objects=%d baseline=%d\n",
                gBarkWaitFrames, gDude->tile, sceneOffsetX, sceneOffsetY,
                outlinedTextCount(gBarkFirstText), outlinedTextCount(gBarkReplacementText),
                textObjectsGetCount(), gBarkBaselineCount);
            fflush(stdout);
        }
        if (outlinedTextCount(gBarkFirstText) != 0 || outlinedTextCount(gBarkReplacementText) == 0) return;
        std::size_t replacementOwnedPixels = 0;
        outlinedTextCount(gBarkReplacementText, &replacementOwnedPixels);
        gBarkReplacementSeen = true;
        gStage = UiFixtureStage::WaitingForBarkExpiration;
        gBarkWaitFrames = 0;
        printf("UI_FIXTURE STEP screen=bark stage=replaced hasOutline=1 ownedPixels=%zu text=short-ru-line\n", replacementOwnedPixels);
        fflush(stdout);
        return;
    }
    if (gStage == UiFixtureStage::WaitingForBarkExpiration
        && textObjectsGetCount() <= gBarkBaselineCount
        && outlinedTextCount(gBarkFirstText) == 0
        && outlinedTextCount(gBarkReplacementText) == 0) {
        gStage = UiFixtureStage::Capturing;
        if (!takeSnapshot()) {
            fixtureFail("bark-expiration-snapshot-failed");
            return;
        }
        printf("UI_FIXTURE PASS screen=bark first=%d replacement=%d cleared_runs=0\n",
            gBarkFirstCaptured ? 1 : 0, gBarkReplacementSeen ? 1 : 0);
        fflush(stdout);
        gStage = UiFixtureStage::Finished;
        _game_user_wants_to_quit = GAME_QUIT_REQUEST_EXIT;
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
    int font101LineHeightMin = 0;
    int font101LineHeightMax = 0;
    int statBoxRuns = 0;
    int statBoxLineHeight = 0;
};

struct OpaqueOverlaySample {
    int logicalX = -1;
    int logicalY = -1;
    int physicalX = -1;
    int physicalY = -1;
    unsigned char rgb[3] = {};
    unsigned char alpha = 0;
};

bool sampleOpaqueOverlayPixel(const scene::TextRecord& record,
    const scene_renderer::PreparedSharpTextFrame& frame, const unsigned char expectedRgb[3],
    OpaqueOverlaySample* result)
{
    if (result == nullptr || !frame.active || !frame.drawn || frame.width <= 0 || frame.height <= 0
        || frame.overlay.width <= 0 || frame.overlay.height <= 0
        || frame.overlay.pitch < frame.overlay.width * 4
        || frame.overlay.pixels.size() < static_cast<std::size_t>(frame.overlay.pitch) * frame.overlay.height) return false;

    scene::TextVisibilityMask mask;
    if (!scene::textCaptureVisibilityMask(record, frame.width, frame.height, &mask) || mask.pixels.empty()) return false;

    const double sx = static_cast<double>(frame.overlay.width) / frame.width;
    const double sy = static_cast<double>(frame.overlay.height) / frame.height;
    const int logicalLeft = std::max(mask.bounds.x, record.run.x);
    const int logicalTop = std::max(mask.bounds.y, record.run.y);
    const int logicalRight = std::min(mask.bounds.x + mask.bounds.width, record.run.x + record.run.width);
    const int logicalBottom = std::min(mask.bounds.y + mask.bounds.height, record.run.y + record.run.lineHeight);
    if (logicalRight <= logicalLeft || logicalBottom <= logicalTop) return false;

    const int left = std::clamp(static_cast<int>(std::floor(logicalLeft * sx)), 0, frame.overlay.width);
    const int top = std::clamp(static_cast<int>(std::floor(logicalTop * sy)), 0, frame.overlay.height);
    const int right = std::clamp(static_cast<int>(std::ceil(logicalRight * sx)), 0, frame.overlay.width);
    const int bottom = std::clamp(static_cast<int>(std::ceil(logicalBottom * sy)), 0, frame.overlay.height);
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            const int logicalX = std::clamp(static_cast<int>(x / sx), 0, frame.width - 1);
            const int logicalY = std::clamp(static_cast<int>(y / sy), 0, frame.height - 1);
            if (!mask.visibleAt(logicalX, logicalY)) continue;
            const unsigned char* pixel = frame.overlay.pixels.data()
                + static_cast<std::size_t>(y) * frame.overlay.pitch + x * 4;
            if (pixel[3] != 255 || pixel[0] != expectedRgb[0]
                || pixel[1] != expectedRgb[1] || pixel[2] != expectedRgb[2]) continue;
            result->logicalX = logicalX;
            result->logicalY = logicalY;
            result->physicalX = x;
            result->physicalY = y;
            std::copy(pixel, pixel + 3, result->rgb);
            result->alpha = pixel[3];
            return true;
        }
    }
    return false;
}

bool currentPaletteColor(unsigned int index, unsigned char rgb[3])
{
    if (rgb == nullptr || gSdlSurface == nullptr || gSdlSurface->format == nullptr
        || gSdlSurface->format->palette == nullptr || gSdlSurface->format->palette->colors == nullptr
        || index >= static_cast<unsigned int>(gSdlSurface->format->palette->ncolors)) return false;
    const SDL_Color& color = gSdlSurface->format->palette->colors[index];
    rgb[0] = color.r;
    rgb[1] = color.g;
    rgb[2] = color.b;
    return true;
}

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
        if (proof.font101LineHeightMin == 0 || record.run.lineHeight < proof.font101LineHeightMin) {
            proof.font101LineHeightMin = record.run.lineHeight;
        }
        proof.font101LineHeightMax = std::max(proof.font101LineHeightMax, record.run.lineHeight);
        if (record.run.verticalAlignmentBoxHeight > 0) {
            proof.statBoxRuns++;
            proof.statBoxLineHeight = record.run.lineHeight;
        }

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

    OpaqueOverlaySample interfaceGreenSample;
    OpaqueOverlaySample hudLogSample;
    OpaqueOverlaySample blackTextSample;
    OpaqueOverlaySample whiteTotalSamples[2];
    bool hasInterfaceGreenSample = false;
    bool hasHudLogSample = false;
    bool hasBlackTextSample = false;
    int whiteTotalSampleCount = 0;
    static constexpr unsigned char kInterfaceGreen[] = { 0x3F, 0xB4, 0x19 };
    static constexpr unsigned char kHudLogGreen[] = { 0x4B, 0xD5, 0x1F };

    if (sharpActive && (gKind == UiFixtureKind::Inventory || gKind == UiFixtureKind::Character)) {
        for (const scene::TextRecord& record : proof.overlayRecords) {
            if (record.run.colorRole != scene::TextColorRole::InterfaceGreen
                || static_cast<Color>(record.run.colorAndFlags & colorMask) != COLOR_GREEN) continue;
            hasInterfaceGreenSample = sampleOpaqueOverlayPixel(record, frame, kInterfaceGreen, &interfaceGreenSample);
            if (hasInterfaceGreenSample) break;
        }
        if (!hasInterfaceGreenSample) {
            fixtureFail("could-not-find-opaque-interface-green-overlay-pixel");
            return;
        }
    } else if (sharpActive && gKind == UiFixtureKind::Log) {
        for (const scene::TextRecord& record : proof.overlayRecords) {
            if (record.run.colorRole != scene::TextColorRole::HudMessageLogGreen
                || static_cast<Color>(record.run.colorAndFlags & colorMask) != COLOR_GREEN) continue;
            hasHudLogSample = sampleOpaqueOverlayPixel(record, frame, kHudLogGreen, &hudLogSample);
            if (hasHudLogSample) break;
        }
        if (!hasHudLogSample) {
            fixtureFail("could-not-find-opaque-hud-log-green-overlay-pixel");
            return;
        }
    }

    if (sharpActive && gKind == UiFixtureKind::Barter) {
        for (const scene::TextRecord& record : overlayEvidence) {
            if (record.run.text != "$0"
                || static_cast<Color>(record.run.colorAndFlags & colorMask) != COLOR_WHITE
                || whiteTotalSampleCount >= 2) continue;
            unsigned char expectedWhite[3] = {};
            if (currentPaletteColor(static_cast<unsigned int>(record.run.colorAndFlags & colorMask), expectedWhite)
                && sampleOpaqueOverlayPixel(record, frame, expectedWhite, &whiteTotalSamples[whiteTotalSampleCount])) {
                whiteTotalSampleCount++;
            }
        }
        if (whiteTotalSampleCount != 2) {
            fixtureFail("white-barter-totals-do-not-match-current-palette");
            return;
        }
    }

    if (sharpActive && gKind == UiFixtureKind::Character) {
        if (proof.statBoxRuns <= 0) {
            fixtureFail("special-stat-box-record-missing");
            return;
        }
        for (const scene::TextRecord& record : proof.overlayRecords) {
            if (record.run.colorRole != scene::TextColorRole::Palette
                || static_cast<Color>(record.run.colorAndFlags & colorMask) != COLOR_BLACK) continue;
            unsigned char expectedBlack[3] = {};
            if (currentPaletteColor(static_cast<unsigned int>(record.run.colorAndFlags & colorMask), expectedBlack)
                && expectedBlack[0] == 0 && expectedBlack[1] == 0 && expectedBlack[2] == 0
                && sampleOpaqueOverlayPixel(record, frame, expectedBlack, &blackTextSample)) {
                hasBlackTextSample = true;
                break;
            }
        }
    }

    int greenCount = 0;
    int whiteCount = 0;
    int yellowCount = 0;
    int redCount = 0;
    int otherCount = 0;
    int compactSizeRoleCount = 0;
    int defaultSizeRoleCount = 0;
    double effectiveSizeMinimum = 100.0;
    double effectiveSizeMaximum = 0.0;
    for (const scene::TextRecord& record : proof.records) {
        const Color color = static_cast<Color>(record.run.colorAndFlags & colorMask);
        if (color == COLOR_GREEN) greenCount++;
        else if (color == COLOR_WHITE) whiteCount++;
        else if (color == COLOR_LIGHT_YELLOW) yellowCount++;
        else if (color == COLOR_RED) redCount++;
        else otherCount++;
        if (record.run.sizeRole == scene::TextSizeRole::CompactUi) compactSizeRoleCount++;
        else defaultSizeRoleCount++;
        const double runScale = scene::font101PreviewScaleForRun(window.id, record.run.sizeRole);
        effectiveSizeMinimum = std::min(effectiveSizeMinimum, runScale);
        effectiveSizeMaximum = std::max(effectiveSizeMaximum, runScale);
    }
    if (settings.scene3d.sharp_text
        && (gKind == UiFixtureKind::Inventory || gKind == UiFixtureKind::Pipboy)
        && (compactSizeRoleCount != static_cast<int>(proof.records.size()) || defaultSizeRoleCount != 0)) {
        fixtureFail("compact-ui-font-size-role-missing");
        return;
    }

    gStage = UiFixtureStage::Capturing;
    if (!takeSnapshot()) {
        fixtureFail("snapshot-failed");
        return;
    }

    printf("UI_FIXTURE PASS screen=%s window=%d generation=%llu font=101 runs=%zu screen_runs=%zu visible_runs=%zu visible_coverage=%u overlay_runs=%zu overlay_pixels=%u font101_lineheight=%d..%d stat_box_runs=%d stat_box_lineheight=%d colors=original green=%d white=%d yellow=%d red=%d other=%d sharp_setting=%d active=%d drawn=%d preview_scale=%.2f preview_weight=%s compact_size_role_runs=%d default_size_role_runs=%d run_scale=%.2f..%.2f",
        fixtureName(), window.id, static_cast<unsigned long long>(window.generation), evidence.size(),
        proof.records.size(), proof.visibleRecords.size(), proof.visibleCoverage, overlayEvidence.size(), proof.overlayPixels,
        proof.font101LineHeightMin, proof.font101LineHeightMax, proof.statBoxRuns, proof.statBoxLineHeight,
        greenCount, whiteCount, yellowCount, redCount, otherCount,
        settings.scene3d.sharp_text ? 1 : 0, frame.active ? 1 : 0, frame.drawn ? 1 : 0,
        scene::font101PreviewScaleForWindow(window.id),
        scene::font101PreviewUsesBold(window.id) ? "bold" : "semibold",
        compactSizeRoleCount, defaultSizeRoleCount,
        effectiveSizeMinimum, effectiveSizeMaximum);
    if (gKind == UiFixtureKind::Barter) {
        printf(" speaker_script=%s speaker=%d speaker_tile=%d white_zero_totals=%d left=$0@%d right=$0@%d",
            gSpeakerScript, gSpeakerId, gSpeakerTile, whiteZeroCount, leftZeroX, rightZeroX);
    } else if (gKind == UiFixtureKind::Log) {
        printf(" baseline_date_runs=%d matching_date_runs=%zu real_date_log=%s",
            gInitialDateLogCount, evidence.size(), evidence.front().run.text.c_str());
    } else {
        printf(" sample=%s", evidence.front().run.text.c_str());
    }
    if (hasInterfaceGreenSample) {
        printf(" interface_green_rgb=%u,%u,%u logical=%d,%d physical=%d,%d alpha=%u",
            interfaceGreenSample.rgb[0], interfaceGreenSample.rgb[1], interfaceGreenSample.rgb[2],
            interfaceGreenSample.logicalX, interfaceGreenSample.logicalY,
            interfaceGreenSample.physicalX, interfaceGreenSample.physicalY, interfaceGreenSample.alpha);
    }
    if (hasHudLogSample) {
        printf(" hud_log_green_rgb=%u,%u,%u logical=%d,%d physical=%d,%d alpha=%u",
            hudLogSample.rgb[0], hudLogSample.rgb[1], hudLogSample.rgb[2],
            hudLogSample.logicalX, hudLogSample.logicalY,
            hudLogSample.physicalX, hudLogSample.physicalY, hudLogSample.alpha);
    }
    if (whiteTotalSampleCount == 2) {
        printf(" barter_white_palette_rgb=%u,%u,%u first=%d,%d second=%d,%d",
            whiteTotalSamples[0].rgb[0], whiteTotalSamples[0].rgb[1], whiteTotalSamples[0].rgb[2],
            whiteTotalSamples[0].physicalX, whiteTotalSamples[0].physicalY,
            whiteTotalSamples[1].physicalX, whiteTotalSamples[1].physicalY);
    }
    if (hasBlackTextSample) {
        printf(" black_text_rgb=%u,%u,%u logical=%d,%d physical=%d,%d alpha=%u",
            blackTextSample.rgb[0], blackTextSample.rgb[1], blackTextSample.rgb[2],
            blackTextSample.logicalX, blackTextSample.logicalY,
            blackTextSample.physicalX, blackTextSample.physicalY, blackTextSample.alpha);
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
    else if (strcmp(value, "bark") == 0) gKind = UiFixtureKind::Bark;
    else {
        gInvalidUiFixture = true;
        fprintf(stderr, "devtools: invalid --dev-ui-fixture value (expected inventory, pipboy, character, barter, log, or bark)\n");
        fflush(stderr);
    }
    return true;
}

bool parseUiFixtureFont101PreviewOption(const char* arg)
{
    const char* prefix = "--dev-font101-size-preview=";
    if (strncmp(arg, prefix, strlen(prefix)) != 0) return false;
    const char* value = arg + strlen(prefix);
    if (strcmp(value, "1.00") == 0) gFont101PreviewScale = 1.0;
    else if (strcmp(value, "1.08") == 0) gFont101PreviewScale = 1.08;
    else if (strcmp(value, "1.15") == 0) gFont101PreviewScale = 1.15;
    else if (strcmp(value, "1.22") == 0) gFont101PreviewScale = 1.22;
    else if (strcmp(value, "1.29") == 0) gFont101PreviewScale = 1.29;
    else {
        gInvalidUiFixture = true;
        fprintf(stderr, "devtools: invalid font-101 size preview (expected 1.00, 1.08, 1.15, 1.22, or 1.29)\n");
        fflush(stderr);
        return true;
    }
    gFont101PreviewRequested = true;
    return true;
}

bool parseUiFixtureFont101WeightOption(const char* arg)
{
    const char* prefix = "--dev-font101-weight-preview=";
    if (strncmp(arg, prefix, strlen(prefix)) != 0) return false;
    const char* value = arg + strlen(prefix);
    if (strcmp(value, "bold") == 0) gFont101PreviewBold = true;
    else if (strcmp(value, "semibold") == 0) gFont101PreviewBold = false;
    else {
        gInvalidUiFixture = true;
        fprintf(stderr, "devtools: invalid font-101 weight preview (expected bold or semibold)\n");
        fflush(stderr);
        return true;
    }
    gFont101PreviewWeightRequested = true;
    return true;
}

bool startUiFixtureWhenReady(bool startupReady, bool blockingState)
{
    if (gFont101PreviewRequested && (gKind != UiFixtureKind::Character || !settings.scene3d.sharp_text)) {
        gInvalidUiFixture = true;
    }
    if (gFont101PreviewWeightRequested && (gKind != UiFixtureKind::Character
            || !settings.scene3d.sharp_text || !gFont101PreviewRequested)) {
        gInvalidUiFixture = true;
    }
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

    if (gKind == UiFixtureKind::Bark) {
        if (gDude == nullptr) {
            fixtureFail("bark-fixture-requires-player-character");
            return false;
        }
        gBarkBaselineCount = textObjectsGetCount();
        Rect refreshRect;
        if (textObjectAdd(gDude, gBarkFirstText, 101, COLOR_LIGHT_YELLOW, COLOR_BLACK, &refreshRect) == -1) {
            fixtureFail("bark-first-add-failed");
            return false;
        }
        tileWindowRefreshRect(&refreshRect, gDude->elevation);
        gStage = UiFixtureStage::WaitingForBark;
        printf("UI_FIXTURE START screen=bark source=textObjectAdd owner=player\n");
        fflush(stdout);
        return false;
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
    if (gKind == UiFixtureKind::Character && gFont101PreviewRequested) {
        if (!scene::setFont101PreviewScale(gFont101PreviewScale, -1)) {
            fixtureFail("invalid-font101-preview-scale");
            return false;
        }
        if (gFont101PreviewWeightRequested) {
            scene::setFont101PreviewBold(gFont101PreviewBold, -1);
        }
        printf("UI_FIXTURE SETUP screen=character preview_scale=%.2f preview_weight=%s process_only=1\n",
            gFont101PreviewScale,
            gFont101PreviewWeightRequested ? (gFont101PreviewBold ? "bold" : "semibold") : "selected-bold");
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
    if (gKind == UiFixtureKind::Bark) {
        runBarkFixtureFrame();
        return;
    }
    if (gStage != UiFixtureStage::WaitingForScreen && gStage != UiFixtureStage::WaitingForBarter) return;
    if (gKind == UiFixtureKind::Barter && gStage != UiFixtureStage::WaitingForBarter) return;

    const int windowId = expectedWindow();
    TextWindowSnapshot window = {};
    if (!snapshotWindow(windowId, &window)) {
        if (gKind == UiFixtureKind::Log && gLogDiagnosticFrames < 8) {
            TextWindowSnapshot first = {};
            int count = 0;
            textCaptureGetWindowSnapshot(0, &first, &count);
            printf("UI_FIXTURE WAIT screen=log callback=entered frame=%d target_window=%d snapshot_count=%d baseline_date_runs=%d\n",
                gLogDiagnosticFrames++, windowId, count, gInitialDateLogCount);
            fflush(stdout);
        }
        return;
    }

    if (gKind == UiFixtureKind::Character && gFont101PreviewRequested && !gFont101PreviewActivated) {
        if (!scene::setFont101PreviewScale(gFont101PreviewScale, window.id)) {
            fixtureFail("invalid-font101-preview-scale");
            return;
        }
        if (gFont101PreviewWeightRequested) {
            scene::setFont101PreviewBold(gFont101PreviewBold, window.id);
        }
        gFont101PreviewActivated = true;
        printf("UI_FIXTURE STEP screen=character preview_scale=%.2f owner_window=%d action=scope\n",
            gFont101PreviewScale, window.id);
        fflush(stdout);
    }

    if (gKind == UiFixtureKind::Pipboy && !gPipboyStatusRequested) {
        gPipboyStatusRequested = true;
        printf("UI_FIXTURE STEP screen=pipboy window=%d generation=%llu input=STATUS-500\n",
            window.id, static_cast<unsigned long long>(window.generation));
        fflush(stdout);
        enqueueInputEvent(500);
        return;
    }

    std::vector<scene::TextRecord> runs = eligibleRuns(window);
    if (gKind == UiFixtureKind::Log && gLogDiagnosticFrames < 8) {
        const scene_renderer::PreparedSharpTextFrame& frame = scene_renderer::preparedSharpTextFrame();
        const ScreenProof proof = screenProof(window, frame);
        int dateRuns = 0;
        for (const scene::TextRecord& record : proof.records) {
            if (isDateLogRun(record)) dateRuns++;
        }
        printf("UI_FIXTURE WAIT screen=log callback=entered frame=%d window=%d generation=%llu eligible=%zu owner_runs=%zu screen_runs=%zu visible_runs=%zu date_runs=%d baseline_date_runs=%d coverage=%u frame_active=%d frame_drawn=%d overlay_runs=%zu\n",
            gLogDiagnosticFrames++, window.id, static_cast<unsigned long long>(window.generation), runs.size(),
            runs.size(), proof.records.size(), proof.visibleRecords.size(), dateRuns, gInitialDateLogCount,
            proof.visibleCoverage, frame.active ? 1 : 0, frame.drawn ? 1 : 0, proof.overlayRecords.size());
        fflush(stdout);
    }
    if (runs.empty()) return;
    captureWindow(window, runs);
}

void uiFixtureScenePresented()
{
    if (gKind == UiFixtureKind::Bark) runBarkFixtureFrame();
}

} // namespace devtools
} // namespace fallout
