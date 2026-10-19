#include "settings.h"
#include "scene/scene_settings.h"
#include "config.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace fallout {

Config gGameConfig = {};
static const char* gConfiguredThreeDValue = nullptr;
static bool gInterruptWalkConfigPresent = true;
static bool gInterruptWalkConfigValue = false;

bool gameConfigInit(bool, int, char**)
{
    settingsWriteToConfig(true);
    return true;
}
bool gameConfigSave() { return true; }
bool gameConfigExit(bool) { return true; }

bool configGetString(Config*, const char* section, const char* key, char** value)
{
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "enabled") == 0 && gConfiguredThreeDValue != nullptr) {
        *value = const_cast<char*>(gConfiguredThreeDValue);
        return true;
    }
    if (std::strcmp(section, "system") == 0 && std::strcmp(key, "interrupt_walk") == 0
        && gInterruptWalkConfigPresent) {
        *value = const_cast<char*>(gInterruptWalkConfigValue ? "1" : "0");
        return true;
    }
    return false;
}
bool configGetString(Config*, const char*, const char*, char** value, const char* defaultValue)
{
    *value = const_cast<char*>(defaultValue);
    return false;
}
bool configGetInt(Config*, const char*, const char*, int*) { return false; }
bool configGetBool(Config*, const char* section, const char* key, bool* value)
{
    if (std::strcmp(section, "system") != 0 || std::strcmp(key, "interrupt_walk") != 0
        || !gInterruptWalkConfigPresent) return false;
    *value = gInterruptWalkConfigValue;
    return true;
}
bool configGetDouble(Config*, const char*, const char*, double*) { return false; }
bool configSetString(Config*, const char* section, const char* key, const char* value)
{
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "enabled") == 0) {
        gConfiguredThreeDValue = value;
    }
    return true;
}
bool configSetInt(Config*, const char*, const char*, int) { return true; }
bool configSetBool(Config*, const char* section, const char* key, bool value)
{
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "enabled") == 0) {
        gConfiguredThreeDValue = value ? "1" : "0";
    }
    if (std::strcmp(section, "system") == 0 && std::strcmp(key, "interrupt_walk") == 0) {
        gInterruptWalkConfigPresent = true;
        gInterruptWalkConfigValue = value;
    }
    return true;
}
bool configSetDouble(Config*, const char*, const char*, double) { return true; }

int compat_stricmp(const char* left, const char* right)
{
    while (*left != '\0' && *right != '\0') {
        const int a = std::tolower(static_cast<unsigned char>(*left++));
        const int b = std::tolower(static_cast<unsigned char>(*right++));
        if (a != b) return a - b;
    }
    return std::tolower(static_cast<unsigned char>(*left)) - std::tolower(static_cast<unsigned char>(*right));
}
void compat_windows_path_to_native(char*) {}
void compat_resolve_path(char*) {}
int debugPrint(const char*, ...) { return 0; }

} // namespace fallout

namespace {

bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

bool initializeWith(const char* configuredValue)
{
    fallout::gConfiguredThreeDValue = configuredValue;
    char executable[] = "settings_3d_tests";
    char* argv[] = { executable };
    return fallout::settingsInit(false, 1, argv);
}

const fallout::SettingDescriptor* findThreeDDescriptor()
{
    for (const auto& descriptor : fallout::settingsGetDescriptors()) {
        if (descriptor.section == "3d" && descriptor.key == "enabled") return &descriptor;
    }
    return nullptr;
}

const fallout::SettingDescriptor* findDescriptor(const char* id)
{
    for (const auto& descriptor : fallout::settingsGetDescriptors()) {
        if (descriptor.id == id) return &descriptor;
    }
    return nullptr;
}

} // namespace

