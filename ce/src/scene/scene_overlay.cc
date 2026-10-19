#include "scene_overlay.h"

#include <cmath>
#include <vector>

#include "scene_view.h"
#include "../color.h"
#include "../game_mouse.h"
#include "../scene_renderer/context.h"
#include "../scene_renderer/game_view.h"
#include "../scene_renderer/picking.h"
#include "../tile.h"
#include "../window_manager.h"

namespace fallout {
namespace scene {

namespace {

// Where the scene shows a hex, relative to where the tile window would draw it.
bool sceneTileShift(int tile, int* dx, int* dy)
{
    double pixel[2];
    double sceneX, sceneY;
    scene_renderer::gameViewTilePixel(tile, pixel);
    if (!scene_renderer::staticScreenFromGamePixel(pixel[0], pixel[1], &sceneX, &sceneY)) return false;
    int screenX, screenY;
    if (tileToScreenXY(tile, &screenX, &screenY) != 0) return false;
    *dx = static_cast<int>(std::lround(sceneX - (screenX + 16)));
    *dy = static_cast<int>(std::lround(sceneY - (screenY + 8)));
    return true;
}

// The hex the scene shows at a screen point.
int sceneTileAt(int x, int y)
{
    scene_renderer::GroundPickResult picked = {};
    return scene_renderer::pickStaticGroundTile(x, y, &picked) && hexGridTileIsValid(picked.tile) ? picked.tile : -1;
}

} // namespace

bool syncOverlay()
{
    const bool on = scene_renderer::isActive() && activeView() == View::Full && shouldPresent();
    const bool was = tileSceneOverlayActive();
    if (on != was) tileSetSceneOverlay(on ? sceneTileShift : nullptr, sceneTileAt);
    // While on, every frame: floating text and the hex cursor follow the
    // scene's camera as it glides.
    if (on || was) tileWindowRefresh();
    return on;
}

void maskOverlay(const unsigned char* indices, int indexPitch, unsigned char* rgba, int rgbaPitch,
    int width, int height)
{
    static std::vector<unsigned char> shown;
    shown.resize(static_cast<size_t>(width) * height);
    if (!windowShownMask(gIsoWindow, shown.data(), width, height)) return;
    for (int y = 0; y < height; y++) {
        const unsigned char* index = indices + static_cast<size_t>(y) * indexPitch;
        const unsigned char* world = shown.data() + static_cast<size_t>(y) * width;
        unsigned char* pixel = rgba + static_cast<size_t>(y) * rgbaPitch;
        for (int x = 0; x < width; x++, pixel += 4) {
            if (world[x] != 0 && index[x] == COLOR_FIRST) {
                pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
            } else {
                pixel[3] = 255;
            }
        }
    }
}

} // namespace scene
} // namespace fallout
