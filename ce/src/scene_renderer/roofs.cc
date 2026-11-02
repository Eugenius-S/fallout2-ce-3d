#include "static_scene.h"

#include <algorithm>
#include <cmath>
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
// Whether the buffer was built for a fly camera (the wall tops stand there).
bool gRoofBufferFlying = false;
// What the roofs cover right now, for the cards' roof cut (114): a square per
// byte, 255 covered, as the shader's uRoofGrid reads it.
unsigned char gRoofGrid[kSquareCount] = {};
bool gRoofGridStale = true;
unsigned int gRoofGridTexture = 0;
unsigned int gRoofVao = 0;
unsigned int gRoofVbo = 0;
int gRoofVertexCount = 0;
// The wall-top sheets (fly camera only) follow the flat squares in the buffer, from their own texture.
int gRoofFlatVertexCount = 0;
unsigned int gRoofFringeTexture = 0;
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

// A wall-top roof tile stands up (task 129, find 6), under the fly camera, in
// place of the flat square. Its art is the broken top of the wall below the
// roof edge: a slab of concrete along the edge with rebar and ragged concrete
// rising from it. The flat picture is the wall top seen from above, so each
// piece is folded up about the tile edge it sits on: the picture's ground
// distance from that edge becomes height, its ground position along the edge
// stays. The piece then stands in the vertical plane of the edge, the plane of
// the wall below, as that wall's own continuation, double sided, the ragged
// texels cut out. Art pixel (ax, ay) is the ground point (u, v) the flat
// square gives it: ax = -squ.x + u squ.x + v sqv.x, ay = u squ.y + v sqv.y.
// The game's camera keeps the flat square, as the game painted it.
void pushFringePiece(std::vector<FloorVertex>& vertices, const RoofSquare& square,
    const RoofSquare::FringePiece& piece, float roofY)
{
    const HeroProjection& p = gHeroProjection;
    const double determinant = p.squ[0] * p.sqv[1] - p.sqv[0] * p.squ[1];
    if (std::abs(determinant) < 1e-9) return;
    const bool alongV = piece.edge >= 2; // a plane u = const runs along v
    auto corner = [&](double ax, double ay) {
        const double bx = ax + p.squ[0];
        const double u = (bx * p.sqv[1] - p.sqv[0] * ay) / determinant;
        const double v = (p.squ[0] * ay - p.squ[1] * bx) / determinant;
        // The distance in from the edge: v (edge 0), 1 - v (1), u (2), 1 - u (3).
        const double inward = piece.edge == 0 ? v : piece.edge == 1 ? 1.0 - v : piece.edge == 2 ? u : 1.0 - u;
        const double x = alongV ? piece.edge - 2 : u, z = alongV ? v : piece.edge;
        const float tu = piece.artU0 + (piece.artU1 - piece.artU0) * static_cast<float>(ax / piece.artWidth);
        const float tv = piece.artV0 + (piece.artV1 - piece.artV0) * static_cast<float>(ay / piece.artHeight);
        return FloorVertex { static_cast<float>(square.tileX + x), roofY + static_cast<float>(inward),
            static_cast<float>(square.tileZ + z), tu, tv };
    };
    const double w = piece.artWidth, h = piece.artHeight;
    const FloorVertex a = corner(0, 0), b = corner(w, 0), c = corner(w, h), d = corner(0, h);
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    vertices.push_back(a);
    vertices.push_back(c);
    vertices.push_back(d);
}

