#include "static_scene.h"

#include <algorithm>
#include <cstring>

#include "gl_api.h"
#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {

using namespace internal;

namespace {

constexpr int kHexGrid = 200;
constexpr double kFullLight = 65536.0;

// The game's light per hex (its tile intensity, at least the ambient level),
// as the engine paints the floor and objects with it.
std::vector<unsigned short> gLightMap;
unsigned int gLightTexture = 0;
bool gLightStale = false;
bool gLightOn = false;
double gAmbient = 1.0;

} // namespace

namespace internal {

bool lightShown()
{
    return gLightOn && !gLightMap.empty();
}

float lightAtTile(int tile)
{
    if (!lightShown() || tile < 0 || tile >= kHexGrid * kHexGrid) return 1.0f;
    return static_cast<float>(gLightMap[tile] / 65535.0);
}

float lightAmbient()
{
    return lightShown() ? static_cast<float>(gAmbient) : 1.0f;
}

bool bindLight()
{
    if (!lightShown()) return false;
    glActiveTexture(kTexture0 + 3);
    if (gLightTexture == 0) {
        glGenTextures(1, &gLightTexture);
        glBindTexture(kTexture2D, gLightTexture);
        glTexParameteri(kTexture2D, kTextureMinFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureMagFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        gLightStale = true;
    }
    glBindTexture(kTexture2D, gLightTexture);
    if (gLightStale) {
        glPixelStorei(kUnpackAlignment, 2);
        glTexImage2D(kTexture2D, 0, kR16, kHexGrid, kHexGrid, 0, kRed, kUnsignedShort, gLightMap.data());
        glPixelStorei(kUnpackAlignment, 4);
        gLightStale = false;
    }
    glUniform1i(gSceneLightMapLocation, 3);
    // Ground (u, v) to the game's pixels, as the exporter's projection.
    glUniform2f(gSceneLightSquLocation, static_cast<float>(gHeroProjection.squ[0]),
        static_cast<float>(gHeroProjection.squ[1]));
    glUniform2f(gSceneLightSqvLocation, static_cast<float>(gHeroProjection.sqv[0]),
        static_cast<float>(gHeroProjection.sqv[1]));
    glUniform2f(gSceneLightOriginLocation, static_cast<float>(gHeroProjection.origin[0]),
        static_cast<float>(gHeroProjection.origin[1]));
    glActiveTexture(kTexture0);
    return true;
}

void destroyStaticLight()
{
    if (gContext != nullptr && gLightTexture != 0) glDeleteTextures(1, &gLightTexture);
    gLightTexture = 0;
    gLightStale = false;
}

} // namespace internal

void setStaticLight(const int* tileIntensity, int ambient)
{
    if (tileIntensity == nullptr) {
        clearStaticLight();
        return;
    }
    std::vector<unsigned short> map(kHexGrid * kHexGrid);
    for (int tile = 0; tile < kHexGrid * kHexGrid; tile++) {
        // The engine lights a hex with the larger of its own and the ambient light.
        const int light = std::clamp(std::max(tileIntensity[tile], ambient), 0, 65536);
        map[tile] = static_cast<unsigned short>(std::min(light, 65535));
    }
    if (map != gLightMap) {
        gLightMap = std::move(map);
        gLightStale = true;
    }
    gAmbient = std::clamp(ambient / kFullLight, 0.0, 1.0);
    gLightOn = true;
}

void clearStaticLight()
{
    gLightOn = false;
}

} // namespace scene_renderer
} // namespace fallout
