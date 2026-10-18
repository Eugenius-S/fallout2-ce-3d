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

double determinant(const SpriteCardProjection& projection)
{
    return projection.squ[0] * projection.sqv[1] - projection.sqv[0] * projection.squ[1];
}

// Ground offset (du, dv) of a screen offset between two points on the floor.
void deltaToGround(const SpriteCardProjection& projection, double dx, double dy, double* du, double* dv)
{
    const double det = determinant(projection);
    *du = (projection.sqv[1] * dx - projection.sqv[0] * dy) / det;
    *dv = (projection.squ[0] * dy - projection.squ[1] * dx) / det;
}

bool projectionIsValid(const SpriteCardProjection& projection)
{
    return std::abs(determinant(projection)) >= 1e-9 && projection.hpx > 0.0 && projection.bbPx > 0.0;
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
    if (std::abs(determinant(projection)) < 1e-9) return false;
    double u, v;
    deltaToGround(projection, pixelX - projection.origin[0], pixelY - projection.origin[1], &u, &v);
    *worldX = u - kMapOffset;
    *worldZ = v - kMapOffset;
    return std::isfinite(*worldX) && std::isfinite(*worldZ);
}

bool spriteTowardCamera(const SpriteCardProjection& projection, double toward[3])
{
    if (!projectionIsValid(projection)) return false;
    // Donor (u, v, z): the ground step that keeps the pixel, and the height
    // that undoes its screen drop; GL is (u, z, v).
    const double u = projection.sqv[0];
    const double v = -projection.squ[0];
    const double z = (u * projection.squ[1] + v * projection.sqv[1]) / projection.hpx;
    const double length = std::sqrt(u * u + v * v + z * z);
    if (!(length > 0.0)) return false;
    toward[0] = u / length;
    toward[1] = z / length;
    toward[2] = v / length;
    return true;
}

bool standingSpriteCard(const SpriteCardProjection& projection, const SpriteCardArt& art,
    SpriteCardVertex corners[4], double* anchorX, double* anchorZ)
{
    if (!artIsValid(art) || !projectionIsValid(projection)) return false;
    double hexX, hexY;
    spriteHexPixel(art.tile, &hexX, &hexY);
    double groundX, groundZ;
    if (!spritePixelToGround(projection, hexX + art.objectX, hexY + art.objectY, &groundX, &groundZ)) return false;

    // The sprite's rows relative to the anchor row, as the engine draws it.
    const double top = static_cast<double>(art.artYOffset) - (art.height - 1);
    const double bottom = top + art.height;
    double shift = 0.0; // pixels down the screen to the middle of the footprint
    double lean = 0.0; // half the footprint depth, in screen pixels
    double z0;
    double z1;
    if (bottom <= 0.0) {
        z0 = -bottom / projection.hpx;
        z1 = -top / projection.hpx;
    } else {
        const double far = std::min(bottom, std::max(0.0, top));
        shift = (bottom + far) / 2.0;
        lean = (bottom - far) / 2.0;
        z0 = 0.0;
        z1 = (far - top) / projection.hpx;
    }

    // Screen right and screen down as ground directions; one pixel down the
    // screen moves the floor point by `down` (not a unit vector).
    double rightU, rightV, downU, downV;
    deltaToGround(projection, 1.0, 0.0, &rightU, &rightV);
    deltaToGround(projection, 0.0, 1.0, &downU, &downV);
    const double rightLength = std::sqrt(rightU * rightU + rightV * rightV);
    if (!(rightLength > 0.0)) return false;
    rightU /= rightLength;
    rightV /= rightLength;

    const double baseX = groundX + downU * shift;
    const double baseZ = groundZ + downV * shift;
    const double left = (static_cast<double>(art.artXOffset) - art.width / 2) / projection.bbPx;
    const double right = left + art.width / projection.bbPx;
    auto corner = [&](double along, bool bottomEdge, double u, double v) {
        const double towards = bottomEdge ? lean : -lean;
        return SpriteCardVertex {
            baseX + rightU * along + downU * towards,
            bottomEdge ? z0 : z1,
            baseZ + rightV * along + downV * towards,
            u, v,
        };
    };
    corners[0] = corner(left, false, 0.0, 0.0);
    corners[1] = corner(left, true, 0.0, 1.0);
    corners[2] = corner(right, true, 1.0, 1.0);
    corners[3] = corner(right, false, 1.0, 0.0);
    if (anchorX != nullptr) *anchorX = groundX;
    if (anchorZ != nullptr) *anchorZ = groundZ;
    return std::isfinite(baseX) && std::isfinite(baseZ) && std::isfinite(z1);
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

double spriteRowPlane(const SpriteCardProjection& projection, int tile, int stack)
{
    if (tile < 0 || tile >= kGridSize) return 0.0;
    const int hx = tile % kGridWidth;
    // All sprites of a row lie in the plane through its even hexes.
    double pixelX, pixelY;
    spriteHexPixel(tile - (hx & 1), &pixelX, &pixelY);
    double x, z;
    if (!spritePixelToGround(projection, pixelX, pixelY, &x, &z)) return 0.0;
    double nudge = projection.rowTilt * hx;
    if (projection.stack > 0.0) {
        nudge += projection.stack * std::min(static_cast<double>(std::max(stack, 0)), projection.rowTilt / projection.stack - 1.0);
    }
    return z + nudge;
}

} // namespace scene_renderer
} // namespace fallout
