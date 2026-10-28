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
constexpr unsigned int kLinear = 0x2601;
constexpr unsigned int kRed = 0x1903;
constexpr unsigned int kRG = 0x8227;
constexpr unsigned int kRG8 = 0x822B;
constexpr unsigned int kR8 = 0x8229;
constexpr unsigned int kRGBA16 = 0x805B;
constexpr unsigned int kUnsignedShort = 0x1403;
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
constexpr unsigned int kOne = 1;
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
// Draws an existing GL texture over the window's framebuffer.
void drawTextureQuad(unsigned int texture, int dstX, int dstY, int dstW, int dstH);

// --- scene_shader.cc: the program every scene pass draws with -------------

extern unsigned int gSceneProgram;
extern int gSceneMvpLocation;
extern int gSceneTextureLocation;
extern int gSceneTexturedLocation;
extern int gSceneCloseupFilterLocation;
extern int gSceneColorLocation;
extern int gSceneCutoffLocation;
extern int gSceneDepthAtLocation;
extern int gSceneTranslucentLocation;
extern int gSceneTintLocation;
extern int gSceneGlassGrayLocation;
extern int gScenePickLocation;
extern int gSceneClipEnabledLocation;
extern int gSceneClipPlaneLocations[4];
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
// The game's light (lighting.cc): how a draw is lit (uLightMode: 0 not, 1 the
// floor at each fragment, 2 walls at each fragment, by the lights they face,
// 3 sprites at their hex (uLightAnchor), 4 uLightValue), the ambient level,
// the point lights, their reach (texture unit 3) and the ground-to-game-pixel
// projection that finds a fragment's hex.
extern int gSceneLightModeLocation;
extern int gSceneLightReachLocation;
extern int gSceneLightMapLocation;
extern int gSceneLightValueLocation;
extern int gSceneLightSquLocation;
extern int gSceneLightSqvLocation;
extern int gSceneLightOriginLocation;
extern int gSceneLightAnchorLocation;
extern int gSceneAmbientLocation;
extern int gSceneHexUnitLocation;
extern int gSceneTowardCameraLocation;
extern int gSceneFlyEyeLocation;
extern int gSceneLightCountLocation;
extern int gSceneLightPosLocation;
extern int gSceneLightColorLocation;
extern int gSceneLightWindowLocation;
extern int gSceneSeeThroughLocation;
extern int gSceneSeeThroughObjectsLocation;
extern int gSceneEggPassLocation;
// The sun's shadow (shadow.cc): on, the world-to-map matrix, the depth map
// (unit 5) and its caster ids (unit 6), the sun's strength, the sun's
// direction, the AA ring's span in GL units and the compare bias.
extern int gSceneShadowOnLocation;
extern int gSceneShadowMatrixLocation;
extern int gSceneShadowMapLocation;
extern int gSceneShadowIdsLocation;
extern int gSceneShadowStrengthLocation;
extern int gSceneShadowDensityLocation;
extern int gSceneShadowLightLocation;
extern int gSceneShadowStepLocation;
extern int gSceneShadowBiasLocation;
extern int gSceneShadowContactLocation;
extern int gSceneShadowDebugLocation;
// 082 round 6: whose map texels count as empty at the compare (mechanism 2).
// The wall draws take the number from their own vertices (uSelfFromVertex 1),
// everything else reads uShadowSelfId; sprites also take their shadow factor
// at the feet point uShadowAnchor instead of per pixel (mechanism 3).
extern int gSceneShadowPassLocation;
extern int gSceneShadowSelfFromVertexLocation;
extern int gSceneShadowSelfIdLocation;
extern int gSceneShadowAnchorLocation;
// 090: x 1 when a low card reads the shadow per pixel on the ground it shows,
// y the height of that ground.
extern int gSceneShadowGroundLocation;
// 109: 1 when a card takes its whole shadow verdict at its feet point (the
// people); a standing card of any other kind reads it per pixel on its own
// plane.
extern int gSceneShadowFeetLocation;

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
// All roof squares (hidden ones included) as the shadow pass's casters; the
// shadow VAO roofs.cc owns, built on demand.
void roofShadowCasters(unsigned int* vao, int* vertexCount);
// Binds the egg's mask (unit 1) and sets its rect and the viewport; false when
// no egg is shown.
bool bindEgg(int viewportX, int viewportY, int viewportWidth, int viewportHeight);

// --- lighting.cc: the game's light as point lights -----------------------------

bool lightShown();
float lightAmbient();
// Sets the hex a sprite is lit at (uLightAnchor).
void setLightAnchor(int tile);
// The ground point a sprite's hex is lit at; false off the grid.
bool lightAnchorGround(int tile, double* x, double* z);
// Places the lights, uploads their reach (unit 3) and the light per hex
// (unit 4) if they changed and sets the light uniforms; false when the scene
// shows no light.
bool bindLight();
void destroyStaticLight();

// --- walls.cc: walls.bin and the objects it draws (baked.bin) ----------------

struct WallBatch {
    unsigned int page = 0;
    unsigned int triangleCount = 0;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int texture = 0;
    bool ownsTexture = false; // a page's second batch draws with its first one's texture
    bool flyOnly = false; // what the game's camera never sees (walls' backs and ends)
    bool gameOnly = false; // what the fly camera draws its own way (a drum's near half)
};

extern std::vector<WallBatch> gWallBatches;
extern bool gWallsLoaded;
// Per walls.bin object row, whether the hero stands behind it (R8, 1024 rows
// wide), and whether any row is set.
extern unsigned int gWallSeeThroughTexture;
extern bool gWallSeeThroughAny;
// Uploads the wall object table (R: see-through, G: the light of the object's
// hex) when it changed.
void flushWallObjectTable();

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
// The fly camera's matrix; false when it does not fly.
bool flyCameraMvp(float aspect, float* out);
// The fly camera's screen right along the floor (GL x, z); false when it does
// not fly.
bool flyCameraRight(double* x, double* z);
// The fly (or turned) camera's yaw round from the game's view, radians.
bool flyCameraTurn(double* radians);
// While the fly camera flies a standing card is seen from anywhere: it stands
// upright on the middle of its footprint and turns to face the camera, as in
// the donor's first person. The game's camera keeps the card it sees.
void faceFlyCamera(SpriteCardVertex corners[4]);
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
    int tile; // its hex, for its light; -1: full light
    float anchorX; // its feet on the ground (bottom-center of the card),
    float anchorZ; // for the whole-sprite sun shadow (082 round 6)
    // A low thing (lying flat, or a card under kLowCardHeight: straw, grass,
    // rubble) takes the sun shadow per pixel on the ground it shows (090).
    bool groundShadow = false;
    float groundY = 0.0f; // the height of that ground
    // A card standing in a wall's plane (a door): that plane's normal (x, z),
    // 0 for any other card. It takes its wall's side of the sun (090).
    float wallNormalX = 0.0f;
    float wallNormalZ = 0.0f;
    // 109: the people (OBJ_TYPE_CRITTER by the fid) keep the whole verdict at
    // the feet point; a standing card of any other kind reads the shadow per
    // pixel on its own plane.
    bool feetShadow = false;
};

// Below this height (GL units) a standing card is ground clutter, not a body.
constexpr float kLowCardHeight = 0.5f;

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
