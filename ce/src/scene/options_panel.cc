#include "options_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../color.h"
#include "../draw.h"
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
constexpr int kBodyHeight = 430;
constexpr int kFooterTop = 430;
constexpr int kPanelTop = 478;
constexpr int kHeaderHeight = 48;
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

bool OptionsPanel::initialize(int windowId, unsigned char* surface, const unsigned char* background, int backgroundHeight)
{
    windowId_ = windowId;
    surface_ = surface;
    model_ = std::make_unique<OptionsSettingsModel>();
    rows_ = optionsPanelRows(model_->edits(), "3d");
    contentHeight_ = std::max(kHeight, kPanelTop + kHeaderHeight + static_cast<int>(rows_.size()) * kRowHeight + 18);
    content_.assign(static_cast<size_t>(kWidth) * contentHeight_, 0);
    footer_.assign(static_cast<size_t>(kWidth) * (kHeight - kFooterTop), 0);
    if (background == nullptr || surface_ == nullptr || backgroundHeight < kHeight) return false;

    std::copy(background, background + static_cast<size_t>(kWidth) * kHeight, content_.begin());
    std::copy(background + static_cast<size_t>(kWidth) * kFooterTop,
        background + static_cast<size_t>(kWidth) * kHeight, footer_.begin());
    scroll_ = 0;
    selectedRow_ = 0;
    return true;
}

unsigned char* OptionsPanel::contentBuffer() { return content_.data(); }
unsigned char* OptionsPanel::footerBuffer() { return footer_.data(); }
int OptionsPanel::contentHeight() const { return contentHeight_; }
int OptionsPanel::rowCount() const { return static_cast<int>(rows_.size()); }
int OptionsPanel::rowTop(int index) const { return kPanelTop + kHeaderHeight + index * kRowHeight; }
int OptionsPanel::scroll() const { return scroll_; }
int OptionsPanel::maxScroll() const { return std::max(0, contentHeight_ - kBodyHeight); }

