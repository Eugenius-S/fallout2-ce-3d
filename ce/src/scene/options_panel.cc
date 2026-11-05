#include "options_panel.h"
#include "text_capture.h"
#include "menu_text.h"
#include "scene_view.h"
#include "options_frame_owner.h"

#include <lodepng.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../color.h"
#include "../devtools/preferences_probe.h"
#include "../art.h"
#include "../draw.h"
#include "../debug.h"
#include "../game.h"
#include "../input.h"
#include "../kb.h"
#include "../mouse.h"
#include "../scripts.h"
#include "../settings.h"
#include "../scene_renderer/context.h"
#include "../scene_renderer/text_draw.h"
#include "../svga.h"
#include "../text_font.h"
#include "../window_manager.h"

namespace fallout {
namespace scene {

namespace {

const char* localized(const char* text)
{
    return menuText(text, settings.system.language);
}

constexpr int kWidth = 640;
constexpr int kHeight = 480;
constexpr int kLegacyColumnsBottom = 439;
constexpr int kLegacyColumnsShift = 35;
constexpr int kLegacyColumnsRight = 365;
constexpr int kPanelTop = 494;
constexpr int kHeaderHeight = 112;
constexpr int kOptionRowHeight = 28;
constexpr int kGroupHeadingHeight = 36;
constexpr int kGroupGap = 38;
constexpr int kPanelBottomPadding = 32;
constexpr int kLabelX = 55;
constexpr int kValueX = 405;
constexpr int kValueWidth = 180;
constexpr int kSliderX = 355;
constexpr int kSliderRight = 550;
constexpr int kSliderValueX = 500;
constexpr int kSliderValueWidth = 85;
constexpr int kLabelTextOffset = 4;
constexpr int kSliderHitTolerance = 7;
constexpr int kFrameNarrowInset = 16;
constexpr int kScrollbarWidth = 12;
constexpr int kScrollbarOutsideGap = 12;
constexpr int kRowButtonX = 42;
constexpr int kRowButtonWidth = 556;
constexpr int kRowEvent = 600;
constexpr int kScrollEvent = 599;
constexpr int kTrackMargin = 8;
constexpr int kScrollbarThumbHeight = 21;
constexpr int kBackgroundTileWidth = 328;
constexpr int kBackgroundTileHeight = 408;
constexpr int kBackgroundSourceX = 24;
constexpr int kBackgroundPatchWidth = 88;
constexpr int kBackgroundPatchHeight = 48;
constexpr int kBackgroundPatchOverlap = 8;
constexpr int kBackgroundPatchStepX = kBackgroundPatchWidth - kBackgroundPatchOverlap;
constexpr int kBackgroundPatchStepY = kBackgroundPatchHeight - kBackgroundPatchOverlap;
constexpr int kBackgroundPatchY[] = { 50, 126, 205, 287, 367 };
constexpr int kBackgroundPatchColumns = 4;
constexpr int kBackgroundPatchRows = 10;
constexpr int kPanelSideCap = 32;
constexpr int kPanelTopCap = 32;
constexpr int kPanelBottomCap = 32;
constexpr int kPanelRightCapX = 608;

struct RgbColor {
    float red;
    float green;
    float blue;
};

RgbColor paletteRgb(unsigned char index)
{
    return { _cmap[3 * index] * 4.0f, _cmap[3 * index + 1] * 4.0f, _cmap[3 * index + 2] * 4.0f };
}

struct OwnerFramePiece {
    std::vector<unsigned char> rgba;
    unsigned width = 0;
    unsigned height = 0;
};

const std::array<OwnerFramePiece, 8>& ownerFramePieces()
{
    static const auto pieces = [] {
        std::array<OwnerFramePiece, 8> result;
        const unsigned char* bytes[] = { owner_frame::topLeft, owner_frame::top, owner_frame::topRight,
            owner_frame::left, owner_frame::right, owner_frame::bottomLeft, owner_frame::bottom, owner_frame::bottomRight };
        const size_t sizes[] = { sizeof(owner_frame::topLeft), sizeof(owner_frame::top), sizeof(owner_frame::topRight),
            sizeof(owner_frame::left), sizeof(owner_frame::right), sizeof(owner_frame::bottomLeft), sizeof(owner_frame::bottom), sizeof(owner_frame::bottomRight) };
        for (size_t i = 0; i < result.size(); ++i) {
            if (lodepng::decode(result[i].rgba, result[i].width, result[i].height, bytes[i], sizes[i]) != 0) {
                debugPrint("3D options: owner frame could not be decoded; using the original frame\n");
                return std::array<OwnerFramePiece, 8>{};
            }
        }
        return result;
    }();
    return pieces;
}

RgbColor mixRgb(const RgbColor& first, const RgbColor& second, float weight)
{
    return {
        first.red + (second.red - first.red) * weight,
        first.green + (second.green - first.green) * weight,
        first.blue + (second.blue - first.blue) * weight,
    };
}

double editNumber(const SettingValue& value)
{
    if (const auto* integer = std::get_if<int>(&value)) return *integer;
    if (const auto* real = std::get_if<double>(&value)) return *real;
    return 0.0;
}

int sliderTextCenterOffset(int ownerWindow)
{
    ScopedFont font(101);
    const int height = std::max(1, fontGetLineHeight());
    int physicalWidth = 0;
    int physicalHeight = 0;
    int sharpCenter = 0;
    if (sharpTextWanted() && scene_renderer::sharpTextLanguageSupported(settings.system.language)
        && scene_renderer::sharpTextOutputSize(&physicalWidth, &physicalHeight)
        && scene_renderer::sharpTextXHeightCenter(height, ownerWindow,
            static_cast<double>(physicalHeight) / screenGetHeight(), &sharpCenter)) {
        return kLabelTextOffset + sharpCenter;
    }
    const int width = std::max(1, fontGetStringWidth("x"));
    std::vector<unsigned char> probe(static_cast<size_t>(width) * height, 0);
    // Measure the lowercase body, without recording this off-screen probe as
    // interface text. Font line boxes include space above/below the letters.
    fontDrawTextPtr(probe.data(), "x", width, width, COLOR_GREEN | DRAW_TEXT_FLAG_NO_BG);
    int top = height;
    int bottom = -1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (probe[static_cast<size_t>(y) * width + x] == 0) continue;
            top = std::min(top, y);
            bottom = y;
        }
    }
    return kLabelTextOffset + (bottom >= top ? (top + bottom + 1) / 2 : height / 2);
}

void drawRightAlignedControlText(unsigned char* panel, int rowY, int left, int right, const char* text, Color color)
{
    const int width = std::min(fontGetStringWidth(text), right - left);
    if (width <= 0) return;
    const int height = std::max(1, fontGetLineHeight());
    const int pitch = width + 4;
    std::vector<unsigned char> pixels(static_cast<size_t>(pitch) * height, 0);
    // Measure the actual original-font ink, not its advance or side bearing.
    // The probe must not be captured as visible text.
    fontDrawTextPtr(pixels.data(), text, width, pitch, color | DRAW_TEXT_FLAG_NO_BG);
    int inkRight = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (pixels[static_cast<size_t>(y) * pitch + x] != 0) inkRight = std::max(inkRight, x + 1);
        }
    }
    if (inkRight == 0) inkRight = width;
    const int x = std::max(left, right - inkRight);
    ScopedTextRightAlign align(right);
    fontDrawText(panel + kWidth * (rowY + kLabelTextOffset) + x, text, width, kWidth, color);
}

std::pair<int, int> controlTextInkBounds(const char* text, int width, Color color)
{
    const int height = std::max(1, fontGetLineHeight());
    const int pitch = width + 4;
    std::vector<unsigned char> pixels(static_cast<size_t>(pitch) * height, 0);
    fontDrawTextPtr(pixels.data(), text, width, pitch, color | DRAW_TEXT_FLAG_NO_BG);
    int inkLeft = width;
    int inkRight = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (pixels[static_cast<size_t>(y) * pitch + x] == 0) continue;
            inkLeft = std::min(inkLeft, x);
            inkRight = std::max(inkRight, x + 1);
        }
    }
    if (inkRight <= inkLeft) return { 0, width };
    return { inkLeft, inkRight };
}

