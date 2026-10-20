#include "static_scene.h"

#include <algorithm>
#include <cmath>
#include <stdio.h>

#include "game_view.h"
#include "gl_api.h"
#include "scene_internal.h"
#include "sprite_card.h"

namespace fallout {
namespace scene_renderer {

using namespace internal;

namespace {

constexpr int kHexGrid = 200;
constexpr double kFullLight = 65536.0;
constexpr double kBaseLight = 655.0; // what the engine's light map starts at
constexpr int kMaxLights = 32; // as many as the shader takes (uLightPos[32])
// Each light's reach is a 21 x 21 window of hexes (columns, rows) around its
// own: the engine spreads light at most 8 hexes.
constexpr int kWindow = 21;
constexpr int kWindowHalf = 10;
constexpr double kLowestHeight = 0.08; // units above the floor (the exporter's glows)

std::vector<StaticPointLight> gLights;
std::vector<int> gHexLight; // HEX_GRID_SIZE, 65536 full
std::vector<float> gHexColor; // rgb per hex
bool gHexStale = false;
unsigned int gHexTexture = 0;
bool gLightOn = false;
bool gLightFailed = false; // the reach texture could not be uploaded: the scene stays unlit
double gAmbient = 1.0;
unsigned int gReachTexture = 0;
std::vector<unsigned char> gReach; // the uploaded reach windows, one per shown light
float gPos[kMaxLights * 4];
float gColor[kMaxLights * 4];
float gWindowOrigin[kMaxLights * 2];
int gShown = 0;

// GL ground point (x, z) of a game pixel; false before the projection is known.
bool groundAt(double pixelX, double pixelY, double* x, double* z)
{
    return gHeroProjectionLoaded && spritePixelToGround(cardProjection(), pixelX, pixelY, x, z);
}

// Picks the lights shown (the nearest to the view center when there are more
// than the shader takes), places them in GL world and fills their reach.
void prepareLights(std::vector<unsigned char>* reach)
{
    std::vector<const StaticPointLight*> lights;
    for (const StaticPointLight& light : gLights) {
        if (light.tile >= 0 && light.tile < kHexGrid * kHexGrid && light.intensity > 0.0f) lights.push_back(&light);
    }
    if (static_cast<int>(lights.size()) > kMaxLights) {
        GameView view;
        double center[2] = { 0.0, 0.0 };
        if (currentGameView(&view)) {
            center[0] = view.centerPixel[0];
            center[1] = view.centerPixel[1];
        }
        auto away = [&](const StaticPointLight* light) {
            double x, y;
            spriteHexPixel(light->tile, &x, &y);
            return (x - center[0]) * (x - center[0]) + (y - center[1]) * (y - center[1]);
        };
        std::sort(lights.begin(), lights.end(),
            [&](const StaticPointLight* a, const StaticPointLight* b) { return away(a) < away(b); });
        lights.resize(kMaxLights);
    }

    const SpriteCardProjection projection = cardProjection();
    reach->assign(static_cast<size_t>(kWindow) * kWindow * kMaxLights, 0);
    gShown = 0;
    for (const StaticPointLight* light : lights) {
        double hexX, hexY;
        spriteHexPixel(light->tile, &hexX, &hexY);
        hexX += light->objectX;
        hexY += light->objectY;
        double x, z, rightX, rightZ;
        if (!groundAt(hexX, hexY, &x, &z) || !groundAt(hexX + 1.0, hexY, &rightX, &rightZ)) continue;
        // The ground point under the shining pixels' column, and their height.
        x += (rightX - x) * light->right;
        z += (rightZ - z) * light->right;
        const double height = std::max(-light->down / projection.hpx, kLowestHeight);

        const int slot = gShown++;
        gPos[slot * 4] = static_cast<float>(x);
        gPos[slot * 4 + 1] = static_cast<float>(height);
        gPos[slot * 4 + 2] = static_cast<float>(z);
        gPos[slot * 4 + 3] = static_cast<float>(std::clamp(light->distance, 0, 8) + 1);
        for (int channel = 0; channel < 3; channel++) gColor[slot * 4 + channel] = light->color[channel];
        gColor[slot * 4 + 3] = std::min(light->intensity, 1.0f);
        const int column0 = light->tile % kHexGrid - kWindowHalf;
        const int row0 = light->tile / kHexGrid - kWindowHalf;
        gWindowOrigin[slot * 2] = static_cast<float>(column0);
        gWindowOrigin[slot * 2 + 1] = static_cast<float>(row0);
        for (int tile : light->reach) {
            const int column = tile % kHexGrid - column0;
            const int row = tile / kHexGrid - row0;
            if (column < 0 || column >= kWindow || row < 0 || row >= kWindow) continue;
            (*reach)[(static_cast<size_t>(slot) * kWindow + row) * kWindow + column] = 255;
        }
    }
}

// The ground length of one hex step, in GL units.
float hexUnit()
{
    double x0, z0, x1, z1, hexX, hexY;
    const int tile = 100 * kHexGrid + 100;
    spriteHexPixel(tile, &hexX, &hexY);
    if (!groundAt(hexX, hexY, &x0, &z0)) return 1.0f;
    spriteHexPixel(tile + 1, &hexX, &hexY);
    if (!groundAt(hexX, hexY, &x1, &z1)) return 1.0f;
    return static_cast<float>(std::sqrt((x1 - x0) * (x1 - x0) + (z1 - z0) * (z1 - z0)));
}

} // namespace

namespace internal {

bool lightShown()
{
    return gLightOn && !gLightFailed;
}

float lightAmbient()
{
    return lightShown() ? static_cast<float>(gAmbient) : 1.0f;
}

void setLightAnchor(int tile)
{
    double hexX, hexY, x = 0.0, z = 0.0;
    if (tile >= 0 && tile < kHexGrid * kHexGrid) {
        spriteHexPixel(tile, &hexX, &hexY);
        groundAt(hexX, hexY, &x, &z);
    }
    glUniform2f(gSceneLightAnchorLocation, static_cast<float>(x), static_cast<float>(z));
}

bool bindLight()
{
    if (!lightShown() || !gHeroProjectionLoaded) return false;
    std::vector<unsigned char> reach;
    prepareLights(&reach);

    glActiveTexture(kTexture0 + 3);
    while (glGetError() != 0) {
    }
    if (gReachTexture == 0) {
        glGenTextures(1, &gReachTexture);
        glBindTexture(kTexture2D, gReachTexture);
        glTexParameteri(kTexture2D, kTextureMinFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureMagFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        gReach.clear();
    }
    glBindTexture(kTexture2D, gReachTexture);
    if (reach != gReach) {
        glPixelStorei(kUnpackAlignment, 1);
        glTexImage2D(kTexture2D, 0, kR8, kWindow, kWindow * kMaxLights, 0, kRed, kUnsignedByte, reach.data());
        glPixelStorei(kUnpackAlignment, 4);
        gReach = std::move(reach);
    }
    if (gReachTexture == 0 || glGetError() != 0) {
        fprintf(stderr, "scene_renderer: the light could not be uploaded; the scene stays unlit\n");
        fflush(stderr);
        gLightFailed = true;
        glActiveTexture(kTexture0);
        return false;
    }
    glUniform1i(gSceneLightReachLocation, 3);

    // The small lights per hex: rgb their colour times a, a their light.
    glActiveTexture(kTexture0 + 4);
    if (gHexTexture == 0) {
        glGenTextures(1, &gHexTexture);
        glBindTexture(kTexture2D, gHexTexture);
        glTexParameteri(kTexture2D, kTextureMinFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureMagFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        gHexStale = true;
    }
    glBindTexture(kTexture2D, gHexTexture);
    if (gHexStale) {
        std::vector<unsigned short> texels(static_cast<size_t>(kHexGrid) * kHexGrid * 4, 0);
        const bool full = gHexLight.size() == static_cast<size_t>(kHexGrid) * kHexGrid
            && gHexColor.size() == gHexLight.size() * 3;
        for (size_t tile = 0; full && tile < gHexLight.size(); tile++) {
            const double light = std::clamp(gHexLight[tile] / kFullLight, 0.0, 1.0);
            for (int channel = 0; channel < 3; channel++) {
                const double premultiplied = std::clamp(static_cast<double>(gHexColor[tile * 3 + channel]), 0.0, 1.0) * light;
                texels[tile * 4 + channel] = static_cast<unsigned short>(premultiplied * 65535.0 + 0.5);
            }
            texels[tile * 4 + 3] = static_cast<unsigned short>(light * 65535.0 + 0.5);
        }
        glPixelStorei(kUnpackAlignment, 2);
        glTexImage2D(kTexture2D, 0, kRGBA16, kHexGrid, kHexGrid, 0, kRGBA, kUnsignedShort, texels.data());
        glPixelStorei(kUnpackAlignment, 4);
        gHexStale = false;
    }
    if (gHexTexture == 0 || glGetError() != 0) {
        fprintf(stderr, "scene_renderer: the light could not be uploaded; the scene stays unlit\n");
        fflush(stderr);
        gLightFailed = true;
        glActiveTexture(kTexture0);
        return false;
    }
    glUniform1i(gSceneLightMapLocation, 4);
    glActiveTexture(kTexture0);

    // Ground (u, v) to the game's pixels, as the exporter's projection.
    glUniform2f(gSceneLightSquLocation, static_cast<float>(gHeroProjection.squ[0]),
        static_cast<float>(gHeroProjection.squ[1]));
    glUniform2f(gSceneLightSqvLocation, static_cast<float>(gHeroProjection.sqv[0]),
        static_cast<float>(gHeroProjection.sqv[1]));
    glUniform2f(gSceneLightOriginLocation, static_cast<float>(gHeroProjection.origin[0]),
        static_cast<float>(gHeroProjection.origin[1]));
    // The engine lights a hex with at least the ambient level; its light map
    // starts at kBaseLight.
    glUniform1f(gSceneAmbientLocation, static_cast<float>(std::max(gAmbient, kBaseLight / kFullLight)));
    glUniform1f(gSceneHexUnitLocation, hexUnit());
    double toward[3];
    if (!spriteTowardCamera(cardProjection(), toward)) {
        toward[0] = 0.0;
        toward[1] = 1.0;
        toward[2] = 0.0;
    }
    glUniform3f(gSceneTowardCameraLocation, static_cast<float>(toward[0]), static_cast<float>(toward[1]),
        static_cast<float>(toward[2]));
    glUniform1i(gSceneLightCountLocation, gShown);
    if (gShown > 0) {
        glUniform4fv(gSceneLightPosLocation, gShown, gPos);
        glUniform4fv(gSceneLightColorLocation, gShown, gColor);
        glUniform2fv(gSceneLightWindowLocation, gShown, gWindowOrigin);
    }
    return true;
}

void destroyStaticLight()
{
    if (gContext != nullptr && gReachTexture != 0) glDeleteTextures(1, &gReachTexture);
    if (gContext != nullptr && gHexTexture != 0) glDeleteTextures(1, &gHexTexture);
    gReachTexture = 0;
    gHexTexture = 0;
    gReach.clear();
}

} // namespace internal

void setStaticLight(int ambient, std::vector<int> hexLight, std::vector<float> hexColor,
    std::vector<StaticPointLight> lights)
{
    if (hexLight != gHexLight || hexColor != gHexColor) {
        gHexLight = std::move(hexLight);
        gHexColor = std::move(hexColor);
        gHexStale = true;
    }
    gLights = std::move(lights);
    gAmbient = std::clamp(ambient / kFullLight, 0.0, 1.0);
    gLightOn = true;
}

void clearStaticLight()
{
    gLightOn = false;
}

} // namespace scene_renderer
} // namespace fallout
