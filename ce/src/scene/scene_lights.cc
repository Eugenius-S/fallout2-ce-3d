#include "scene_lights.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <map>
#include <tuple>
#include <vector>

#include "../art.h"
#include "../light.h"
#include "../map.h"
#include "../object.h"
#include "../scripts.h"
#include "../scene_renderer/static_scene.h"
#include "../settings.h"
#include "../svga.h"
#include "../tile.h"

namespace fallout {
namespace scene {

namespace {

// Where a sprite frame shines from: game pixels right of and below its
// anchor, and how many of its shining pixels have each palette index (their
// colour follows the palette's cycling).
struct Emitter {
    bool found = false;
    double right = 0.0;
    double down = 0.0;
    std::array<int, 256> counts = {};
};

// A light whose sprite shows nothing that shines: this far above its hex, in
// game pixels, and in this colour (warm lamp light; critters white).
constexpr double kPlainHeight = 40.0;
constexpr float kLampColor[3] = { 1.0f, 0.86f, 0.6f };
// How much of a light's own colour it gives (the rest is white).
constexpr float kSaturation = 0.75f;
// A light this far reaching (hexes) is lit from its own point; up to
// kMaxSmoothLights of them, the nearest to the view; the rest, mostly the many
// one-hex lights of slime and embers, are added up per hex as the engine does.
constexpr int kSmoothDistance = 3;
constexpr int kMaxSmoothLights = 32;
// A light with no shining sprite of its own (maps light fires and slime with
// invisible light objects beside them) takes the colour of the shining things
// within its reach, at least this near, in hexes.
constexpr int kBorrowDistance = 4;
constexpr int kBaseLight = 655; // what the engine's light map starts at

struct Candidate {
    Object* object;
    scene_renderer::StaticPointLight light;
    bool shines; // it has a shining sprite of its own
    bool critter;
};

std::map<std::tuple<int, int, int>, Emitter> gEmitters;

// The engine's spreads cost the most (it checks walls hex by hex, for some
// three hundred lights on a Klamath map): they are kept while the lights and
// the engine's light map stay the same. Colours follow the palette's cycling
// and are added up again every frame.
struct SpreadCache {
    std::vector<int> key; // what the spreads depend on
    std::vector<int> hexLight; // the engine's light less its base and the big lights' share
    std::vector<std::vector<int>> reach; // each big light's hexes, in the order lit
    std::vector<std::vector<std::pair<int, int>>> spreads; // each candidate lit per hex: (tile, light)
};

SpreadCache gSpreads;

bool gDaylight = false;
SunTracker gSun;

// The palette indices the engine cycles and paints at full light (cycle.cc):
// slime, monitors, slow and fast fire, the alarm; not the shoreline.
bool shining(unsigned char index)
{
    return (index >= 229 && index <= 247) || index == 254;
}

Emitter findEmitter(Object* object)
{
    Emitter emitter;
    FrmImage image;
    if (!image.lock(FrmId(object), object->frame, object->rotation)) return emitter;
    const int width = image.getWidth();
    const int height = image.getHeight();
    const unsigned char* indices = image.getData();
    unsigned char* palette = directDrawGetPalette();
    if (indices == nullptr || palette == nullptr || width <= 0 || height <= 0 || width > 2048 || height > 2048) {
        return emitter;
    }
    auto brightness = [&](unsigned char index) {
        return palette[index * 3] + palette[index * 3 + 1] + palette[index * 3 + 2];
    };
    // The animated colours; else the brightest pixels of a bright sprite.
    bool animated = false;
    int brightest = 0;
    for (int at = 0; at < width * height; at++) {
        if (indices[at] == 0) continue;
        animated = animated || shining(indices[at]);
        brightest = std::max(brightest, brightness(indices[at]));
    }
    const int threshold = brightest * 9 / 10;
    if (!animated && brightest < 150) return emitter; // nothing bright (of 189, 6-bit white)
    double sumX = 0.0;
    double sumY = 0.0;
    int count = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const unsigned char index = indices[y * width + x];
            if (index == 0) continue;
            if (animated ? !shining(index) : brightness(index) < threshold) continue;
            sumX += x + 0.5;
            sumY += y + 0.5;
            emitter.counts[index]++;
            count++;
        }
    }
    if (count == 0) return emitter;
    emitter.found = true;
    // _obj_render_object places a frame's left column at its anchor minus half
    // its width plus its offset, and its bottom row at its anchor plus its offset.
    emitter.right = image.getXOffset() - width / 2 + sumX / count;
    emitter.down = image.getYOffset() - (height - 1) + sumY / count;
    return emitter;
}

void emitterColor(const Emitter& emitter, float color[3])
{
    unsigned char* palette = directDrawGetPalette();
    double sum[3] = { 0.0, 0.0, 0.0 };
    for (int index = 0; index < 256 && palette != nullptr; index++) {
        if (emitter.counts[index] == 0) continue;
        for (int channel = 0; channel < 3; channel++) sum[channel] += emitter.counts[index] * palette[index * 3 + channel];
    }
    const double top = std::max({ sum[0], sum[1], sum[2] });
    for (int channel = 0; channel < 3; channel++) {
        const float own = top > 0.0 ? static_cast<float>(sum[channel] / top) : kLampColor[channel];
        color[channel] = 1.0f - kSaturation + kSaturation * own;
    }
}

void collectLight(Object* object, void* context)
{
    auto* candidates = static_cast<std::vector<Candidate>*>(context);
    if (object->lightIntensity <= 0 || (object->flags & OBJECT_LIGHTING) == OBJECT_NONE
        || (object->flags & OBJECT_HIDDEN) != OBJECT_NONE || !hexGridTileIsValid(object->tile)) {
        return;
    }
    Candidate candidate = { object, {}, false, FrmId(object).objectType() == OBJ_TYPE_CRITTER };
    scene_renderer::StaticPointLight& light = candidate.light;
    light.tile = object->tile;
    light.objectX = object->x;
    light.objectY = object->y;
    light.intensity = static_cast<float>(std::min(object->lightIntensity, 65536) / 65536.0);
    light.distance = std::min(object->lightDistance, 8);

    if (!candidate.critter) {
        const auto key = std::make_tuple(object->fid, object->frame, static_cast<int>(object->rotation));
        auto found = gEmitters.find(key);
        if (found == gEmitters.end()) found = gEmitters.emplace(key, findEmitter(object)).first;
        candidate.shines = found->second.found;
        if (candidate.shines) {
            light.right = found->second.right;
            light.down = found->second.down;
            emitterColor(found->second, light.color);
        }
    }
    if (!candidate.shines) {
        light.down = -kPlainHeight;
        for (int channel = 0; channel < 3; channel++) {
            light.color[channel] = candidate.critter ? 1.0f : 1.0f - kSaturation + kSaturation * kLampColor[channel];
        }
    }
    candidates->push_back(std::move(candidate));
}

// Hexes between two tiles, as tileDistanceBetween counts them, at once: the
// engine's walks there step by step, an arctangent a step, which for every
// pair of three hundred lights took most of a frame. Columns with an even x
// sit half a hex lower (cube coordinates of an "even-q" grid).
int hexDistance(int tile1, int tile2)
{
    if (!tileIsValid(tile1) || !tileIsValid(tile2)) return TILE_MAX_DISTANCE;
    const auto cube = [](int tile, int* x, int* z) {
        const int column = tile % HEX_GRID_WIDTH;
        *x = column;
        *z = tile / HEX_GRID_WIDTH - (column + (column & 1)) / 2;
    };
    int x1, z1, x2, z2;
    cube(tile1, &x1, &z1);
    cube(tile2, &x2, &z2);
    const int dx = x1 - x2;
    const int dz = z1 - z2;
    return std::max({ std::abs(dx), std::abs(dz), std::abs(dx + dz) });
}

// A light without a shining sprite takes the colour of the shining lights
// within its reach, by their intensity.
void borrowColor(Candidate* candidate, const std::vector<Candidate>& candidates)
{
    if (candidate->shines || candidate->critter) return;
    scene_renderer::StaticPointLight& light = candidate->light;
    const int within = std::max(kBorrowDistance, light.distance);
    double sum[3] = { 0.0, 0.0, 0.0 };
    for (const Candidate& other : candidates) {
        if (!other.shines || hexDistance(light.tile, other.light.tile) > within) continue;
        for (int channel = 0; channel < 3; channel++) sum[channel] += other.light.intensity * other.light.color[channel];
    }
    const double top = std::max({ sum[0], sum[1], sum[2] });
    if (top <= 0.0) return;
    for (int channel = 0; channel < 3; channel++) light.color[channel] = static_cast<float>(sum[channel] / top);
}

} // namespace

