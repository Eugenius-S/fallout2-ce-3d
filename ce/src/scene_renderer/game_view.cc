#include "game_view.h"

#include <algorithm>
#include <cmath>

#include "sprite_card.h"

namespace fallout {
namespace scene_renderer {

namespace {

constexpr double kMapOffset = 50.0;
// Depth range in front of the eye, as shares of its distance. Cards on one hex
// row are told apart by depth steps of 1e-4 units (donor proj.STACK) and a
// solid is 2e-5 nearer than its card (OVER_CARD), so the near plane sits as far
// out as the view allows: nothing comes nearer than about 12 units short of
// the view center. With a near plane at 0.1 the 24-bit depth buffer resolved
// only about 8e-4 units, and such sprites flickered.
constexpr double kNearShare = 0.6;
constexpr double kFarShare = 2.5;

struct Frame {
    double centerPixel[2]; // game pixel of the center hex
    double screenCenter[2]; // logical pixel it is shown at
    double center[3]; // its ground point, GL
    // How much nearer the eye a point is (units): per game pixel its ground
    // point lies further down the screen, and per unit of height.
    double nearPerPixel;
    double nearPerHeight;
    SpriteCardProjection projection;
    double width;
    double height;
    double distance;
    double zoom;
    double nearPlane;
    double farPlane;
};

bool makeFrame(const GameView& view, Frame* frame)
{
    if (view.logicalWidth <= 0 || view.logicalHeight <= 0 || !(view.distance > 20.0) || !(view.distance < 10000.0)
        || !std::isfinite(view.centerPixel[0]) || !std::isfinite(view.centerPixel[1]) || !(view.hpx > 0.0)
        || !std::isfinite(view.zoom) || !(view.zoom > 0.01) || !(view.zoom < 100.0)) {
        return false;
    }
    const SpriteCardProjection projection = {
        { view.squ[0], view.squ[1] }, { view.sqv[0], view.sqv[1] }, { view.origin[0], view.origin[1] },
        view.hpx, 1.0, 0.0, 0.0,
    };
    frame->centerPixel[0] = view.centerPixel[0];
    frame->centerPixel[1] = view.centerPixel[1];
    double toward[3];
    if (!spritePixelToGround(projection, frame->centerPixel[0], frame->centerPixel[1],
            &frame->center[0], &frame->center[2])
        || !spriteTowardCamera(projection, toward)) {
        return false;
    }
    frame->center[1] = 0.0;
    frame->projection = projection;
    // Nearness follows the game's view ray, but only through how far down the
    // screen and how high a point is. The game's screen axes are not square on
    // the floor (the projection is oblique), so the ray's own depth also
    // changes along a screen row: the right of the picture came out further
    // away than the left, and the perspective looked skewed.
    double downX, downZ;
    double originX, originZ;
    if (!spritePixelToGround(projection, frame->centerPixel[0], frame->centerPixel[1] + 1.0, &downX, &downZ)) return false;
    originX = frame->center[0];
    originZ = frame->center[2];
    frame->nearPerPixel = toward[0] * (downX - originX) + toward[2] * (downZ - originZ);
    frame->nearPerHeight = toward[1];
    if (!(frame->nearPerPixel * view.hpx + frame->nearPerHeight > 0.0)) return false;
    // Where the engine shows the center hex of a width x height screen.
    frame->screenCenter[0] = view.screenCenterSet ? view.screenCenter[0] : std::floor((view.logicalWidth - 32) / 2.0) + 16.0;
    frame->screenCenter[1] = view.screenCenterSet ? view.screenCenter[1] : std::floor((view.logicalHeight - 16) / 2.0) + 8.0;
    frame->width = view.logicalWidth;
    frame->height = view.logicalHeight;
    frame->distance = view.distance;
    // Zooming out shows more scene rows, so both planes grow while the zoom
    // shrinks; zoom 1 keeps today's exact planes.
    if (view.zoom == 1.0) {
        frame->nearPlane = view.distance * kNearShare;
        frame->farPlane = view.distance * kFarShare;
    } else {
        frame->nearPlane = view.distance * (1.0 - (1.0 - kNearShare) / view.zoom);
        frame->farPlane = view.distance * kFarShare / view.zoom;
    }
    frame->zoom = view.zoom;
    return true;
}

// Clip coordinates of a GL world point; every component is affine in it.
void clip(const GameView& view, const Frame& frame, double x, double y, double z, double out[4])
{
    const double u = x + kMapOffset;
    const double v = z + kMapOffset;
    const double pixelX = view.origin[0] + u * view.squ[0] + v * view.sqv[0];
    const double pixelY = view.origin[1] + u * view.squ[1] + v * view.sqv[1] - view.hpx * y;
    const double groundPixelY = pixelY + view.hpx * y;
    const double near = frame.nearPerPixel * (groundPixelY - frame.centerPixel[1]) + frame.nearPerHeight * y;
    const double w = (frame.distance - near) / frame.distance;
    out[0] = (pixelX - frame.centerPixel[0]) * 2.0 * frame.zoom / frame.width + (2.0 * frame.screenCenter[0] / frame.width - 1.0) * w;
    out[1] = -(pixelY - frame.centerPixel[1]) * 2.0 * frame.zoom / frame.height + (1.0 - 2.0 * frame.screenCenter[1] / frame.height) * w;
    const double n = frame.nearPlane;
    const double f = frame.farPlane;
    out[2] = (f + n) / (f - n) * w - 2.0 * f * n / ((f - n) * frame.distance);
    out[3] = w;
}

} // namespace

void gameViewTilePixel(int tile, double pixel[2])
{
    spriteHexPixel(tile, &pixel[0], &pixel[1]);
}

void gameViewGlide(double pixel[2], const double target[2], double seconds, bool first)
{
    constexpr double kGlideSeconds = 0.08;
    constexpr double kJumpPixels = 400.0;
    const double dx = target[0] - pixel[0];
    const double dy = target[1] - pixel[1];
    if (first || !std::isfinite(seconds) || std::sqrt(dx * dx + dy * dy) > kJumpPixels) {
        pixel[0] = target[0];
        pixel[1] = target[1];
        return;
    }
    const double share = 1.0 - std::exp(-std::min(std::max(seconds, 0.0), 0.1) / kGlideSeconds);
    pixel[0] += dx * share;
    pixel[1] += dy * share;
}

bool gameViewMatrix(const GameView& view, float matrix[16])
{
    Frame frame;
    if (!makeFrame(view, &frame)) return false;
    double origin[4];
    clip(view, frame, 0.0, 0.0, 0.0, origin);
    for (int column = 0; column < 3; column++) {
        double unit[4];
        clip(view, frame, column == 0 ? 1.0 : 0.0, column == 1 ? 1.0 : 0.0, column == 2 ? 1.0 : 0.0, unit);
        for (int row = 0; row < 4; row++) matrix[column * 4 + row] = static_cast<float>(unit[row] - origin[row]);
    }
    for (int row = 0; row < 4; row++) matrix[12 + row] = static_cast<float>(origin[row]);
    return true;
}

bool gameViewEye(const GameView& view, double eye[3])
{
    Frame frame;
    if (!makeFrame(view, &frame)) return false;
    // The point every line of sight goes through: on the center's game pixel,
    // `distance` nearer.
    const double height = frame.distance / (frame.nearPerPixel * view.hpx + frame.nearPerHeight);
    eye[1] = height;
    return spritePixelToGround(frame.projection, frame.centerPixel[0], frame.centerPixel[1] + view.hpx * height,
        &eye[0], &eye[2]);
}

bool gameViewCenter(const GameView& view, double* worldX, double* worldZ)
{
    Frame frame;
    if (!makeFrame(view, &frame)) return false;
    *worldX = frame.center[0];
    *worldZ = frame.center[2];
    return true;
}

bool gameViewProject(const GameView& view, double x, double y, double z, double* logicalX, double* logicalY)
{
    Frame frame;
    if (!makeFrame(view, &frame)) return false;
    double c[4];
    clip(view, frame, x, y, z, c);
    if (!(c[3] > 1e-9)) return false;
    *logicalX = (c[0] / c[3] + 1.0) * 0.5 * frame.width;
    *logicalY = (1.0 - c[1] / c[3]) * 0.5 * frame.height;
    return std::isfinite(*logicalX) && std::isfinite(*logicalY);
}

bool gameViewProjectGroundPixel(const GameView& view, double pixelX, double pixelY, double* logicalX, double* logicalY)
{
    Frame frame;
    double x, z;
    if (!makeFrame(view, &frame) || !spritePixelToGround(frame.projection, pixelX, pixelY, &x, &z)) return false;
    return gameViewProject(view, x, 0.0, z, logicalX, logicalY);
}

bool gameViewPickGround(const GameView& view, double logicalX, double logicalY, double* worldX, double* worldZ)
{
    Frame frame;
    if (!makeFrame(view, &frame) || !std::isfinite(logicalX) || !std::isfinite(logicalY)) return false;
    const double ndcX = 2.0 * logicalX / frame.width - 1.0;
    const double ndcY = 1.0 - 2.0 * logicalY / frame.height;
    // On the floor clip is affine in (x, z): solve clip.x = ndcX w, clip.y = ndcY w.
    double c0[4], cx[4], cz[4];
    clip(view, frame, 0.0, 0.0, 0.0, c0);
    clip(view, frame, 1.0, 0.0, 0.0, cx);
    clip(view, frame, 0.0, 0.0, 1.0, cz);
    double a[2][2], b[2];
    const double ndc[2] = { ndcX, ndcY };
    for (int row = 0; row < 2; row++) {
        a[row][0] = (cx[row] - c0[row]) - ndc[row] * (cx[3] - c0[3]);
        a[row][1] = (cz[row] - c0[row]) - ndc[row] * (cz[3] - c0[3]);
        b[row] = -(c0[row] - ndc[row] * c0[3]);
    }
    const double determinant = a[0][0] * a[1][1] - a[0][1] * a[1][0];
    if (std::abs(determinant) < 1e-12) return false;
    const double x = (b[0] * a[1][1] - a[0][1] * b[1]) / determinant;
    const double z = (a[0][0] * b[1] - b[0] * a[1][0]) / determinant;
    double c[4];
    clip(view, frame, x, 0.0, z, c);
    if (!(c[3] > 1e-9) || !std::isfinite(x) || !std::isfinite(z)) return false; // behind the eye
    *worldX = x;
    *worldZ = z;
    return true;
}

} // namespace scene_renderer
} // namespace fallout
