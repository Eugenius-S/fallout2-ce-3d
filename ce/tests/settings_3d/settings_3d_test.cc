#include "settings.h"
#include "scene/menu_text.h"
#include "scene/scene_settings.h"
#include "scene/camera_turn.h"
#include "config.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace fallout {

Config gGameConfig = {};
static const char* gConfiguredThreeDValue = nullptr;
static const char* gConfiguredCleanEdgesValue = nullptr;
static const char* gConfiguredLanguage = nullptr;
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
    if (std::strcmp(section, "system") == 0 && std::strcmp(key, "language") == 0 && gConfiguredLanguage != nullptr) {
        *value = const_cast<char*>(gConfiguredLanguage);
        return true;
    }
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "enabled") == 0 && gConfiguredThreeDValue != nullptr) {
        *value = const_cast<char*>(gConfiguredThreeDValue);
        return true;
    }
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "clean_edges") == 0 && gConfiguredCleanEdgesValue != nullptr) {
        *value = const_cast<char*>(gConfiguredCleanEdgesValue);
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
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "clean_edges") == 0) {
        gConfiguredCleanEdgesValue = value;
    }
    return true;
}
bool configSetInt(Config*, const char*, const char*, int) { return true; }
bool configSetBool(Config*, const char* section, const char* key, bool value)
{
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "enabled") == 0) {
        gConfiguredThreeDValue = value ? "1" : "0";
    }
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "clean_edges") == 0) {
        gConfiguredCleanEdgesValue = value ? "1" : "0";
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

const fallout::SettingDescriptor* findCleanEdgesDescriptor()
{
    for (const auto& descriptor : fallout::settingsGetDescriptors()) {
        if (descriptor.section == "3d" && descriptor.key == "clean_edges") return &descriptor;
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
    const SettingDescriptor* cleanEdgesDescriptor = findCleanEdgesDescriptor();
    if (!check(descriptor != nullptr, "real [3d].enabled descriptor is registered")) return 1;
    if (!check(cleanEdgesDescriptor != nullptr, "real [3d].clean_edges descriptor is registered")) return 1;
    if (!check(cleanEdgesDescriptor->id == "3d.clean_edges"
            && cleanEdgesDescriptor->valueType == SettingValueType::Boolean
            && std::get<bool>(cleanEdgesDescriptor->defaultValue), "clean_edges is a default-on boolean")) return 1;
    if (!check(cleanEdgesDescriptor->fallbackLabel == "Clean map edges"
            && !cleanEdgesDescriptor->fallbackDescription.empty(), "clean_edges has English fallback menu metadata")) return 1;
    if (!check(scene::cleanEdges() && scene::cleanEdgesRevision() == 0,
            "missing clean_edges key uses its literal true default")) return 1;
    if (!check(descriptor->id == "3d.enabled" && descriptor->valueType == SettingValueType::Boolean,
            "descriptor id and type")) return 1;
    if (!check(std::get<bool>(descriptor->defaultValue), "schema default is true")) return 1;
    if (!check(descriptor->applyPolicy == SettingApplyPolicy::OnClose
            && descriptor->fallbackLabel == "Enable 3D scene" && !descriptor->fallbackDescription.empty(),
            "descriptor has apply policy and fallback menu metadata")) return 1;
    const SettingDescriptor* unrelatedDescriptor = findDescriptor("system.interrupt_walk");
    const SettingDescriptor* rotation = findDescriptor("3d.camera_rotation");
    const SettingDescriptor* unlimited = findDescriptor("3d.camera_rotation_unlimited");
    if (!check(rotation && unlimited && !std::get<bool>(rotation->defaultValue)
            && !std::get<bool>(unlimited->defaultValue)
            && !settings.scene3d.camera_rotation_unlimited
            && unlimited->applyPolicy == SettingApplyPolicy::OnClose
            && unlimited->fallbackLabel.find("not recommended") != std::string::npos,
            "rotation and unrestricted rotation are independent default-off menu options")) return 1;
    double angle = 0.0;
    for (int i = 0; i < 100; ++i) angle = scene::cameraTurnTarget(angle, 1, false);
    if (!check(angle == 45.0 && scene::cameraTurnTarget(angle, -1, false) == 0.0,
            "held right stops at +45 and one left returns to game view")) return 1;
    for (int i = 0; i < 100; ++i) angle = scene::cameraTurnTarget(angle, -1, false);
    if (!check(angle == -45.0 && scene::cameraTurnTarget(angle, 1, false) == 0.0,
            "held left stops at -45 and one right returns to game view")) return 1;
    angle = 0.0;
    for (int i = 0; i < 8; ++i) angle = scene::cameraTurnTarget(angle, 1, true);
    if (!check(angle == 360.0 && scene::cameraTurnTarget(0.0, -8, true) == -360.0,
            "own-risk option permits a full turn in both directions")) return 1;
    if (!check(scene::limitedCameraTurn(270.0) == -45.0
            && scene::limitedCameraTurn(-270.0) == 45.0
            && scene::limitedCameraTurn(360.0) == 0.0
            && scene::limitedCameraTurn(22.5) == 22.5,
            "revoking unlimited rotation wraps then clamps the current view")) return 1;
    if (!check(unrelatedDescriptor != nullptr, "real unrelated setting descriptor is registered")) return 1;
    if (!check(settingsGetRevision(*descriptor) == 0 && settingsGetRevision(*unrelatedDescriptor) == 1,
            "config read increments only the changed setting revision")) return 1;
    if (!check(scene::enabled() && scene::settingsRevision() == 0, "missing value uses true default")) return 1;
    const SettingDescriptor* lights = nullptr;
    for (const auto& candidate : settingsGetDescriptors()) {
        if (candidate.section == "3d" && candidate.key == "lights") lights = &candidate;
    }
    if (!check(lights != nullptr && std::get<bool>(lights->defaultValue) && settings.scene3d.lights
            && lights->fallbackLabel == "Light", "[3d].lights is registered, default on, with a menu label")) return 1;
    const SettingDescriptor* zoom = findDescriptor("3d.zoom");
    const SettingDescriptor* zoomMin = findDescriptor("3d.zoom_min");
    const SettingDescriptor* zoomMax = findDescriptor("3d.zoom_max");
    if (!check(zoom != nullptr && zoomMin != nullptr && zoomMax != nullptr,
            "camera zoom descriptors are registered")) return 1;
    for (const SettingDescriptor* numericSetting : { zoom, zoomMin, zoomMax }) {
        if (!check(numericSetting->applyPolicy == SettingApplyPolicy::OnClose
                && numericSetting->numericRange.has_value()
                && numericSetting->numericRange->minimum == 0.1
                && numericSetting->numericRange->maximum == 10.0
                && numericSetting->numericRange->step == 0.1,
                "camera zoom descriptors expose OnClose policy and slider bounds")) return 1;
    }
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
    if (!check(settingsSetValue(*lights, SettingValue(false), &error) && !settings.scene3d.lights,
            "[3d].lights can be switched off")) return 1;
    if (!check(settingsGetRevision(*lights) == 1 && settingsGetRevision(*descriptor) == 0
            && scene::settingsRevision() == 0, "[3d].lights leaves the enabled-key revision unchanged")) return 1;
    if (!check(unrelatedRevision.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::NoChange,
            "a second [3d] key does not switch the scene")) return 1;
    if (!check(settingsSetValue(*lights, SettingValue(true), &error) && settings.scene3d.lights,
            "[3d].lights can be switched back on")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error), "registered setting accepts false")) return 1;
    if (!check(!scene::enabled() && scene::settingsRevision() == 1, "change increments the monotonic revision")) return 1;
    const auto firstReaderRevision = scene::settingsRevision();
    const auto secondReaderRevision = scene::settingsRevision();
    if (!check(firstReaderRevision == 1 && secondReaderRevision == 1, "reading revision does not consume it")) return 1;
    if (!check(settingsSetValue(*descriptor, SettingValue(false), &error)
            && scene::settingsRevision() == 1, "setting the same value does not increment revision")) return 1;

    scene::SettingsLifecycle cleanEdgesIsolation;
    if (!check(cleanEdgesIsolation.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::Disable,
            "enabled lifecycle observes the current enabled key")) return 1;
    const auto enabledRevisionBeforeCleanEdges = scene::settingsRevision();
    const auto cleanEdgesRevisionBefore = scene::cleanEdgesRevision();
    if (!check(settingsSetValue(*cleanEdgesDescriptor, SettingValue(false), &error)
            && !scene::cleanEdges(), "registered clean_edges setting can be changed")) return 1;
    if (!check(scene::cleanEdgesRevision() == cleanEdgesRevisionBefore + 1
            && scene::settingsRevision() == enabledRevisionBeforeCleanEdges,
            "clean_edges change advances only its own per-key revision")) return 1;
    if (!check(cleanEdgesIsolation.update(scene::enabled(), scene::settingsRevision(), false) == scene::SettingsAction::NoChange,
            "clean_edges change does not activate or disable the 3D scene")) return 1;
    if (!check(settingsSetValue(*cleanEdgesDescriptor, SettingValue(true), &error)
            && scene::cleanEdgesRevision() == cleanEdgesRevisionBefore + 2,
            "clean_edges can be restored through the same setting descriptor")) return 1;
    if (!check(settingsSetValue(*cleanEdgesDescriptor, SettingValue(false), &error),
            "clean_edges can be disabled before testing missing fallback")) return 1;
    gConfiguredCleanEdgesValue = nullptr;
    if (!check(initializeWith(nullptr) && scene::cleanEdges(),
            "missing clean_edges is inserted with its schema default before registry read")) return 1;
    if (!check(settingsSetValue(*cleanEdgesDescriptor, SettingValue(false), &error),
            "clean_edges can be disabled before testing invalid fallback")) return 1;
    gConfiguredCleanEdgesValue = "2";
    if (!check(initializeWith(nullptr) && scene::cleanEdges(),
            "invalid numeric clean_edges restores its schema default")) return 1;

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
    std::string zoomError;
    if (!check(settingsSetValue(*zoom, SettingValue(1.25), &zoomError) && settings.scene3d.zoom == 1.25,
            "camera zoom applies immediately through its registered OnClose descriptor")) return 1;
    if (!check(settingsSetValue(*zoom, SettingValue(1.0), &zoomError), "camera zoom test restores the default")) return 1;
    for (const auto& item : settingsGetDescriptors()) {
        if (item.section != "3d") continue;
        if (!check(scene::hasMenuTranslation(item.fallbackLabel.c_str()),
                (item.id + " needs a Russian menu label").c_str())) return 1;
        if (!check(scene::hasMenuTranslation(item.fallbackDescription.c_str()),
                (item.id + " needs a Russian menu description").c_str())) return 1;
    }
    if (!check(std::string(scene::menuText("On", "RuSsIaN")) == "\xC2\xEA\xEB",
            "Russian menu text uses CP1251 and case-insensitive language selection")) return 1;
    if (!check(std::string(scene::menuText("Camera rotation", "russian")).find("-45\xB0") != std::string::npos
            && std::string(scene::menuText("Camera rotation", "russian")).find("+45\xB0") != std::string::npos
            && std::string(scene::menuText("Camera rotation", "russian")).find('?') == std::string::npos,
            "camera range preserves signs and degree symbols in the game's encoding")) return 1;
    if (!check(static_cast<unsigned char>(scene::menuText("Sharp interface text", "russian")[1]) == 0xB8,
            "Russian yo is encoded for the existing game fonts")) return 1;
    if (!check(std::string(scene::menuText("On", "german")) == "On"
            && std::string(scene::menuText("Missing translation", "russian")) == "Missing translation"
            && !scene::hasMenuTranslation("Missing translation"),
            "unknown languages and missing translations keep the English fallback")) return 1;
    auto localizedDescriptors = settingsGetDescriptors();
    scene::localizeMenuDescriptors(localizedDescriptors, "russian");
    for (std::size_t i = 0; i < localizedDescriptors.size(); ++i) {
        const auto& original = settingsGetDescriptors()[i];
        if (original.section != "3d") continue;
        if (!check(localizedDescriptors[i].fallbackLabel != original.fallbackLabel
                && localizedDescriptors[i].fallbackDescription != original.fallbackDescription
                && localizedDescriptors[i].id == original.id,
                "Russian labels and descriptions change without changing setting IDs")) return 1;
    }
    scene::localizeMenuDescriptors(localizedDescriptors, "english");
    for (std::size_t i = 0; i < localizedDescriptors.size(); ++i) {
        if (!check(localizedDescriptors[i].fallbackLabel == settingsGetDescriptors()[i].fallbackLabel
                && localizedDescriptors[i].fallbackDescription == settingsGetDescriptors()[i].fallbackDescription,
                "reinitialization restores exact English metadata")) return 1;
    }
    const std::string status = scene::menuStatusText("snapshot: example.png", "russian");
    if (!check(status != "snapshot: example.png" && status.find("example.png") != std::string::npos,
            "localized status keeps dynamic filenames intact")) return 1;
    const auto englishDescriptors = settingsGetDescriptors();
    gConfiguredLanguage = "russian";
    if (!check(initializeWith(nullptr), "initialize real descriptors in Russian")) return 1;
    for (std::size_t i = 0; i < englishDescriptors.size(); ++i) {
        if (englishDescriptors[i].section != "3d") continue;
        if (!check(settingsGetDescriptors()[i].fallbackLabel != englishDescriptors[i].fallbackLabel
                && settingsGetDescriptors()[i].fallbackDescription != englishDescriptors[i].fallbackDescription,
                "settings initialization localizes all real 3D labels and descriptions")) return 1;
    }
    gConfiguredLanguage = "english";
    if (!check(initializeWith(nullptr), "initialize real descriptors in English again")) return 1;
    for (std::size_t i = 0; i < englishDescriptors.size(); ++i) {
        if (!check(settingsGetDescriptors()[i].fallbackLabel == englishDescriptors[i].fallbackLabel
                && settingsGetDescriptors()[i].fallbackDescription == englishDescriptors[i].fallbackDescription,
                "language changes do not retain Russian descriptor metadata")) return 1;
    }
    const auto* unlimitedSetting = findDescriptor("3d.camera_rotation_unlimited");
    const auto* rotationSetting = findDescriptor("3d.camera_rotation");
    if (!check(settingsSetValue(*unlimitedSetting, SettingValue(true), &error)
            && settings.scene3d.camera_rotation_unlimited && !settings.scene3d.camera_rotation,
            "own-risk menu toggle does not enable rotation by itself")) return 1;
    if (!check(settingsSetValue(*rotationSetting, SettingValue(true), &error)
            && settings.scene3d.camera_rotation
            && settingsSetValue(*unlimitedSetting, SettingValue(false), &error)
            && !settings.scene3d.camera_rotation_unlimited && settings.scene3d.camera_rotation,
            "revoking unlimited through the real registry keeps limited rotation enabled")) return 1;
    return 0;
}