void drawCheckboxControl(unsigned char* panel, int rowY, int right, bool checked, Color color)
{
    const int leftBracketAdvance = fontGetStringWidth("[");
    const int centerAdvance = std::max(fontGetStringWidth("X"), fontGetStringWidth(" "));
    const int rightBracketAdvance = fontGetStringWidth("]");
    const int left = right - leftBracketAdvance - centerAdvance - rightBracketAdvance;
    const auto leftBracketInk = controlTextInkBounds("[", leftBracketAdvance, color);
    const auto rightBracketInk = controlTextInkBounds("]", rightBracketAdvance, color);
    const auto xInk = controlTextInkBounds("X", fontGetStringWidth("X"), color);
    const int rightBracketOrigin = right - rightBracketAdvance;
    const int openLeft = left + leftBracketInk.second;
    const int openRight = rightBracketOrigin + rightBracketInk.first;
    const int openingCenterTwice = openLeft + openRight;
    const int xInkCenterTwice = xInk.first + xInk.second;
    int xOrigin = (openingCenterTwice - xInkCenterTwice + 1) / 2;
    int sharpWidth = 0;
    int sharpHeight = 0;
    if (sharpTextWanted() && scene_renderer::sharpTextLanguageSupported(settings.system.language)
        && scene_renderer::sharpTextOutputSize(&sharpWidth, &sharpHeight)) {
        // The sharp font has a different X bearing from font 101. Correct its
        // visible position in the logical framebuffer.
        ++xOrigin;
    }
    unsigned char* text = panel + kWidth * (rowY + kLabelTextOffset);
    fontDrawText(text + left, "[", leftBracketAdvance, kWidth, color);
    if (checked) fontDrawText(text + xOrigin, "X", centerAdvance, kWidth, color);
    fontDrawText(text + rightBracketOrigin, "]", rightBracketAdvance, kWidth, color);
}

} // namespace

OptionsPanel::OptionsPanel() = default;
OptionsPanel::~OptionsPanel()
{
    if (!content_.empty()) textCaptureUnregisterExternalBuffer(content_.data());
}

bool OptionsPanel::initialize(int windowId, unsigned char* surface, const unsigned char* background, int backgroundHeight,
    int screenWidth, int screenHeight, int pageX, int pageY)
{
    if (!content_.empty()) textCaptureUnregisterExternalBuffer(content_.data());
    windowId_ = windowId;
    surface_ = surface;
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    pageX_ = pageX;
    pageY_ = pageY;
    model_ = std::make_unique<OptionsSettingsModel>();
    displayIndex_ = gSdlWindow != nullptr ? std::max(0, SDL_GetWindowDisplayIndex(gSdlWindow)) : 0;
    std::vector<SDL_DisplayMode> availableModes;
    const int displayModeCount = SDL_GetNumDisplayModes(displayIndex_);
    for (int modeIndex = 0; modeIndex < displayModeCount; modeIndex++) {
        SDL_DisplayMode mode = {};
        if (SDL_GetDisplayMode(displayIndex_, modeIndex, &mode) != 0 || mode.w < 640 || mode.h < 480
            || mode.w > 7680 || mode.h > 4320) continue;
        if (std::none_of(availableModes.begin(), availableModes.end(), [&mode](const SDL_DisplayMode& existing) {
                return existing.w == mode.w && existing.h == mode.h;
            })) availableModes.push_back(mode);
    }
    std::sort(availableModes.begin(), availableModes.end(), [](const SDL_DisplayMode& left, const SDL_DisplayMode& right) {
        const long long leftArea = static_cast<long long>(left.w) * left.h;
        const long long rightArea = static_cast<long long>(right.w) * right.h;
        if (leftArea != rightArea) return leftArea > rightArea;
        if (left.w != right.w) return left.w > right.w;
        return left.h > right.h;
    });
    std::vector<SettingChoice> resolutionChoices;
    resolutionChoices.reserve(availableModes.size() + 1);
    for (const SDL_DisplayMode& mode : availableModes) {
        resolutionChoices.push_back({ mode.w * 10000 + mode.h, std::to_string(mode.w) + "x" + std::to_string(mode.h) });
    }
    const int selectedResolution = settings.screen.resolution_mode > 0
        ? settings.screen.resolution_mode
        : settings.screen.resolution_x * 10000 + settings.screen.resolution_y;
    if (std::none_of(resolutionChoices.begin(), resolutionChoices.end(), [selectedResolution](const SettingChoice& choice) {
            return choice.value == selectedResolution;
        })) {
        resolutionChoices.insert(resolutionChoices.begin(), { selectedResolution,
            std::to_string(selectedResolution / 10000) + "x" + std::to_string(selectedResolution % 10000) });
    }
    model_->configureChoice("screen.resolution_mode", resolutionChoices);
    model_->setValue("screen.resolution_mode", selectedResolution);
    updateScaleRange();
    rows_ = optionsPanelRows(model_->edits(), "3d");
    sliderTrackOffset_ = sliderTextCenterOffset(windowId_);
    buildRowLayout();
    if (background == nullptr || surface_ == nullptr || backgroundHeight < kHeight || screenWidth_ < kWidth || screenHeight_ < kHeight) return false;
    background_.assign(background, background + static_cast<size_t>(kWidth) * kHeight);
    buildBackgroundTexture();
    content_.resize(static_cast<size_t>(kWidth) * contentHeight_);
    textCaptureRegisterExternalBuffer(content_.data(), content_.size(), kWidth, contentHeight_, kWidth, true);
    for (int y = 0; y < contentHeight_; y++) {
        for (int x = 0; x < kWidth; x++) {
            content_[static_cast<size_t>(kWidth) * y + x] = backgroundTexturePixel(pageX_ + x, pageY_ + y);
        }
    }
    struct Rect { int x; int y; int width; int height; int feather = 0; };
    constexpr Rect legacyBlocks[] = {
        { 6, 40, 190, 74 }, { 6, 116, 190, 80 }, { 6, 197, 190, 82 },
        { 6, 279, 190, 81 }, { 6, 359, 190, 80 },
        { 200, 39, 165, 70 }, { 200, 109, 165, 66 }, { 200, 175, 165, 66 },
        { 200, 241, 165, 66 }, { 200, 307, 165, 66 }, { 200, 373, 165, 66 },
        { 367, 5, 271, 73, 6 }, { 367, 78, 271, 73, 6 },
        { 367, 151, 271, 65, 6 }, { 367, 216, 271, 65, 6 },
        { 367, 281, 271, 65, 6 }, { 367, 346, 271, 65, 6 },
        { 367, 411, 271, 67, 6 },
        { 12, 443, 112, 31 }, { 135, 443, 104, 31 }, { 248, 443, 112, 31 },
    };
    const int legacyCropRight = kWidth - frameInset();
    for (const Rect& rect : legacyBlocks) {
        if (screenHeight_ > kHeight && rect.y == 443) continue;
        const int left = std::max(0, rect.x);
        const int right = std::min({ kWidth, legacyCropRight, rect.x + rect.width });
        const int top = std::max(0, rect.y);
        const int bottom = std::min(kHeight, rect.y + rect.height);
        for (int y = top; y < bottom; y++) {
            for (int x = left; x < right; x++) {
                // Clip the corners of each original settings block against the
                // extended background instead of copying rectangular corners.
                const int radius = rect.y >= 39 && rect.y < 443 ? 5 : 0;
                const int dx = std::max({ rect.x + radius - x, x - (rect.x + rect.width - 1 - radius), 0 });
                const int dy = std::max({ rect.y + radius - y, y - (rect.y + rect.height - 1 - radius), 0 });
                if (dx * dx + dy * dy > radius * radius) continue;
                const unsigned char source = background_[static_cast<size_t>(kWidth) * y + x];
                if (rect.feather == 0) {
                    content_[static_cast<size_t>(kWidth) * y + x] = source;
                    continue;
                }
                const int edgeDistance = std::min({ x - rect.x, right - 1 - x, y - rect.y, rect.y + rect.height - 1 - y });
                const float opacity = std::clamp(static_cast<float>(edgeDistance + 1) / rect.feather, 0.0f, 1.0f);
                const RgbColor base = paletteRgb(backgroundTexturePixel(pageX_ + x, pageY_ + y));
                const RgbColor blended = mixRgb(base, paletteRgb(source), opacity);
                const int red = static_cast<int>(std::lround(std::clamp(blended.red, 0.0f, 255.0f)));
                const int green = static_cast<int>(std::lround(std::clamp(blended.green, 0.0f, 255.0f)));
                const int blue = static_cast<int>(std::lround(std::clamp(blended.blue, 0.0f, 255.0f)));
                content_[static_cast<size_t>(kWidth) * y + x] = _colorTable[rgb555((red << 16) | (green << 8) | blue)];
            }
        }
    }
    scrollbarGroove_.resize(static_cast<size_t>(12) * 240);
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 12; x++) {
            scrollbarGroove_[static_cast<size_t>(y) * 12 + x] = background_[static_cast<size_t>(52) * kWidth + 384 + x];
        }
    }
    frameImage_ = std::make_unique<FrmImage>();
    if (!frameImage_->lock(InterfaceFrameId::PipBoyWindow)) return false;
    frame_ = frameImage_->getData();
    frameWidth_ = frameImage_->getWidth();
    frameHeight_ = frameImage_->getHeight();
    scrollbarKnob_ = std::make_unique<FrmImage>();
    if (!scrollbarKnob_->lock(InterfaceFrameId::PreferencesSliderOff)) scrollbarKnob_.reset();

    scroll_ = 0;
    selectedRow_ = 0;
    return true;
}

