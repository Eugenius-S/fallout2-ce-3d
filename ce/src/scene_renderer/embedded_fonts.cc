#include "embedded_fonts.h"

namespace fallout {
namespace scene_renderer {

namespace font_data {
extern const unsigned char kSemiBold[];
extern const std::size_t kSemiBoldSize;
extern const unsigned char kBold[];
extern const std::size_t kBoldSize;
} // namespace font_data

EmbeddedFont embeddedFont(InterfaceFontWeight weight)
{
    switch (weight) {
    case InterfaceFontWeight::SemiBold:
        return { font_data::kSemiBold, font_data::kSemiBoldSize };
    case InterfaceFontWeight::Bold:
        return { font_data::kBold, font_data::kBoldSize };
    }
    return { nullptr, 0 };
}

} // namespace scene_renderer
} // namespace fallout
