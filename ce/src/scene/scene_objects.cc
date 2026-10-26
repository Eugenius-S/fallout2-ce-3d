#include "scene_objects.h"

#include <stdio.h>
#include <string>

#include "../art.h"
#include "../color.h"
#include "../critter.h"
#include "../game_mouse.h"
#include "../map.h"
#include "../object.h"
#include "clean_edges_boundary_mask.h"
#include "../scene_renderer/board_shadows.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/live_objects.h"
#include "../scene_renderer/picking.h"
#include "../scene_renderer/static_scene.h"
#include "scene_settings.h"
#include "../settings.h"
#include "../svga.h"
#include "../tile.h"

namespace fallout {
namespace scene {

namespace {

// A sprite texel's alpha: 0 for the transparent index; the engine's animated
// colours (index 229 and up), which it paints at full light whatever the light
// (_dark_trans_buf_to_buf), are marked with 254 for the scene's shader.
unsigned char spriteAlpha(unsigned char colorIndex)
{
    if (colorIndex == 0) return 0;
    return colorIndex >= 0xE5 ? 254 : 255;
}

// The art and rotation a standing sprite is drawn in (087). A fly or turned
// camera sees another side of a critter, item or projectile whose art has more
// than one: its art in the rotation that side faces. Art with fewer sides
// shows the nearest one it has, as the game would for that rotation. Scenery
// keeps its own rotation: its art spends rotations on variants (wrecks, cacti,
// a shelf breaking up; tools/shapes/art_sides.py), not on sides. So do flat
// sprites, which turn with the ground, and things walls.bin draws.
struct ShownArt {
    int fid;
    int rotation;
};

ShownArt shownArt(Object* object, bool flat)
{
    const ShownArt own = { object->fid, object->rotation };
    const FrmId frmId(object);
    const ObjectType type = frmId.objectType();
    if (!settings.scene3d.sprite_sides || flat || (type != OBJ_TYPE_CRITTER && type != OBJ_TYPE_ITEM && type != OBJ_TYPE_MISC)) return own;
    const int rotation = scene_renderer::staticShownRotation(object->rotation);
    if (rotation == object->rotation) return own;
    // Art stored a file per side (.fr0 .. .fr5) names its side in the fid; a
    // side without its file keeps the game's own art.
    if (frmId.hasRotation() && frmId.rotation() != ROTATION_NE) {
        constexpr int kRotationBits = 0x70000000;
        const FrmId side((object->fid & ~kRotationBits) | ((rotation + 1) << 28));
        return side.exist() ? ShownArt { side.fid(), rotation } : own;
    }
    CacheEntry* handle = nullptr;
    Art* art = artLock(frmId, &handle);
    if (art == nullptr) return own;
    bool sides = false;
    for (int index = 1; index < ROTATION_COUNT; index++) sides = sides || art->dataOffsets[index] != art->dataOffsets[0];
    artUnlock(handle);
    return sides ? ShownArt { object->fid, rotation } : own;
}

std::vector<PickEntry> gPickObjects;
int gHeroPickId = -1;
int gHeroStack = 0; // standing sprites the game paints on the hero's hex before the hero
unsigned int gObjectCardsSkipped = 0;
bool gHeroUnavailableLogged = false;

bool updateHeroFromGame()
{
    auto unavailable = [&](const char* reason) {
        scene_renderer::clearStaticHero();
        if (!gHeroUnavailableLogged) {
            printf("scene: hero card skipped: %s\n", reason);
            fflush(stdout);
            gHeroUnavailableLogged = true;
        }
        return false;
    };
    if (gDude == nullptr) return unavailable("player object is missing");
    if (gDude->elevation != gElevation || !hexGridTileIsValid(gDude->tile)
        || (gDude->flags & OBJECT_HIDDEN) != OBJECT_NONE) {
        return unavailable("player is hidden, off-map, or on another elevation");
    }
    if (gDude->rotation < ROTATION_FIRST || gDude->rotation >= ROTATION_COUNT) {
        return unavailable("player rotation is invalid");
    }

    const ShownArt shown = shownArt(gDude, (gDude->flags & OBJECT_FLAT) != OBJECT_NONE);
    FrmImage image;
    const FrmId frmId(shown.fid);
    if (!image.lock(frmId, gDude->frame, static_cast<Rotation>(shown.rotation))) {
        return unavailable("current player FRM frame could not be locked");
    }
    const int width = image.getWidth();
    const int height = image.getHeight();
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048
        || gDude->x < -32768 || gDude->x > 32768 || gDude->y < -32768 || gDude->y > 32768) {
        return unavailable("current player FRM dimensions or movement are out of bounds");
    }
    const unsigned char* indices = image.getData();
    unsigned char* palette = directDrawGetPalette();
    if (indices == nullptr || palette == nullptr) return unavailable("player palette or pixels are unavailable");

    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    std::vector<unsigned char> rgba(pixelCount * 4);
    for (size_t index = 0; index < pixelCount; index++) {
        const unsigned char colorIndex = indices[index];
        rgba[index * 4] = static_cast<unsigned char>(palette[colorIndex * 3] << 2);
        rgba[index * 4 + 1] = static_cast<unsigned char>(palette[colorIndex * 3 + 1] << 2);
        rgba[index * 4 + 2] = static_cast<unsigned char>(palette[colorIndex * 3 + 2] << 2);
        rgba[index * 4 + 3] = spriteAlpha(colorIndex);
    }

