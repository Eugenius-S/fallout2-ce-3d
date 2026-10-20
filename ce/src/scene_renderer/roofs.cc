#include "static_scene.h"

#include <cstring>

#include "gl_api.h"
#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {

using namespace internal;

namespace {

constexpr int kSquareCount = 100 * 100;
// The engine draws roofs 96 game pixels above the floor (squareTileToRoofScreenXY).
constexpr double kRoofPixels = 96.0;

std::vector<RoofSquare> gRoofSquares;
unsigned char gRoofVisible[kSquareCount] = {};
bool gRoofVisibilityKnown = false;
bool gRoofBufferStale = true;
unsigned int gRoofVao = 0;
unsigned int gRoofVbo = 0;
int gRoofVertexCount = 0;

// The see-through circle around the hero (the engine's egg).
unsigned int gEggTexture = 0;
const unsigned char* gEggMask = nullptr;
int gEggWidth = 0;
int gEggHeight = 0;
int gEggLeft = 0;
int gEggTop = 0;
bool gEggShown = false;

void rebuildRoofBuffer()
{
    std::vector<FloorVertex> vertices;
    const float y = static_cast<float>(kRoofPixels / gHeroProjection.hpx);
    for (const RoofSquare& square : gRoofSquares) {
        if (gRoofVisibilityKnown && gRoofVisible[square.index] == 0) continue;
        vertices.push_back({ square.x0, y, square.z0, square.u0, square.v0 });
        vertices.push_back({ square.x1, y, square.z0, square.u1, square.v0 });
        vertices.push_back({ square.x1, y, square.z1, square.u1, square.v1 });
        vertices.push_back({ square.x0, y, square.z0, square.u0, square.v0 });
        vertices.push_back({ square.x1, y, square.z1, square.u1, square.v1 });
        vertices.push_back({ square.x0, y, square.z1, square.u0, square.v1 });
    }
    if (gRoofVao == 0) glGenVertexArrays(1, &gRoofVao);
    if (gRoofVbo == 0) glGenBuffers(1, &gRoofVbo);
    glBindVertexArray(gRoofVao);
    glBindBuffer(kArrayBuffer, gRoofVbo);
    glBufferData(kArrayBuffer, static_cast<long long>(vertices.size() * sizeof(FloorVertex)), vertices.data(),
        0x88E8 /* GL_DYNAMIC_DRAW */);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    gRoofVertexCount = static_cast<int>(vertices.size());
    gRoofBufferStale = false;
}

} // namespace

namespace internal {

void setRoofSquares(std::vector<RoofSquare> squares)
{
    gRoofSquares = std::move(squares);
    gRoofVisibilityKnown = false;
    gRoofBufferStale = true;
}

void destroyRoofs()
{
    if (gContext != nullptr) {
        if (gRoofVbo != 0) glDeleteBuffers(1, &gRoofVbo);
        if (gRoofVao != 0) glDeleteVertexArrays(1, &gRoofVao);
        if (gEggTexture != 0) glDeleteTextures(1, &gEggTexture);
    }
    gRoofVbo = 0;
    gRoofVao = 0;
    gRoofVertexCount = 0;
    gRoofSquares.clear();
    gRoofVisibilityKnown = false;
    gRoofBufferStale = true;
    gEggTexture = 0;
    gEggMask = nullptr;
    gEggShown = false;
}

void drawRoofs(int viewportX, int viewportY, int viewportWidth, int viewportHeight)
{
    if (gRoofSquares.empty() || !gHeroProjectionLoaded) return;
    if (gRoofBufferStale) rebuildRoofBuffer();
    if (gRoofVertexCount == 0) return;

    // The engine paints roofs over everything on the map, so they come last
    // and are not hidden by anything.
    glDisable(kDepthTest);
    glEnable(kBlend);
    glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    glUniform1i(gSceneDepthAtLocation, 0);
    glUniform1i(gSceneTranslucentLocation, 0);
    glUniform1i(gScenePickLocation, 0);
    glUniform1i(gSceneTexturedLocation, 1);
    glUniform1f(gSceneCutoffLocation, 0.01f);
    if (bindEgg(viewportX, viewportY, viewportWidth, viewportHeight)) glUniform1i(gSceneEggLocation, 1);
    glBindTexture(kTexture2D, gFloorTexture);
    glBindVertexArray(gRoofVao);
    glDrawArrays(kTriangles, 0, gRoofVertexCount);
    glUniform1i(gSceneEggLocation, 0);
    glDisable(kBlend);
}

bool bindEgg(int viewportX, int viewportY, int viewportWidth, int viewportHeight)
{
    // The egg is a place on the game's screen: a fly camera has none.
    if (!gEggShown || gEggTexture == 0 || staticFlyCameraActive()) return false;
    glActiveTexture(kTexture0 + 1);
    glBindTexture(kTexture2D, gEggTexture);
    glUniform1i(gSceneEggMaskLocation, 1);
    glUniform4f(gSceneEggRectLocation, static_cast<float>(gEggLeft), static_cast<float>(gEggTop),
        static_cast<float>(gEggWidth), static_cast<float>(gEggHeight));
    glUniform4f(gSceneViewportLocation, static_cast<float>(viewportX), static_cast<float>(viewportY),
        static_cast<float>(viewportWidth), static_cast<float>(viewportHeight));
    glUniform2f(gSceneLogicalLocation, static_cast<float>(gLogicalWidth), static_cast<float>(gLogicalHeight));
    glActiveTexture(kTexture0);
    return true;
}

} // namespace internal

void setStaticRoofVisibility(const unsigned char* visible)
{
    if (gRoofVisibilityKnown && std::memcmp(gRoofVisible, visible, kSquareCount) == 0) return;
    std::memcpy(gRoofVisible, visible, kSquareCount);
    gRoofVisibilityKnown = true;
    gRoofBufferStale = true;
}

void setStaticEgg(const unsigned char* mask, int width, int height, int left, int top)
{
    if (!gInitialized || mask == nullptr || width <= 0 || height <= 0) {
        gEggShown = false;
        return;
    }
    if (mask != gEggMask || width != gEggWidth || height != gEggHeight) {
        if (gEggTexture == 0) glGenTextures(1, &gEggTexture);
        glBindTexture(kTexture2D, gEggTexture);
        glPixelStorei(kUnpackAlignment, 1);
        glPixelStorei(kUnpackRowLength, 0);
        glTexImage2D(kTexture2D, 0, 0x8229 /* GL_R8 */, width, height, 0, 0x1903 /* GL_RED */, kUnsignedByte, mask);
        glPixelStorei(kUnpackAlignment, 4);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        gEggMask = mask;
        gEggWidth = width;
        gEggHeight = height;
    }
    gEggLeft = left;
    gEggTop = top;
    gEggShown = true;
}

void clearStaticEgg()
{
    gEggShown = false;
}

} // namespace scene_renderer
} // namespace fallout
