#include "sprite_card.h"

#include <algorithm>
#include <cmath>

namespace fallout {
namespace scene_renderer {

namespace {

constexpr double kMapOffset = 50.0;
constexpr int kGridWidth = 200;
constexpr int kGridSize = 200 * 200;

bool artIsValid(const SpriteCardArt& art)
{
    return art.tile >= 0 && art.tile < kGridSize
        && art.width > 0 && art.height > 0 && art.width <= 2048 && art.height <= 2048
        && std::abs(art.objectX) <= 32768 && std::abs(art.objectY) <= 32768
        && std::abs(art.artXOffset) <= 32768 && std::abs(art.artYOffset) <= 32768;
}

} // namespace

void spriteHexPixel(int tile, double* x, double* y)
{
    const int hx = tile % kGridWidth;
    const int hy = tile / kGridWidth;
    const int a = kGridWidth - 1 - hx;
    *x = 48.0 * (a >> 1) + 32.0 * (a & 1) + 16.0 * hy + 16.0;
    *y = -12.0 * (a >> 1) + 12.0 * hy + 8.0;
}

bool spritePixelToGround(const SpriteCardProjection& projection, double pixelX, double pixelY,
    double* worldX, double* worldZ)
{
    const double determinant = projection.squ[0] * projection.sqv[1] - projection.sqv[0] * projection.squ[1];
    if (std::abs(determinant) < 1e-9) return false;
    const double dx = pixelX - projection.origin[0];
    const double dy = pixelY - projection.origin[1];
    *worldX = (projection.sqv[1] * dx - projection.sqv[0] * dy) / determinant - kMapOffset;
    *worldZ = (projection.squ[0] * dy - projection.squ[1] * dx) / determinant - kMapOffset;
    return std::isfinite(*worldX) && std::isfinite(*worldZ);
}

bool standingSpriteCard(const SpriteCardProjection& projection, const SpriteCardArt& art,
    double screenRightX, double screenRightZ, SpriteCardVertex corners[4], double* anchorX, double* anchorZ)
{
    if (!artIsValid(art) || projection.hpx <= 0.0 || projection.bbPx <= 0.0) return false;
    double hexX, hexY;
    spriteHexPixel(art.tile, &hexX, &hexY);
    double groundX, groundZ;
    if (!spritePixelToGround(projection, hexX + art.objectX, hexY + art.objectY, &groundX, &groundZ)) return false;

    // Donor standing card: rows below the anchor row lean back onto the
    // ground plane, so a sprite drawn above its anchor stands from the floor.
    const double top = static_cast<double>(art.artYOffset) - (art.height - 1);
    const double bottom = top + art.height;
    double up0;
    double up1;
    if (bottom <= 0.0) {
        up0 = -bottom / projection.hpx;
        up1 = -top / projection.hpx;
    } else {
        const double farRow = std::min(bottom, std::max(0.0, top));
        up0 = 0.0;
        up1 = (farRow - top + 0.5 * (bottom - farRow)) / projection.hpx;
    }
    const double left = (static_cast<double>(art.artXOffset) - art.width / 2) / projection.bbPx;
    const double right = left + art.width / projection.bbPx;
    auto corner = [&](double along, double height, double u, double v) {
        return SpriteCardVertex { groundX + screenRightX * along, height, groundZ + screenRightZ * along, u, v };
    };
    corners[0] = corner(left, up1, 0.0, 0.0);
    corners[1] = corner(left, up0, 0.0, 1.0);
    corners[2] = corner(right, up0, 1.0, 1.0);
    corners[3] = corner(right, up1, 1.0, 0.0);
    if (anchorX != nullptr) *anchorX = groundX;
    if (anchorZ != nullptr) *anchorZ = groundZ;
    return std::isfinite(up0) && std::isfinite(up1);
}

bool flatSpriteCard(const SpriteCardProjection& projection, const SpriteCardArt& art, double height,
    SpriteCardVertex corners[4])
{
    if (!artIsValid(art)) return false;
    double hexX, hexY;
    spriteHexPixel(art.tile, &hexX, &hexY);
    // The pixel rectangle _obj_render_object draws, as edges of pixels.
    const double left = hexX + art.objectX + art.artXOffset - art.width / 2;
    const double top = hexY + art.objectY + art.artYOffset - (art.height - 1);
    const double right = left + art.width;
    const double bottom = top + art.height;
    const double pixels[4][4] = {
        { left, top, 0.0, 0.0 },
        { left, bottom, 0.0, 1.0 },
        { right, bottom, 1.0, 1.0 },
        { right, top, 1.0, 0.0 },
    };
    for (int index = 0; index < 4; index++) {
        double x, z;
        if (!spritePixelToGround(projection, pixels[index][0], pixels[index][1], &x, &z)) return false;
        corners[index] = { x, height, z, pixels[index][2], pixels[index][3] };
    }
    return true;
}

} // namespace scene_renderer
} // namespace fallout
