#ifndef FALLOUT_SCENE_RENDERER_SPRITE_CARD_H
#define FALLOUT_SCENE_RENDERER_SPRITE_CARD_H

namespace fallout {
namespace scene_renderer {

// The donor projection between game screen pixels and the scene ground.
struct SpriteCardProjection {
    double squ[2];
    double sqv[2];
    double origin[2];
    double hpx; // screen pixels per unit of height
    double bbPx; // screen pixels per unit along the card
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

// An upright card facing the camera, standing on the object's anchor. Corners
// are top-left, bottom-left, bottom-right, top-right; `screenRightX/Z` is the
// camera's horizontal screen direction on the ground.
bool standingSpriteCard(const SpriteCardProjection& projection, const SpriteCardArt& art,
    double screenRightX, double screenRightZ, SpriteCardVertex corners[4], double* anchorX, double* anchorZ);

// A card lying on the floor at `height`: every pixel lands on the ground point
// the game's camera sees under it (rugs, blood, corpses). Same corner order.
bool flatSpriteCard(const SpriteCardProjection& projection, const SpriteCardArt& art, double height,
    SpriteCardVertex corners[4]);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SPRITE_CARD_H */
