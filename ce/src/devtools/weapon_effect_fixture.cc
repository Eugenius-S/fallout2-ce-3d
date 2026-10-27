#include "weapon_effect_fixture.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>

#include "../art.h"
#include "../object.h"
#include "../proto.h"
#include "../light.h"
#include "../scene/scene_lights.h"
#include "../settings.h"
#include "../tile.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/sprite_card.h"

namespace fallout {
namespace devtools {
namespace {

struct Effect {
    const char* name;
    int miscIndex;
    ObjectType type = OBJ_TYPE_MISC;
    bool existing = false;
};

constexpr Effect kEffects[] = {
    { "laser", 5 },
    { "plasma-bolt", 6 },
    { "plasma-pistol", 7 },
    { "plasma-rifle", 8 },
    { "fire-explosion", 29 },
    { "plasma-explosion", 31 },
    { "emp-explosion", 2 },
    { "rocket-explosion", 10 },
    { "electric", 14 },
    { "flamethrower", 26 },
    { "rocket", 9 },
    { "sign40", 1750, OBJ_TYPE_SCENERY, true },
    { "sign41", 1751, OBJ_TYPE_SCENERY, true },
    // The toxic slime's cycling pixels (089): spawned at the view tile; the
    // green goop reads black when the palette's cycle entries are zeros.
    { "toxic-goo", 996, OBJ_TYPE_SCENERY, false },
};
const Effect* gEffect = nullptr;
Object* gEffectObject = nullptr;
int gFrame = -1;
int gFrames = 0;
bool gFocused = false;
bool gLightCheck = false;
bool gLightCheckPassed = false;
scene_renderer::StaticFlyCamera gCamera;

} // namespace

void requestWeaponEffectLightCheck() { gLightCheck = true; }

bool configureWeaponEffectFixture(const char* name)
{
    for (const auto& effect : kEffects) {
        if (std::strcmp(name, effect.name) == 0) {
            gEffect = &effect;
            return true;
        }
    }
    std::fprintf(stderr, "devtools: unknown weapon effect: %s\n", name);
    return false;
}

void setWeaponEffectFrame(int frame)
{
    gFrame = frame;
}

bool startWeaponEffectFixture(int tile, int elevation)
{
    if (gEffect == nullptr || gEffectObject != nullptr) return true;
    const FrmId fid((static_cast<int>(gEffect->type) << 24) | gEffect->miscIndex);
    CacheEntry* handle = nullptr;
    Art* art = artLock(fid, &handle);
    gFrames = art != nullptr ? artGetFrameCount(art) : 0;
    if (handle != nullptr) artUnlock(handle);
    if (gFrame < 0) gFrame = gFrames / 2;
    if (gFrames <= 0 || gFrame >= gFrames) {
        std::fprintf(stderr, "devtools: effect art/frame unavailable: %s frame %d/%d\n",
            gEffect->name, gFrame, gFrames);
        return false;
    }
    if (gEffect->existing) {
        for (Object* object = objectFindFirstAtElevation(static_cast<MapElevation>(elevation)); object != nullptr;
             object = objectFindNextAtElevation()) {
            if (object->tile == tile && object->fid == fid.fid()) {
                gEffectObject = object;
                gFrame = object->frame;
                return true;
            }
        }
        std::fprintf(stderr, "devtools: mapped sign not found: %s tile %d\n", gEffect->name, tile);
        return false;
    }
    if (objectCreateWithFrmIdProtoId(&gEffectObject, fid, ProtoId::Empty()) == -1) return false;
    gEffectObject->flags |= OBJECT_NO_SAVE;
    const Rotation rotation = static_cast<Rotation>((gEffect->miscIndex >= 5 && gEffect->miscIndex <= 9)
        || gEffect->miscIndex == 14 || gEffect->miscIndex == 26 ? 1 : 0);
    if (objectSetLocation(gEffectObject, tile, static_cast<MapElevation>(elevation), nullptr) == -1
        || objectSetRotation(gEffectObject, rotation, nullptr) == -1
        || objectSetFrame(gEffectObject, gFrame, nullptr) == -1) {
        std::fprintf(stderr, "devtools: effect placement failed: %s\n", gEffect->name);
        return false;
    }
    std::printf("devtools: weapon effect %s fid=%d frame=%d/%d tile=%d (no-save)\n",
        gEffect->name, gEffectObject->fid, gFrame, gFrames, tile);
    std::fflush(stdout);
    if (gLightCheck) {
        // Exercise the real submission path without combat or live-game edits.
        auto check = [&]() { scene::updateLightsFromGame();
            return scene::effectLightMatches(gEffectObject, gEffectObject->tile, gEffectObject->x, gEffectObject->y); };
        auto hexHash = [&]() { unsigned int hash = 2166136261u;
            for (int t = 0; t < HEX_GRID_SIZE; ++t) hash = (hash ^ lightGetTrueTileIntensity(gEffectObject->elevation, t)) * 16777619u;
            return hash; };
        const unsigned int before = hexHash();
        const int originalTile = gEffectObject->tile;
        const auto originalFlags = gEffectObject->flags;
        const int originalIntensity = gEffectObject->lightIntensity;
        const int originalDistance = gEffectObject->lightDistance;
        bool ok = check();
        objectSetLocation(gEffectObject, originalTile + 1, static_cast<MapElevation>(elevation), nullptr);
        gEffectObject->x += 12;
        ok = check() && ok;
        gEffectObject->flags |= OBJECT_HIDDEN;
        ok = !check() && ok;
        gEffectObject->flags = originalFlags;
        ok = check() && ok;
        const bool lights = settings.scene3d.lights;
        settings.scene3d.lights = false;
        ok = !check() && ok;
        settings.scene3d.lights = lights;
        objectSetLocation(gEffectObject, originalTile, static_cast<MapElevation>(elevation), nullptr);
        ok = check() && ok;
        gLightCheckPassed = ok && hexHash() == before && gEffectObject->flags == originalFlags
            && gEffectObject->lightIntensity == originalIntensity && gEffectObject->lightDistance == originalDistance;
        std::printf("EFFECT_LIGHT_CHECK %s move/offset/hide/restore/lights-off/native-map\n", gLightCheckPassed ? "PASS" : "FAIL");
        std::fflush(stdout);
        if (!gLightCheckPassed) return false;
    }
    return true;
}

bool focusWeaponEffectFixture()
{
    if (gEffectObject == nullptr || gFocused) return false;
    if (gEffect->existing) {
        scene_renderer::staticGameViewCamera(&gCamera);
        return false; // retain the native game projection for mapped sign sprites
    }
    scene_renderer::SpriteCardProjection projection;
    FrmImage image;
    if (!scene_renderer::currentGameProjection(&projection)
        || !scene_renderer::staticGameViewCamera(&gCamera)
        || !image.lock(FrmId(gEffectObject), gEffectObject->frame, gEffectObject->rotation)) return false;
    const scene_renderer::SpriteCardArt art = { gEffectObject->tile, gEffectObject->x, gEffectObject->y,
        image.getXOffset(), image.getYOffset(), image.getWidth(), image.getHeight() };
    scene_renderer::SpriteCardVertex corners[4];
    if (!scene_renderer::standingSpriteCard(projection, art, corners, nullptr, nullptr)) return false;
    float target[3] = {};
    for (const auto& corner : corners) {
        target[0] += static_cast<float>(corner.x * 0.25);
        target[1] += static_cast<float>(corner.y * 0.25);
        target[2] += static_cast<float>(corner.z * 0.25);
    }
    float forward[3], right[3];
    scene_renderer::staticFlyCameraAxes(gCamera, forward, right);
    const float distance = static_cast<float>(std::max(4.0,
        std::max(image.getWidth() / projection.bbPx, image.getHeight() / projection.hpx) * 8.0));
    for (int axis = 0; axis < 3; ++axis) gCamera.eye[axis] = target[axis] - forward[axis] * distance;
    gCamera.fovY = 25.0f;
    scene_renderer::setStaticFlyCamera(&gCamera);
    gFocused = true;
    return true; // redraw from the centered camera before capturing
}

std::string weaponEffectMetadata()
{
    if (gEffectObject == nullptr) return "null";
    Rect rect;
    objectGetRect(gEffectObject, &rect);
    FrmImage image;
    if (!image.lock(FrmId(gEffectObject), gEffectObject->frame, gEffectObject->rotation)) return "null";
    char json[1024];
    std::snprintf(json, sizeof(json),
        "{\"name\":\"%s\",\"fid\":%d,\"frame\":%d,\"frames\":%d,\"tile\":%d,\"rotation\":%d,\"no_save\":%s,\"existing_object\":%s,\"light_check\":%s,\"screen_rect\":[%d,%d,%d,%d],\"art_offset\":[%d,%d],\"size\":[%d,%d],\"focused\":%s,\"camera_eye\":[%.6f,%.6f,%.6f],\"camera_yaw\":%.6f,\"camera_pitch\":%.6f,\"camera_fov\":%.1f}",
        gEffect->name, gEffectObject->fid, gEffectObject->frame, gFrames,
        gEffectObject->tile, static_cast<int>(gEffectObject->rotation),
        (gEffectObject->flags & OBJECT_NO_SAVE) != OBJECT_NONE ? "true" : "false", gEffect->existing ? "true" : "false",
        gLightCheck ? (gLightCheckPassed ? "true" : "false") : "null",
        rect.left, rect.top, rect.right, rect.bottom, image.getXOffset(), image.getYOffset(), image.getWidth(), image.getHeight(),
        gFocused ? "true" : "false", gCamera.eye[0], gCamera.eye[1], gCamera.eye[2], gCamera.yaw, gCamera.pitch, gCamera.fovY);
    return json;
}

} // namespace devtools
} // namespace fallout
