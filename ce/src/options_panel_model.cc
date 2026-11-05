#include "options_panel_model.h"

#include <cstring>

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
        // The display scale belongs at the start of the 3D settings page,
        // where the player now chooses the complete game presentation.
        if (descriptor.section != section
            && !(std::strcmp(section, "3d") == 0
                && (descriptor.id == "screen.scale_factor" || descriptor.id == "screen.resolution_mode"
                    || descriptor.id == "screen.windowed" || descriptor.id == "preferences.mouse_sensitivity"))) continue;
        rows.push_back({ &edit, controlFor(descriptor) });
    }
    return rows;
}

} // namespace fallout
