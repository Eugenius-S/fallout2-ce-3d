#include "sprite_card.h"

#include <cmath>
#include <cstdio>

using namespace fallout::scene_renderer;

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)
#define NEAR(a, b) (std::abs((a) - (b)) < 1e-9)
    // KLADWTWN projection; tile 25075 is the hero's start, whose anchor the
    // hero preview recorded as (-12.541666667, 12.6875).
    const SpriteCardProjection projection = { { -48, 12 }, { 32, 24 }, { 4784, -1190 }, 20.0, 40.0 };
    double hexX = 0, hexY = 0;
    spriteHexPixel(25075, &hexX, &hexY);
    CHECK(NEAR(hexX, 4992.0) && NEAR(hexY, 764.0));
    double groundX = 0, groundZ = 0;
    CHECK(spritePixelToGround(projection, hexX, hexY, &groundX, &groundZ));
    CHECK(std::abs(groundX + 12.541666667) < 1e-6 && NEAR(groundZ, 12.6875));

    // A 30x60 sprite whose bottom row is the anchor row stands on the floor:
    // its bottom edge is at height 0, centered on the anchor along the screen.
    SpriteCardArt art = { 25075, 0, 0, 0, 0, 30, 60 };
    SpriteCardVertex corners[4] = {};
    double anchorX = 0, anchorZ = 0;
    CHECK(standingSpriteCard(projection, art, 1.0, 0.0, corners, &anchorX, &anchorZ));
    CHECK(NEAR(anchorX, groundX) && NEAR(anchorZ, groundZ));
    CHECK(NEAR(corners[1].y, 0.0) && NEAR(corners[2].y, 0.0));
    CHECK(NEAR(corners[0].y, (59.0 + 0.5) / 20.0));
    CHECK(NEAR(corners[1].x, groundX - 15.0 / 40.0) && NEAR(corners[2].x, groundX + 15.0 / 40.0));
    CHECK(NEAR(corners[0].z, groundZ) && NEAR(corners[3].z, groundZ));
    CHECK(corners[0].u == 0.0 && corners[0].v == 0.0 && corners[2].u == 1.0 && corners[2].v == 1.0);

    // Drawn wholly above its anchor (a sign on a post): the card floats.
    art.artYOffset = -10;
    CHECK(standingSpriteCard(projection, art, 1.0, 0.0, corners, nullptr, nullptr));
    CHECK(NEAR(corners[1].y, 9.0 / 20.0) && NEAR(corners[0].y, 69.0 / 20.0));

    // The object's own pixel movement shifts the anchor, as in the engine.
    art = { 25075, 16, -8, 0, 0, 30, 60 };
    CHECK(standingSpriteCard(projection, art, 1.0, 0.0, corners, &anchorX, &anchorZ));
    double movedX = 0, movedZ = 0;
    CHECK(spritePixelToGround(projection, hexX + 16, hexY - 8, &movedX, &movedZ));
    CHECK(NEAR(anchorX, movedX) && NEAR(anchorZ, movedZ));

    // A flat sprite covers the ground under the pixels the engine draws:
    // left = x - width/2, top = y - (height - 1), edges of pixels.
    art = { 25075, 0, 0, 4, 2, 31, 20 };
    CHECK(flatSpriteCard(projection, art, 0.01, corners));
    const double left = hexX + 4 - 15;
    const double top = hexY + 2 - 19;
    const double expected[4][2] = { { left, top }, { left, top + 20 }, { left + 31, top + 20 }, { left + 31, top } };
    for (int index = 0; index < 4; index++) {
        double x = 0, z = 0;
        CHECK(spritePixelToGround(projection, expected[index][0], expected[index][1], &x, &z));
        CHECK(NEAR(corners[index].x, x) && NEAR(corners[index].z, z) && NEAR(corners[index].y, 0.01));
    }
    CHECK(corners[1].u == 0.0 && corners[1].v == 1.0 && corners[3].u == 1.0 && corners[3].v == 0.0);

    // Off-grid tiles and empty frames are refused.
    art = { -1, 0, 0, 0, 0, 30, 60 };
    CHECK(!standingSpriteCard(projection, art, 1.0, 0.0, corners, nullptr, nullptr));
    CHECK(!flatSpriteCard(projection, art, 0.0, corners));
    art = { 25075, 0, 0, 0, 0, 0, 60 };
    CHECK(!standingSpriteCard(projection, art, 1.0, 0.0, corners, nullptr, nullptr));
    std::puts("sprite_card_tests: ok");
    return 0;
}
