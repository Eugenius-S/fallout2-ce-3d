#include "options_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../color.h"
#include "../art.h"
#include "../draw.h"
#include "../game.h"
#include "../input.h"
#include "../kb.h"
#include "../mouse.h"
#include "../settings.h"
#include "../svga.h"
#include "../text_font.h"
#include "../window_manager.h"

namespace fallout {
namespace scene {

namespace {

constexpr int kWidth = 640;
constexpr int kHeight = 480;
constexpr int kPanelTop = 494;
constexpr int kHeaderHeight = 90;
constexpr int kRowHeight = 44;
constexpr int kRowEvent = 600;
constexpr int kScrollEvent = 599;
constexpr int kTrackTop = 40;
constexpr int kTrackHeight = 382;

double editNumber(const SettingValue& value)
{
    if (const auto* integer = std::get_if<int>(&value)) return *integer;
    if (const auto* real = std::get_if<double>(&value)) return *real;
    return 0.0;
}

} // namespace

OptionsPanel::OptionsPanel() = default;
OptionsPanel::~OptionsPanel() = default;

bool OptionsPanel::initialize(int windowId, unsigned char* surface, const unsigned char* background, int backgroundHeight,
    int screenWidth, int screenHeight, int pageX, int pageY)
{
    windowId_ = windowId;
    surface_ = surface;
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    pageX_ = pageX;
    pageY_ = pageY;
    model_ = std::make_unique<OptionsSettingsModel>();
    rows_ = optionsPanelRows(model_->edits(), "3d");
    contentHeight_ = std::max(kHeight, kPanelTop + kHeaderHeight + static_cast<int>(rows_.size()) * kRowHeight + 18);
    if (background == nullptr || surface_ == nullptr || backgroundHeight < kHeight || screenWidth_ < kWidth || screenHeight_ < kHeight) return false;
    content_.assign(static_cast<size_t>(kWidth) * contentHeight_, 0);
    background_.assign(background, background + static_cast<size_t>(kWidth) * kHeight);
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

    std::copy(background_.begin(), background_.end(), content_.begin());
    scroll_ = 0;
    selectedRow_ = 0;
    return true;
}

unsigned char* OptionsPanel::contentBuffer() { return content_.data(); }
unsigned char* OptionsPanel::footerBuffer() { return content_.data() + static_cast<size_t>(kWidth) * 430; }
int OptionsPanel::contentHeight() const { return contentHeight_; }
int OptionsPanel::rowCount() const { return static_cast<int>(rows_.size()); }
int OptionsPanel::rowTop(int index) const { return kPanelTop + kHeaderHeight + index * kRowHeight; }
int OptionsPanel::scroll() const { return scroll_; }
int OptionsPanel::maxScroll() const { return std::max(0, contentHeight_ - screenHeight_); }
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
        const int tileY = (y - kHeight) % 38;
        for (int x = 0; x < kWidth; x++) {
            const int tileX = (x / 152) % 2 == 0 ? x % 152 : 151 - x % 152;
            content_[static_cast<size_t>(kWidth) * y + x] = background_[static_cast<size_t>(385 + tileY) * kWidth + 24 + tileX];
        }
    }
    unsigned char* panel = content_.data() + static_cast<size_t>(kWidth) * kPanelTop;
    int panelHeight = contentHeight_ - kPanelTop;
    bufferFill(panel, kWidth, panelHeight, kWidth, COLOR_BLACK);
    if (frame_ != nullptr && frameWidth_ >= 640 && frameHeight_ >= 480) {
        constexpr int sideCap = 38;
        constexpr int topCap = 48;
        constexpr int bottomCap = 42;
        constexpr int bezelLeft = 608;
        constexpr int glassX = 340;
        constexpr int glassY = 200;
        for (int y = 0; y < panelHeight; y++) {
            for (int x = 0; x < kWidth; x++) {
                const bool left = x < sideCap;
                const bool right = x >= kWidth - sideCap;
                const bool top = y < topCap;
                const bool bottom = y >= panelHeight - bottomCap;
                const int tileX = ((x - sideCap) / 32) % 2 == 0 ? (x - sideCap) % 32 : 31 - (x - sideCap) % 32;
                const int tileY = ((y - topCap) / 32) % 2 == 0 ? (y - topCap) % 32 : 31 - (y - topCap) % 32;
                int sx = glassX + tileX;
                int sy = glassY + tileY;
                if (left || right) {
                    const int edgeX = std::min(sideCap - 1, (left ? x : x - (kWidth - sideCap)) * 32 / sideCap);
                    sx = left ? frameWidth_ - 1 - edgeX : bezelLeft + edgeX;
                    if (top || bottom) {
                        const int cornerY = top ? y * 50 / topCap : frameHeight_ - 50 + (y - (panelHeight - bottomCap)) * 50 / bottomCap;
                        sy = cornerY;
                    } else {
                        const int edgeY = y - topCap;
                        sy = 50 + (edgeY / 380 % 2 == 0 ? edgeY % 380 : 379 - edgeY % 380);
                    }
                } else if (top) {
                    sx = 260 + ((x - sideCap) / 135 % 2 == 0 ? (x - sideCap) % 135 : 134 - (x - sideCap) % 135);
                    sy = 10 + y;
                } else if (bottom) {
                    const int offset = x - sideCap;
                    sx = 260 + (offset / 135 % 2 == 0 ? offset % 135 : 134 - offset % 135);
                    sy = frameHeight_ - bottomCap + y - (panelHeight - bottomCap);
                }
                panel[static_cast<size_t>(kWidth) * y + x] = frame_[static_cast<size_t>(frameWidth_) * sy + sx];
            }
        }
    }
    rowFrameBackground_.assign(panel, panel + static_cast<size_t>(kWidth) * panelHeight);
    fontSetCurrent(104);
    fontDrawText(panel + kWidth * 62 + 52, "3D OPTIONS", 520, kWidth, COLOR_GREEN);
    fontSetCurrent(101);
    drawRows();
    refresh();
}

