#include "sprite_card.h"

#include <cmath>
#include <cstdio>

using namespace fallout::scene_renderer;

namespace {

// The game's own picture of a scene point (GL x, y = height, z).
void gameProject(const SpriteCardProjection& p, const SpriteCardVertex& point, double* x, double* y)
{
    const double u = point.x + 50.0;
    const double v = point.z + 50.0;
    *x = p.origin[0] + u * p.squ[0] + v * p.sqv[0];
    *y = p.origin[1] + u * p.squ[1] + v * p.sqv[1] - p.hpx * point.y;
}

} // namespace

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)
#define NEAR(a, b) (std::abs((a) - (b)) < 1e-9)
    // KLADWTWN projection (donor proj.py); tile 25075 is the hero's start,
    // whose anchor the hero preview recorded as (-12.541666667, 12.6875).
    const double hpx = std::sqrt(48.0 * 48.0 + 32.0 * 32.0 - 12.0 * 12.0 - 24.0 * 24.0);
    const SpriteCardProjection projection = { { -48, 12 }, { 32, 24 }, { 4784, -1190 }, hpx, 57.24334, 5e-4, 1e-4 };
    double hexX = 0, hexY = 0;
    spriteHexPixel(25075, &hexX, &hexY);
    CHECK(NEAR(hexX, 4992.0) && NEAR(hexY, 764.0));
    double groundX = 0, groundZ = 0;
    CHECK(spritePixelToGround(projection, hexX, hexY, &groundX, &groundZ));
    CHECK(std::abs(groundX + 12.541666667) < 1e-6 && NEAR(groundZ, 12.6875));

    // Points that share one game pixel lie along the camera direction.
    double toward[3] = {};
    CHECK(spriteTowardCamera(projection, toward));
    const SpriteCardVertex onFloor = { groundX, 0.0, groundZ, 0, 0 };
    const SpriteCardVertex alongRay = { groundX + toward[0] * 3.0, toward[1] * 3.0, groundZ + toward[2] * 3.0, 0, 0 };
    double ax, ay, bx, by;
    gameProject(projection, onFloor, &ax, &ay);
    gameProject(projection, alongRay, &bx, &by);
    CHECK(std::abs(ax - bx) < 1e-9 && std::abs(ay - by) < 1e-9 && toward[1] > 0.0);

    // A person (bottom row at the anchor), a bed (drawn far below its
    // anchor) and a cabinet (drawn wholly above it): from the game's camera
    // every card corner lands on the corner of the sprite the engine draws.
    const int yOffsets[3] = { 0, 20, -10 };
    for (int yOffset : yOffsets) {
        const SpriteCardArt art = { 25075, 3, -2, 4, yOffset, 30, 60 };
        SpriteCardVertex corners[4] = {};
        double anchorX = 0, anchorZ = 0;
        CHECK(standingSpriteCard(projection, art, corners, &anchorX, &anchorZ));
        const double left = hexX + 3 + 4 - 15;
        const double top = hexY - 2 + yOffset - 59;
        const double expected[4][2] = { { left, top }, { left, top + 60 }, { left + 30, top + 60 }, { left + 30, top } };
        for (int index = 0; index < 4; index++) {
            double x, y;
            gameProject(projection, corners[index], &x, &y);
            CHECK(std::abs(x - expected[index][0]) < 1e-6 && std::abs(y - expected[index][1]) < 1e-6);
        }
        CHECK(corners[0].u == 0.0 && corners[0].v == 0.0 && corners[2].u == 1.0 && corners[2].v == 1.0);
        if (yOffset >= 0) {
            // Standing on the floor: the bottom edge is on the ground.
            CHECK(NEAR(corners[1].y, 0.0) && NEAR(corners[2].y, 0.0) && corners[0].y > 0.0);
        } else {
            CHECK(NEAR(corners[1].y, 9.0 / hpx) && NEAR(corners[0].y, 69.0 / hpx));
        }
    }

    // A flat sprite covers the ground under the pixels the engine draws:
    // left = x - width/2, top = y - (height - 1), edges of pixels.
    SpriteCardArt art = { 25075, 0, 0, 4, 2, 31, 20 };
    SpriteCardVertex corners[4] = {};
    CHECK(flatSpriteCard(projection, art, 0.01, corners));
    const double left = hexX + 4 - 15;
    const double top = hexY + 2 - 19;
    const double expected[4][2] = { { left, top }, { left, top + 20 }, { left + 31, top + 20 }, { left + 31, top } };
    for (int index = 0; index < 4; index++) {
        double x = 0, z = 0;
        CHECK(spritePixelToGround(projection, expected[index][0], expected[index][1], &x, &z));
        CHECK(NEAR(corners[index].x, x) && NEAR(corners[index].z, z) && NEAR(corners[index].y, 0.01));
    }

    // Row planes: a hex row shares one plane, a hair nearer for the hexes the
    // game paints later (further left, larger hx) and for later sprites on a hex.
    const double even = spriteRowPlane(projection, 25074, 0);
    const double odd = spriteRowPlane(projection, 25075, 0);
    CHECK(odd > even && std::abs(odd - even - 5e-4) < 1e-9);
    CHECK(spriteRowPlane(projection, 25075, 2) > odd);
    CHECK(NEAR(spriteRowPlane(projection, 25075, 99) - odd, 4.0 * 1e-4));
    CHECK(spriteRowPlane(projection, 25075 + 200, 0) > odd); // the next row is nearer

    // Off-grid tiles and empty frames are refused.
    art = { -1, 0, 0, 0, 0, 30, 60 };
    CHECK(!standingSpriteCard(projection, art, corners, nullptr, nullptr));
    CHECK(!flatSpriteCard(projection, art, 0.0, corners));
    art = { 25075, 0, 0, 0, 0, 0, 60 };
    CHECK(!standingSpriteCard(projection, art, corners, nullptr, nullptr));
    std::puts("sprite_card_tests: ok");
    return 0;
}
