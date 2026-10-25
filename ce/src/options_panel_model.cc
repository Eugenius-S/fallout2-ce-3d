#include "options_panel_model.h"

namespace fallout {

namespace {

    OptionsPanelControl controlFor(const SettingDescriptor& descriptor)
    {
        switch (descriptor.valueType) {
        case SettingValueType::Boolean:
            return OptionsPanelControl::Toggle;
        case SettingValueType::Integer:
        case SettingValueType::Real:
            return descriptor.numericRange.has_value() ? OptionsPanelControl::Slider : OptionsPanelControl::Value;
        case SettingValueType::Choice:
            return OptionsPanelControl::Choice;
        case SettingValueType::Text:
        case SettingValueType::KeyBinding:
        default:
            return OptionsPanelControl::Value;
        }
    }

} // namespace

std::vector<OptionsPanelRow> optionsPanelRows(const std::vector<OptionsSettingEdit>& edits, const char* section)
{
    std::vector<OptionsPanelRow> rows;
    if (section == nullptr) return rows;

    for (const auto& edit : edits) {
        if (edit.descriptor == nullptr) continue;
        const SettingDescriptor& descriptor = *edit.descriptor;
        if (descriptor.section != section) continue;
        rows.push_back({ &edit, controlFor(descriptor) });
    }
    return rows;
}

} // namespace fallout
