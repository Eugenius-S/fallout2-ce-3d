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

} // namespace scene
} // namespace fallout
