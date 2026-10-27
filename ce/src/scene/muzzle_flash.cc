#include "muzzle_flash.h"

#include <algorithm>
#include <array>
#include <map>
#include <utility>

#include "../art.h"
#include "../color.h"
#include "../settings.h"
#include "../svga.h"
#include "../interface.h"
#include "../inventory.h"
#include "../item.h"
#include "../object.h"
#include "../proto.h"

namespace fallout { namespace scene {
namespace {
bool gPreview = true;
bool gEnergyPreview = false;
bool gEnergyPreviewOverride = false;
std::map<std::pair<int,int>, std::vector<MuzzleMask>> gMasks;
std::map<std::pair<int,int>, std::array<float,3>> gColors;

MuzzleImage loadFrame(const FrmId& fid, int index, int rotation, int dx, int dy)
{
    MuzzleImage out;
    FrmImage image;
    if (!image.lock(fid, index, static_cast<Rotation>(rotation))) return out;
    out.width = image.getWidth(); out.height = image.getHeight();
    if (out.width <= 0 || out.height <= 0 || out.width > 1024 || out.height > 1024 || image.getData() == nullptr) return {};
    out.anchorX = out.width / 2 - image.getXOffset() - dx;
    out.anchorY = out.height - 1 - image.getYOffset() - dy;
    out.indices.assign(image.getData(), image.getData() + out.width * out.height);
    return out;
}
std::vector<MuzzleMask> inspect(int fid, int rotation)
{
    CacheEntry* handle = nullptr;
    Art* art = artLock(FrmId(fid), &handle);
    if (art == nullptr) return {};
    const int count = artGetFrameCount(art), cue = artGetActionFrame(art);
    if (count <= 0 || count > 64) { artUnlock(handle); return {}; }
    std::vector<MuzzleImage> frames;
    int dx = 0, dy = 0;
    for (int i = 0; i < count; ++i) {
        int x = 0, y = 0;
        artGetFrameOffsets(art, i, static_cast<Rotation>(rotation), &x, &y);
        dx += x; dy += y;
        frames.push_back(loadFrame(FrmId(fid), i, rotation, dx, dy));
    }
    artUnlock(handle);
    // Preserve the actual critter/weapon/side bits; change only the animation.
    const FrmId aimed((fid & ~0x00FF0000) | (ANIM_POINT << 16));
    art = artLock(aimed, &handle);
    if (art == nullptr) return {};
    const int aimCount = artGetFrameCount(art);
    dx = dy = 0;
    for (int i = 0; i < aimCount; ++i) {
        int x = 0, y = 0;
        artGetFrameOffsets(art, i, static_cast<Rotation>(rotation), &x, &y);
        dx += x; dy += y;
    }
    const MuzzleImage aim = loadFrame(aimed, aimCount - 1, rotation, dx, dy);
    artUnlock(handle);
    // Ordinary base colors stay stable through palette fades. Animated colors
    // use the live palette (critters normally paint their flash in ordinary colors).
    std::array<unsigned char, 768> palette;
    std::copy(_cmap, _cmap + 768, palette.begin());
    if (const auto* live = directDrawGetPalette()) std::copy(live + 229 * 3, live + 255 * 3, palette.begin() + 229 * 3);
    const auto animation = FrmId(fid).animationType();
    return detectMuzzleMasks(frames, aim, rotation, cue, animation == ANIM_FIRE_SINGLE,
        animation == ANIM_FIRE_CONTINUOUS, palette.data());
}
}

bool muzzleFlashesEnabled() { return gPreview && settings.scene3d.glow; }
void setMuzzleFlashPreview(bool enabled) { gPreview = enabled; }
void setEnergyMuzzlePreview(bool enabled) { gEnergyPreviewOverride = true; gEnergyPreview = enabled; }
void useDefaultEnergyMuzzle() { gEnergyPreviewOverride = false; }

MuzzleMaterial muzzleMaterial(Object* object)
{
    MuzzleMaterial result;
    if (!muzzleFlashesEnabled() || object == nullptr || FrmId(object).objectType()!=OBJ_TYPE_CRITTER) return result;
    Object* right = critterGetItem2(object);
    Object* left = critterGetItem1(object);
    Object* weapon = object == gDude && interfaceGetCurrentHand() == HAND_LEFT ? left : right;
    auto matches = [&](Object* item) { return item != nullptr && itemGetType(item) == ITEM_TYPE_WEAPON
        && weaponGetAnimationCode(item) == FrmId(object).weaponAnimation(); };
    if (!matches(weapon)) weapon = matches(right) ? right : (matches(left) ? left : nullptr);
    Proto* proto = nullptr;
    if (weapon == nullptr || protoGetProto(weapon,&proto) != 0 || proto == nullptr
        || proto->item.data.weapon.damageType == DAMAGE_TYPE_NORMAL) return result;
    // Separate bolts already light the barrel through 086. The owner selected
    // a small extra flash for laser and plasma weapons with separate bolts.
    result.allowNative = proto->item.data.weapon.projectilePid <= 0;
    const int damage = proto->item.data.weapon.damageType;
    const bool extra = gEnergyPreviewOverride ? gEnergyPreview
        : (proto->item.data.weapon.projectilePid > 0 && (damage == DAMAGE_TYPE_LASER || damage == DAMAGE_TYPE_PLASMA));
    if (!extra) return result;
    result.extra = true;
    result.weaponPid = ProtoId(weapon).pid();
    const auto key = std::make_pair(result.weaponPid, static_cast<int>(object->rotation));
    auto cached = gColors.find(key);
    if (cached == gColors.end()) {
        std::array<double,3> sum = {};
        Proto* projectile = nullptr;
        const int pid = proto->item.data.weapon.projectilePid;
        if (pid > 0 && protoGetProto(ProtoId(pid),&projectile) == 0 && projectile != nullptr) {
            CacheEntry* handle = nullptr;
            const FrmId fid(projectile);
            Art* art = artLock(fid,&handle);
            if (art != nullptr) {
                const int frames = std::min(artGetFrameCount(art),64);
                const auto* live = directDrawGetPalette();
                for (int frame=0;frame<frames;++frame) {
                    FrmImage im;
                    if (!im.lock(fid,frame,object->rotation) || im.getData()==nullptr || im.getWidth()>1024 || im.getHeight()>1024) continue;
                    for (int i=0;i<im.getWidth()*im.getHeight();++i) {
                        const int c=im.getData()[i]; if (!c) continue;
                        const auto* palette = c>=229 && live!=nullptr ? live : _cmap;
                        const int r=palette[c*3],g=palette[c*3+1],b=palette[c*3+2];
                        if ((std::max({r,g,b})>=28 && std::max({r,g,b})-std::min({r,g,b})>=12) || std::min({r,g,b})>=42) {
                            sum[0]+=r;sum[1]+=g;sum[2]+=b;
                        }
                    }
                }
                artUnlock(handle);
            }
        }
        const double peak=std::max({sum[0],sum[1],sum[2]});
        std::array<float,3> color = {1.0f,0.8f,0.5f}; // the native warm flash if no projectile exists
        if (peak>0.0) for (int k=0;k<3;++k) color[k]=static_cast<float>(sum[k]/peak);
        cached=gColors.emplace(key,color).first;
    }
    for (int k=0;k<3;++k) result.color[k]=cached->second[k];
    return result;
}

void tintMuzzlePixel(unsigned char* rgba, const MuzzleMaterial& material)
{
    if (!material.extra) return;
    const float peak=static_cast<float>(std::max({rgba[0],rgba[1],rgba[2]}));
    for (int k=0;k<3;++k) rgba[k]=static_cast<unsigned char>(std::clamp(rgba[k]*0.3f+peak*material.color[k]*0.7f,0.0f,255.0f));
}

const MuzzleMask* muzzleFlashFor(int fid, int frame, int rotation, bool respectSwitch)
{
    const FrmId art(fid);
    const auto animation = art.animationType();
    if ((respectSwitch && !muzzleFlashesEnabled()) || art.objectType() != OBJ_TYPE_CRITTER || frame < 0
        || rotation < 0 || rotation >= 6 || static_cast<int>(art.weaponAnimation()) < 5
        || (animation != ANIM_FIRE_SINGLE && animation != ANIM_FIRE_BURST && animation != ANIM_FIRE_CONTINUOUS)) return nullptr;
    const auto key = std::make_pair(fid, rotation);
    auto found = gMasks.find(key);
    if (found == gMasks.end()) {
        if (gMasks.size() >= 128) gMasks.clear();
        found = gMasks.emplace(key, inspect(fid, rotation)).first;
    }
    if (frame >= static_cast<int>(found->second.size())) return nullptr;
    const auto& mask = found->second[frame];
    return mask.corePixels > 0 ? &mask : nullptr;
}
} }
