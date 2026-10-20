#include "options_panel_model.h"
#include "options_schema.h"

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace fallout {

static std::unordered_map<std::string, std::string> gConfigValues;

bool configGetString(Config*, const char* section, const char* key, char** value)
{
    auto it = gConfigValues.find(std::string(section) + "." + key);
    if (it == gConfigValues.end()) return false;
    *value = const_cast<char*>(it->second.c_str());
    return true;
}

} // namespace fallout

namespace {

bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

fallout::SettingDescriptor descriptor(const char* id, const char* section, fallout::SettingValueType type)
{
    fallout::SettingDescriptor result;
    result.id = id;
    result.section = section;
    result.valueType = type;
    return result;
}

void setSchemaValue(const char* key, const char* value)
{
    fallout::gConfigValues[std::string("3d.test.") + key] = value;
}

bool parseSchema(fallout::SettingDescriptor* result)
{
    fallout::Config config {};
    return fallout::optionsSchemaParseSection(&config, "3d.test", result);
}

} // namespace

int main()
{
    using namespace fallout;

    std::vector<SettingDescriptor> descriptors;
    descriptors.push_back(descriptor("3d.first", "3d", SettingValueType::Boolean));
    descriptors.push_back(descriptor("ui.other", "ui", SettingValueType::Boolean));
    descriptors.push_back(descriptor("3d.second", "3d", SettingValueType::Real));
    descriptors.back().numericRange = SettingNumericRange { 0.1, 4.0, 0.1 };
    descriptors.push_back(descriptor("3d.third", "3d", SettingValueType::Choice));
    descriptors.back().choices = { { 0, "Low" }, { 1, "High" } };
    descriptors.push_back(descriptor("3d.future", "3d", SettingValueType::Text));
    std::vector<OptionsSettingEdit> edits {
        { &descriptors[0], SettingValue(false), SettingValue(true) },
        { &descriptors[1], SettingValue(false), SettingValue(false) },
        { &descriptors[2], SettingValue(1.0), SettingValue(1.5) },
        { &descriptors[3], SettingValue(0), SettingValue(1) },
        { &descriptors[4], SettingValue(std::string("old")), SettingValue(std::string("new")) },
    };

    const auto rows = optionsPanelRows(edits, "3d");
    if (!check(rows.size() == 4, "panel projection includes every descriptor in the requested section")) return 1;
    if (!check(rows[0].edit == &edits[0] && std::get<bool>(rows[0].edit->value)
            && rows[0].control == OptionsPanelControl::Toggle,
            "boolean descriptor becomes a toggle in registry order")) return 1;
    if (!check(rows[1].edit == &edits[2] && std::get<double>(rows[1].edit->value) == 1.5
            && rows[1].control == OptionsPanelControl::Slider,
            "numeric descriptor with range metadata becomes a slider")) return 1;
    if (!check(rows[2].edit == &edits[3] && std::get<int>(rows[2].edit->value) == 1
            && rows[2].control == OptionsPanelControl::Choice,
            "choice descriptor becomes a choice row")) return 1;
    if (!check(rows[3].edit == &edits[4] && rows[3].control == OptionsPanelControl::Value,
            "future unsupported descriptor remains visible as a value row")) return 1;
    if (!check(optionsPanelRows(edits, nullptr).empty(), "null section returns no rows")) return 1;

    gConfigValues.clear();
    setSchemaValue("type", "real");
    setSchemaValue("category", "gameplay");
    setSchemaValue("default", "1.0");
    setSchemaValue("min", "0.1");
    setSchemaValue("max", "4.0");
    setSchemaValue("step", "0.1");
    SettingDescriptor parsed;
    if (!check(parseSchema(&parsed), "schema accepts a bounded real setting")) return 1;
    if (!check(parsed.numericRange.has_value() && parsed.numericRange->minimum == 0.1
            && parsed.numericRange->maximum == 4.0 && parsed.numericRange->step == 0.1,
            "schema preserves optional numeric range and step metadata")) return 1;

    gConfigValues.erase("3d.test.max");
    if (!check(!parseSchema(&parsed), "schema rejects an incomplete numeric range")) return 1;
    setSchemaValue("max", "4.0");
    setSchemaValue("step", "0");
    if (!check(!parseSchema(&parsed), "schema rejects a nonpositive slider step")) return 1;
    setSchemaValue("step", "0.1");
    setSchemaValue("default", "5.0");
    if (!check(!parseSchema(&parsed), "schema rejects a default outside its numeric range")) return 1;

    return 0;
}
