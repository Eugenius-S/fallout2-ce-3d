#include "weapon_effect_fixture.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>

#include "../art.h"
#include "../object.h"
#include "../proto.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/sprite_card.h"

namespace fallout {
namespace devtools {
namespace {

struct Effect {
    const char* name;
    int miscIndex;
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
};
const Effect* gEffect = nullptr;
Object* gEffectObject = nullptr;
int gFrame = -1;
int gFrames = 0;
bool gFocused = false;
scene_renderer::StaticFlyCamera gCamera;

} // namespace

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
    const FrmId fid(static_cast<MiscFrameId>(gEffect->miscIndex));
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
    if (objectCreateWithFrmIdProtoId(&gEffectObject, fid, ProtoId::Empty()) == -1) return false;
    gEffectObject->flags |= OBJECT_NO_SAVE;
    const Rotation rotation = static_cast<Rotation>(gEffect->miscIndex >= 5 && gEffect->miscIndex <= 8 ? 1 : 0);
    if (objectSetLocation(gEffectObject, tile, static_cast<MapElevation>(elevation), nullptr) == -1
        || objectSetRotation(gEffectObject, rotation, nullptr) == -1
        || objectSetFrame(gEffectObject, gFrame, nullptr) == -1) {
        std::fprintf(stderr, "devtools: effect placement failed: %s\n", gEffect->name);
        return false;
    }
    std::printf("devtools: weapon effect %s fid=%d frame=%d/%d tile=%d (no-save)\n",
        gEffect->name, gEffectObject->fid, gFrame, gFrames, tile);
    std::fflush(stdout);
    return true;
}

bool focusWeaponEffectFixture()
{
    if (gEffectObject == nullptr || gFocused) return false;
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
    char json[512];
    std::snprintf(json, sizeof(json),
        "{\"name\":\"%s\",\"fid\":%d,\"frame\":%d,\"frames\":%d,\"tile\":%d,\"rotation\":%d,\"no_save\":true,\"screen_rect\":[%d,%d,%d,%d],\"art_offset\":[%d,%d],\"size\":[%d,%d],\"focused\":%s,\"camera_eye\":[%.6f,%.6f,%.6f],\"camera_yaw\":%.6f,\"camera_pitch\":%.6f,\"camera_fov\":%.1f}",
        gEffect->name, gEffectObject->fid, gEffectObject->frame, gFrames,
        gEffectObject->tile, static_cast<int>(gEffectObject->rotation),
        rect.left, rect.top, rect.right, rect.bottom, image.getXOffset(), image.getYOffset(), image.getWidth(), image.getHeight(),
        gFocused ? "true" : "false", gCamera.eye[0], gCamera.eye[1], gCamera.eye[2], gCamera.yaw, gCamera.pitch, gCamera.fovY);
    return json;
}

} // namespace devtools
} // namespace fallout
