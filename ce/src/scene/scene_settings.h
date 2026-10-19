#ifndef FALLOUT_SCENE_SCENE_SETTINGS_H
#define FALLOUT_SCENE_SCENE_SETTINGS_H

#include <cstdint>

namespace fallout {
namespace scene {

bool enabled();
// Revision for the enabled key only; reads never consume it.
std::uint64_t settingsRevision();

enum class SettingsAction {
    NoChange,
    EnableFull,
    Disable,
};

// Tracks the setting once per consumer. Same-value revisions do not cause an action.
class SettingsLifecycle {
public:
    SettingsAction update(bool enabled, std::uint64_t revision, bool suppressInitialEnable);

private:
    bool initialized_ = false;
    bool enabled_ = false;
    std::uint64_t revision_ = 0;
};

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_SCENE_SETTINGS_H */