void OptionsPanel::updateScaleRange()
{
    if (model_ == nullptr) return;
    int outputWidth = settings.screen.resolution_x;
    int outputHeight = settings.screen.resolution_y;
    const int resolutionRow = findRowBySettingId("screen.resolution_mode");
    if (resolutionRow >= 0) {
        const int code = std::get<int>(rows_[resolutionRow].edit->value);
        if (code > 0) {
            outputWidth = code / 10000;
            outputHeight = code % 10000;
        }
    }
    const int modeRow = findRowBySettingId("screen.windowed");
    if (modeRow >= 0 && std::get<int>(rows_[modeRow].edit->value) == static_cast<int>(WindowMode::WindowedFullscreen)) {
        SDL_DisplayMode desktop = {};
        if (SDL_GetDesktopDisplayMode(displayIndex_, &desktop) == 0 && desktop.w >= 640 && desktop.h >= 480) {
            outputWidth = desktop.w;
            outputHeight = desktop.h;
        }
    }
    const double available = std::min(outputWidth / 640.0, outputHeight / 480.0);
    const double maximum = std::clamp(1.0 + std::floor((available - 1.0 + 0.000001) / 0.2) * 0.2, 1.0, 3.0);
    model_->configureNumericRange("screen.scale_factor", { 1.0, maximum, 0.2 });
    const int scaleRow = findRowBySettingId("screen.scale_factor");
    if (scaleRow >= 0 && editNumber(rows_[scaleRow].edit->value) > maximum) {
        model_->setValue("screen.scale_factor", maximum);
    }
}

void OptionsPanel::buildBackgroundTexture()
{
    using Texture = std::vector<RgbColor>;
    Texture assembled(static_cast<size_t>(kBackgroundTileWidth) * kBackgroundTileHeight);

    RgbColor commonMean = {};
    int commonCount = 0;
    for (int row = 0; row < kBackgroundPatchRows; row++) {
        for (int column = 0; column < kBackgroundPatchColumns; column++) {
            const int sourceX = kBackgroundSourceX + ((row * 17 + column * 29) % 73);
            const int sourceY = kBackgroundPatchY[(row * 3 + column * 2) % 5];
            for (int y = 0; y < kBackgroundPatchHeight; y++) {
                for (int x = 0; x < kBackgroundPatchWidth; x++) {
                    const RgbColor sample = paletteRgb(background_[static_cast<size_t>(kWidth) * (sourceY + y) + sourceX + x]);
                    commonMean.red += sample.red;
                    commonMean.green += sample.green;
                    commonMean.blue += sample.blue;
                    commonCount++;
                }
            }
        }
    }
    commonMean.red /= commonCount;
    commonMean.green /= commonCount;
    commonMean.blue /= commonCount;

    constexpr int kLocalMeanRadius = 8;
    constexpr float kGrainContrast = 0.6f;
    for (int row = 0; row < kBackgroundPatchRows; row++) {
        for (int column = 0; column < kBackgroundPatchColumns; column++) {
            const int sourceX = kBackgroundSourceX + ((row * 17 + column * 29) % 73);
            const int sourceY = kBackgroundPatchY[(row * 3 + column * 2) % 5];
            Texture source(static_cast<size_t>(kBackgroundPatchWidth) * kBackgroundPatchHeight);
            for (int y = 0; y < kBackgroundPatchHeight; y++) {
                for (int x = 0; x < kBackgroundPatchWidth; x++) {
                    source[static_cast<size_t>(kBackgroundPatchWidth) * y + x] = paletteRgb(
                        background_[static_cast<size_t>(kWidth) * (sourceY + y) + sourceX + x]);
                }
            }

            const int destinationX = column * kBackgroundPatchStepX;
            const int destinationY = row * kBackgroundPatchStepY;
            for (int y = 0; y < kBackgroundPatchHeight; y++) {
                for (int x = 0; x < kBackgroundPatchWidth; x++) {
                    RgbColor localMean = {};
                    int count = 0;
                    for (int localY = std::max(0, y - kLocalMeanRadius); localY <= std::min(kBackgroundPatchHeight - 1, y + kLocalMeanRadius); localY++) {
                        for (int localX = std::max(0, x - kLocalMeanRadius); localX <= std::min(kBackgroundPatchWidth - 1, x + kLocalMeanRadius); localX++) {
                            const RgbColor& sample = source[static_cast<size_t>(kBackgroundPatchWidth) * localY + localX];
                            localMean.red += sample.red;
                            localMean.green += sample.green;
                            localMean.blue += sample.blue;
                            count++;
                        }
                    }
                    localMean.red /= count;
                    localMean.green /= count;
                    localMean.blue /= count;
                    const RgbColor& sample = source[static_cast<size_t>(kBackgroundPatchWidth) * y + x];
                    RgbColor normalized = {
                        std::clamp(commonMean.red + (sample.red - localMean.red) * kGrainContrast, 0.0f, 255.0f),
                        std::clamp(commonMean.green + (sample.green - localMean.green) * kGrainContrast, 0.0f, 255.0f),
                        std::clamp(commonMean.blue + (sample.blue - localMean.blue) * kGrainContrast, 0.0f, 255.0f),
                    };
                    const size_t destination = static_cast<size_t>(kBackgroundTileWidth) * (destinationY + y) + destinationX + x;
                    float weight = 1.0f;
                    if (column > 0 && x < kBackgroundPatchOverlap) weight *= static_cast<float>(x + 1) / (kBackgroundPatchOverlap + 1);
                    if (row > 0 && y < kBackgroundPatchOverlap) weight *= static_cast<float>(y + 1) / (kBackgroundPatchOverlap + 1);
                    if (weight < 1.0f) {
                        assembled[destination] = mixRgb(assembled[destination], normalized, weight);
                    } else {
                        assembled[destination] = normalized;
                    }
                }
            }
        }
    }

    constexpr int kHorizontalBlendWidth = 24;
    constexpr int kVerticalBlendHeight = 28;
    const Texture horizontalInput = assembled;
    for (int y = 0; y < kBackgroundTileHeight; y++) {
        for (int distance = 0; distance < kHorizontalBlendWidth; distance++) {
            const float blend = 0.5f * static_cast<float>(kHorizontalBlendWidth - distance) / kHorizontalBlendWidth;
            const int left = distance;
            const int right = kBackgroundTileWidth - 1 - distance;
            const size_t leftIndex = static_cast<size_t>(kBackgroundTileWidth) * y + left;
            const size_t rightIndex = static_cast<size_t>(kBackgroundTileWidth) * y + right;
            assembled[leftIndex] = mixRgb(horizontalInput[leftIndex], horizontalInput[rightIndex], blend);
            assembled[rightIndex] = mixRgb(horizontalInput[rightIndex], horizontalInput[leftIndex], blend);
        }
    }

    const Texture verticalInput = assembled;
    for (int x = 0; x < kBackgroundTileWidth; x++) {
        for (int distance = 0; distance < kVerticalBlendHeight; distance++) {
            const float blend = 0.5f * static_cast<float>(kVerticalBlendHeight - distance) / kVerticalBlendHeight;
            const int top = distance;
            const int bottom = kBackgroundTileHeight - 1 - distance;
            const size_t topIndex = static_cast<size_t>(kBackgroundTileWidth) * top + x;
            const size_t bottomIndex = static_cast<size_t>(kBackgroundTileWidth) * bottom + x;
            assembled[topIndex] = mixRgb(verticalInput[topIndex], verticalInput[bottomIndex], blend);
            assembled[bottomIndex] = mixRgb(verticalInput[bottomIndex], verticalInput[topIndex], blend);
        }
    }

    backgroundTexture_.resize(assembled.size());
    for (size_t index = 0; index < assembled.size(); index++) {
        const RgbColor& color = assembled[index];
        const int red = static_cast<int>(std::lround(std::clamp(color.red, 0.0f, 255.0f)));
        const int green = static_cast<int>(std::lround(std::clamp(color.green, 0.0f, 255.0f)));
        const int blue = static_cast<int>(std::lround(std::clamp(color.blue, 0.0f, 255.0f)));
        const int rgb = (red << 16) | (green << 8) | blue;
        backgroundTexture_[index] = _colorTable[rgb555(rgb)];
    }
}

unsigned char OptionsPanel::backgroundTexturePixel(int x, int y) const
{
    const int textureX = ((x % kBackgroundTileWidth) + kBackgroundTileWidth) % kBackgroundTileWidth;
    const int textureY = ((y % kBackgroundTileHeight) + kBackgroundTileHeight) % kBackgroundTileHeight;
    return backgroundTexture_[static_cast<size_t>(kBackgroundTileWidth) * textureY + textureX];
}

