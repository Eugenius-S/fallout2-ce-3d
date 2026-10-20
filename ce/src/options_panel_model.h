#ifndef FALLOUT_OPTIONS_PANEL_MODEL_H_
#define FALLOUT_OPTIONS_PANEL_MODEL_H_

#include <vector>

#include "options_settings_model.h"

namespace fallout {

enum class OptionsPanelControl {
    Toggle,
    Slider,
    Choice,
    Value,
};

struct OptionsPanelRow {
    const OptionsSettingEdit* edit;
    OptionsPanelControl control;
};

// Projects registered descriptors into menu rows without depending on setting ids.
std::vector<OptionsPanelRow> optionsPanelRows(const std::vector<OptionsSettingEdit>& edits, const char* section);

} // namespace fallout

#endif /* FALLOUT_OPTIONS_PANEL_MODEL_H_ */
