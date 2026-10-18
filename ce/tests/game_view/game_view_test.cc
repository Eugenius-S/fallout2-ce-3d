#include "game_view.h"
#include "sprite_card.h"

#include <cmath>
#include <cstdio>

using namespace fallout::scene_renderer;

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)
    const double hpx = std::sqrt(48.0 * 48.0 + 32.0 * 32.0 - 12.0 * 12.0 - 24.0 * 24.0);
    const GameView view = { { -48, 12 }, { 32, 24 }, { 4784, -1190 }, hpx, 25075, 1710, 665, 36.9 };
    auto gamePixel = [&](double x, double y, double z, double* px, double* py) {
        const double u = x + 50.0, v = z + 50.0;
        *px = view.origin[0] + u * view.squ[0] + v * view.sqv[0];
        *py = view.origin[1] + u * view.squ[1] + v * view.sqv[1] - view.hpx * y;
    };

    // The center hex is shown where the engine shows it.
    double centerX = 0, centerZ = 0, lx = 0, ly = 0;
    CHECK(gameViewCenter(view, &centerX, &centerZ));
    CHECK(std::abs(centerX + 12.541666667) < 1e-6 && std::abs(centerZ - 12.6875) < 1e-9);
    CHECK(gameViewProject(view, centerX, 0.0, centerZ, &lx, &ly));
    CHECK(std::abs(lx - 855.0) < 1e-6 && std::abs(ly - 332.0) < 1e-6);

    // On the plane through the center facing the game's camera every point is
    // on the 2D game's pixel: a wall top, a lamp, a floor point alike.
    const SpriteCardProjection cards = { { -48, 12 }, { 32, 24 }, { 4784, -1190 }, hpx, 57.24334, 0, 0 };
    double toward[3];
    CHECK(spriteTowardCamera(cards, toward));
    double centerPixelX, centerPixelY;
    gamePixel(centerX, 0.0, centerZ, &centerPixelX, &centerPixelY);
    const double offsets[3][3] = { { 3.0, 0.0, -1.0 }, { -2.0, 1.5, 4.0 }, { 0.5, 2.0, 0.0 } };
    for (const auto& offset : offsets) {
        const double along = offset[0] * toward[0] + offset[1] * toward[1] + offset[2] * toward[2];
        const double x = centerX + offset[0] - along * toward[0];
        const double y = offset[1] - along * toward[1];
        const double z = centerZ + offset[2] - along * toward[2];
        double px, py;
        gamePixel(x, y, z, &px, &py);
        CHECK(gameViewProject(view, x, y, z, &lx, &ly));
        CHECK(std::abs(lx - (px - centerPixelX + 855.0)) < 1e-6 && std::abs(ly - (py - centerPixelY + 332.0)) < 1e-6);
    }

    // A pinhole: everything on a line through the eye lands on one pixel.
    double eye[3];
    CHECK(gameViewEye(view, eye));
    const double target[3] = { centerX + 4.0, 0.7, centerZ - 3.0 };
    double tx, ty;
    CHECK(gameViewProject(view, target[0], target[1], target[2], &tx, &ty));
    const double scales[2] = { 0.5, 1.7 };
    for (double s : scales) {
        CHECK(gameViewProject(view, eye[0] + s * (target[0] - eye[0]), eye[1] + s * (target[1] - eye[1]),
            eye[2] + s * (target[2] - eye[2]), &lx, &ly));
        CHECK(std::abs(lx - tx) < 1e-6 && std::abs(ly - ty) < 1e-6);
    }

    // Things nearer the eye than that plane look bigger than in 2D.
    double nearX = centerX + toward[0] * 5.0 + 1.0, nearY = toward[1] * 5.0, nearZ = centerZ + toward[2] * 5.0;
    double flatX, flatY;
    gamePixel(nearX, nearY, nearZ, &flatX, &flatY);
    CHECK(gameViewProject(view, nearX, nearY, nearZ, &lx, &ly));
    CHECK(std::abs(lx - 855.0) > std::abs(flatX - centerPixelX) + 1.0);

    // Picking the floor undoes projecting it, also far from the center.
    const double ground[3][2] = { { centerX, centerZ }, { centerX - 9.0, centerZ + 6.0 }, { centerX + 7.5, centerZ - 8.0 } };
    for (const auto& point : ground) {
        CHECK(gameViewProject(view, point[0], 0.0, point[1], &lx, &ly));
        double x = 0, z = 0;
        CHECK(gameViewPickGround(view, lx, ly, &x, &z));
        CHECK(std::abs(x - point[0]) < 1e-6 && std::abs(z - point[1]) < 1e-6);
    }

    // The matrix is the same map.
    float matrix[16];
    CHECK(gameViewMatrix(view, matrix));
    const double p[3] = { centerX - 3.0, 1.2, centerZ + 2.0 };
    double c[4];
    for (int row = 0; row < 4; row++) {
        c[row] = matrix[row] * p[0] + matrix[4 + row] * p[1] + matrix[8 + row] * p[2] + matrix[12 + row];
    }
    CHECK(gameViewProject(view, p[0], p[1], p[2], &lx, &ly));
    CHECK(std::abs((c[0] / c[3] + 1.0) * 0.5 * 1710 - lx) < 1e-3 && std::abs((1.0 - c[1] / c[3]) * 0.5 * 665 - ly) < 1e-3);
    CHECK(std::abs(c[2] / c[3]) < 1.0); // inside the depth range

    GameView bad = view;
    bad.centerTile = -1;
    CHECK(!gameViewMatrix(bad, matrix) && !gameViewPickGround(bad, 10, 10, &lx, &ly));
    std::puts("game_view_tests: ok");
    return 0;
}