unsigned char* OptionsPanel::contentBuffer() { return content_.data(); }
unsigned char* OptionsPanel::footerBuffer() { return content_.data() + static_cast<size_t>(kWidth) * (footerButtonTop_ - 19); }
int OptionsPanel::contentHeight() const { return contentHeight_; }
int OptionsPanel::footerButtonTop() const { return footerButtonTop_; }
int OptionsPanel::rowCount() const { return static_cast<int>(rows_.size()); }
int OptionsPanel::rowTop(int index) const
{
    return index >= 0 && index < static_cast<int>(rowLayouts_.size()) ? rowLayouts_[index].top : kPanelTop + kHeaderHeight;
}
int OptionsPanel::rowHeight(int index) const
{
    return index >= 0 && index < static_cast<int>(rowLayouts_.size()) ? rowLayouts_[index].height : kOptionRowHeight;
}
int OptionsPanel::rowButtonX() const { return kRowButtonX; }
int OptionsPanel::rowButtonWidth() const { return kRowButtonWidth; }
int OptionsPanel::frameInset() const
{
    const int pageRight = pageX_ + kWidth;
    return screenWidth_ - pageRight >= kScrollbarWidth + kScrollbarOutsideGap ? 0 : kFrameNarrowInset;
}
int OptionsPanel::legacyColumnOffset() const { return screenHeight_ > kHeight ? kLegacyColumnsShift : 0; }
int OptionsPanel::sliderTrackLeft() const { return kSliderX + frameInset(); }
int OptionsPanel::sliderTrackRight() const { return kSliderRight - frameInset(); }
int OptionsPanel::sliderTrackTargetX(double ratio) const
{
    const double clampedRatio = std::clamp(ratio, 0.0, 1.0);
    return sliderTrackLeft() + static_cast<int>(std::round(clampedRatio * (sliderTrackRight() - sliderTrackLeft())));
}
int OptionsPanel::sliderTrackY(int index) const { return rowTop(index) + sliderTrackOffset_; }

void OptionsPanel::buildRowLayout()
{
    constexpr const char* kGroupTitles[] = { "General", "Lighting", "Camera", "Other" };
    std::vector<std::vector<int>> groupedRows(4);
    for (int index = 0; index < rowCount(); index++) {
        const std::string& id = rows_[index].edit->descriptor->id;
        const size_t separator = id.rfind('.');
        const std::string key = separator == std::string::npos ? id : id.substr(separator + 1);
        int group = 3;
        if (key == "enabled" || key == "clean_edges" || key == "sharp_text" || key == "scale_factor"
            || key == "mouse_sensitivity") group = 0;
        else if (key == "resolution_mode" || key == "windowed") group = 0;
        else if (key == "sun" || key == "shadows" || key == "cloud_shadows" || key == "shadow_pixel_size" || key == "shadow_density"
            || key == "sun_swing" || key == "lights" || key == "glow" || key == "glow_strength") group = 1;
        else if (key == "camera_rotation" || key == "camera_rotation_unlimited" || key == "sprite_sides" || key == "zoom" || key == "zoom_min" || key == "zoom_max") group = 2;
        groupedRows[group].push_back(index);
    }

    displayOrder_.clear();
    rowLayouts_.assign(rows_.size(), {});
    groupLayouts_.clear();
    int cursor = kPanelTop + kHeaderHeight;
    for (size_t group = 0; group < groupedRows.size(); group++) {
        if (groupedRows[group].empty()) continue;
        GroupLayout layout;
        layout.title = kGroupTitles[group];
        layout.top = cursor;
        cursor += kGroupHeadingHeight;
        for (int index : groupedRows[group]) {
            const int height = rows_[index].edit->descriptor->section == "screen" ? 42 : kOptionRowHeight;
            rowLayouts_[index] = { cursor, height };
            displayOrder_.push_back(index);
            cursor += height;
        }
        groupLayouts_.push_back(std::move(layout));
        if (group + 1 < groupedRows.size()) {
            bool hasFollowingGroup = false;
            for (size_t next = group + 1; next < groupedRows.size(); next++) {
                if (!groupedRows[next].empty()) { hasFollowingGroup = true; break; }
            }
            if (hasFollowingGroup) cursor += kGroupGap;
        }
    }
    panelBottom_ = std::max(kPanelTop + kPanelBottomCap, cursor + kPanelBottomPadding);
    footerButtonTop_ = screenHeight_ > kHeight ? panelBottom_ + 16 : 450;
    contentHeight_ = std::max({ kHeight, panelBottom_, footerButtonTop_ + 48 });
}
int OptionsPanel::findRowBySettingId(const std::string& id) const
{
    for (int index = 0; index < rowCount(); index++) {
        if (rows_[index].edit->descriptor->id == id) return index;
    }
    return -1;
}
std::string OptionsPanel::settingValueForProbe(const std::string& id) const
{
    const int index = findRowBySettingId(id);
    if (index < 0) return {};
    const SettingValue& value = rows_[index].edit->value;
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
    if (const auto* integer = std::get_if<int>(&value)) return std::to_string(*integer);
    if (const auto* real = std::get_if<double>(&value)) return std::to_string(*real);
    if (const auto* text = std::get_if<std::string>(&value)) return *text;
    return {};
}
int OptionsPanel::scroll() const { return scroll_; }
int OptionsPanel::maxScroll() const { return std::max(0, contentHeight_ - screenHeight_); }
int OptionsPanel::scrollbarX() const
{
    const int pageRight = pageX_ + kWidth;
    if (screenWidth_ - pageRight >= kScrollbarWidth) {
        return pageRight + std::min(kScrollbarOutsideGap, screenWidth_ - pageRight - kScrollbarWidth);
    }
    return pageRight - kScrollbarWidth;
}
int OptionsPanel::scrollbarTop() const { return kTrackMargin; }
int OptionsPanel::scrollbarHeight() const
{
    // On tall screens the footer sits below the frame. End the track at the
    // frame's lower edge when the page is scrolled to the bottom.
    const int footerSpace = screenHeight_ > kHeight ? contentHeight_ - panelBottom_ : kTrackMargin;
    return std::max(kScrollbarThumbHeight, screenHeight_ - kTrackMargin - std::max(kTrackMargin, footerSpace));
}
int OptionsPanel::scrollbarThumbHeight() const { return kScrollbarThumbHeight; }
int OptionsPanel::scrollbarThumbTop() const
{
    const int travel = std::max(0, scrollbarHeight() - scrollbarThumbHeight());
    return scrollbarTop() + travel * scroll_ / std::max(1, maxScroll());
}
void OptionsPanel::mouseToPageCoordinates(int& x, int& y) const
{
    x -= pageX_;
    y = y - pageY_ + scroll_;
    if (screenHeight_ > kHeight && x < kLegacyColumnsRight
        && y >= 40 + kLegacyColumnsShift && y < kLegacyColumnsBottom + kLegacyColumnsShift) {
        y -= kLegacyColumnsShift;
    }
}
void OptionsPanel::setLabelResolver(LabelResolver resolver) { labelResolver_ = resolver; }

void OptionsPanel::setButtonLayout(const std::vector<std::pair<int, int>>& legacyButtons, int speedButton, int speedTop, const std::vector<int>& rowButtons)
{
    legacyButtons_ = legacyButtons;
    speedButton_ = speedButton;
    speedTop_ = speedTop;
    rowButtons_ = rowButtons;
    Window* window = windowGetWindow(windowId_);
    if (window != nullptr) {
        for (Button* button = window->buttonListHead; button != nullptr; button = button->next) {
            buttonLefts_[button->id] = button->rect.left;
            buttonWidths_[button->id] = button->rect.right - button->rect.left + 1;
        }
    }
    updateButtons();
}

void OptionsPanel::setScroll(int value)
{
    int next = std::clamp(value, 0, maxScroll());
    if (next == scroll_) return;
    scroll_ = next;
    updateButtons();
    refresh();
}

void OptionsPanel::showPanelForSnapshot(bool bottom)
{
    setScroll(bottom ? maxScroll() : 0);
}

void OptionsPanel::showDropdownForSnapshot(const std::string& settingId)
{
    const std::string id = settingId == "mode" ? "screen.windowed" : "screen.resolution_mode";
    dropdownRow_ = findRowBySettingId(id);
    if (dropdownRow_ < 0) return;
    const int targetRowTop = std::max(0, screenHeight_ / 3 - rowHeight(dropdownRow_) / 2);
    setScroll(pageY_ + rowTop(dropdownRow_) - targetRowTop);
    const auto& choices = rows_[dropdownRow_].edit->descriptor->choices;
    const int selected = std::get<int>(rows_[dropdownRow_].edit->value);
    const auto found = std::find_if(choices.begin(), choices.end(), [selected](const SettingChoice& choice) {
        return choice.value == selected;
    });
    dropdownHighlight_ = found == choices.end() ? 0 : static_cast<int>(found - choices.begin());
    const int visibleCount = dropdownVisibleItemCount();
    dropdownScroll_ = std::clamp(dropdownHighlight_ - visibleCount / 2, 0,
        std::max(0, static_cast<int>(choices.size()) - visibleCount));
    drawRows();
    refresh();
}

