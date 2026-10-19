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
    return settingsGetThreeDRevision();
}

SettingsAction SettingsLifecycle::update(bool value, std::uint64_t revision, bool suppressInitialEnable)
{
    if (!initialized_) {
        initialized_ = true;
        revision_ = revision;
        if (!value) return SettingsAction::Disable;
        return suppressInitialEnable ? SettingsAction::NoChange : SettingsAction::EnableFull;
    }
    if (revision == revision_) return SettingsAction::NoChange;
    revision_ = revision;
    return value ? SettingsAction::EnableFull : SettingsAction::Disable;
}

} // namespace scene
} // namespace fallout