void updateLightsFromGame()
{
    if (!settings.scene3d.lights) {
        scene_renderer::clearStaticLight();
        gSpreads = {};
        return;
    }
    std::vector<Candidate> candidates;
    objectForEachAtElevation(gElevation, collectLight, &candidates);

    // The big lights nearest to the view are lit from their own point.
    std::vector<int> smooth;
    std::vector<bool> isSmooth(candidates.size(), false);
    for (int index = 0; index < static_cast<int>(candidates.size()); index++) {
        if (candidates[index].light.distance >= kSmoothDistance) smooth.push_back(index);
    }
    std::stable_sort(smooth.begin(), smooth.end(), [&candidates](int a, int b) {
        return hexDistance(gCenterTile, candidates[a].light.tile) < hexDistance(gCenterTile, candidates[b].light.tile);
    });
    if (static_cast<int>(smooth.size()) > kMaxSmoothLights) smooth.resize(kMaxSmoothLights);
    for (int index : smooth) isSmooth[index] = true;

    // What the spreads depend on: the map, the lights, which of them are big,
    // and the engine's light map (a light moved, a door changed it).
    std::vector<int> key = { gMapHeader.index, gElevation, static_cast<int>(candidates.size()) };
    for (const Candidate& candidate : candidates) {
        const Object* object = candidate.object;
        key.insert(key.end(), { object->tile, object->lightIntensity, object->lightDistance });
    }
    key.insert(key.end(), smooth.begin(), smooth.end());
    for (int tile = 0; tile < HEX_GRID_SIZE; tile++) key.push_back(lightGetTrueTileIntensity(gElevation, tile));

    if (key != gSpreads.key) {
        // The engine's light per hex, less its base and the big lights' share.
        SpreadCache cache;
        cache.hexLight.resize(HEX_GRID_SIZE);
        for (int tile = 0; tile < HEX_GRID_SIZE; tile++) {
            cache.hexLight[tile] = lightGetTrueTileIntensity(gElevation, tile) - kBaseLight;
        }
        struct Spread {
            std::vector<int>* hexLight;
            std::vector<int>* reach;
        };
        for (int index : smooth) {
            cache.reach.emplace_back();
            Spread spread = { &cache.hexLight, &cache.reach.back() };
            objectSpreadLight(candidates[index].object, [](int tile, int intensity, void* context) {
                auto* spread = static_cast<Spread*>(context);
                (*spread->hexLight)[tile] -= intensity;
                if (intensity > 0) spread->reach->push_back(tile);
            }, &spread);
        }
        cache.spreads.resize(candidates.size());
        for (int index = 0; index < static_cast<int>(candidates.size()); index++) {
            if (isSmooth[index]) continue;
            objectSpreadLight(candidates[index].object, [](int tile, int intensity, void* context) {
                if (intensity > 0) static_cast<std::vector<std::pair<int, int>>*>(context)->emplace_back(tile, intensity);
            }, &cache.spreads[index]);
        }
        cache.key = std::move(key);
        gSpreads = std::move(cache);
    }

    std::vector<scene_renderer::StaticPointLight> lights;
    for (int order = 0; order < static_cast<int>(smooth.size()); order++) {
        Candidate& candidate = candidates[smooth[order]];
        borrowColor(&candidate, candidates);
        lights.push_back(candidate.light);
        lights.back().reach = gSpreads.reach[order];
    }

    // The colour of the rest, per hex: each light's colour by the light it
    // gives each hex, as the engine spreads it (walls stop it).
    std::vector<float> color(HEX_GRID_SIZE * 3, 0.0f);
    for (int index = 0; index < static_cast<int>(candidates.size()); index++) {
        if (isSmooth[index]) continue;
        borrowColor(&candidates[index], candidates);
        const float* rgb = candidates[index].light.color;
        for (const auto& [tile, intensity] : gSpreads.spreads[index]) {
            for (int channel = 0; channel < 3; channel++) {
                color[tile * 3 + channel] += static_cast<float>(intensity) * rgb[channel];
            }
        }
    }
    std::vector<int> hexLight = gSpreads.hexLight;
    for (int tile = 0; tile < HEX_GRID_SIZE; tile++) {
        hexLight[tile] = std::max(hexLight[tile], 0);
        const float top = std::max({ color[tile * 3], color[tile * 3 + 1], color[tile * 3 + 2] });
        for (int channel = 0; channel < 3; channel++) {
            color[tile * 3 + channel] = top > 0.0f ? color[tile * 3 + channel] / top : 1.0f;
        }
    }
    scene_renderer::setStaticLight(lightGetAmbientIntensity(), std::move(hexLight), std::move(color), std::move(lights));
}

void setSceneDaylight(bool daylight)
{
    gDaylight = daylight;
    gSun.reset();
}

bool sceneDaylight()
{
    return gDaylight;
}

void updateSunFromGame()
{
    using Clock = std::chrono::steady_clock;
    static Clock::time_point last = Clock::now();
    const Clock::time_point now = Clock::now();
    const double seconds = std::chrono::duration<double>(now - last).count();
    last = now;
    gSun.update(gameTimeGetTime(), static_cast<double>(lightGetAmbientIntensity()) / LIGHT_INTENSITY_MAX,
        settings.scene3d.sun && gDaylight, seconds, settings.scene3d.sun_swing);
}

const Sun& currentSun()
{
    return gSun.sun();
}

} // namespace scene
} // namespace fallout