void OptionsPanel::setButtonLayout(const std::vector<std::pair<int, int>>& legacyButtons, int speedButton, int speedTop, const std::vector<int>& rowButtons)
{
    legacyButtons_ = legacyButtons;
    speedButton_ = speedButton;
    speedTop_ = speedTop;
    rowButtons_ = rowButtons;
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

void OptionsPanel::showPanelForSnapshot()
{
    setScroll(kPanelTop);
}

void OptionsPanel::updateButtons()
{
    Window* window = windowGetWindow(windowId_);
    if (window == nullptr) return;
    auto place = [this](int id, int originalTop, int viewportHeight) {
        if (id == -1) return;
        for (Button* button = windowGetWindow(windowId_)->buttonListHead; button != nullptr; button = button->next) {
            if (button->id != id) continue;
            int height = button->rect.bottom - button->rect.top + 1;
            int top = originalTop - scroll_;
            int visibleHeight = scroll_ == 0 && originalTop >= kBodyHeight ? kHeight : viewportHeight;
            bool visible = top >= 0 && top + height <= visibleHeight;
            button->rect.top = visible ? top : -height;
            button->rect.bottom = button->rect.top + height - 1;
            break;
        }
    };
    for (const auto& item : legacyButtons_) place(item.first, item.second, kBodyHeight);
    place(speedButton_, speedTop_, kBodyHeight);
    for (size_t index = 0; index < rowButtons_.size(); index++) place(rowButtons_[index], rowTop(static_cast<int>(index)), kBodyHeight);
}

void OptionsPanel::draw()
{
    if (content_.empty()) return;
    unsigned char* panel = content_.data() + static_cast<size_t>(kWidth) * kPanelTop;
    int panelHeight = contentHeight_ - kPanelTop;
    bufferFill(panel, kWidth, panelHeight, kWidth, COLOR_DARK_GREEN_2);
    bufferDrawRect(panel, kWidth, 12, 8, 627, panelHeight - 8, COLOR_GREEN);
    fontSetCurrent(104);
    fontDrawText(panel + kWidth * 12 + 28, "3D OPTIONS", 560, kWidth, COLOR_GREEN);
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
        bufferDrawRect(panel, kWidth, 22, rowY, 604, rowY + kRowHeight - 5,
            static_cast<int>(index) == selectedRow_ ? COLOR_LIGHT_GREEN : COLOR_GREEN);
        fontDrawText(panel + kWidth * (rowY + 4) + 32, descriptor.fallbackLabel.c_str(), 300, kWidth, color);

        char valueText[128] = {};
        switch (row.control) {
        case OptionsPanelControl::Toggle: {
            bool value = std::get<bool>(row.edit->value);
            std::snprintf(valueText, sizeof(valueText), "[%c] %s", value ? 'X' : ' ', value ? "On" : "Off");
            fontDrawText(panel + kWidth * (rowY + 4) + 500, valueText, 100, kWidth, color);
            break;
        }
        case OptionsPanelControl::Slider: {
            double value = editNumber(row.edit->value);
            const SettingNumericRange& range = *descriptor.numericRange;
            double ratio = std::clamp((value - range.minimum) / (range.maximum - range.minimum), 0.0, 1.0);
            std::snprintf(valueText, sizeof(valueText), "%.2f", value);
            fontDrawText(panel + kWidth * (rowY + 4) + 550, valueText, 76, kWidth, color);
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
            fontDrawText(panel + kWidth * (rowY + 4) + 430, valueText, 180, kWidth, color);
            break;
        }
        case OptionsPanelControl::Value:
            if (const auto* value = std::get_if<bool>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%s", *value ? "On" : "Off");
            else if (const auto* value = std::get_if<int>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%d", *value);
            else if (const auto* value = std::get_if<double>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%.2f", *value);
            else if (const auto* value = std::get_if<std::string>(&row.edit->value)) std::snprintf(valueText, sizeof(valueText), "%s", value->c_str());
            fontDrawText(panel + kWidth * (rowY + 4) + 430, valueText, 180, kWidth, color);
            break;
        }
    }
}

void OptionsPanel::compose()
{
    if (surface_ == nullptr || content_.empty()) return;
    for (int y = 0; y < kBodyHeight; y++) {
        int sourceY = scroll_ + y;
        std::memcpy(surface_ + kWidth * y, content_.data() + kWidth * sourceY, kWidth);
    }
    std::memcpy(surface_ + kWidth * kFooterTop, footer_.data(), footer_.size());
    if (scroll_ == 0) {
        for (int y = kFooterTop; y < kHeight; y++) {
            std::memcpy(surface_ + kWidth * y + 384, content_.data() + kWidth * y + 384, kWidth - 384);
        }
    }
    if (maxScroll() > 0) {
        int thumbHeight = std::max(24, kTrackHeight * kBodyHeight / contentHeight_);
        int travel = kTrackHeight - thumbHeight;
        int thumbTop = kTrackTop + travel * scroll_ / maxScroll();
        bufferDrawRect(surface_, kWidth, 629, kTrackTop, 636, kTrackTop + kTrackHeight, COLOR_DARK_GREEN);
        bufferDrawRect(surface_, kWidth, 628, thumbTop, 637, thumbTop + thumbHeight, COLOR_GREEN);
    }
    if (!error_.empty()) {
        bufferFill(surface_ + kWidth * 4 + 18, 604, 22, kWidth, COLOR_DARK_GREEN_2);
        fontSetCurrent(101);
        fontDrawText(surface_ + kWidth * 6 + 22, error_.c_str(), 590, kWidth, COLOR_LIGHT_RED);
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
    int x = 0, y = 0;
    while (true) {
        inputGetInput();
        mouseGetPositionInWindow(windowId_, &x, &y);
        double ratio = std::clamp((x - 350.0) / 185.0, 0.0, 1.0);
        double value = range.minimum + ratio * (range.maximum - range.minimum);
        if (range.step.has_value()) value = range.minimum + std::round((value - range.minimum) / *range.step) * *range.step;
        value = std::clamp(value, range.minimum, range.maximum);
        if (descriptor.valueType == SettingValueType::Integer) setRowValue(index, static_cast<int>(std::round(value)));
        else setRowValue(index, value);
        renderPresent();
        if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) break;
    }
}

void OptionsPanel::dragScrollbar(int y)
{
    int thumbHeight = std::max(24, kTrackHeight * kBodyHeight / contentHeight_);
    int travel = kTrackHeight - thumbHeight;
    while (true) {
        int position = std::clamp(y, kTrackTop, kTrackTop + kTrackHeight) - kTrackTop - thumbHeight / 2;
        setScroll(travel > 0 ? position * maxScroll() / travel : 0);
        renderPresent();
        sharedFpsLimiter.mark();
        inputGetInput();
        int x = 0;
        mouseGetPositionInWindow(windowId_, &x, &y);
        if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) break;
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
    const OptionsPanelRow& row = rows_[index];
    const SettingDescriptor& descriptor = *row.edit->descriptor;
    if (descriptor.readOnly || descriptor.commandLineOverride) return true;
    if (row.control == OptionsPanelControl::Slider) {
        dragSlider(index);
        return true;
    }
    stepRow(index, 1);
    return true;
}

bool OptionsPanel::handleKey(int keyCode)
{
    if (rowCount() == 0) return false;
    switch (keyCode) {
    case KEY_ARROW_UP: selectedRow_ = std::max(0, selectedRow_ - 1); setScroll(std::max(0, rowTop(selectedRow_) - kBodyHeight + kRowHeight)); drawRows(); refresh(); return true;
    case KEY_ARROW_DOWN: selectedRow_ = std::min(rowCount() - 1, selectedRow_ + 1); if (rowTop(selectedRow_) - scroll_ >= kBodyHeight) setScroll(rowTop(selectedRow_) - kBodyHeight + kRowHeight); drawRows(); refresh(); return true;
    case KEY_PAGE_UP: setScroll(scroll_ - 360); return true;
    case KEY_PAGE_DOWN: setScroll(scroll_ + 360); return true;
    case KEY_HOME: setScroll(0); return true;
    case KEY_END: setScroll(maxScroll()); return true;
    case KEY_ARROW_LEFT: return stepRow(selectedRow_, -1);
    case KEY_ARROW_RIGHT:
    case KEY_SPACE: return stepRow(selectedRow_, 1);
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
