#ifndef FALLOUT_SCENE_RENDERER_EMBEDDED_FONTS_H
#define FALLOUT_SCENE_RENDERER_EMBEDDED_FONTS_H

#include <cstddef>

namespace fallout {
namespace scene_renderer {

enum class InterfaceFontWeight { SemiBold, Bold };

struct EmbeddedFont {
    const unsigned char* data;
    std::size_t size;
};

// Trusted TTF data with static lifetime; unknown weights return an empty view.
EmbeddedFont embeddedFont(InterfaceFontWeight weight);

} // namespace scene_renderer
} // namespace fallout

#endif