    scene_renderer::StaticHeroFrame frame = {
        gDude->tile, static_cast<int>(gDude->elevation), gDude->fid, gDude->frame,
        shown.rotation, gDude->x, gDude->y,
        image.getXOffset(), image.getYOffset(), width, height, rgba.data(), rgba.size(),
    };
    frame.stack = gHeroStack;
    frame.pickId = gHeroPickId;
    std::string error;
    if (!scene_renderer::updateStaticHero(frame, &error)) return unavailable(error.c_str());
    gHeroUnavailableLogged = false;
    return true;
}

struct ObjectCardPass {
    unsigned char* palette;
    std::vector<unsigned char> rgba;
    unsigned int skipped;
    int lastTile; // tile of the previous standing sprite, for the per-hex count
    int onTile; // standing sprites painted on lastTile so far
};

// How many standing sprites the game paints on this object's hex before it
// (the walk is in the game's own painting order).
int nextStackOnTile(ObjectCardPass& pass, Object* object)
{
    if ((object->flags & OBJECT_FLAT) != OBJECT_NONE) return 0;
    if (object->tile != pass.lastTile) {
        pass.lastTile = object->tile;
        pass.onTile = 0;
    }
    return pass.onTile++;
}

void addObjectCard(Object* object, void* context)
{
    ObjectCardPass& pass = *static_cast<ObjectCardPass*>(context);
    // The mouse's cursor objects are drawn over the scene with the interface.
    if ((object->flags & OBJECT_HIDDEN) != OBJECT_NONE || !hexGridTileIsValid(object->tile)
        || object == gGameMouseHexCursor || object == gGameMouseBouncingCursor) {
        return;
    }
    const FrmId objectFid(object);
    if (scene::isCleanEdgesBoundaryMask(scene::cleanEdges(), scene_renderer::staticMapClip().enabled,
            objectFid.objectType() == OBJ_TYPE_SCENERY, objectFid.filePath())) {
        return;
    }
    const int pickId = static_cast<int>(gPickObjects.size());
    gPickObjects.push_back({ object, object->tile });
    // walls.bin draws it: its card is only picked, and the game's count of
    // sprites on its hex does not include it.
    const bool pickOnly = scene_renderer::staticObjectBaked(object->tile, object->fid);
    // The game shows the hero through walls and scenery he stands behind.
    const bool seeThrough = objectSeeThroughAroundDude(object);
    if (pickOnly && seeThrough) scene_renderer::markStaticWallSeeThrough(object->tile, object->fid);
    if (pickOnly && (object->flags & OBJECT_FLAG_0xFC000) == OBJECT_TRANS_WALL) {
        scene_renderer::markStaticWallFullLight(object->tile, object->fid);
    }
    const int stack = pickOnly ? (object->tile == pass.lastTile ? pass.onTile : 0) : nextStackOnTile(pass, object);
    if (object == gDude) {
        gHeroStack = stack; // the hero card is drawn on its own
        gHeroPickId = pickId;
        return;
    }
    if (object->rotation < ROTATION_FIRST || object->rotation >= ROTATION_COUNT) {
        pass.skipped++;
        return;
    }
    // Under a free camera a door standing wide open is not drawn: the game's
    // picture of the open leaf, laid on the wall's plane, reads as a crooked
    // panel. A door caught between frames keeps its picture on the plane (072).
    float doorPlane[3];
    if (object->frame > 0 && scene_renderer::staticFlyCameraActive()
        && scene_renderer::staticObjectPlane(object->tile, object->fid, doorPlane)) {
        CacheEntry* handle = nullptr;
        Art* art = artLock(objectFid, &handle);
        const int frames = art != nullptr ? artGetFrameCount(art) : 0;
        if (handle != nullptr) artUnlock(handle);
        if (frames > 1 && object->frame == frames - 1) return;
    }
    const bool flat = (object->flags & OBJECT_FLAT) != OBJECT_NONE
        || scene_renderer::staticObjectFlat(object->tile, object->fid);
    const bool flyFlat = scene_renderer::staticFlyCameraActive()
        && scene_renderer::staticObjectFlyFlat(object->tile, object->fid);
    const ShownArt shown = shownArt(object, flat || flyFlat || pickOnly);
    FrmImage image;
    if (!image.lock(FrmId(shown.fid), object->frame, static_cast<Rotation>(shown.rotation))) {
        pass.skipped++;
        return;
    }
    int width = image.getWidth();
    int height = image.getHeight();
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048) {
        pass.skipped++;
        return;
    }
    // A board whose art carries a painted shadow draws its shadow-free sprite
    // while the scene's sun shadows are on (046); only in the frame and
    // rotation the sprite was built from, so an animated board keeps its art.
    const unsigned char* body = nullptr;
    int bodyX = 0, bodyY = 0;
    if (settings.scene3d.shadows) {
        int bodyWidth = 0, bodyHeight = 0;
        body = scene_renderer::staticObjectShadowFreeArt(object->tile, static_cast<unsigned int>(object->fid),
            object->frame, object->rotation, &bodyWidth, &bodyHeight, &bodyX, &bodyY);
        if (body != nullptr) {
            width = bodyWidth;
            height = bodyHeight;
        }
    }
    const int variant = body != nullptr ? 1 : 0;
    const bool cached = scene_renderer::staticObjectArtCached(object->fid, object->frame, shown.rotation, variant);
    if (!cached && body == nullptr) {
        const unsigned char* indices = image.getData();
        if (indices == nullptr) {
            pass.skipped++;
            return;
        }
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        pass.rgba.resize(pixelCount * 4);
        for (size_t index = 0; index < pixelCount; index++) {
            const unsigned char colorIndex = indices[index];
            pass.rgba[index * 4] = static_cast<unsigned char>(pass.palette[colorIndex * 3] << 2);
            pass.rgba[index * 4 + 1] = static_cast<unsigned char>(pass.palette[colorIndex * 3 + 1] << 2);
            pass.rgba[index * 4 + 2] = static_cast<unsigned char>(pass.palette[colorIndex * 3 + 2] << 2);
            pass.rgba[index * 4 + 3] = spriteAlpha(colorIndex);
        }
    }
    scene_renderer::StaticObjectFrame frame = {
        object->tile, object->fid, object->frame, shown.rotation, object->x, object->y,
        body != nullptr ? bodyX : image.getXOffset(), body != nullptr ? bodyY : image.getYOffset(), width, height,
        flat,
        cached ? nullptr : (body != nullptr ? body : pass.rgba.data()),
        cached ? 0 : (body != nullptr ? static_cast<size_t>(width) * static_cast<size_t>(height) * 4
                                      : pass.rgba.size()),
    };
    frame.variant = variant;
    frame.stack = stack;
    frame.pickId = pickId;
    frame.pickOnly = pickOnly;
    frame.seeThrough = seeThrough;
    // The engine's translucent kinds and their tints (_obj_render_object).
    Color tint = COLOR_WHITE;
    bool translucent = true;
    switch (object->flags & OBJECT_FLAG_0xFC000) {
    case OBJECT_TRANS_RED:
        tint = COLOR_RED;
        break;
    case OBJECT_TRANS_WALL:
        tint = COLOR_PALE_BLUE;
        frame.fullLight = true;
        break;
    case OBJECT_TRANS_GLASS:
        tint = COLOR_CYAN;
        frame.glassGray = true;
        break;
    case OBJECT_TRANS_STEAM:
        tint = COLOR_WHITE;
        break;
    case OBJECT_TRANS_ENERGY:
        tint = COLOR_YELLOW_GREEN;
        break;
    default:
        translucent = false;
        break;
    }
    if (translucent) {
        const int rgb = Color2RGB(tint);
        frame.tint[0] = ((rgb >> 10) & 0x1F) / 31.0f;
        frame.tint[1] = ((rgb >> 5) & 0x1F) / 31.0f;
        frame.tint[2] = (rgb & 0x1F) / 31.0f;
    }
    std::string error;
    if (!scene_renderer::addStaticObject(frame, &error)) pass.skipped++;
}

struct PickFilter {
    int objectType;
};

bool pickWanted(int pickId, void* context)
{
    const PickFilter& filter = *static_cast<const PickFilter*>(context);
    if (pickId < 0 || pickId >= static_cast<int>(gPickObjects.size())) return false;
    const PickEntry& entry = gPickObjects[pickId];
    if (!objectIsOnTile(entry.object, entry.tile)) return false;
    return filter.objectType == OBJ_TYPE_INVALID || FrmId(entry.object).objectType() == filter.objectType;
}

} // namespace

