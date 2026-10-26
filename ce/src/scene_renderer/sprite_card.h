#ifndef FALLOUT_SCENE_RENDERER_SPRITE_CARD_H
#define FALLOUT_SCENE_RENDERER_SPRITE_CARD_H

namespace fallout {
namespace scene_renderer {

// The donor projection between game screen pixels and the scene ground:
//     px(u, v, z) = origin + u * squ + v * sqv - (0, hpx * z)        (y down)
struct SpriteCardProjection {
    double squ[2];
    double sqv[2];
    double origin[2];
    double hpx; // screen pixels per unit of height
    double bbPx; // screen pixels per unit along the screen's horizontal
    double rowTilt; // depth nudge per hex column (row_plane)
    double stack; // depth nudge per sprite painted earlier on the same hex
};

// One FRM frame placed the way _obj_render_object places it: the hex center
// of `tile`, plus the object's pixel movement, plus the art's offsets.
struct SpriteCardArt {
    int tile;
    int objectX;
    int objectY;
    int artXOffset;
    int artYOffset;
    int width;
    int height;
};

struct SpriteCardVertex {
    double x;
    double y;
    double z;
    double u;
    double v;
};

// Screen pixel of the hex center, as tileToScreenXY + (16, 8) gives it for a
// screen whose origin is the donor's.
void spriteHexPixel(int tile, double* x, double* y);

// Scene ground point (GL x, z) under a screen pixel.
bool spritePixelToGround(const SpriteCardProjection& projection, double pixelX, double pixelY,
    double* worldX, double* worldZ);

// Unit direction (GL x, y, z) from the scene towards the game's camera: the
// line of points that share one screen pixel.
bool spriteTowardCamera(const SpriteCardProjection& projection, double toward[3]);

// A sprite card as the donor viewer's game views stand it (card() in geom.js):
// from the floor under its lowest row up to its top row, leaning back over its
// footprint, so that from the game's camera every texel is where the game
// draws it: a person stands almost upright, a bed lies back over the floor, a
// cabinet drawn above its hex hangs upright. Corners are top-left,
// bottom-left, bottom-right, top-right.
bool standingSpriteCard(const SpriteCardProjection& projection, const SpriteCardArt& art,
    SpriteCardVertex corners[4], double* anchorX, double* anchorZ);

// A card lying on the floor at `height`: every pixel lands on the ground point
// the game's camera sees under it (rugs, blood, corpses). Same corner order.
bool flatSpriteCard(const SpriteCardProjection& projection, const SpriteCardArt& art, double height,
    SpriteCardVertex corners[4]);

// Depth plane of a card on `tile` (row_plane in the donor): the GL z of an
// upright plane through its hex row. The game paints hex rows back to front
// and each row from right to left; `stack` = sprites the game paints on the
// same hex before this one. Cards take the depth of this plane, so they hide
// each other and walls as the game's painting does.
double spriteRowPlane(const SpriteCardProjection& projection, int tile, int stack);

// The yaw on the scene ground (atan2(z, x), as a fly camera's) of the way a
// sprite in `rotation` faces: 0..5 are NE, E, SE, SW, W, NW on the game's
// screen. The ground's hexes are a little squashed: neighbouring facings lie
// 53 or 63 degrees apart, all turning one way.
bool spriteRotationYaw(const SpriteCardProjection& projection, int rotation, double* yaw);

// The sides of a turning sprite (087): a camera whose yaw is `turn` radians
// round from the game's view sees the side `steps` rotations on from the
// object's own. The art's sides are 60 degrees apart; `rotationSign` is the
// way the yaw turns from one rotation to the next (spriteRotationYaw).
// Near the half-way between two sides the count keeps `previous`, so a sprite
// does not flicker while the camera hovers there.
int spriteTurnSteps(double turn, int rotationSign, int previous);

// The rotation drawn for an object in `rotation` when the sides move `steps`.
int spriteShownRotation(int rotation, int steps);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SPRITE_CARD_H */
