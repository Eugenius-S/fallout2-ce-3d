#ifndef FALLOUT_SCENE_RENDERER_SCENE_INTERNAL_H
#define FALLOUT_SCENE_RENDERER_SCENE_INTERNAL_H

// Shared between the scene renderer's own modules only. Each variable belongs
// to the module named above it, which alone creates and destroys it.

#include <SDL.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "camera.h"
#include "game_view.h"
#include "live_objects.h"
#include "sprite_card.h"

namespace fallout {
namespace scene_renderer {
namespace internal {

constexpr unsigned int kTexture2D = 0x0DE1;
constexpr unsigned int kRGBA = 0x1908;
constexpr unsigned int kUnsignedByte = 0x1401;
constexpr unsigned int kRgba8 = 0x8058;
constexpr unsigned int kNearest = 0x2600;
constexpr unsigned int kClampToEdge = 0x812F;
constexpr unsigned int kTextureMinFilter = 0x2801;
constexpr unsigned int kTextureMagFilter = 0x2800;
constexpr unsigned int kTextureWrapS = 0x2802;
constexpr unsigned int kTextureWrapT = 0x2803;
constexpr unsigned int kUnpackRowLength = 0x0CF2;
constexpr unsigned int kTriangles = 0x0004;
constexpr unsigned int kArrayBuffer = 0x8892;
constexpr unsigned int kStaticDraw = 0x88E4;
constexpr unsigned int kColorBufferBit = 0x4000;
constexpr unsigned int kBlend = 0x0BE2;
constexpr unsigned int kTexture0 = 0x84C0;
constexpr unsigned int kDepthBufferBit = 0x0100;
constexpr unsigned int kDepthTest = 0x0B71;
constexpr unsigned int kCullFace = 0x0B44;
constexpr unsigned int kLess = 0x0201;
constexpr unsigned int kSrcAlpha = 0x0302;
constexpr unsigned int kOneMinusSrcAlpha = 0x0303;
constexpr unsigned int kPackAlignment = 0x0D05;
constexpr unsigned int kUnpackAlignment = 0x0CF5;
constexpr int kDefaultPackAlignment = 4;

struct FloorVertex {
    float x;
    float y;
    float z;
    float u;
    float v;
};

// A FloorVertex that takes the depth of another point, as the donor's game
// view does: a sprite card that of its hex row's upright plane on the same
// view ray (so cards hide each other and walls as the game paints them, row
// by row), a wall top that of the front-plane point its texel was painted on.
struct DepthVertex {
    float x;
    float y;
    float z;
    float u;
    float v;
    float depthX;
    float depthY;
    float depthZ;
};

DepthVertex cardVertex(const SpriteCardVertex& corner, double row, const float eye[3]);
void depthVertexLayout();

// Little-endian readers for the exporter's sidecars.
bool readBinary(const std::filesystem::path& path, std::vector<unsigned char>& bytes);

inline unsigned short readU16(const unsigned char* data)
{
    return static_cast<unsigned short>(data[0] | (static_cast<unsigned short>(data[1]) << 8));
}

inline unsigned int readU32(const unsigned char* data)
{
    return static_cast<unsigned int>(data[0])
        | (static_cast<unsigned int>(data[1]) << 8)
        | (static_cast<unsigned int>(data[2]) << 16)
        | (static_cast<unsigned int>(data[3]) << 24);
}

inline unsigned long long readU64(const unsigned char* data)
{
    unsigned long long value = 0;
    for (int index = 0; index < 8; index++) {
        value |= static_cast<unsigned long long>(data[index]) << (index * 8);
    }
    return value;
}

inline float readF32(const unsigned char* data)
{
    const unsigned int bits = readU32(data);
    float value = 0.0f;
    static_assert(sizeof(value) == sizeof(bits), "wall sidecar requires 32-bit floats");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

inline double readF64(const unsigned char* data)
{
    const unsigned long long bits = readU64(data);
    double value = 0.0;
    static_assert(sizeof(value) == sizeof(bits), "projection sidecar requires 64-bit doubles");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

// --- context.cc: the window, its GL context and the game's frame quad ------

extern SDL_Window* gWindow;
extern SDL_GLContext gContext;
extern bool gInitialized;
extern int gTextureWidth;
extern int gTextureHeight;
extern int gLogicalWidth;
extern int gLogicalHeight;
extern int gWindowWidth;
extern int gWindowHeight;
// The game's own picture over the scene (drawSceneOverlay) is in the quad texture.
extern bool gSceneOverlayReady;

unsigned int compileShader(unsigned int type, const char* source);
// The game frame scaled into the window with aspect kept and letterboxed.
bool fitRect(int imageWidth, int imageHeight, int windowWidth, int windowHeight, int* outX, int* outY, int* outW, int* outH);
// `pixels` null: draw the texture the last call uploaded.
void drawQuad(int srcWidth, int srcHeight, int srcPitch, const void* pixels, int dstX, int dstY, int dstW, int dstH);

// --- scene_shader.cc: the program every scene pass draws with -------------

extern unsigned int gSceneProgram;
extern int gSceneMvpLocation;
extern int gSceneTextureLocation;
extern int gSceneTexturedLocation;
extern int gSceneColorLocation;
extern int gSceneCutoffLocation;
extern int gSceneDepthAtLocation;
extern int gSceneTranslucentLocation;
extern int gSceneTintLocation;
extern int gSceneGlassGrayLocation;
extern int gScenePickLocation;
// The see-through circle (roofs.cc): on/off, its mask (texture unit 1), its
// rect in game logical pixels, and the GL viewport and logical size to find
// a fragment's game pixel.
extern int gSceneEggLocation;
extern int gSceneEggMaskLocation;
extern int gSceneEggRectLocation;
extern int gSceneViewportLocation;
extern int gSceneLogicalLocation;
// The egg over walls and scenery the hero stands behind: which draws it cuts
// (uSeeThrough, per walls.bin object row from a texture on unit 2) and the pass.
extern int gSceneSeeThroughLocation;
extern int gSceneSeeThroughObjectsLocation;
extern int gSceneEggPassLocation;

bool createSceneShader(std::string* error);
void destroySceneShader();

// --- floor.cc ----------------------------------------------------------------

extern unsigned int gFloorVao;
extern unsigned int gFloorTexture;
extern int gFloorVertexCount;
extern bool gFloorLoaded;

void destroyStaticFloor();

// --- roofs.cc: roof squares from floor.bin, drawn over the scene ------------

struct RoofSquare {
    int index; // square index, y * 100 + x, as the engine's tileFid
    float x0, z0, x1, z1;
    float u0, v0, u1, v1;
};

void setRoofSquares(std::vector<RoofSquare> squares);
void destroyRoofs();
// With the scene program in use and the game view's matrix set.
void drawRoofs(int viewportX, int viewportY, int viewportWidth, int viewportHeight);
// Binds the egg's mask (unit 1) and sets its rect and the viewport; false when
// no egg is shown.
bool bindEgg(int viewportX, int viewportY, int viewportWidth, int viewportHeight);

// --- walls.cc: walls.bin and the objects it draws (baked.bin) ----------------

struct WallBatch {
    unsigned int page = 0;
    unsigned int triangleCount = 0;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int texture = 0;
};

extern std::vector<WallBatch> gWallBatches;
extern bool gWallsLoaded;
// Per walls.bin object row, whether the hero stands behind it (R8, 1024 rows
// wide), and whether any row is set.
extern unsigned int gWallSeeThroughTexture;
extern bool gWallSeeThroughAny;

void destroyStaticWalls();

// --- camera.cc: the game's projection (projection.bin) and the view center ---

struct HeroProjection {
    double squ[2] = {};
    double sqv[2] = {};
    double origin[2] = {};
    double hpx = 0.0;
    double bbPx = 0.0;
    double rowTilt = 0.0;
    double stack = 0.0;
};

extern bool gHeroProjectionLoaded;
extern HeroProjection gHeroProjection;

SpriteCardProjection cardProjection();
// The game's own projection centered on the camera's hex; false before both
// are known.
bool currentGameView(GameView* view);
// The fixed preview camera's matrix (floor-only overview).
void makeFloorMvp(float aspect, const StaticPreviewCamera& camera, float* out);
void resetCamera();

// --- live_objects.cc: live map objects and the hero, as cards ----------------

struct ObjectDraw {
    unsigned int texture;
    int firstVertex;
    bool flat;
    float tint[3]; // translucent look, tint[0] < 0: opaque
    bool glassGray;
    int pickId; // the caller's id for picking, -1: not picked
    bool pickOnly; // drawn by walls.bin; its card is only picked
    bool seeThrough; // the hero stands behind it: the egg cuts it
};

extern std::vector<ObjectDraw> gObjectDraws;
extern unsigned int gObjectVao;
extern unsigned int gHeroVao;
extern unsigned int gHeroTexture;
extern bool gHeroLoaded;
extern int gHeroPickId;

void destroyStaticHero();
void destroyStaticObjects();

} // namespace internal
} // namespace scene_renderer
} // namespace fallout

#endif /* FALLOUT_SCENE_RENDERER_SCENE_INTERNAL_H */
