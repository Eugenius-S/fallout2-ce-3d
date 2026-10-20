#include "scene_lights.h"

#include <algorithm>
#include <array>
#include <map>
#include <tuple>
#include <vector>

#include "../art.h"
#include "../light.h"
#include "../map.h"
#include "../object.h"
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

// A light without a shining sprite takes the colour of the shining lights
// within its reach, by their intensity.
void borrowColor(Candidate* candidate, const std::vector<Candidate>& candidates)
{
    if (candidate->shines || candidate->critter) return;
    scene_renderer::StaticPointLight& light = candidate->light;
    const int within = std::max(kBorrowDistance, light.distance);
    double sum[3] = { 0.0, 0.0, 0.0 };
    for (const Candidate& other : candidates) {
        if (!other.shines || tileDistanceBetween(light.tile, other.light.tile) > within) continue;
        for (int channel = 0; channel < 3; channel++) sum[channel] += other.light.intensity * other.light.color[channel];
    }
    const double top = std::max({ sum[0], sum[1], sum[2] });
    if (top <= 0.0) return;
    for (int channel = 0; channel < 3; channel++) light.color[channel] = static_cast<float>(sum[channel] / top);
}

// Adds a light's colour, by the share of its light each hex gets, around its
// hex (walls do not stop it here: the light itself comes from the engine).
void splatColor(const scene_renderer::StaticPointLight& light, std::vector<float>* color)
{
    const int column0 = light.tile % HEX_GRID_WIDTH;
    const int row0 = light.tile / HEX_GRID_WIDTH;
    for (int row = row0 - light.distance; row <= row0 + light.distance; row++) {
        for (int column = column0 - light.distance; column <= column0 + light.distance; column++) {
            if (row < 0 || column < 0 || row >= HEX_GRID_HEIGHT || column >= HEX_GRID_WIDTH) continue;
            const int tile = row * HEX_GRID_WIDTH + column;
            const int away = tileDistanceBetween(light.tile, tile);
            if (away > light.distance) continue;
            const float share = light.intensity * (1.0f - static_cast<float>(away) / (light.distance + 1));
            for (int channel = 0; channel < 3; channel++) (*color)[tile * 3 + channel] += share * light.color[channel];
        }
    }
}

} // namespace

void updateLightsFromGame()
{
    if (!settings.scene3d.lights) {
        scene_renderer::clearStaticLight();
        return;
    }
    std::vector<Candidate> candidates;
    objectForEachAtElevation(gElevation, collectLight, &candidates);

    // The big lights nearest to the view are lit from their own point.
    std::vector<Candidate*> smooth;
    for (Candidate& candidate : candidates) {
        if (candidate.light.distance >= kSmoothDistance) smooth.push_back(&candidate);
    }
    std::sort(smooth.begin(), smooth.end(), [](const Candidate* a, const Candidate* b) {
        return tileDistanceBetween(gCenterTile, a->light.tile) < tileDistanceBetween(gCenterTile, b->light.tile);
    });
    if (static_cast<int>(smooth.size()) > kMaxSmoothLights) smooth.resize(kMaxSmoothLights);

    // The engine's light per hex, less its base and the big lights' share.
    std::vector<int> hexLight(HEX_GRID_SIZE);
    for (int tile = 0; tile < HEX_GRID_SIZE; tile++) {
        hexLight[tile] = lightGetTrueTileIntensity(gElevation, tile) - kBaseLight;
    }
    struct Spread {
        std::vector<int>* hexLight;
        std::vector<int>* reach;
    };
    std::vector<scene_renderer::StaticPointLight> lights;
    for (Candidate* candidate : smooth) {
        borrowColor(candidate, candidates);
        scene_renderer::StaticPointLight light = candidate->light;
        Spread spread = { &hexLight, &light.reach };
        objectSpreadLight(candidate->object, [](int tile, int intensity, void* context) {
            auto* spread = static_cast<Spread*>(context);
            (*spread->hexLight)[tile] -= intensity;
            if (intensity > 0) spread->reach->push_back(tile);
        }, &spread);
        lights.push_back(std::move(light));
        candidate->object = nullptr; // lit from its own point
    }

    // The colour of the rest, per hex.
    std::vector<float> color(HEX_GRID_SIZE * 3, 0.0f);
    for (Candidate& candidate : candidates) {
        if (candidate.object == nullptr) continue;
        borrowColor(&candidate, candidates);
        splatColor(candidate.light, &color);
    }
    for (int tile = 0; tile < HEX_GRID_SIZE; tile++) {
        hexLight[tile] = std::max(hexLight[tile], 0);
        const float top = std::max({ color[tile * 3], color[tile * 3 + 1], color[tile * 3 + 2] });
        for (int channel = 0; channel < 3; channel++) {
            color[tile * 3 + channel] = top > 0.0f ? color[tile * 3 + channel] / top : 1.0f;
        }
    }
    scene_renderer::setStaticLight(lightGetAmbientIntensity(), std::move(hexLight), std::move(color), std::move(lights));
}

} // namespace scene
} // namespace fallout
