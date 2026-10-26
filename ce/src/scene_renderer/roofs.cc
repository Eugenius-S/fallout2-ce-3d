#include "static_scene.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "gl_api.h"
#include "scene_internal.h"
#include "shadow.h"

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
// All squares, hidden ones included, for the sun's shadow map (task 045).
unsigned int gRoofShadowVao = 0;
unsigned int gRoofShadowVbo = 0;
int gRoofShadowVertexCount = 0;
bool gRoofShadowStale = true;

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
    gRoofShadowStale = true;
}

void roofShadowCasters(unsigned int* vao, int* vertexCount)
{
    *vao = 0;
    *vertexCount = 0;
    if (gRoofSquares.empty() || !gHeroProjectionLoaded) return;
    if (gRoofShadowStale) {
        if (gRoofShadowVao == 0) glGenVertexArrays(1, &gRoofShadowVao);
        if (gRoofShadowVbo == 0) glGenBuffers(1, &gRoofShadowVbo);
        const float y = static_cast<float>(kRoofPixels / gHeroProjection.hpx);
        std::vector<float> positions;
        positions.reserve(gRoofSquares.size() * 6 * 3);
        for (const RoofSquare& square : gRoofSquares) {
            const float x0 = square.x0, z0 = square.z0, x1 = square.x1, z1 = square.z1;
            const float corners[6][3] = {
                { x0, y, z0 }, { x1, y, z0 }, { x1, y, z1 },
                { x0, y, z0 }, { x1, y, z1 }, { x0, y, z1 },
            };
            for (const auto& corner : corners) {
                positions.push_back(corner[0]);
                positions.push_back(corner[1]);
                positions.push_back(corner[2]);
            }
        }
        glBindVertexArray(gRoofShadowVao);
        glBindBuffer(kArrayBuffer, gRoofShadowVbo);
        glBufferData(kArrayBuffer,
            static_cast<long long>(positions.size() * sizeof(float)), positions.data(), kStaticDraw);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);
        gRoofShadowVertexCount = static_cast<int>(positions.size() / 3);
        gRoofShadowStale = false;
        // One-shot load diagnostics (082 round 6), alongside the walls'
        // CASTERS print: the shadow map's content vs the receivers' lookups.
        float rx0 = 1e9f, rx1 = -1e9f, rz0 = 1e9f, rz1 = -1e9f;
        for (size_t p = 0; p + 2 < positions.size(); p += 3) {
            rx0 = std::min(rx0, positions[p]);
            rx1 = std::max(rx1, positions[p]);
            rz0 = std::min(rz0, positions[p + 2]);
            rz1 = std::max(rz1, positions[p + 2]);
        }
        fprintf(stderr, "ROOF CASTERS: squares %zu, vertices %d, y %.2f, x %.2f..%.2f, z %.2f..%.2f\n",
            gRoofSquares.size(), gRoofShadowVertexCount, y, rx0, rx1, rz0, rz1);
        fflush(stderr);
    }
    if (gRoofShadowVertexCount == 0) return;
    *vao = gRoofShadowVao;
    *vertexCount = gRoofShadowVertexCount;
}

void destroyRoofs()
{
    if (gContext != nullptr) {
        if (gRoofVbo != 0) glDeleteBuffers(1, &gRoofVbo);
        if (gRoofVao != 0) glDeleteVertexArrays(1, &gRoofVao);
        if (gRoofShadowVbo != 0) glDeleteBuffers(1, &gRoofShadowVbo);
        if (gRoofShadowVao != 0) glDeleteVertexArrays(1, &gRoofShadowVao);
        if (gEggTexture != 0) glDeleteTextures(1, &gEggTexture);
    }
    setShadowRoofCasters(0, 0);
    gRoofVbo = 0;
    gRoofVao = 0;
    gRoofVertexCount = 0;
    gRoofShadowVbo = 0;
    gRoofShadowVao = 0;
    gRoofShadowVertexCount = 0;
    gRoofShadowStale = true;
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
    // and are not hidden by anything. A fly camera sees them from any side:
    // there what is nearer hides them.
    const bool flying = staticFlyCameraActive();
    if (flying) {
        glEnable(kDepthTest);
        glDepthMask(0);
    } else {
        glDisable(kDepthTest);
    }
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
    if (flying) glDepthMask(1);
}

bool bindEgg(int viewportX, int viewportY, int viewportWidth, int viewportHeight)
{
    // The egg is a place on the game's screen: a fly camera has none.
    if (!gEggShown || gEggTexture == 0 || staticFlyCameraActive()) return false;
    // The zoom shows the game's own picture scaled around the screen center,
    // and the egg belongs to that picture: its rect scales with the shown
    // zoom around the mask's sprite anchor (half across, one pixel up from
    // the bottom, the point updateRoofsFromGame projected), so the circle
    // keeps the hero's on-screen size at any zoom.
    double zoom = 1.0;
    GameView view;
    if (currentGameView(&view) && view.zoom > 0.01) zoom = view.zoom;
    const float zx = static_cast<float>(static_cast<double>(gEggWidth) * zoom);
    const float zy = static_cast<float>(static_cast<double>(gEggHeight) * zoom);
    const float anchorX = static_cast<float>(gEggLeft) + static_cast<float>(gEggWidth) * 0.5f;
    const float anchorY = static_cast<float>(gEggTop) + static_cast<float>(gEggHeight) - 1.0f;
    glActiveTexture(kTexture0 + 1);
    glBindTexture(kTexture2D, gEggTexture);
    glUniform1i(gSceneEggMaskLocation, 1);
    glUniform4f(gSceneEggRectLocation, anchorX - zx * 0.5f, anchorY - (zy - 1.0f), zx, zy);
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
    // F2CE_NO_EGG: a test switch, the egg off for an egg on/off capture pair.
    if (!gInitialized || mask == nullptr || width <= 0 || height <= 0
        || SDL_getenv("F2CE_NO_EGG") != nullptr) {
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

bool staticEggInfo(int* left, int* top, int* width, int* height, double* zoom)
{
    if (!gEggShown) return false;
    *left = gEggLeft;
    *top = gEggTop;
    *width = gEggWidth;
    *height = gEggHeight;
    GameView view;
    *zoom = currentGameView(&view) && view.zoom > 0.01 ? view.zoom : 1.0;
    return true;
}

} // namespace scene_renderer
} // namespace fallout
