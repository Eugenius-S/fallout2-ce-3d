#include "scene_overlay.h"

#include <cmath>
#include <vector>

#include "scene_view.h"
#include "../color.h"
#include "../game_mouse.h"
#include "../map.h"
#include "../object.h"
#include "../scene_renderer/context.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/game_view.h"
#include "../scene_renderer/picking.h"
#include "../scene_renderer/sprite_card.h"
#include "scene_objects.h"
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

// Combat outlines (friend, foe, line of fire) round each critter, at the hex
// the scene shows it on, as the 2D picture draws them after the roofs.
struct OutlineProjection {
    scene_renderer::SpriteCardVertex corners[4];
};

bool projectOutlinePixel(double u, double v, int* x, int* y, void* context)
{
    const auto* projection = static_cast<const OutlineProjection*>(context);
    const auto& tl = projection->corners[0];
    const auto& bl = projection->corners[1];
    const auto& br = projection->corners[2];
    const auto& tr = projection->corners[3];
    const double weights[4] = { (1.0 - u) * (1.0 - v), (1.0 - u) * v, u * v, u * (1.0 - v) };
    const scene_renderer::SpriteCardVertex* corners[4] = { &tl, &bl, &br, &tr };
    double worldX = 0.0;
    double worldY = 0.0;
    double worldZ = 0.0;
    for (int index = 0; index < 4; index++) {
        worldX += weights[index] * corners[index]->x;
        worldY += weights[index] * corners[index]->y;
        worldZ += weights[index] * corners[index]->z;
    }
    double screenX = 0.0;
    double screenY = 0.0;
    if (!scene_renderer::staticScreenFromWorld(worldX, worldY, worldZ, &screenX, &screenY)) return false;
    *x = static_cast<int>(std::lround(screenX));
    *y = static_cast<int>(std::lround(screenY));
    return true;
}

bool shownOutlineProjection(Object* object, OutlineProjection* context, int* fid, int* rotation)
{
    if (object == nullptr || context == nullptr || fid == nullptr || rotation == nullptr) return false;
    const SceneObjectArt shown = shownObjectArt(object);
    CacheEntry* handle = nullptr;
    Art* art = artLock(FrmId(shown.fid), &handle);
    if (art == nullptr) return false;
    int width = 0;
    int height = 0;
    artGetSize(art, object->frame, static_cast<Rotation>(shown.rotation), &width, &height);
    const int artX = art->xOffsets[shown.rotation];
    const int artY = art->yOffsets[shown.rotation];
    artUnlock(handle);
    scene_renderer::SpriteCardProjection projection;
    if (width <= 0 || height <= 0 || !scene_renderer::currentGameProjection(&projection)) return false;
    const scene_renderer::SpriteCardArt card = {
        object->tile, object->x, object->y, artX, artY, width, height,
    };
    if (!scene_renderer::standingSpriteCard(projection, card, context->corners, nullptr, nullptr)) return false;
    scene_renderer::staticFaceSpriteCardToCamera(context->corners);
    *fid = shown.fid;
    *rotation = shown.rotation;
    return true;
}

void drawOutline(Object* object, void* context)
{
    if (scene_renderer::sceneCameraTransformed()) {
        OutlineProjection projection = {};
        int fid = object->fid;
        int rotation = object->rotation;
        if (shownOutlineProjection(object, &projection, &fid, &rotation)) {
            objectDrawSceneOutline(object, fid, rotation, projectOutlinePixel, &projection);
            return;
        }
    }
    int dx = 0;
    int dy = 0;
    if (!sceneTileShift(object->tile, &dx, &dy)) return;
    object->x += dx;
    object->y += dy;
    objectDrawOutline(object, static_cast<Rect*>(context));
    object->x -= dx;
    object->y -= dy;
}

void drawOutlines(Rect* rect)
{
    objectVisitOutlined(gElevation, drawOutline, rect);
}

} // namespace

bool floatingTextScreenPosition(int tile, int offsetX, int offsetY, int* screenX, int* screenY)
{
    if (!scene_renderer::sceneCameraTransformed() || screenX == nullptr || screenY == nullptr) return false;
    scene_renderer::SpriteCardProjection projection;
    if (!scene_renderer::currentGameProjection(&projection)) return false;
    double pixel[2];
    scene_renderer::gameViewTilePixel(tile, pixel);
    double worldX = 0.0;
    double worldZ = 0.0;
    if (!scene_renderer::spritePixelToGround(projection, pixel[0] + offsetX - 16, pixel[1], &worldX, &worldZ)) return false;
    const double worldY = (8.0 - offsetY) / projection.hpx;
    double x = 0.0;
    double y = 0.0;
    if (!scene_renderer::staticScreenFromWorld(worldX, worldY, worldZ, &x, &y)) return false;
    *screenX = static_cast<int>(std::lround(x));
    *screenY = static_cast<int>(std::lround(y));
    return true;
}

bool syncOverlay()
{
    const bool on = scene_renderer::isActive() && activeView() == View::Full && shouldPresent();
    const bool was = tileSceneOverlayActive();
    if (on != was) {
        tileSetSceneOverlay(on ? sceneTileShift : nullptr, sceneTileAt);
        objectSetSceneOutlines(on ? drawOutlines : nullptr);
    }
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
