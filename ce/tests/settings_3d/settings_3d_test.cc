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
static double gConfiguredShadowDensityValue = -1.0;
static const char* gConfiguredLanguage = nullptr;
static bool gInterruptWalkConfigPresent = true;
static bool gInterruptWalkConfigValue = false;
static bool gConfiguredScalePresent = false;
static int gConfiguredScale = 1;
static bool gConfiguredScaleFactorPresent = false;
static double gConfiguredScaleFactor = 1.0;
static bool gConfiguredWindowedPresent = false;
static int gConfiguredWindowed = 0;
static int gWrittenWidth = 0;
static int gWrittenHeight = 0;

bool gameConfigInit(bool, int, char**)
{
    settingsWriteToConfig(true);
    return true;
}
bool gameConfigSave() { return true; }
bool gameConfigExit(bool) { return true; }

bool configGetString(Config*, const char* section, const char* key, char** value)
{
    if (std::strcmp(section, "screen") == 0 && std::strcmp(key, "scale") == 0 && gConfiguredScalePresent) {
        *value = const_cast<char*>("legacy-scale");
        return true;
    }
    if (std::strcmp(section, "screen") == 0 && std::strcmp(key, "scale_factor") == 0
        && gConfiguredScaleFactorPresent) {
        *value = const_cast<char*>("configured");
        return true;
    }
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
bool configGetInt(Config*, const char* section, const char* key, int* value)
{
    if (std::strcmp(section, "screen") == 0 && std::strcmp(key, "windowed") == 0 && gConfiguredWindowedPresent) {
        *value = gConfiguredWindowed;
        return true;
    }
    if (std::strcmp(section, "screen") != 0 || std::strcmp(key, "scale") != 0 || !gConfiguredScalePresent) return false;
    *value = gConfiguredScale;
    return true;
}
bool configGetBool(Config*, const char* section, const char* key, bool* value)
{
    if (std::strcmp(section, "system") != 0 || std::strcmp(key, "interrupt_walk") != 0
        || !gInterruptWalkConfigPresent) return false;
    *value = gInterruptWalkConfigValue;
    return true;
}
bool configGetDouble(Config*, const char* section, const char* key, double* value)
{
    if (std::strcmp(section, "screen") != 0 || std::strcmp(key, "scale_factor") != 0 || !gConfiguredScaleFactorPresent) return false;
    *value = gConfiguredScaleFactor;
    return true;
}
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
bool configSetInt(Config*, const char* section, const char* key, int value)
{
    if (std::strcmp(section, "screen") == 0 && std::strcmp(key, "resolution_x") == 0) gWrittenWidth = value;
    if (std::strcmp(section, "screen") == 0 && std::strcmp(key, "resolution_y") == 0) gWrittenHeight = value;
    return true;
}
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
bool configSetDouble(Config*, const char* section, const char* key, double value)
{
    if (std::strcmp(section, "screen") == 0 && std::strcmp(key, "scale_factor") == 0) {
        gConfiguredScaleFactorPresent = true;
        gConfiguredScaleFactor = value;
    }
    if (std::strcmp(section, "3d") == 0 && std::strcmp(key, "shadow_density") == 0) {
        gConfiguredShadowDensityValue = value;
    }
    return true;
}

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
    const SettingDescriptor* scaleFactor = findDescriptor("screen.scale_factor");
    const SettingDescriptor* resolutionMode = findDescriptor("screen.resolution_mode");
    const SettingDescriptor* windowMode = findDescriptor("screen.windowed");
    if (!check(scaleFactor != nullptr && scaleFactor->valueType == SettingValueType::Real
            && scaleFactor->applyPolicy == SettingApplyPolicy::Restart
            && scaleFactor->numericRange.has_value()
            && scaleFactor->numericRange->minimum == 1.0
            && scaleFactor->numericRange->maximum == 3.0
            && scaleFactor->numericRange->step == 0.2,
            "screen scale factor is a restart setting from 1.0x to 3.0x in 0.2x steps")) return 1;
    if (!check(resolutionMode != nullptr && resolutionMode->valueType == SettingValueType::Choice
            && resolutionMode->applyPolicy == SettingApplyPolicy::Restart
            && windowMode != nullptr && windowMode->valueType == SettingValueType::Choice
            && windowMode->choices.size() == 3,
            "resolution and window mode are registered restart choices")) return 1;
    if (!check(settingsSetValue(*resolutionMode, 25601440, nullptr) && settingsSave()
            && gWrittenWidth == 2560 && gWrittenHeight == 1440,
            "a pending resolution choice writes both legacy resolution keys")) return 1;
    gConfiguredWindowedPresent = true;
    gConfiguredWindowed = static_cast<int>(WindowMode::Windowed);
    if (!check(initializeWith(nullptr) && settings.screen.windowed == WindowMode::Windowed,
            "a saved windowed=0 (Window) survives the load instead of clamping to fullscreen")) return 1;
    gConfiguredWindowed = static_cast<int>(WindowMode::WindowedFullscreen);
    if (!check(initializeWith(nullptr) && settings.screen.windowed == WindowMode::WindowedFullscreen,
            "a saved windowed=2 (Borderless) survives the load")) return 1;
    gConfiguredWindowedPresent = false;
    gConfiguredScalePresent = true;
    gConfiguredScale = 2;
    gConfiguredScaleFactorPresent = false;
    if (!check(initializeWith(nullptr) && settings.screen.scale_factor == 2.0,
            "legacy integer screen scale remains the fractional scale when scale_factor is absent")) return 1;
    gConfiguredScaleFactorPresent = true;
    gConfiguredScaleFactor = 1.4;
    if (!check(initializeWith(nullptr) && settings.screen.scale_factor == 1.4,
            "screen scale_factor overrides the legacy integer scale")) return 1;
    gConfiguredScalePresent = false;
    gConfiguredScaleFactorPresent = false;
    if (!check(initializeWith(nullptr), "settings initialize with missing key")) return 1;
    const SettingDescriptor* descriptor = findThreeDDescriptor();
    const SettingDescriptor* cleanEdgesDescriptor = findCleanEdgesDescriptor();
    const SettingDescriptor* fpsCounter = findDescriptor("3d.fps_counter");
    const SettingDescriptor* shadowDensity = findDescriptor("3d.shadow_density");
    if (!check(descriptor != nullptr, "real [3d].enabled descriptor is registered")) return 1;
    if (!check(cleanEdgesDescriptor != nullptr, "real [3d].clean_edges descriptor is registered")) return 1;
    if (!check(fpsCounter != nullptr && fpsCounter->valueType == SettingValueType::Boolean
            && !std::get<bool>(fpsCounter->defaultValue) && !settings.scene3d.fps_counter
            && fpsCounter->applyPolicy == SettingApplyPolicy::OnClose
            && fpsCounter->fallbackLabel == "FPS counter" && !fpsCounter->fallbackDescription.empty(),
            "FPS counter is registered as a default-off 3D setting with menu text")) return 1;
    const SettingDescriptor* fpsLimit = findDescriptor("3d.fps_limit");
    if (!check(fpsLimit != nullptr && fpsLimit->valueType == SettingValueType::Real
            && std::get<double>(fpsLimit->defaultValue) == 60.0 && settings.scene3d.fps_limit == 60.0
            && fpsLimit->applyPolicy == SettingApplyPolicy::OnClose
            && fpsLimit->fallbackLabel == "FPS limit" && !fpsLimit->fallbackDescription.empty()
            && fpsLimit->numericRange.has_value() && fpsLimit->numericRange->minimum == 0.0
            && fpsLimit->numericRange->maximum == 360.0 && fpsLimit->numericRange->step == 5.0,
            "FPS limit is registered as a 60-default numeric 3D setting with menu text")) return 1;
    const SettingDescriptor* frameLog = findDescriptor("3d.frame_log");
    if (!check(frameLog != nullptr && frameLog->valueType == SettingValueType::Boolean
            && !std::get<bool>(frameLog->defaultValue) && !settings.scene3d.frame_log
            && frameLog->applyPolicy == SettingApplyPolicy::OnClose
            && frameLog->fallbackLabel == "Frame time log" && !frameLog->fallbackDescription.empty(),
            "frame time log is registered as a default-off 3D setting with menu text")) return 1;
    if (!check(shadowDensity != nullptr && std::get<double>(shadowDensity->defaultValue) == 0.8
            && settings.scene3d.shadow_density == 0.8 && gConfiguredShadowDensityValue == 0.8,
            "new settings write shadow density as 0.8")) return 1;
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
    if (!check(scene::kCameraTurnStep == 10.0 && scene::kCameraTurnLimit == 30.0
            && scene::cameraTurnTarget(0.0, 1, false) == 10.0,
            "one press turns 10 degrees, the limit is 30")) return 1;
    for (int i = 0; i < 100; ++i) angle = scene::cameraTurnTarget(angle, 1, false);
    if (!check(angle == 30.0 && scene::cameraTurnTarget(angle, -3, false) == 0.0,
            "held right stops at +30 and three left return to game view")) return 1;
    for (int i = 0; i < 100; ++i) angle = scene::cameraTurnTarget(angle, -1, false);
    if (!check(angle == -30.0 && scene::cameraTurnTarget(angle, 3, false) == 0.0,
            "held left stops at -30 and three right return to game view")) return 1;
    angle = 0.0;
    for (int i = 0; i < 36; ++i) angle = scene::cameraTurnTarget(angle, 1, true);
    if (!check(angle == 360.0 && scene::cameraTurnTarget(0.0, -36, true) == -360.0
            && std::fmod(angle, 360.0) == 0.0,
            "own-risk option permits a full turn in 36 steps both ways and lands on the game view")) return 1;
    if (!check(scene::limitedCameraTurn(270.0) == -30.0
            && scene::limitedCameraTurn(-270.0) == 30.0
            && scene::limitedCameraTurn(360.0) == 0.0
            && scene::limitedCameraTurn(22.5) == 22.5
            && scene::limitedCameraTurn(40.0) == 30.0,
            "revoking unlimited rotation wraps then clamps the current view")) return 1;
    // Edge scroll on a turned camera: (dx, dy) is the edge on the turned screen,
    // dy down. Unturned it is untouched; turned, the game step whose real ground
    // move looks closest to that edge on the turned screen wins.
    const double pi = 3.14159265358979323846;
    const double gameYaw = 0.6; // the unturned camera's yaw (rad)
    const double gamePitch = -0.62; // looking down
    // The ground (x, z) under a screen move of `right`, `up` units at the view
    // center of the unturned camera: right along the camera's right, up along
    // the way it faces, stretched by 1 / sin(pitch) (the floor is foreshortened).
    auto groundFor = [&](double right, double up, double out[2]) {
        const double stretch = up / -std::sin(gamePitch);
        out[0] = right * std::sin(gameYaw) + stretch * std::cos(gameYaw);
        out[1] = -right * std::cos(gameYaw) + stretch * std::sin(gameYaw);
    };
    scene::ScrollGroundBasis basis {};
    groundFor(scene::kScrollStepX, 0.0, basis.horizontal);
    groundFor(0.0, -scene::kScrollStepY, basis.vertical);
    basis.yaw = gameYaw;
    basis.pitch = gamePitch;
    int sx = 9, sy = 9;
    bool unturnedSame = true;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            scene::cameraTurnScrollStep(dx, dy, 0.0, basis, &sx, &sy);
            unturnedSame = unturnedSame && sx == dx && sy == dy;
        }
    }
    if (!check(unturnedSame, "unturned edge scroll is unchanged")) return 1;
    // The turned screen, spelled out with the camera vectors: forward, right and
    // up in GL world, then each step's ground move dotted with right and up.
    auto projectedAngle = [&](int stepDx, int stepDy, double turn, int edgeDx, int edgeDy) {
        const double yaw = gameYaw + turn * pi / 180.0;
        const double right[3] = { std::sin(yaw), 0.0, -std::cos(yaw) };
        const double up[3] = { -std::sin(gamePitch) * std::cos(yaw), std::cos(gamePitch),
            -std::sin(gamePitch) * std::sin(yaw) };
        const double gx = stepDx * basis.horizontal[0] + stepDy * basis.vertical[0];
        const double gz = stepDx * basis.horizontal[1] + stepDy * basis.vertical[1];
        const double screenRight = gx * right[0] + gz * right[2];
        const double screenUp = gx * up[0] + gz * up[2];
        return std::abs(std::remainder(std::atan2(screenUp, screenRight) - std::atan2(-edgeDy, edgeDx), 2.0 * pi));
    };
    bool nearest = true;
    for (const double turn : { 0.0, 10.0, 20.0, 30.0, -30.0 }) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                scene::cameraTurnScrollStep(dx, dy, turn, basis, &sx, &sy);
                const double chosen = projectedAngle(sx, sy, turn, dx, dy);
                for (int candidateDx = -1; candidateDx <= 1; ++candidateDx) {
                    for (int candidateDy = -1; candidateDy <= 1; ++candidateDy) {
                        if (candidateDx == 0 && candidateDy == 0) continue;
                        nearest = nearest && chosen <= projectedAngle(candidateDx, candidateDy, turn, dx, dy) + 1e-9;
                    }
                }
            }
        }
    }
    if (!check(nearest, "every edge and corner at 0, 10, 20, 30 and -30 degrees picks the step with the smallest projected angle")) return 1;
    // The basis measured in the game on kladwtwn (the game view's real, oblique
    // ground move of one step, and its camera yaw and pitch).
    scene::ScrollGroundBasis measured {};
    measured.horizontal[0] = -0.5;
    measured.horizontal[1] = 0.25;
    measured.vertical[0] = 0.500032;
    measured.vertical[1] = 0.750048;
    measured.yaw = -2.158799;
    measured.pitch = -0.480597;
    // The reviewer's case: at +20 the top edge is 38 degrees off vertical for
    // the plain up step (0,-1) but 28 for up-and-left (-1,-1), which wins.
    const double gameStepAngleTop = [&] {
        double right, up;
        scene::groundToTurnedScreen(measured, 20.0, measured.vertical[0] * -1.0, measured.vertical[1] * -1.0, &right, &up);
        return scene::screenAngleBetween(right, up, 0.0, 1.0) * 180.0 / pi;
    }();
    scene::cameraTurnScrollStep(0, -1, 20.0, measured, &sx, &sy);
    if (!check(sx == -1 && sy == -1 && gameStepAngleTop > 37.0 && gameStepAngleTop < 39.0,
            "at +20 degrees the top edge picks up-and-left, not the plain up step")) return 1;
    // Unturned, the measured basis puts each step nearest its own screen direction
    // (so the projection and its signs agree with the game's own scroll).
    bool measuredUnturned = true;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;
            scene::cameraTurnScrollStep(dx, dy, 1e-9, measured, &sx, &sy);
            measuredUnturned = measuredUnturned && sx == dx && sy == dy;
        }
    }
    if (!check(measuredUnturned, "with the measured basis a barely turned camera still picks the same step")) return 1;
    scene::cameraTurnScrollStep(1, 0, 30.0, measured, &sx, &sy);
    const bool rightAt30 = sx == 1 && sy == 0;
    scene::cameraTurnScrollStep(0, -1, 30.0, measured, &sx, &sy);
    const bool topAt30 = sx == -1 && sy == -1;
    if (!check(rightAt30 && topAt30, "measured basis at +30: right edge steps right, top edge steps up-and-left")) return 1;
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
    if (!check(std::string(scene::menuText("Camera rotation", "russian")).find("-30\xB0") != std::string::npos
            && std::string(scene::menuText("Camera rotation", "russian")).find("+30\xB0") != std::string::npos
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
    const auto* glowStrength = findDescriptor("3d.glow_strength");
    if (!check(glowStrength && std::get<double>(glowStrength->defaultValue) == 2.0
            && glowStrength->numericRange->maximum == 8.0
            && settingsSetValue(*glowStrength, SettingValue(8.0), &error)
            && settings.scene3d.glow_strength == 8.0
            && settingsSetValue(*glowStrength, SettingValue(2.0), &error),
            "glow ladder's highest level is selectable without changing the default")) return 1;
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
