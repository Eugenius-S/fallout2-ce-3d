#include "scene_settings.h"

#include "../settings.h"

namespace fallout {
namespace scene {

bool enabled()
{
    return settings.scene3d.enabled;
}

std::uint64_t settingsRevision()
{
    static const SettingDescriptor* enabledDescriptor = nullptr;
    if (enabledDescriptor == nullptr) {
        for (const auto& descriptor : settingsGetDescriptors()) {
            if (descriptor.id == "3d.enabled") {
                enabledDescriptor = &descriptor;
                break;
            }
        }
    }
    return enabledDescriptor != nullptr ? settingsGetRevision(*enabledDescriptor) : 0;
}

SettingsAction SettingsLifecycle::update(bool value, std::uint64_t revision, bool suppressInitialEnable)
{
    if (!initialized_) {
        initialized_ = true;
        revision_ = revision;
        enabled_ = value;
        if (!value) return SettingsAction::Disable;
        return suppressInitialEnable ? SettingsAction::NoChange : SettingsAction::EnableFull;
    }
    if (revision == revision_) return SettingsAction::NoChange;
    revision_ = revision;
    if (enabled_ == value) return SettingsAction::NoChange;
    enabled_ = value;
    return value ? SettingsAction::EnableFull : SettingsAction::Disable;
}

} // namespace scene
} // namespace fallout