void OptionsPanel::updateButtons()
{
    Window* window = windowGetWindow(windowId_);
    if (window == nullptr) return;
    auto place = [this, window](int id, int originalTop, int viewportHeight) {
        if (id == -1) return;
        for (Button* button = windowGetWindow(windowId_)->buttonListHead; button != nullptr; button = button->next) {
            if (button->id != id) continue;
            int height = button->rect.bottom - button->rect.top + 1;
            int top = pageY_ + originalTop - scroll_;
            int visibleHeight = screenHeight_;
            bool visible = top >= 0 && top + height <= visibleHeight;
            button->rect.left = pageX_ + buttonLefts_[id];
            button->rect.right = button->rect.left + buttonWidths_[id] - 1;
            button->rect.top = visible ? top : -height;
            button->rect.bottom = button->rect.top + height - 1;
            break;
        }
    };
    for (const auto& item : legacyButtons_) {
        const bool shifted = screenHeight_ > kHeight && item.second < kLegacyColumnsBottom
            && buttonLefts_[item.first] < kLegacyColumnsRight;
        place(item.first, item.second + (shifted ? kLegacyColumnsShift : 0), screenHeight_);
    }
    place(speedButton_, speedTop_, screenHeight_);
    for (size_t index = 0; index < rowButtons_.size(); index++) place(rowButtons_[index], rowTop(static_cast<int>(index)), screenHeight_);
}

void OptionsPanel::draw()
{
    if (content_.empty()) return;
    for (int y = kHeight; y < kPanelTop; y++) {
        for (int x = 0; x < kWidth; x++) {
            content_[static_cast<size_t>(kWidth) * y + x] = backgroundTexturePixel(pageX_ + x, pageY_ + y);
        }
    }
    unsigned char* panel = content_.data() + static_cast<size_t>(kWidth) * kPanelTop;
    int panelHeight = panelBottom_ - kPanelTop;
    for (int y = 0; y < panelHeight; y++) {
        for (int x = 0; x < kWidth; x++) {
            panel[static_cast<size_t>(kWidth) * y + x] = backgroundTexturePixel(pageX_ + x, pageY_ + kPanelTop + y);
        }
    }
    if (frame_ != nullptr && frameWidth_ >= 640 && frameHeight_ >= 480) {
        constexpr int glassX = 340;
        constexpr int glassY = 200;
        const int inset = frameInset();
        const int localFrameWidth = kWidth - 2 * inset;
        for (int y = 0; y < panelHeight; y++) {
            for (int x = inset; x < kWidth - inset; x++) {
                const int frameX = x - inset;
                const bool left = frameX < kPanelSideCap;
                const bool right = frameX >= localFrameWidth - kPanelSideCap;
                const bool top = y < kPanelTopCap;
                const bool bottom = y >= panelHeight - kPanelBottomCap;
                const int tileX = (frameX - kPanelSideCap) % 32;
                const int tileY = (y - kPanelTopCap) % 32;
                int sx = glassX + tileX;
                int sy = glassY + tileY;
                if (left || right) {
                    const int edgeX = left ? frameX : frameX - (localFrameWidth - kPanelSideCap);
                    sx = left ? frameWidth_ - 1 - edgeX : kPanelRightCapX + edgeX;
                    if (top || bottom) {
                        sy = top ? 16 + y : 47 - (y - (panelHeight - kPanelBottomCap));
                    } else {
                        sy = 48;
                    }
                } else if (top) {
                    constexpr int edgeLeft = 520;
                    constexpr int edgeWidth = kPanelRightCapX - edgeLeft;
                    sx = edgeLeft + (frameX - kPanelSideCap) % edgeWidth;
                    sy = 16 + y;
                } else if (bottom) {
                    constexpr int edgeLeft = 520;
                    constexpr int edgeWidth = kPanelRightCapX - edgeLeft;
                    sx = edgeLeft + (frameX - kPanelSideCap) % edgeWidth;
                    sy = 47 - (y - (panelHeight - kPanelBottomCap));
                }
                const int cornerX = frameX < kPanelSideCap ? kPanelSideCap - 1 - frameX : frameX - (localFrameWidth - kPanelSideCap);
                const int cornerY = y < kPanelTopCap ? kPanelTopCap - 1 - y : y - (panelHeight - kPanelBottomCap);
                if ((left || right) && (top || bottom) && cornerX * cornerX + cornerY * cornerY > 31 * 31) continue;
                panel[static_cast<size_t>(kWidth) * y + x] = frame_[static_cast<size_t>(frameWidth_) * sy + sx];
            }
        }
        const unsigned char black = COLOR_BLACK;
        for (int y = 16; y < panelHeight - 16; y++) {
            for (int x = inset + 16; x < kWidth - inset - 16; x++) {
                const int frameX = x - inset;
                const int cornerX = frameX < 32 ? 31 - frameX : (frameX >= localFrameWidth - 32 ? frameX - (localFrameWidth - 32) : -1);
                const int cornerY = y < 32 ? 31 - y : (y >= panelHeight - 32 ? y - (panelHeight - 32) : -1);
                if (cornerX >= 0 && cornerY >= 0 && cornerX * cornerX + cornerY * cornerY > 16 * 16) continue;
                panel[static_cast<size_t>(kWidth) * y + x] = black;
            }
        }
        for (int y = kPanelTopCap; y < panelHeight - kPanelBottomCap; y++) {
            for (int x = inset + kPanelSideCap; x < kWidth - inset - kPanelSideCap; x++) {
                const int frameX = x - inset;
                const int cornerX = frameX < 48 ? 48 - frameX : (frameX >= localFrameWidth - 48 ? frameX - (localFrameWidth - 48) + 1 : -1);
                const int cornerY = y < 48 ? 48 - y : (y >= panelHeight - 48 ? y - (panelHeight - 48) + 1 : -1);
                if (cornerX >= 0 && cornerY >= 0 && cornerX * cornerX + cornerY * cornerY > 16 * 16) continue;
                const int sourceX = glassX + (frameX - kPanelSideCap) % 32;
                const int sourceY = glassY + (y - kPanelTopCap) % 32;
                panel[static_cast<size_t>(kWidth) * y + x] = frame_[static_cast<size_t>(frameWidth_) * sourceY + sourceX];
            }
        }
    }
    // The supplied pieces include the dark inset and transparent outer corners.
    // Preserve the tiled glass centre while repeating only the edited edges.
    const auto& pieces = ownerFramePieces();
    if (pieces[0].width == 47 && pieces[1].width == 352 && pieces[2].width == 47
        && pieces[3].width == 47 && pieces[4].width == 47 && pieces[5].width == 47
        && pieces[6].width == 352 && pieces[7].width == 47 && panelHeight >= 96) {
        const int inset = frameInset();
        const int width = kWidth - 2 * inset;
        for (int y = 0; y < panelHeight; ++y) {
            for (int x = 0; x < width; ++x) {
                panel[static_cast<size_t>(kWidth) * y + inset + x] = backgroundTexturePixel(pageX_ + inset + x, pageY_ + kPanelTop + y);
                // Do not draw the outermost source column: the user's trimmed
                // corner and side pieces must share the same outside edge.
                if (x == 0 || x == width - 1) continue;
                int part = -1, sx = 0, sy = 0;
                if (y < 48) {
                    part = x < 47 ? 0 : (x >= width - 47 ? 2 : 1);
                    sx = x < 47 ? x : (x >= width - 47 ? x - (width - 47) : (x - 47) % 352);
                    sy = y;
                } else if (y >= panelHeight - 48) {
                    part = x < 47 ? 5 : (x >= width - 47 ? 7 : 6);
                    sx = x < 47 ? x : (x >= width - 47 ? x - (width - 47) : (x - 47) % 352);
                    sy = y - (panelHeight - 48);
                } else if (x < 47 || x >= width - 47) {
                    part = x < 47 ? 3 : 4;
                    sx = x < 47 ? x : x - (width - 47);
                    sy = (y - 48) % 32;
                } else {
                    panel[static_cast<size_t>(kWidth) * y + inset + x] = frame_[static_cast<size_t>(frameWidth_) * (200 + (y - 32) % 32) + 340 + (x - 32) % 32];
                }
                if (part >= 0 && sy < static_cast<int>(pieces[part].height)) {
                    const size_t offset = (static_cast<size_t>(sy) * pieces[part].width + sx) * 4;
                    const auto& rgba = pieces[part].rgba;
                    if (rgba[offset + 3] >= 128) {
                        const int rgb = (rgba[offset] << 16) | (rgba[offset + 1] << 8) | rgba[offset + 2];
                        panel[static_cast<size_t>(kWidth) * y + inset + x] = _colorTable[rgb555(rgb)];
                    }
                }
            }
        }
    }
    rowFrameBackground_.assign(panel, panel + static_cast<size_t>(kWidth) * panelHeight);
    fontSetCurrent(103);
    fontDrawText(panel + kWidth * 62 + 52 + frameInset(), localized("3D OPTIONS"), 520, kWidth, COLOR_GREEN);
    fontSetCurrent(101);
    drawRows();
    refresh();
}

