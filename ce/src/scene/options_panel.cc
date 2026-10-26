#include "options_panel.h"
#include "text_capture.h"
#include "menu_text.h"
#include "scene_view.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../color.h"
#include "../devtools/preferences_probe.h"
#include "../art.h"
#include "../draw.h"
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
constexpr int kPanelTop = 494;
constexpr int kHeaderHeight = 90;
constexpr int kToggleRowHeight = 28;
constexpr int kSliderRowHeight = 40;
constexpr int kGroupHeadingHeight = 14;
constexpr int kGroupGap = 24;
constexpr int kPanelBottomPadding = 32;
constexpr int kLabelX = 55;
constexpr int kValueX = 405;
constexpr int kValueWidth = 180;
constexpr int kToggleX = 485;
constexpr int kToggleWidth = 100;
constexpr int kSliderX = 355;
constexpr int kSliderRight = 540;
constexpr int kSliderValueX = 500;
constexpr int kSliderValueWidth = 85;
constexpr int kLabelTextOffset = 4;
constexpr int kSliderHitTolerance = 7;
constexpr int kFrameNarrowInset = 16;
constexpr int kScrollbarWidth = 12;
constexpr int kScrollbarOutsideGap = 4;
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
    fontDrawText(panel + kWidth * (rowY + kLabelTextOffset) + right - width, text, width, kWidth, color);
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
        { 7, 4, 44, 31 }, { 301, 4, 44, 31 },
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
        const int left = std::max(0, rect.x);
        const int right = std::min({ kWidth, legacyCropRight, rect.x + rect.width });
        const int top = std::max(0, rect.y);
        const int bottom = std::min(kHeight, rect.y + rect.height);
        for (int y = top; y < bottom; y++) {
            for (int x = left; x < right; x++) {
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
            scrollbarGroove_[static_cast<size_t>(y) * 12 + x] = background_[static_cast<size_t>(52 + 11 - x) * kWidth + 384 + y];
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
unsigned char* OptionsPanel::footerBuffer() { return content_.data() + static_cast<size_t>(kWidth) * 430; }
int OptionsPanel::contentHeight() const { return contentHeight_; }
int OptionsPanel::rowCount() const { return static_cast<int>(rows_.size()); }
int OptionsPanel::rowTop(int index) const
{
    return index >= 0 && index < static_cast<int>(rowLayouts_.size()) ? rowLayouts_[index].top : kPanelTop + kHeaderHeight;
}
int OptionsPanel::rowHeight(int index) const
{
    return index >= 0 && index < static_cast<int>(rowLayouts_.size()) ? rowLayouts_[index].height : kToggleRowHeight;
}
int OptionsPanel::rowButtonX() const { return kRowButtonX; }
int OptionsPanel::rowButtonWidth() const { return kRowButtonWidth; }
int OptionsPanel::frameInset() const
{
    const int pageRight = pageX_ + kWidth;
    return screenWidth_ - pageRight >= kScrollbarWidth + kScrollbarOutsideGap ? 0 : kFrameNarrowInset;
}
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
        if (key == "enabled" || key == "clean_edges" || key == "sharp_text") group = 0;
        else if (key == "sun" || key == "shadows" || key == "shadow_pixel_size" || key == "lights" || key == "glow" || key == "glow_strength") group = 1;
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
            const int height = rows_[index].control == OptionsPanelControl::Slider ? kSliderRowHeight : kToggleRowHeight;
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
    contentHeight_ = std::max(kHeight, cursor + kPanelBottomPadding);
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
    if (screenWidth_ - pageRight >= kScrollbarWidth + kScrollbarOutsideGap) return pageRight + kScrollbarOutsideGap;
    return pageRight - kScrollbarWidth;
}
int OptionsPanel::scrollbarTop() const { return kTrackMargin; }
int OptionsPanel::scrollbarHeight() const { return std::max(1, screenHeight_ - 2 * kTrackMargin); }
int OptionsPanel::scrollbarThumbHeight() const { return kScrollbarThumbHeight; }
int OptionsPanel::scrollbarThumbTop() const
{
    const int travel = std::max(0, scrollbarHeight() - scrollbarThumbHeight());
    return scrollbarTop() + travel * scroll_ / std::max(1, maxScroll());
}
void OptionsPanel::mouseToPageCoordinates(int& x, int& y) const { x -= pageX_; y = y - pageY_ + scroll_; }
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
    for (const auto& item : legacyButtons_) place(item.first, item.second, screenHeight_);
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
    int panelHeight = contentHeight_ - kPanelTop;
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
                        sy = 48 + (y - kPanelTopCap) % 5;
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
    rowFrameBackground_.assign(panel, panel + static_cast<size_t>(kWidth) * panelHeight);
    fontSetCurrent(104);
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
    for (const GroupLayout& group : groupLayouts_) {
        const int groupY = group.top - kPanelTop;
        fontSetCurrent(101);
        fontDrawText(panel + kWidth * (groupY + 1) + kLabelX + inset, localized(group.title.c_str()), 270, kWidth, COLOR_GREEN);
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

        char valueText[128] = {};
        switch (row.control) {
        case OptionsPanelControl::Toggle: {
            bool value = std::get<bool>(row.edit->value);
            std::snprintf(valueText, sizeof(valueText), "[%c] %s", value ? 'X' : ' ', localized(value ? "On" : "Off"));
            drawRightAlignedControlText(panel, rowY, kToggleX - inset, kToggleX + kToggleWidth - inset, valueText, color);
            break;
        }
        case OptionsPanelControl::Slider: {
            double value = editNumber(row.edit->value);
            const SettingNumericRange& range = *descriptor.numericRange;
            double ratio = std::clamp((value - range.minimum) / (range.maximum - range.minimum), 0.0, 1.0);
            std::snprintf(valueText, sizeof(valueText), "%.2f", value);
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
            std::snprintf(valueText, sizeof(valueText), "< %s >", label);
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
    if (firstVisibleRow < pastVisibleRow && left < right) {
        const int sourceY = sourceTop + firstVisibleRow;
        blitBufferToBuffer(content_.data() + static_cast<size_t>(kWidth) * sourceY + left - pageX_,
            right - left, pastVisibleRow - firstVisibleRow, kWidth,
            surface_ + static_cast<size_t>(screenWidth_) * firstVisibleRow + left, screenWidth_);
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
    if (row.control != OptionsPanelControl::Slider) stepRow(index, 1);
    return true;
}

bool OptionsPanel::handleKey(int keyCode)
{
    if (rowCount() == 0) return false;
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
