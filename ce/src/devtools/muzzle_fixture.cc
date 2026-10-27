#include "muzzle_fixture.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../art.h"
#include "../item.h"
#include "../inventory.h"
#include "../object.h"
#include "../proto.h"
#include "../scene/muzzle_flash.h"
#include "../scene/scene_lights.h"
#include "../scene/scene_objects.h"
#include "../scene_renderer/camera.h"
#include "../scene_renderer/sprite_card.h"

namespace fallout { namespace devtools {
namespace {
int gWeapon = 0, gAnimation = 45, gRotation = 1, gFrames = 0, gShot = 0, gFid = -1;
int gAimX = 0, gAimY = 0;
int gModes = 2;
bool gUsesDefaultExtra = false;
Object* gProjectile = nullptr;
bool gFocused = false, gDone = false;
std::vector<std::pair<int,int>> gOffsets;

void showFrame()
{
    const int frame = gShot % gFrames;
    objectSetFrame(gDude, frame, nullptr);
    gDude->x = gAimX + gOffsets[frame].first;
    gDude->y = gAimY + gOffsets[frame].second;
    const int mode=gShot/gFrames;
    scene::setMuzzleFlashPreview(mode!=1);
    if (mode==2 && gUsesDefaultExtra) scene::useDefaultEnergyMuzzle();
    else scene::setEnergyMuzzlePreview(mode==2);
    if (gProjectile != nullptr) {
        // Material comparison only: park native projectile art at the painted
        // barrel. This is not a combat trajectory or a new animation schedule.
        const auto* flash=scene::muzzleFlashFor(gFid,frame,gRotation,false);
        FrmImage actor,bolt;
        const bool visible=flash!=nullptr && actor.lock(FrmId(gFid),frame,static_cast<Rotation>(gRotation))
            && bolt.lock(FrmId(gProjectile),0,static_cast<Rotation>(gRotation));
        if (visible) {
            gProjectile->x=gDude->x; gProjectile->y=gDude->y;
            const double right=actor.getXOffset()-actor.getWidth()/2+flash->rootX;
            const double down=actor.getYOffset()-(actor.getHeight()-1)+flash->rootY;
            scene::setObjectArtOffsetForInspection(gProjectile,
                static_cast<int>(std::lround(right-bolt.getXOffset())),
                static_cast<int>(std::lround(down-bolt.getYOffset()+(bolt.getHeight()-1)/2.0)));
            objectShow(gProjectile,nullptr);
        } else objectHide(gProjectile,nullptr);
    }
}
}

void configureMuzzleFixture(int weaponPid, int animation, int rotation)
{
    if (weaponPid >= 0) gWeapon = weaponPid;
    if (animation >= 0) gAnimation = animation;
    if (rotation >= 0) gRotation = rotation;
}

bool startMuzzleFixture(int tile, int elevation)
{
    if (gWeapon == 0) return true;
    if (gDude == nullptr || gRotation < 0 || gRotation > 5 || gAnimation < 45 || gAnimation > 47) return false;
    Object* weapon = nullptr;
    if (objectCreateWithProtoId(&weapon, ProtoId(gWeapon)) == -1 || itemGetType(weapon) != ITEM_TYPE_WEAPON) return false;
    weapon->flags |= OBJECT_NO_SAVE;
    for (int i=0;i<gDude->data.inventory.length;++i) gDude->data.inventory.items[i].item->flags &= ~OBJECT_IN_RIGHT_HAND;
    const int style = static_cast<int>(weaponGetAnimationCode(weapon));
    if (style < 5 || style > 15 || itemAdd(gDude, weapon, 1) != 0) return false;
    weapon->flags |= OBJECT_IN_RIGHT_HAND;
    gModes = weaponGetDamageType(gDude,weapon)==DAMAGE_TYPE_NORMAL ? 2 : 3;
    Proto* weaponProto=nullptr;
    Proto* projectileProto=nullptr;
    if (gModes==3 && protoGetProto(weapon,&weaponProto)==0 && weaponProto!=nullptr) {
        const int damage=weaponProto->item.data.weapon.damageType;
        gUsesDefaultExtra=weaponProto->item.data.weapon.projectilePid>0
            && (damage==DAMAGE_TYPE_LASER || damage==DAMAGE_TYPE_PLASMA);
    }
    if (gModes==3 && protoGetProto(weapon,&weaponProto)==0 && weaponProto!=nullptr
        && weaponProto->item.data.weapon.projectilePid>0
        && protoGetProto(ProtoId(weaponProto->item.data.weapon.projectilePid),&projectileProto)==0
        && projectileProto!=nullptr) {
        if (objectCreateWithFrmIdProtoId(&gProjectile,FrmId(projectileProto),ProtoId::Empty())==-1) return false;
        gProjectile->flags|=OBJECT_NO_SAVE;
        objectSetLocation(gProjectile,tile,static_cast<MapElevation>(elevation),nullptr);
        objectSetRotation(gProjectile,static_cast<Rotation>(gRotation),nullptr);
        objectSetLight(gProjectile,0,0,nullptr);
    }
    gFid = 0x0100000B | (style << 12) | (gAnimation << 16); // native HMJMPS
    CacheEntry* handle = nullptr;
    Art* art = artLock(FrmId(gFid), &handle);
    if (art == nullptr) return false;
    gFrames = artGetFrameCount(art);
    int x = 0, y = 0;
    for (int i = 0; i < gFrames; ++i) {
        int dx = 0, dy = 0;
        artGetFrameOffsets(art, i, static_cast<Rotation>(gRotation), &dx, &dy);
        x += dx; y += dy; gOffsets.emplace_back(x,y);
    }
    artUnlock(handle);
    if (gFrames <= 0 || gFrames > 64) return false;
    const FrmId aimed((gFid & ~0x00FF0000) | (ANIM_POINT << 16));
    art = artLock(aimed, &handle);
    if (art == nullptr) return false;
    for (int i = 0; i < artGetFrameCount(art); ++i) {
        int dx = 0, dy = 0;
        artGetFrameOffsets(art, i, static_cast<Rotation>(gRotation), &dx, &dy);
        gAimX += dx; gAimY += dy;
    }
    artUnlock(handle);
    gDude->flags |= OBJECT_NO_SAVE;
    objectSetLocation(gDude, tile, static_cast<MapElevation>(elevation), nullptr);
    objectSetRotation(gDude, static_cast<Rotation>(gRotation), nullptr);
    objectSetFrmId(gDude, FrmId(gFid), nullptr);
    // Isolate the flash in the night comparison; the stock player's lamp
    // otherwise keeps the whole pose lit even in the no-flash control.
    objectSetLight(gDude,0,0,nullptr);
    showFrame();
    std::printf("MUZZLE_FIXTURE start weapon=%d fid=%d frames=%d rotation=%d\n", gWeapon, gFid, gFrames, gRotation);
    std::fflush(stdout);
    return true;
}

bool focusMuzzleFixture()
{
    if (gWeapon == 0 || gFocused || gFid < 0) return false;
    scene_renderer::SpriteCardProjection projection;
    scene_renderer::StaticFlyCamera camera;
    if (!scene_renderer::currentGameProjection(&projection) || !scene_renderer::staticGameViewCamera(&camera)) return false;
    std::vector<scene_renderer::SpriteCardVertex> vertices;
    double size = 1.0;
    for (int i = 0; i < gFrames; ++i) {
        FrmImage image;
        if (!image.lock(FrmId(gFid), i, static_cast<Rotation>(gRotation))) continue;
        scene_renderer::SpriteCardArt art = {gDude->tile, gAimX + gOffsets[i].first, gAimY + gOffsets[i].second,
            image.getXOffset(), image.getYOffset(), image.getWidth(), image.getHeight()};
        scene_renderer::SpriteCardVertex corners[4];
        if (!scene_renderer::standingSpriteCard(projection,art,corners,nullptr,nullptr)) continue;
        vertices.insert(vertices.end(),corners,corners+4);
        size = std::max(size,std::max(image.getWidth()/projection.bbPx,image.getHeight()/projection.hpx));
    }
    if (vertices.empty()) return false;
    float target[3] = {};
    for (const auto& v : vertices) {
        target[0] += static_cast<float>(v.x/vertices.size()); target[1] += static_cast<float>(v.y/vertices.size()); target[2] += static_cast<float>(v.z/vertices.size());
    }
    float forward[3],right[3]; scene_renderer::staticFlyCameraAxes(camera,forward,right);
    const float distance = static_cast<float>(std::max(8.0,size*8.0));
    for (int k=0;k<3;++k) camera.eye[k]=target[k]-forward[k]*distance;
    camera.fovY=25.0f; scene_renderer::setStaticFlyCamera(&camera); gFocused=true;
    return true;
}

bool advanceMuzzleFixture()
{
    if (gWeapon == 0 || gDone) return false;
    if (++gShot >= gFrames * gModes) {
        gDone=true;
        std::printf("MUZZLE_FIXTURE PASS frames=%d captures=%d\n",gFrames,gFrames*gModes);
        std::fflush(stdout);
        return false;
    }
    showFrame(); return true;
}

std::string muzzleFixtureMetadata()
{
    if (gWeapon == 0 || gFid < 0) return "null";
    const auto* flash=scene::muzzleFlashFor(gFid,gDude->frame,gRotation,false);
    const bool light=scene::effectLightMatches(gDude,gDude->tile,gDude->x,gDude->y);
    char json[768];
    std::snprintf(json,sizeof(json),"{\"weapon_pid\":%d,\"fid\":%d,\"animation\":%d,\"rotation\":%d,\"frame\":%d,\"frames\":%d,\"flash_enabled\":%s,\"energy_extra\":%s,\"core_pixels\":%d,\"core\":[%.3f,%.3f],\"local_light\":%s,\"offset\":[%d,%d],\"no_save\":true,\"player_lamp\":0}",
        gWeapon,gFid,gAnimation,gRotation,gDude->frame,gFrames,gShot/gFrames!=1?"true":"false",gShot/gFrames==2?"true":"false",flash?flash->corePixels:0,flash?flash->x:0.0,flash?flash->y:0.0,light?"true":"false",gDude->x,gDude->y);
    std::string result(json);
    result.pop_back();
    result+=",\"projectile_fid\":"+std::to_string(gProjectile?gProjectile->fid:-1);
    result+=",\"projectile_visible\":";
    result+=gProjectile && (gProjectile->flags & OBJECT_HIDDEN)==OBJECT_NONE ? "true" : "false";
    result+="}";
    return result;
}
} }
