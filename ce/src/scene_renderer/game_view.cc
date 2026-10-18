#include "game_view.h"

#include <cmath>

#include "sprite_card.h"

namespace fallout {
namespace scene_renderer {

namespace {

constexpr double kMapOffset = 50.0;
constexpr double kNear = 0.1; // units in front of the eye
constexpr double kFar = 400.0;

struct Frame {
    double centerPixel[2]; // game pixel of the center hex
    double screenCenter[2]; // logical pixel it is shown at
    double center[3]; // its ground point, GL
    double toward[3]; // unit direction to the game's camera, GL
    double width;
    double height;
    double distance;
};

bool makeFrame(const GameView& view, Frame* frame)
{
    if (view.logicalWidth <= 0 || view.logicalHeight <= 0 || !(view.distance > kNear)
        || view.centerTile < 0 || view.centerTile >= 200 * 200 || !(view.hpx > 0.0)) {
        return false;
    }
    const SpriteCardProjection projection = {
        { view.squ[0], view.squ[1] }, { view.sqv[0], view.sqv[1] }, { view.origin[0], view.origin[1] },
        view.hpx, 1.0, 0.0, 0.0,
    };
    spriteHexPixel(view.centerTile, &frame->centerPixel[0], &frame->centerPixel[1]);
    if (!spritePixelToGround(projection, frame->centerPixel[0], frame->centerPixel[1],
            &frame->center[0], &frame->center[2])
        || !spriteTowardCamera(projection, frame->toward)) {
        return false;
    }
    frame->center[1] = 0.0;
    // Where the engine shows the center hex of a width x height screen.
    frame->screenCenter[0] = std::floor((view.logicalWidth - 32) / 2.0) + 16.0;
    frame->screenCenter[1] = std::floor((view.logicalHeight - 16) / 2.0) + 8.0;
    frame->width = view.logicalWidth;
    frame->height = view.logicalHeight;
    frame->distance = view.distance;
    return true;
}

// Clip coordinates of a GL world point; every component is affine in it.
void clip(const GameView& view, const Frame& frame, double x, double y, double z, double out[4])
{
    const double u = x + kMapOffset;
    const double v = z + kMapOffset;
    const double pixelX = view.origin[0] + u * view.squ[0] + v * view.sqv[0];
    const double pixelY = view.origin[1] + u * view.squ[1] + v * view.sqv[1] - view.hpx * y;
    // How far the point is towards the eye from the plane through the center.
    const double near = frame.toward[0] * (x - frame.center[0]) + frame.toward[1] * (y - frame.center[1])
        + frame.toward[2] * (z - frame.center[2]);
    const double w = (frame.distance - near) / frame.distance;
    out[0] = (pixelX - frame.centerPixel[0]) * 2.0 / frame.width + (2.0 * frame.screenCenter[0] / frame.width - 1.0) * w;
    out[1] = -(pixelY - frame.centerPixel[1]) * 2.0 / frame.height + (1.0 - 2.0 * frame.screenCenter[1] / frame.height) * w;
    out[2] = (kFar + kNear) / (kFar - kNear) * w - 2.0 * kFar * kNear / ((kFar - kNear) * frame.distance);
    out[3] = w;
}

} // namespace

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
    for (int axis = 0; axis < 3; axis++) eye[axis] = frame.center[axis] + frame.toward[axis] * frame.distance;
    return true;
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