bool updateObjectsFromGame()
{
    scene_renderer::beginStaticObjects();
    scene_renderer::beginStaticWallSeeThrough();
    gPickObjects.clear();
    gHeroPickId = -1;
    ObjectCardPass pass = { directDrawGetPalette(), {}, 0, -1, 0 };
    if (pass.palette != nullptr) objectForEachAtElevation(gElevation, addObjectCard, &pass);
    scene_renderer::endStaticObjects();
    scene_renderer::endStaticWallSeeThrough();
    gObjectCardsSkipped = pass.skipped;
    return updateHeroFromGame();
}

unsigned int objectCardsSkipped()
{
    return gObjectCardsSkipped;
}

const std::vector<PickEntry>& pickEntries()
{
    return gPickObjects;
}

void resetHeroLog()
{
    gHeroUnavailableLogged = false;
}

bool pickObject(int mouseX, int mouseY, int objectType, bool includeDude, Object** found)
{
    if (!tileSceneOverlayActive()) return false;
    // gameMouseGetObjectUnderCursor on the scene's picture: from the nearest
    // down, the first opaque one that is not a dead or unconscious critter,
    // else the last one under the pointer.
    PickFilter filter = { objectType };
    int picked[8];
    const int count = scene_renderer::pickStaticObjects(mouseX, mouseY, pickWanted, &filter, picked, 8);
    *found = nullptr;
    for (int index = 0; index < count; index++) {
        Object* object = gPickObjects[picked[index]].object;
        if (!includeDude && object == gDude) continue;
        *found = object;
        const bool translucent = (object->flags & OBJECT_FLAG_0xFC000) != OBJECT_NONE
            && (object->flags & OBJECT_TRANS_NONE) == OBJECT_NONE;
        const bool down = FrmId(object).objectType() == OBJ_TYPE_CRITTER
            && (object->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD)) != 0;
        if (!translucent && !down) break;
    }
    return true;
}

} // namespace scene
} // namespace fallout
