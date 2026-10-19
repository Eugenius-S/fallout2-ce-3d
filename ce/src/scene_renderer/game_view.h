#ifndef FALLOUT_SCENE_RENDERER_GAME_VIEW_H
#define FALLOUT_SCENE_RENDERER_GAME_VIEW_H

namespace fallout {
namespace scene_renderer {

// The game's own projection, as the donor viewer's game view uses it, with a
// mild perspective added. The game draws the oblique parallel projection
//     px(u, v, z) = origin + u * squ + v * sqv - (0, hpx * z)        (y down)
// which no ordinary camera reproduces: it is sheared. This view keeps it on
// the plane through the view center that faces the game's camera: there every
// point is on the screen pixel the 2D game shows it on, at the same scale, so
// sprite cards meet the floor and each other as in the game. Nearer and
// further away things grow and shrink, as seen from an eye `distance` units
// up the game's view ray. GL world is (u - 50, z, v - 50).
struct GameView {
    double squ[2];
    double sqv[2];
    double origin[2];
    double hpx;
    double centerPixel[2]; // game pixel shown at the screen center: the engine's
                           // gCenterTile hex center, or a point gliding towards it
    int logicalWidth; // the game's screen, in game pixels
    int logicalHeight;
    double distance; // units from the view center to the eye
    // Where on the screen the engine draws the center hex (logical pixels):
    // the middle of its map window above the interface, moved by the map's
    // edge alignment. Not set: the middle of the whole screen.
    bool screenCenterSet = false;
    double screenCenter[2] = {};
};

// Game pixel of a hex center (what the engine centers the screen on).
void gameViewTilePixel(int tile, double pixel[2]);

// One step of the camera gliding to the center the engine chose (`target`,
// a game pixel) `seconds` after the last: the way left shrinks by e every
// 80 ms whatever the frame rate. The first step, and a target further than a
// map's reach (a new map, a load), are taken at once.
void gameViewGlide(double pixel[2], const double target[2], double seconds, bool first);

// Column-major OpenGL clip matrix for GL world points.
bool gameViewMatrix(const GameView& view, float matrix[16]);

// The eye: every point on a line through it lands on one screen pixel.
bool gameViewEye(const GameView& view, double eye[3]);

// Ground point (GL x, z) at the center of the view.
bool gameViewCenter(const GameView& view, double* worldX, double* worldZ);

// Screen pixel (game logical pixels) of a GL world point.
bool gameViewProject(const GameView& view, double x, double y, double z, double* logicalX, double* logicalY);

// Screen pixel (game logical pixels) of the floor point the game draws at a
// game pixel.
bool gameViewProjectGroundPixel(const GameView& view, double pixelX, double pixelY, double* logicalX, double* logicalY);

// Ground point (GL x, z) seen at a screen pixel.
bool gameViewPickGround(const GameView& view, double logicalX, double logicalY, double* worldX, double* worldZ);

} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_GAME_VIEW_H */