void OptionsPanel::drawRows()
{
    if (model_ == nullptr) return;
    const int inset = frameInset();
    unsigned char* panel = content_.data() + static_cast<size_t>(kWidth) * kPanelTop;
    // A dropdown can cover gaps and section headings, not just control rows.
    // Restore the whole panel before redrawing so closing it leaves no residue.
    std::memcpy(panel, rowFrameBackground_.data(), rowFrameBackground_.size());
    fontSetCurrent(103);
    fontDrawText(panel + kWidth * 62 + 52 + inset, localized("3D OPTIONS"), 520, kWidth, COLOR_GREEN);
    for (size_t groupIndex = 0; groupIndex < groupLayouts_.size(); ++groupIndex) {
        const GroupLayout& group = groupLayouts_[groupIndex];
        const int groupY = group.top - kPanelTop;
        fontSetCurrent(101);
        fontDrawText(panel + kWidth * (groupY + 1) + kLabelX + inset, localized(group.title.c_str()), 270, kWidth, COLOR_GREEN);
        const int dividerY = groupY + fontGetLineHeight() + 3;
        bufferDrawLine(panel, kWidth, kLabelX + inset, dividerY, kWidth - kLabelX - inset, dividerY, COLOR_DARK_GREEN);
    }
    for (int index : displayOrder_) {
        const OptionsPanelRow& row = rows_[index];
        const SettingDescriptor& descriptor = *row.edit->descriptor;
        const int rowY = rowTop(index) - kPanelTop;
        const int height = rowHeight(index);
        const bool disabled = descriptor.readOnly || descriptor.commandLineOverride;
        const Color color = disabled ? COLOR_DARK_YELLOW : COLOR_GREEN;
        for (int fillY = rowY; fillY < rowY + height; fillY++) {
            std::memcpy(panel + kWidth * fillY + kRowButtonX,
                rowFrameBackground_.data() + kWidth * fillY + kRowButtonX, kRowButtonWidth);
        }
        const char* localizedLabel = descriptor.labelMessageId > 0 && labelResolver_ != nullptr
            ? labelResolver_(descriptor.labelMessageId)
            : nullptr;
        const std::string displayLabel = localizedLabel != nullptr && localizedLabel[0] != '\0'
            ? localizedLabel
            : (descriptor.fallbackLabel.empty() ? descriptor.id : descriptor.fallbackLabel);
        fontDrawText(panel + kWidth * (rowY + kLabelTextOffset) + kLabelX + inset, displayLabel.c_str(), 270, kWidth, color);
        if (descriptor.section == "screen" && descriptor.applyPolicy == SettingApplyPolicy::Restart) {
            fontDrawText(panel + kWidth * (rowY + 23) + kLabelX + inset, localized("Requires restart"), 270, kWidth, COLOR_DARK_YELLOW);
        }

        char valueText[128] = {};
        switch (row.control) {
        case OptionsPanelControl::Toggle: {
            bool value = std::get<bool>(row.edit->value);
            drawCheckboxControl(panel, rowY, sliderTrackRight(), value, color);
            const int valueRight = kSliderValueX + kSliderValueWidth - inset;
            const int valueWidth = std::max(fontGetStringWidth(localized("On")), fontGetStringWidth(localized("Off")));
            drawRightAlignedControlText(panel, rowY, valueRight - valueWidth, valueRight,
                localized(value ? "On" : "Off"), color);
            break;
        }
        case OptionsPanelControl::Slider: {
            double value = editNumber(row.edit->value);
            const SettingNumericRange& range = *descriptor.numericRange;
            double ratio = std::clamp((value - range.minimum) / (range.maximum - range.minimum), 0.0, 1.0);
            if (descriptor.id == "screen.scale_factor") std::snprintf(valueText, sizeof(valueText), "%.1fx", value);
            else std::snprintf(valueText, sizeof(valueText), "%.2f", value);
            drawRightAlignedControlText(panel, rowY, kSliderValueX - inset, kSliderValueX + kSliderValueWidth - inset, valueText, color);
            const int trackY = sliderTrackY(index) - kPanelTop;
            bufferDrawLine(panel, kWidth, sliderTrackLeft(), trackY, sliderTrackRight(), trackY, color);
            int knobX = sliderTrackTargetX(ratio);
            bufferDrawRect(panel, kWidth, knobX - 3, trackY - 5, knobX + 3, trackY + 5, COLOR_LIGHT_GREEN);
            break;
        }
        case OptionsPanelControl::Choice: {
            int selected = std::get<int>(row.edit->value);
            const char* label = "";
            for (const SettingChoice& choice : descriptor.choices) {
                if (choice.value == selected) { label = choice.fallbackLabel.c_str(); break; }
            }
            std::snprintf(valueText, sizeof(valueText), "< %s v >", localized(label));
            drawRightAlignedControlText(panel, rowY, kValueX - inset, kValueX + kValueWidth - inset, valueText, color);
            break;
        }
        case OptionsPanelControl::Value:
            if (const auto* value = std::get_if<bool>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%s", localized(*value ? "On" : "Off"));
            else if (const auto* value = std::get_if<int>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%d", *value);
            else if (const auto* value = std::get_if<double>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%.2f", *value);
            else if (const auto* value = std::get_if<std::string>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%s", value->c_str());
            drawRightAlignedControlText(panel, rowY, kValueX - inset, kValueX + kValueWidth - inset, valueText, color);
            break;
        }
    }
    drawDropdown();
}

void OptionsPanel::drawDropdown()
{
    if (dropdownRow_ < 0 || dropdownRow_ >= rowCount()) return;
    const SettingDescriptor& descriptor = *rows_[dropdownRow_].edit->descriptor;
    if (descriptor.choices.empty()) return;
    constexpr int itemHeight = 22;
    const int inset = frameInset();
    const int left = kValueX - inset;
    const int right = kValueX + kValueWidth - inset;
    const int visibleCount = dropdownVisibleItemCount();
    const int popupTop = dropdownAbove_
        ? rowTop(dropdownRow_) - visibleCount * itemHeight - 2
        : rowTop(dropdownRow_) + rowHeight(dropdownRow_);
    const int top = popupTop - kPanelTop;
    const int bottom = std::min(panelBottom_ - kPanelTop - 2, top + visibleCount * itemHeight + 2);
    auto fillRect = [this](int x, int y, int width, int height, Color color) {
        for (int row = std::max(0, y); row < std::min(panelBottom_ - kPanelTop, y + height); row++) {
            std::fill(content_.begin() + static_cast<size_t>(kPanelTop + row) * kWidth + std::max(0, x),
                content_.begin() + static_cast<size_t>(kPanelTop + row) * kWidth + std::min(kWidth, x + width), color);
        }
    };
    fillRect(left, top, right - left, bottom - top, COLOR_DARK_GREEN_2);
    bufferDrawRect(content_.data() + static_cast<size_t>(kWidth) * kPanelTop, kWidth, left, top, right - 1, bottom - 1, COLOR_GREEN);
    fontSetCurrent(101);
    for (int item = 0; item < visibleCount; item++) {
        const int choiceIndex = dropdownScroll_ + item;
        if (choiceIndex >= static_cast<int>(descriptor.choices.size())) break;
        const SettingChoice& choice = descriptor.choices[choiceIndex];
        const int itemTop = top + 2 + item * itemHeight;
        if (choiceIndex == dropdownHighlight_) fillRect(left + 1, itemTop, right - left - 2, itemHeight - 1, COLOR_DARK_GREEN);
        fontDrawText(content_.data() + static_cast<size_t>(kWidth) * (kPanelTop + itemTop + 3) + left + 5,
            localized(choice.fallbackLabel.c_str()), right - left - 10, kWidth, COLOR_GREEN);
    }
}

int OptionsPanel::dropdownVisibleItemCount()
{
    if (dropdownRow_ < 0 || dropdownRow_ >= rowCount()) return 0;
    constexpr int itemHeight = 22;
    const int choiceCount = static_cast<int>(rows_[dropdownRow_].edit->descriptor->choices.size());
    const int rowScreenTop = pageY_ + rowTop(dropdownRow_) - scroll_;
    const int rowScreenBottom = rowScreenTop + rowHeight(dropdownRow_);
    const int visiblePanelTop = pageY_ + kPanelTop - scroll_ + 8;
    const int availableAbove = std::max(0, rowScreenTop - visiblePanelTop);
    const int availableBelow = std::max(0, screenHeight_ - rowScreenBottom - 8);
    dropdownAbove_ = availableBelow / itemHeight < std::min(choiceCount, 3) && availableAbove > availableBelow;
    const int available = dropdownAbove_ ? availableAbove : availableBelow;
    return std::max(1, std::min(choiceCount, std::max(1, (available - 4) / itemHeight)));
}

void OptionsPanel::compose()
{
    if (surface_ == nullptr || content_.empty()) return;
    for (int y = 0; y < screenHeight_; y++) {
        unsigned char* destination = surface_ + static_cast<size_t>(screenWidth_) * y;
        for (int x = 0; x < screenWidth_; x++) {
            destination[x] = backgroundTexturePixel(x, y + scroll_);
        }
    }
    const int sourceTop = scroll_ - pageY_;
    const int firstVisibleRow = std::max(0, -sourceTop);
    const int pastVisibleRow = std::min(screenHeight_, contentHeight_ - sourceTop);
    const int left = std::max(0, pageX_);
    const int right = std::min(screenWidth_, pageX_ + kWidth);
    if (screenHeight_ <= kHeight && firstVisibleRow < pastVisibleRow && left < right) {
        const int sourceY = sourceTop + firstVisibleRow;
        blitBufferToBuffer(content_.data() + static_cast<size_t>(kWidth) * sourceY + left - pageX_,
            right - left, pastVisibleRow - firstVisibleRow, kWidth,
            surface_ + static_cast<size_t>(screenWidth_) * firstVisibleRow + left, screenWidth_);
    }
    if (screenHeight_ > kHeight) {
        // Keep the standard title at the top, but give the two left columns
        // more air below it. Copy each region separately so sharp text follows
        // the same transformation as the indexed framebuffer.
        auto copyRegion = [&](int sx, int sy, int width, int height, int offsetY) {
            int dx = pageX_ + sx;
            int dy = pageY_ + sy + offsetY - scroll_;
            if (dx < 0) { sx -= dx; width += dx; dx = 0; }
            if (dy < 0) { sy -= dy; height += dy; dy = 0; }
            width = std::min(width, screenWidth_ - dx);
            height = std::min(height, screenHeight_ - dy);
            if (width > 0 && height > 0) {
                blitBufferToBuffer(content_.data() + static_cast<size_t>(sy) * kWidth + sx,
                    width, height, kWidth, surface_ + static_cast<size_t>(dy) * screenWidth_ + dx, screenWidth_);
            }
        };
        copyRegion(0, 0, kLegacyColumnsRight, 40, 0);
        copyRegion(0, 40, kLegacyColumnsRight, kLegacyColumnsBottom - 40, kLegacyColumnsShift);
        copyRegion(0, kLegacyColumnsBottom + kLegacyColumnsShift, kLegacyColumnsRight,
            contentHeight_ - kLegacyColumnsBottom - kLegacyColumnsShift, 0);
        copyRegion(kLegacyColumnsRight, 0, kWidth - kLegacyColumnsRight, contentHeight_, 0);
    }
    if (maxScroll() > 0) {
        const int barX = scrollbarX();
        const int trackTop = scrollbarTop();
        const int trackHeight = scrollbarHeight();
        const int thumbHeight = scrollbarThumbHeight();
        const int thumbTop = scrollbarThumbTop();
        for (int y = 0; y < trackHeight; y++) {
            const int tileY = y % 240;
            const int sampleY = (y / 240) % 2 == 0 ? tileY : (239 - tileY);
            if (trackTop + y < 0 || trackTop + y >= screenHeight_) continue;
            std::memcpy(surface_ + static_cast<size_t>(screenWidth_) * (trackTop + y) + barX,
                scrollbarGroove_.data() + static_cast<size_t>(sampleY) * 12, 12);
        }
        const unsigned char* knob = scrollbarKnob_ != nullptr ? scrollbarKnob_->getData() : nullptr;
        if (knob != nullptr) {
            const int knobY = thumbTop + thumbHeight / 2 - 10;
            for (int y = 0; y < kScrollbarThumbHeight; y++) {
                for (int x = 0; x < 12; x++) {
                    unsigned char pixel = knob[x * kScrollbarThumbHeight + (kScrollbarThumbHeight - 1 - y)];
                    if (pixel != 0 && knobY + y >= 0 && knobY + y < screenHeight_ && barX + x >= 0 && barX + x < screenWidth_) {
                        surface_[static_cast<size_t>(screenWidth_) * (knobY + y) + barX + x] = pixel;
                    }
                }
            }
        }
    }
    if (!error_.empty()) {
        bufferFill(surface_ + screenWidth_ * 4 + 18, 604, 22, screenWidth_, COLOR_DARK_GREEN_2);
        fontSetCurrent(101);
        fontDrawText(surface_ + screenWidth_ * 6 + 22, error_.c_str(), 590, screenWidth_, COLOR_LIGHT_RED);
    }
}

void OptionsPanel::refresh()
{
    if (windowId_ == -1) return;
    compose();
    windowRefresh(windowId_);
}

void OptionsPanel::showError(const std::string& message)
{
    error_ = message;
    refresh();
}

bool OptionsPanel::setRowValue(int index, const SettingValue& value)
{
    if (index < 0 || index >= rowCount() || model_ == nullptr) return false;
    std::string error;
    const std::string& id = rows_[index].edit->descriptor->id;
    if (!model_->setValue(id, value, &error)) {
        showError(error);
        return false;
    }
    if (id == "screen.resolution_mode" || id == "screen.windowed") updateScaleRange();
    error_.clear();
    drawRows();
    refresh();
    return true;
}

bool OptionsPanel::stepRow(int index, int direction)
{
    if (index < 0 || index >= rowCount()) return false;
    const OptionsPanelRow& row = rows_[index];
    const SettingDescriptor& descriptor = *row.edit->descriptor;
    if (descriptor.readOnly || descriptor.commandLineOverride) return true;
    if (row.control == OptionsPanelControl::Toggle) return setRowValue(index, !std::get<bool>(row.edit->value));
    if (row.control == OptionsPanelControl::Choice) {
        if (descriptor.choices.empty()) return true;
        int current = std::get<int>(row.edit->value);
        auto found = std::find_if(descriptor.choices.begin(), descriptor.choices.end(), [current](const SettingChoice& choice) { return choice.value == current; });
        int offset = found == descriptor.choices.end() ? 0 : static_cast<int>(found - descriptor.choices.begin());
        offset = (offset + direction + static_cast<int>(descriptor.choices.size())) % static_cast<int>(descriptor.choices.size());
        return setRowValue(index, descriptor.choices[offset].value);
    }
    if (row.control == OptionsPanelControl::Slider) {
        const SettingNumericRange& range = *descriptor.numericRange;
        double value = editNumber(row.edit->value);
        double step = range.step.value_or((range.maximum - range.minimum) / 100.0);
        value = std::clamp(value + direction * step, range.minimum, range.maximum);
        if (descriptor.valueType == SettingValueType::Integer) return setRowValue(index, static_cast<int>(std::round(value)));
        return setRowValue(index, value);
    }
    return true;
}

void OptionsPanel::dragSlider(int index)
{
    if (index < 0 || index >= rowCount()) return;
    const SettingDescriptor& descriptor = *rows_[index].edit->descriptor;
    if (!descriptor.numericRange.has_value()) return;
    const SettingNumericRange range = *descriptor.numericRange;
    auto updateFromMouse = [this, index, range, &descriptor]() {
        int x = 0, y = 0;
        mouseGetPositionInWindow(windowId_, &x, &y);
        mouseToPageCoordinates(x, y);
        const int trackY = sliderTrackY(index);
        if (x < sliderTrackLeft() || x > sliderTrackRight()
            || y < trackY - kSliderHitTolerance || y > trackY + kSliderHitTolerance) return;
        double ratio = std::clamp((x - static_cast<double>(sliderTrackLeft())) / (sliderTrackRight() - sliderTrackLeft()), 0.0, 1.0);
        double value = range.minimum + ratio * (range.maximum - range.minimum);
        if (range.step.has_value()) value = range.minimum + std::round((value - range.minimum) / *range.step) * *range.step;
        value = std::clamp(value, range.minimum, range.maximum);
        if (descriptor.valueType == SettingValueType::Integer) setRowValue(index, static_cast<int>(std::round(value)));
        else setRowValue(index, value);
    };
    updateFromMouse();
    while (true) {
        int rawX = 0, rawY = 0, buttons = 0;
        if (devtools::preferencesProbeUsesSimulatedMouse()) buttons = mouse_get_last_buttons();
        else _mouse_get_raw_state(&rawX, &rawY, &buttons);
        if ((buttons & MOUSE_STATE_LEFT_BUTTON_DOWN) == 0) break;
        devtools::scriptedInputPump();
        const int event = inputGetInput();
        devtools::preferencesProbePump();
        updateFromMouse();
        renderPresent();
        sharedFpsLimiter.mark();
        sharedFpsLimiter.throttle();
        if (devtools::preferencesProbeUsesSimulatedMouse()) buttons = mouse_get_last_buttons();
        else _mouse_get_raw_state(&rawX, &rawY, &buttons);
        if (event == KEY_ESCAPE || _game_user_wants_to_quit != GAME_QUIT_REQUEST_NONE
            || (mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_UP) != 0 || (buttons & MOUSE_STATE_LEFT_BUTTON_DOWN) == 0) break;
    }
}

void OptionsPanel::dragScrollbar(int y)
{
    const int thumbHeight = scrollbarThumbHeight();
    const int trackTop = scrollbarTop();
    const int travel = scrollbarHeight() - thumbHeight;
    const int thumbTop = scrollbarThumbTop();
    const int grabOffset = y >= thumbTop && y < thumbTop + thumbHeight ? y - thumbTop : thumbHeight / 2;
    while (true) {
        int position = std::clamp(y - trackTop - grabOffset, 0, travel);
        setScroll(travel > 0 ? position * maxScroll() / travel : 0);
        renderPresent();
        sharedFpsLimiter.mark();
        const int event = inputGetInput();
        devtools::preferencesProbePump();
        if (event == KEY_ESCAPE || _game_user_wants_to_quit != GAME_QUIT_REQUEST_NONE) break;
        int x = 0;
        mouseGetPositionInWindow(windowId_, &x, &y);
        int rawX = 0, rawY = 0, buttons = 0;
        if (devtools::preferencesProbeUsesSimulatedMouse()) buttons = mouse_get_last_buttons();
        else _mouse_get_raw_state(&rawX, &rawY, &buttons);
        sharedFpsLimiter.throttle();
        if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_UP) != 0 || (buttons & MOUSE_STATE_LEFT_BUTTON_DOWN) == 0) break;
    }
}

bool OptionsPanel::handleEvent(int eventCode)
{
    if (dropdownWaitRelease_) {
        if ((mouse_get_last_buttons() & MOUSE_STATE_LEFT_BUTTON_DOWN) == 0) dropdownWaitRelease_ = false;
        return true;
    }
    if (dropdownRow_ >= 0) {
        int x = 0, y = 0;
        mouseGetPositionInWindow(windowId_, &x, &y);
        mouseToPageCoordinates(x, y);
        const SettingDescriptor& dropdownDescriptor = *rows_[dropdownRow_].edit->descriptor;
        constexpr int itemHeight = 22;
        const int inset = frameInset();
        const int left = kValueX - inset;
        const int right = kValueX + kValueWidth - inset;
        const int visibleCount = dropdownVisibleItemCount();
        const int popupTop = dropdownAbove_
            ? rowTop(dropdownRow_) - visibleCount * itemHeight - 2
            : rowTop(dropdownRow_) + rowHeight(dropdownRow_);
        const int itemTop = popupTop + 2;
        const bool mousePress = eventCode == -2 && (mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0;
        if ((eventCode >= kRowEvent || mousePress) && x >= left && x < right && y >= itemTop && y < itemTop + visibleCount * itemHeight) {
            const int choiceIndex = dropdownScroll_ + (y - itemTop) / itemHeight;
            if (choiceIndex >= 0 && choiceIndex < static_cast<int>(dropdownDescriptor.choices.size())) {
                setRowValue(dropdownRow_, dropdownDescriptor.choices[choiceIndex].value);
                dropdownRow_ = -1;
                dropdownWaitRelease_ = true;
                drawRows();
                refresh();
            }
            return true;
        }
        if (eventCode == kRowEvent + dropdownRow_) {
            dropdownRow_ = -1;
            drawRows();
            refresh();
            return true;
        }
        if ((eventCode >= 505 && eventCode <= 524) || eventCode == 527) {
            dropdownRow_ = -1;
            drawRows();
            refresh();
        } else if (eventCode >= kRowEvent || mousePress) {
            dropdownRow_ = -1;
            drawRows();
            refresh();
        }
    }
    if (eventCode == kScrollEvent) {
        int x = 0, y = 0;
        mouseGetPositionInWindow(windowId_, &x, &y);
        dragScrollbar(y);
        return true;
    }
    int index = eventCode - kRowEvent;
    if (index < 0 || index >= rowCount()) return false;
    selectedRow_ = index;
    int mouseX = 0, mouseY = 0;
    mouseGetPositionInWindow(windowId_, &mouseX, &mouseY);
    mouseToPageCoordinates(mouseX, mouseY);
    drawRows();
    refresh();
    const OptionsPanelRow& row = rows_[index];
    const SettingDescriptor& descriptor = *row.edit->descriptor;
    if (descriptor.readOnly || descriptor.commandLineOverride) return true;
    const int trackY = sliderTrackY(index);
    if (row.control == OptionsPanelControl::Slider && mouseX >= sliderTrackLeft() && mouseX <= sliderTrackRight()
        && mouseY >= trackY - kSliderHitTolerance && mouseY <= trackY + kSliderHitTolerance) {
        dragSlider(index);
        return true;
    }
    if (row.control == OptionsPanelControl::Choice) {
        dropdownRow_ = index;
        dropdownWaitRelease_ = true;
        const int selected = std::get<int>(row.edit->value);
        const auto found = std::find_if(descriptor.choices.begin(), descriptor.choices.end(), [selected](const SettingChoice& choice) {
            return choice.value == selected;
        });
        dropdownHighlight_ = found == descriptor.choices.end() ? 0 : static_cast<int>(found - descriptor.choices.begin());
        const int visibleCount = dropdownVisibleItemCount();
        dropdownScroll_ = std::clamp(dropdownHighlight_ - visibleCount / 2, 0,
            std::max(0, static_cast<int>(descriptor.choices.size()) - visibleCount));
        drawRows();
        refresh();
    } else if (row.control != OptionsPanelControl::Slider) {
        stepRow(index, 1);
    }
    return true;
}

bool OptionsPanel::handleKey(int keyCode)
{
    if (rowCount() == 0) return false;
    if (dropdownRow_ >= 0) {
        const int choiceCount = static_cast<int>(rows_[dropdownRow_].edit->descriptor->choices.size());
        const int visibleCount = dropdownVisibleItemCount();
        if (keyCode == KEY_ESCAPE) {
            dropdownRow_ = -1;
            drawRows();
            refresh();
            return true;
        }
        if (keyCode == KEY_ARROW_UP || keyCode == KEY_ARROW_DOWN) {
            dropdownHighlight_ = std::clamp(dropdownHighlight_ + (keyCode == KEY_ARROW_DOWN ? 1 : -1), 0, std::max(0, choiceCount - 1));
            if (dropdownHighlight_ < dropdownScroll_) dropdownScroll_ = dropdownHighlight_;
            if (dropdownHighlight_ >= dropdownScroll_ + visibleCount) dropdownScroll_ = dropdownHighlight_ - visibleCount + 1;
            drawRows();
            refresh();
            return true;
        }
        if (keyCode == KEY_RETURN || keyCode == KEY_SPACE) {
            const auto& choices = rows_[dropdownRow_].edit->descriptor->choices;
            if (dropdownHighlight_ >= 0 && dropdownHighlight_ < static_cast<int>(choices.size())) {
                setRowValue(dropdownRow_, choices[dropdownHighlight_].value);
            }
            dropdownRow_ = -1;
            drawRows();
            refresh();
            return true;
        }
        return false;
    }
    // T / Shift+T step the game hour by one hour, forth and back: the owner
    // reads each hour's shadow. The sun snaps to the jump, as on waiting.
    if (keyCode == KEY_LOWERCASE_T || keyCode == KEY_UPPERCASE_T) {
        const unsigned int ticks = gameTimeGetTime();
        const int hours = keyCode == KEY_LOWERCASE_T ? 1 : -1;
        const unsigned int hour = (ticks / 36000u + 24u + static_cast<unsigned int>(hours)) % 24u;
        gameTimeSetTime(hour * 36000u + ticks % 36000u);
        return true;
    }
    switch (keyCode) {
    case KEY_ARROW_UP: setScroll(scroll_ - rowHeight(selectedRow_)); return true;
    case KEY_ARROW_DOWN: setScroll(scroll_ + rowHeight(selectedRow_)); return true;
    case KEY_PAGE_UP: setScroll(scroll_ - 360); return true;
    case KEY_PAGE_DOWN: setScroll(scroll_ + 360); return true;
    case KEY_HOME: setScroll(0); return true;
    case KEY_END: setScroll(maxScroll()); return true;
    case KEY_ARROW_LEFT: setScroll(std::clamp(rowTop(selectedRow_) + rowHeight(selectedRow_) / 2 - screenHeight_ / 2, 0, maxScroll())); return stepRow(selectedRow_, -1);
    case KEY_ARROW_RIGHT:
    case KEY_SPACE: setScroll(std::clamp(rowTop(selectedRow_) + rowHeight(selectedRow_) / 2 - screenHeight_ / 2, 0, maxScroll())); return stepRow(selectedRow_, 1);
    default: return false;
    }
}

void OptionsPanel::handleWheel()
{
    int x = 0, y = 0;
    mouseGetWheel(&x, &y);
    if (dropdownRow_ >= 0 && y != 0) {
        const int count = static_cast<int>(rows_[dropdownRow_].edit->descriptor->choices.size());
        const int visibleCount = dropdownVisibleItemCount();
        dropdownScroll_ = std::clamp(dropdownScroll_ + (y > 0 ? -3 : 3), 0, std::max(0, count - visibleCount));
        dropdownHighlight_ = std::clamp(dropdownHighlight_, dropdownScroll_, std::min(count - 1, dropdownScroll_ + visibleCount - 1));
        drawRows();
        refresh();
        return;
    }
    if (y != 0) setScroll(scroll_ - (y > 0 ? 48 : -48));
}

bool OptionsPanel::commit()
{
    if (model_ == nullptr) return true;
    std::string error;
    if (!model_->commit(&error)) {
        showError(error);
        return false;
    }
    error_.clear();
    return true;
}

void OptionsPanel::rollback()
{
    if (model_ != nullptr) model_->rollback();
    error_.clear();
}

void OptionsPanel::resetToDefaults()
{
    if (model_ == nullptr) return;
    for (const OptionsPanelRow& row : rows_) {
        if (!row.edit->descriptor->readOnly && !row.edit->descriptor->commandLineOverride) model_->resetToDefault(row.edit->descriptor->id);
    }
    error_.clear();
    drawRows();
    refresh();
}

} // namespace scene
} // namespace fallout