void OptionsPanel::drawRows()
{
    if (model_ == nullptr) return;
    for (size_t index = 0; index < rows_.size(); index++) {
        const OptionsPanelRow& row = rows_[index];
        const SettingDescriptor& descriptor = *row.edit->descriptor;
        const int rowY = kHeaderHeight + static_cast<int>(index) * kRowHeight;
        unsigned char* panel = content_.data() + static_cast<size_t>(kWidth) * kPanelTop;
        const bool disabled = descriptor.readOnly || descriptor.commandLineOverride;
        const Color color = disabled ? COLOR_DARK_YELLOW : COLOR_GREEN;
        for (int fillY = rowY; fillY < rowY + kRowHeight - 5; fillY++) {
            std::memcpy(panel + kWidth * fillY + 42,
                rowFrameBackground_.data() + kWidth * fillY + 42, 542);
        }
        if (static_cast<int>(index) == selectedRow_) {
            bufferFill(panel + kWidth * rowY + 42, 4, kRowHeight - 5, kWidth, COLOR_GREEN);
        }
        const char* localizedLabel = descriptor.labelMessageId > 0 && labelResolver_ != nullptr
            ? labelResolver_(descriptor.labelMessageId)
            : nullptr;
        const std::string displayLabel = localizedLabel != nullptr && localizedLabel[0] != '\0'
            ? localizedLabel
            : (descriptor.fallbackLabel.empty() ? descriptor.id : descriptor.fallbackLabel);
        fontDrawText(panel + kWidth * (rowY + 4) + 50, displayLabel.c_str(), 270, kWidth, color);

        char valueText[128] = {};
        switch (row.control) {
        case OptionsPanelControl::Toggle: {
            bool value = std::get<bool>(row.edit->value);
            std::snprintf(valueText, sizeof(valueText), "[%c] %s", value ? 'X' : ' ', value ? "On" : "Off");
            fontDrawText(panel + kWidth * (rowY + 4) + 480, valueText, 100, kWidth, color);
            break;
        }
        case OptionsPanelControl::Slider: {
            double value = editNumber(row.edit->value);
            const SettingNumericRange& range = *descriptor.numericRange;
            double ratio = std::clamp((value - range.minimum) / (range.maximum - range.minimum), 0.0, 1.0);
            std::snprintf(valueText, sizeof(valueText), "%.2f", value);
            fontDrawText(panel + kWidth * (rowY + 4) + 490, valueText, 85, kWidth, color);
            int trackY = rowY + 31;
            bufferDrawLine(panel, kWidth, 350, trackY, 535, trackY, color);
            int knobX = 350 + static_cast<int>(std::round(ratio * 185.0));
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
            fontDrawText(panel + kWidth * (rowY + 4) + 400, valueText, 180, kWidth, color);
            break;
        }
        case OptionsPanelControl::Value:
            if (const auto* value = std::get_if<bool>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%s", *value ? "On" : "Off");
            else if (const auto* value = std::get_if<int>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%d", *value);
            else if (const auto* value = std::get_if<double>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%.2f", *value);
            else if (const auto* value = std::get_if<std::string>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%s", value->c_str());
            fontDrawText(panel + kWidth * (rowY + 4) + 400, valueText, 180, kWidth, color);
            break;
        }
    }
}

void OptionsPanel::compose()
{
    if (surface_ == nullptr || content_.empty()) return;
    const unsigned char* texture = background_.data();
    constexpr int sampleX = 24;
    constexpr int sampleY = 385;
    constexpr int sampleWidth = 152;
    constexpr int sampleHeight = 38;
    for (int y = 0; y < screenHeight_; y++) {
        unsigned char* destination = surface_ + static_cast<size_t>(screenWidth_) * y;
        const int tileY = (y / sampleHeight) % 2 == 0 ? (y % sampleHeight) : (sampleHeight - 1 - y % sampleHeight);
        const int sourceY = sampleY + tileY;
        for (int x = 0; x < screenWidth_; x++) {
            const int mirroredX = (x / sampleWidth) % 2 == 0 ? (x % sampleWidth) : (sampleWidth - 1 - x % sampleWidth);
            destination[x] = texture[static_cast<size_t>(sourceY) * kWidth + sampleX + mirroredX];
        }
    }
    for (int y = 0; y < screenHeight_; y++) {
        const int sourceY = scroll_ + y - pageY_;
        if (sourceY < 0 || sourceY >= contentHeight_) continue;
        const int left = std::max(0, pageX_);
        const int right = std::min(screenWidth_, pageX_ + kWidth);
        if (left >= right) continue;
        std::memcpy(surface_ + static_cast<size_t>(screenWidth_) * y + left,
            content_.data() + static_cast<size_t>(kWidth) * sourceY + left - pageX_, right - left);
    }
    if (maxScroll() > 0) {
        int barX = pageX_ <= 8 ? 8 : std::min(screenWidth_ - 12, pageX_ + kWidth + 4);
        int thumbHeight = 21;
        int travel = kTrackHeight - thumbHeight;
        int trackTop = std::max(0, pageY_ + kTrackTop);
        int thumbTop = trackTop + travel * scroll_ / maxScroll();
        for (int y = 0; y < kTrackHeight; y++) {
            const int tileY = y % 240;
            const int sampleY = (y / 240) % 2 == 0 ? tileY : (239 - tileY);
            if (trackTop + y < 0 || trackTop + y >= screenHeight_) continue;
            std::memcpy(surface_ + static_cast<size_t>(screenWidth_) * (trackTop + y) + barX,
                scrollbarGroove_.data() + static_cast<size_t>(sampleY) * 12, 12);
        }
        const unsigned char* knob = scrollbarKnob_ != nullptr ? scrollbarKnob_->getData() : nullptr;
        if (knob != nullptr) {
            const int knobY = thumbTop + thumbHeight / 2 - 10;
            for (int y = 0; y < 21; y++) {
                for (int x = 0; x < 12; x++) {
                    unsigned char pixel = knob[x * 21 + (20 - y)];
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
        const int rowY = rowTop(index);
        if (x < 350 || x > 535 || y < rowY + 25 || y > rowY + 38) return;
        double ratio = std::clamp((x - 350.0) / 185.0, 0.0, 1.0);
        double value = range.minimum + ratio * (range.maximum - range.minimum);
        if (range.step.has_value()) value = range.minimum + std::round((value - range.minimum) / *range.step) * *range.step;
        value = std::clamp(value, range.minimum, range.maximum);
        if (descriptor.valueType == SettingValueType::Integer) setRowValue(index, static_cast<int>(std::round(value)));
        else setRowValue(index, value);
    };
    updateFromMouse();
    while (true) {
        int rawX = 0, rawY = 0, buttons = 0;
        _mouse_get_raw_state(&rawX, &rawY, &buttons);
        if ((buttons & MOUSE_STATE_LEFT_BUTTON_DOWN) == 0) break;
        const int event = inputGetInput();
        updateFromMouse();
        renderPresent();
        sharedFpsLimiter.mark();
        sharedFpsLimiter.throttle();
        _mouse_get_raw_state(&rawX, &rawY, &buttons);
        if (event == KEY_ESCAPE || _game_user_wants_to_quit != GAME_QUIT_REQUEST_NONE
            || (mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_UP) != 0 || (buttons & MOUSE_STATE_LEFT_BUTTON_DOWN) == 0) break;
    }
}

void OptionsPanel::dragScrollbar(int y)
{
    int thumbHeight = 21;
    int travel = kTrackHeight - thumbHeight;
    const int trackTop = pageY_ + kTrackTop;
    const int thumbTop = trackTop + travel * scroll_ / std::max(1, maxScroll());
    const int grabOffset = y >= thumbTop && y < thumbTop + thumbHeight ? y - thumbTop : thumbHeight / 2;
    while (true) {
        int position = std::clamp(y - trackTop - grabOffset, 0, travel);
        setScroll(travel > 0 ? position * maxScroll() / travel : 0);
        renderPresent();
        sharedFpsLimiter.mark();
        const int event = inputGetInput();
        if (event == KEY_ESCAPE || _game_user_wants_to_quit != GAME_QUIT_REQUEST_NONE) break;
        int x = 0;
        mouseGetPositionInWindow(windowId_, &x, &y);
        int rawX = 0, rawY = 0, buttons = 0;
        _mouse_get_raw_state(&rawX, &rawY, &buttons);
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
    if (row.control == OptionsPanelControl::Slider && mouseX >= 350 && mouseX <= 535
        && mouseY >= rowTop(index) + 25 && mouseY <= rowTop(index) + 38) {
        dragSlider(index);
        return true;
    }
    if (row.control != OptionsPanelControl::Slider) stepRow(index, 1);
    return true;
}

bool OptionsPanel::handleKey(int keyCode)
{
    if (rowCount() == 0) return false;
    switch (keyCode) {
    case KEY_ARROW_UP: setScroll(scroll_ - kRowHeight); return true;
    case KEY_ARROW_DOWN: setScroll(scroll_ + kRowHeight); return true;
    case KEY_PAGE_UP: setScroll(scroll_ - 360); return true;
    case KEY_PAGE_DOWN: setScroll(scroll_ + 360); return true;
    case KEY_HOME: setScroll(0); return true;
    case KEY_END: setScroll(maxScroll()); return true;
    case KEY_ARROW_LEFT: setScroll(std::clamp(rowTop(selectedRow_) - screenHeight_ / 2, 0, maxScroll())); return stepRow(selectedRow_, -1);
    case KEY_ARROW_RIGHT:
    case KEY_SPACE: setScroll(std::clamp(rowTop(selectedRow_) - screenHeight_ / 2, 0, maxScroll())); return stepRow(selectedRow_, 1);
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