int main()
{
    using namespace fallout;
    if (!check(initializeWith(nullptr), "settings initialize with missing key")) return 1;
    const SettingDescriptor* descriptor = findThreeDDescriptor();
    if (!check(descriptor != nullptr, "real [3d].enabled descriptor is registered")) return 1;
    if (!check(descriptor->id == "3d.enabled" && descriptor->valueType == SettingValueType::Boolean,
            "descriptor id and type")) return 1;
    if (!check(std::get<bool>(descriptor->defaultValue), "schema default is true")) return 1;
    if (!check(descriptor->applyPolicy == SettingApplyPolicy::OnClose
            && descriptor->fallbackLabel == "Enable 3D scene" && !descriptor->fallbackDescription.empty(),
            "descriptor has apply policy and fallback menu metadata")) return 1;
    const SettingDescriptor* unrelatedDescriptor = findDescriptor("system.interrupt_walk");
    if (!check(unrelatedDescriptor != nullptr, "real unrelated setting descriptor is registered")) return 1;
    if (!check(settingsGetRevision(*descriptor) == 0 && settingsGetRevision(*unrelatedDescriptor) == 1,
            "config read increments only the changed setting revision")) return 1;
    if (!check(scene::enabled() && scene::settingsRevision() == 0, "missing value uses true default")) return 1;
    if (!check(settingsGetThreeDRevision() == 0, "unrelated config read does not change the 3D section revision")) return 1;

    scene::SettingsLifecycle unrelatedRevision;
    if (!check(unrelatedRevision.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::EnableFull,
            "setting lifecycle initializes from its enabled-key revision")) return 1;
    if (!check(unrelatedRevision.update(scene::enabled(), settingsGetThreeDRevision() + 1, false) == scene::SettingsAction::NoChange,
            "unrelated section revision does not change the enabled state")) return 1;
    gInterruptWalkConfigValue = true;
    if (!check(initializeWith(nullptr), "read changed unrelated setting through registry")) return 1;
    if (!check(settingsGetRevision(*unrelatedDescriptor) == 2 && settingsGetRevision(*descriptor) == 0,
            "unrelated setting read leaves enabled-key revision unchanged")) return 1;
    if (!check(scene::settingsRevision() == 0 && settingsGetThreeDRevision() == 0,
            "unrelated setting read leaves both enabled and 3D revisions unchanged")) return 1;
    if (!check(unrelatedRevision.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::NoChange,
            "unrelated registered setting does not trigger scene lifecycle")) return 1;

    std::string error;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error), "registered setting accepts false")) return 1;
    if (!check(!scene::enabled() && scene::settingsRevision() == 1, "change increments the monotonic revision")) return 1;
    const auto firstReaderRevision = scene::settingsRevision();
    const auto secondReaderRevision = scene::settingsRevision();
    if (!check(firstReaderRevision == 1 && secondReaderRevision == 1, "reading revision does not consume it")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error)
            && scene::settingsRevision() == 1, "setting the same value does not increment revision")) return 1;

    if (!check(initializeWith("not-a-bool") && scene::enabled(), "invalid value restores literal true default after false")) return 1;
    if (!check(scene::settingsRevision() == 2, "invalid fallback transition increments revision")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error), "can disable again through registered setter")) return 1;
    if (!check(initializeWith(nullptr) && scene::enabled(), "missing value restores true after false")) return 1;
    if (!check(scene::settingsRevision() == 4, "missing fallback transition increments revision")) return 1;
    if (!check(initializeWith("off") && !scene::enabled(), "valid textual false value is read")) return 1;
    if (!check(scene::settingsRevision() == 5, "valid config transition increments revision")) return 1;
    if (!check(initializeWith("  TrUe  ") && scene::enabled(), "boolean text is trimmed and case-insensitive")) return 1;
    if (!check(scene::settingsRevision() == 6, "textual true transition increments revision")) return 1;
    if (!check(initializeWith("0") && !scene::enabled(), "serialized zero reads as false")) return 1;
    if (!check(scene::settingsRevision() == 7, "serialized zero transition increments revision")) return 1;
    if (!check(initializeWith("1") && scene::enabled(), "serialized one reads as true")) return 1;
    if (!check(scene::settingsRevision() == 8, "serialized one transition increments revision")) return 1;
    if (!check(initializeWith("2") && scene::enabled(), "invalid numeric value falls back to true")) return 1;
    if (!check(scene::settingsRevision() == 8, "unchanged true fallback does not increment revision")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error), "registered setter can disable before invalid fallback")) return 1;
    if (!check(initializeWith("2") && scene::enabled(), "invalid numeric value restores true after false")) return 1;
    if (!check(scene::settingsRevision() == 10, "invalid numeric fallback transition increments revision")) return 1;
    if (!check(!settingsSetValue(*descriptor, SettingValue(1), &error), "wrong value type is rejected")) return 1;

    scene::SettingsLifecycle initialZeroRevision;
    if (!check(initialZeroRevision.update(true, 0, false) == scene::SettingsAction::EnableFull,
            "default-on setting activates Full from initial revision zero")) return 1;

    scene::SettingsLifecycle lifecycle;
    if (!check(lifecycle.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::EnableFull,
            "default-on setting activates Full once at current revision")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error), "lifecycle test disables through schema")) return 1;
    if (!check(lifecycle.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::Disable,
            "setting revision applies OFF to live scene")) return 1;
    if (!check(lifecycle.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::NoChange,
            "manual view changes are not overridden without a setting revision")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(true), &error), "lifecycle test enables through schema")) return 1;
    if (!check(lifecycle.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::EnableFull,
            "OFF-to-ON revision reactivates Full")) return 1;

    scene::SettingsLifecycle debugStartup;
    if (!check(debugStartup.update(scene::enabled(), scene::settingsRevision(), true) == scene::SettingsAction::NoChange,
            "explicit debug startup suppresses only default automatic activation")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error), "debug lifecycle test disables through schema")) return 1;
    if (!check(debugStartup.update(scene::enabled(), scene::settingsRevision(), true) == scene::SettingsAction::Disable,
            "debug startup still responds to later setting revisions")) return 1;
    settingsWriteToConfig(false);
    if (!check(gConfiguredThreeDValue != nullptr && std::strcmp(gConfiguredThreeDValue, "0") == 0,
            "normal settings save writes the current disabled value")) return 1;
    return 0;
}