void rebuildRoofBuffer()
{
    std::vector<FloorVertex> vertices;
    std::vector<FloorVertex> sheets;
    const float y = static_cast<float>(kRoofPixels / gHeroProjection.hpx);
    const bool flying = staticFlyCameraActive();
    gRoofBufferFlying = flying;
    for (const RoofSquare& square : gRoofSquares) {
        if (gRoofVisibilityKnown && gRoofVisible[square.index] == 0) continue;
        // Only the fly camera sees the sheets: the game's view is slightly
        // in perspective and keeps the game's own pixels on the flat square.
        if (square.fringe && flying && gRoofFringeTexture != 0) {
            for (int piece = 0; piece < square.pieceCount; piece++) {
                pushFringePiece(sheets, square, square.pieces[piece], y);
            }
            continue;
        }
        vertices.push_back({ square.x0, y, square.z0, square.u0, square.v0 });
        vertices.push_back({ square.x1, y, square.z0, square.u1, square.v0 });
        vertices.push_back({ square.x1, y, square.z1, square.u1, square.v1 });
        vertices.push_back({ square.x0, y, square.z0, square.u0, square.v0 });
        vertices.push_back({ square.x1, y, square.z1, square.u1, square.v1 });
        vertices.push_back({ square.x0, y, square.z1, square.u0, square.v1 });
    }
    gRoofFlatVertexCount = static_cast<int>(vertices.size());
    vertices.insert(vertices.end(), sheets.begin(), sheets.end());
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

// The grid as the shader reads it: a square's roof, less where the egg shows
// through it right now.
void rebuildRoofGrid()
{
    std::memset(gRoofGrid, 0, sizeof(gRoofGrid));
    for (const RoofSquare& square : gRoofSquares) {
        if (gRoofVisibilityKnown && gRoofVisible[square.index] == 0) continue;
        gRoofGrid[square.index] = 255;
    }
    if (gRoofGridTexture == 0) glGenTextures(1, &gRoofGridTexture);
    glBindTexture(kTexture2D, gRoofGridTexture);
    glPixelStorei(kUnpackAlignment, 1);
    glPixelStorei(kUnpackRowLength, 0);
    glTexImage2D(kTexture2D, 0, 0x8229 /* GL_R8 */, 100, 100, 0, 0x1903 /* GL_RED */, kUnsignedByte, gRoofGrid);
    glPixelStorei(kUnpackAlignment, 4);
    glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    gRoofGridStale = false;
}

} // namespace

namespace internal {

void setRoofSquares(std::vector<RoofSquare> squares)
{
    gRoofSquares = std::move(squares);
    gRoofVisibilityKnown = false;
    gRoofBufferStale = true;
    gRoofGridStale = true;
    gRoofShadowStale = true;
}

void setRoofFringeTexture(unsigned int texture)
{
    if (gRoofFringeTexture != 0 && gContext != nullptr) glDeleteTextures(1, &gRoofFringeTexture);
    gRoofFringeTexture = texture;
    gRoofBufferStale = true;
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
        if (gRoofGridTexture != 0) glDeleteTextures(1, &gRoofGridTexture);
        if (gEggTexture != 0) glDeleteTextures(1, &gEggTexture);
        if (gRoofFringeTexture != 0) glDeleteTextures(1, &gRoofFringeTexture);
    }
    gRoofFringeTexture = 0;
    gRoofFlatVertexCount = 0;
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
    gRoofGridTexture = 0;
    gRoofGridStale = true;
    gEggTexture = 0;
    gEggMask = nullptr;
    gEggShown = false;
}

void drawRoofs(int viewportX, int viewportY, int viewportWidth, int viewportHeight)
{
    if (gRoofSquares.empty() || !gHeroProjectionLoaded) return;
    if (gRoofBufferStale || gRoofBufferFlying != staticFlyCameraActive()) rebuildRoofBuffer();
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
    glDrawArrays(kTriangles, 0, gRoofFlatVertexCount);
    if (gRoofVertexCount > gRoofFlatVertexCount) {
        // The flat roofs do not write depth, so a fringe sheet behind a nearer
        // roof would paint over it: the opaque roof texels (the cutoff drops
        // the rest) go into the depth buffer, colors off, before the sheets.
        glColorMask(0, 0, 0, 0);
        glDepthMask(1);
        glDepthFunc(kLess);
        glDrawArrays(kTriangles, 0, gRoofFlatVertexCount);
        glDepthMask(0);
        glColorMask(1, 1, 1, 1);
        glBindTexture(kTexture2D, gRoofFringeTexture);
        glDrawArrays(kTriangles, gRoofFlatVertexCount, gRoofVertexCount - gRoofFlatVertexCount);
        glBindTexture(kTexture2D, gFloorTexture);
    }
    glUniform1i(gSceneEggLocation, 0);
    glDisable(kBlend);
    if (flying) glDepthMask(1);
}

// The height the cards are cut at where a roof covers them (114): the roofs'
// plane, 0 with no roofs on the map.
float roofClipHeight()
{
    return gRoofSquares.empty() || !gHeroProjectionLoaded
        ? 0.0f
        : static_cast<float>(kRoofPixels / gHeroProjection.hpx);
}

void bindRoofGrid(int unit)
{
    if (gRoofGridStale) rebuildRoofGrid();
    glActiveTexture(kTexture0 + unit);
    glBindTexture(kTexture2D, gRoofGridTexture);
    glUniform1i(gSceneRoofGridLocation, unit);
    glActiveTexture(kTexture0);
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
    gRoofGridStale = true;
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
