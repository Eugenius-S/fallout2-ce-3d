#include "scene_gl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lodepng.h>

#include "gl_api.h"
#include "game_view.h"
#include "sprite_card.h"
#include "../tile.h"

namespace fallout {
namespace scene_renderer {

namespace {

constexpr const char* kVertexShader = R"(#version 330 core
layout(location = 0) in vec2 aPosition; // -1..1 quad corners
layout(location = 1) in vec2 aTexcoord;
uniform vec4 uRect; // xy = NDC top-left (y down), zw = NDC size
out vec2 vTexcoord;
void main() {
    vec2 ndc = uRect.xy + (aPosition * 0.5 + 0.5) * uRect.zw;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    vTexcoord = aTexcoord;
}
)";

constexpr const char* kFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
out vec4 outColor;
void main() {
    outColor = texture(uTexture, vTexcoord);
}
)";

constexpr const char* kFloorVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec3 aDepthAt; // the point whose depth this vertex takes
uniform mat4 uMvp;
uniform int uDepthAt;
out vec2 vTexcoord;
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vTexcoord = aTexcoord;
    if (uDepthAt != 0) {
        vec4 q = uMvp * vec4(aDepthAt, 1.0);
        gl_Position.z = q.z / q.w * gl_Position.w;
    }
}
)";

constexpr const char* kFloorFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
uniform int uTextured;
uniform vec4 uColor;
uniform float uCutoff;
uniform int uTranslucent; // the engine's translucent blend: steam, glass, energy, ...
uniform vec3 uTint;
uniform int uGlassGray;
uniform int uPick; // the pick pass: every texel shown is the object's id colour
out vec4 outColor;
void main() {
    if (uTextured == 0) {
        outColor = uColor;
    } else {
        vec4 texel = texture(uTexture, vTexcoord);
        if (texel.a < uCutoff) discard;
        if (uPick != 0) {
            outColor = uColor;
        } else if (uTranslucent != 0) {
            // _dark_translucent_trans_buf_to_buf: a texel's gray level (0..7,
            // from its RGB555 colour) mixes the tint over what lies behind, in
            // sevenths; level 0 leaves it unchanged.
            vec3 c = floor(texel.rgb * 255.0 / 8.0);
            float gray = uGlassGray != 0 ? floor(floor((c.r + 5.0 * c.g + 4.0 * c.b) / 10.0) / 4.0)
                                         : floor(floor((c.b + 3.0 * c.r + 6.0 * c.g) / 10.0) / 4.0);
            if (gray < 1.0) discard;
            outColor = vec4(uTint, min(gray, 7.0) / 7.0);
        } else {
            outColor = texel;
        }
    }
}
)";

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

SDL_Window* gWindow = nullptr;
SDL_GLContext gContext = nullptr;
bool gInitialized = false;
unsigned int gProgram = 0;
unsigned int gVao = 0;
unsigned int gVbo = 0;
unsigned int gTexture = 0;
int gTextureWidth = 0;
int gTextureHeight = 0;
int gLogicalWidth = 0;
int gLogicalHeight = 0;
int gWindowWidth = 0;
int gWindowHeight = 0;
int gTextureUniformLocation = -1;
int gRectUniformLocation = -1;

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

DepthVertex cardVertex(const SpriteCardVertex& corner, double row, const float eye[3])
{
    const double ray[3] = { corner.x - eye[0], corner.y - eye[1], corner.z - eye[2] };
    const double t = std::abs(ray[2]) > 1e-9 ? (row - corner.z) / ray[2] : 0.0;
    return { static_cast<float>(corner.x), static_cast<float>(corner.y), static_cast<float>(corner.z),
        static_cast<float>(corner.u), static_cast<float>(corner.v),
        static_cast<float>(corner.x + ray[0] * t), static_cast<float>(corner.y + ray[1] * t),
        static_cast<float>(row) };
}

void depthVertexLayout()
{
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(DepthVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(DepthVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, 0x1406, 0, sizeof(DepthVertex), reinterpret_cast<const void*>(5 * sizeof(float)));
}

struct WallBatch {
    unsigned int page = 0;
    unsigned int triangleCount = 0;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int texture = 0;
};

struct HeroProjection {
    double squ[2] = {};
    double sqv[2] = {};
    double origin[2] = {};
    double hpx = 0.0;
    double bbPx = 0.0;
    double rowTilt = 0.0;
    double stack = 0.0;
};

unsigned int gFloorProgram = 0;
unsigned int gFloorVao = 0;
unsigned int gFloorVbo = 0;
unsigned int gFloorTexture = 0;
int gFloorMvpLocation = -1;
int gFloorTextureLocation = -1;
int gFloorTexturedLocation = -1;
int gFloorColorLocation = -1;
int gFloorCutoffLocation = -1;
int gFloorDepthAtLocation = -1;
int gFloorTranslucentLocation = -1;
int gFloorTintLocation = -1;
int gFloorGlassGrayLocation = -1;
int gFloorPickLocation = -1;
int gFloorVertexCount = 0;
bool gFloorLoaded = false;
std::vector<WallBatch> gWallBatches;
unsigned int gWallTriangleCount = 0;
bool gWallsLoaded = false;
unsigned int gHeroVao = 0;
unsigned int gHeroVbo = 0;
unsigned int gHeroTexture = 0;
bool gHeroProjectionLoaded = false;
bool gHeroLoaded = false;
int gHeroPickId = -1;
HeroProjection gHeroProjection;
StaticHeroInfo gHeroInfo = {};
// The hex at the center of the game's screen (gCenterTile); with the hero
// projection loaded the wall/hero view is the game's own projection there.
int gCameraCenterTile = -1;
// The camera glides towards the game's view center instead of jumping a hex
// at a time: the game pixel at the screen center now, and when it last moved.
double gCameraPixel[2] = {};
Uint64 gCameraMoved = 0;
// The game's own picture over the scene (drawSceneOverlay) is in gTexture.
bool gSceneOverlayReady = false;

struct ObjectDraw {
    unsigned int texture;
    int firstVertex;
    bool flat;
    float tint[3]; // translucent look, tint[0] < 0: opaque
    bool glassGray;
    int pickId; // the caller's id for picking, -1: not picked
    bool pickOnly; // drawn by walls.bin; its card is only picked
};

struct BakedObject {
    bool lift; // false: walls.bin draws it, skip
    float offset[3]; // GL x, y, z when lifted
};

// Live map objects: one textured quad each, rebuilt every presented frame.
// Frame textures are cached by (fid, frame, rotation) across frames.
constexpr size_t kMaxObjectTextures = 4096;
constexpr float kFlatObjectHeight = 0.01f;
std::unordered_map<unsigned long long, BakedObject> gBakedObjects;
std::unordered_map<unsigned long long, unsigned int> gObjectTextures;
std::vector<DepthVertex> gObjectVertices;
std::vector<ObjectDraw> gObjectDraws;
unsigned int gObjectVao = 0;
unsigned int gObjectVbo = 0;

unsigned long long bakedObjectKey(int tile, int fid)
{
    return (static_cast<unsigned long long>(static_cast<unsigned int>(tile)) << 32) | static_cast<unsigned int>(fid);
}

unsigned long long objectArtKey(int fid, int frame, int rotation)
{
    return (static_cast<unsigned long long>(static_cast<unsigned int>(fid)) << 32)
        | ((static_cast<unsigned long long>(static_cast<unsigned int>(frame)) & 0xFFFFFF) << 8)
        | (static_cast<unsigned int>(rotation) & 0xFF);
}

bool readBinary(const std::filesystem::path& path, std::vector<unsigned char>& bytes)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) return false;
    const std::streamoff size = stream.tellg();
    if (size < 0 || size > 64 * 1024 * 1024) return false;
    bytes.resize(static_cast<size_t>(size));
    stream.seekg(0, std::ios::beg);
    return bytes.empty() || static_cast<bool>(stream.read(reinterpret_cast<char*>(bytes.data()), size));
}

unsigned short readU16(const unsigned char* data)
{
    return static_cast<unsigned short>(data[0] | (static_cast<unsigned short>(data[1]) << 8));
}

unsigned int readU32(const unsigned char* data)
{
    return static_cast<unsigned int>(data[0])
        | (static_cast<unsigned int>(data[1]) << 8)
        | (static_cast<unsigned int>(data[2]) << 16)
        | (static_cast<unsigned int>(data[3]) << 24);
}

unsigned long long readU64(const unsigned char* data)
{
    unsigned long long value = 0;
    for (int index = 0; index < 8; index++) {
        value |= static_cast<unsigned long long>(data[index]) << (index * 8);
    }
    return value;
}

float readF32(const unsigned char* data)
{
    const unsigned int bits = readU32(data);
    float value = 0.0f;
    static_assert(sizeof(value) == sizeof(bits), "wall sidecar requires 32-bit floats");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

double readF64(const unsigned char* data)
{
    const unsigned long long bits = readU64(data);
    double value = 0.0;
    static_assert(sizeof(value) == sizeof(bits), "projection sidecar requires 64-bit doubles");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void destroyStaticFloor()
{
    if (gContext != nullptr) {
        if (gFloorTexture != 0) glDeleteTextures(1, &gFloorTexture);
        if (gFloorVbo != 0) glDeleteBuffers(1, &gFloorVbo);
        if (gFloorVao != 0) glDeleteVertexArrays(1, &gFloorVao);
        if (gFloorProgram != 0) glDeleteProgram(gFloorProgram);
    }
    gFloorProgram = 0;
    gFloorVao = 0;
    gFloorVbo = 0;
    gFloorTexture = 0;
    gFloorMvpLocation = -1;
    gFloorTextureLocation = -1;
    gFloorTexturedLocation = -1;
    gFloorColorLocation = -1;
    gFloorCutoffLocation = -1;
    gFloorVertexCount = 0;
    gFloorLoaded = false;
}

void destroyStaticWalls()
{
    if (gContext != nullptr) {
        for (WallBatch& batch : gWallBatches) {
            if (batch.texture != 0) glDeleteTextures(1, &batch.texture);
            if (batch.vbo != 0) glDeleteBuffers(1, &batch.vbo);
            if (batch.vao != 0) glDeleteVertexArrays(1, &batch.vao);
        }
    }
    gWallBatches.clear();
    gWallTriangleCount = 0;
    gWallsLoaded = false;
    gBakedObjects.clear();
}

void destroyStaticHero()
{
    if (gContext != nullptr) {
        if (gHeroTexture != 0) glDeleteTextures(1, &gHeroTexture);
        if (gHeroVbo != 0) glDeleteBuffers(1, &gHeroVbo);
        if (gHeroVao != 0) glDeleteVertexArrays(1, &gHeroVao);
    }
    gHeroTexture = 0;
    gHeroVbo = 0;
    gHeroVao = 0;
    gHeroLoaded = false;
    gHeroInfo = {};
}

void destroyStaticObjectTextures()
{
    if (gContext != nullptr) {
        for (const auto& entry : gObjectTextures) {
            glDeleteTextures(1, &entry.second);
        }
    }
    gObjectTextures.clear();
}

void destroyStaticObjects()
{
    destroyStaticObjectTextures();
    if (gContext != nullptr) {
        if (gObjectVbo != 0) glDeleteBuffers(1, &gObjectVbo);
        if (gObjectVao != 0) glDeleteVertexArrays(1, &gObjectVao);
    }
    gObjectVbo = 0;
    gObjectVao = 0;
    gObjectVertices.clear();
    gObjectDraws.clear();
}

SpriteCardProjection cardProjection()
{
    return {
        { gHeroProjection.squ[0], gHeroProjection.squ[1] },
        { gHeroProjection.sqv[0], gHeroProjection.sqv[1] },
        { gHeroProjection.origin[0], gHeroProjection.origin[1] },
        gHeroProjection.hpx, gHeroProjection.bbPx, gHeroProjection.rowTilt, gHeroProjection.stack,
    };
}

void normalize3(float* v)
{
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length > 0.0f) {
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
    }
}

void multiply4x4(const float* a, const float* b, float* out)
{
    float result[16] = {};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            for (int k = 0; k < 4; k++) {
                result[col * 4 + row] += a[k * 4 + row] * b[col * 4 + k];
            }
        }
    }
    std::copy(result, result + 16, out);
}

void makeFloorMvp(float aspect, const StaticPreviewCamera& camera, float* out)
{
    constexpr float kPi = 3.14159265358979323846f;
    const float f = 1.0f / std::tan(camera.fovY * kPi / 360.0f);
    const float kNear = camera.nearPlane;
    const float kFar = camera.farPlane;
    float projection[16] = {};
    projection[0] = f / aspect;
    projection[5] = f;
    projection[10] = (kFar + kNear) / (kNear - kFar);
    projection[11] = -1.0f;
    projection[14] = (2.0f * kFar * kNear) / (kNear - kFar);

    // Fixed preview camera above the square floor, aimed at map center.
    const float* eye = camera.eye;
    const GroundPickCamera basisCamera = {
        { eye[0], eye[1], eye[2] },
        { camera.target[0], camera.target[1], camera.target[2] }, camera.fovY, camera.donorScreenRight,
    };
    double forwardD[3], sideD[3], cameraUpD[3];
    if (!previewCameraBasis(basisCamera, forwardD, sideD, cameraUpD)) {
        std::fill(out, out + 16, 0.0f);
        return;
    }
    float forward[3], side[3], cameraUp[3];
    for (int i = 0; i < 3; i++) {
        forward[i] = static_cast<float>(forwardD[i]);
        side[i] = static_cast<float>(sideD[i]);
        cameraUp[i] = static_cast<float>(cameraUpD[i]);
    }
    float view[16] = {
        side[0], cameraUp[0], -forward[0], 0.0f,
        side[1], cameraUp[1], -forward[1], 0.0f,
        side[2], cameraUp[2], -forward[2], 0.0f,
        -(side[0] * eye[0] + side[1] * eye[1] + side[2] * eye[2]),
        -(cameraUp[0] * eye[0] + cameraUp[1] * eye[1] + cameraUp[2] * eye[2]),
        forward[0] * eye[0] + forward[1] * eye[1] + forward[2] * eye[2],
        1.0f,
    };
    multiply4x4(projection, view, out);
}

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int compiled = 0;
    glGetShaderiv(shader, 0x8B81 /* GL_COMPILE_STATUS */, &compiled);
    if (!compiled) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "scene_renderer: shader compile failed: %s\n", log);
        fflush(stderr);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool fitRect(int imageWidth, int imageHeight, int windowWidth, int windowHeight, int* outX, int* outY, int* outW, int* outH)
{
    if (imageWidth <= 0 || imageHeight <= 0 || windowWidth <= 0 || windowHeight <= 0) {
        return false;
    }

    // SDL_RenderSetLogicalSize equivalent: scale to fit, center, letterbox.
    int w = windowWidth;
    int h = windowWidth * imageHeight / imageWidth;
    if (h > windowHeight) {
        h = windowHeight;
        w = windowHeight * imageWidth / imageHeight;
    }
    *outW = std::max(w, 1);
    *outH = std::max(h, 1);
    *outX = (windowWidth - *outW) / 2;
    *outY = (windowHeight - *outH) / 2;
    return true;
}

// `pixels` null: draw the texture the last call uploaded.
void drawQuad(int srcWidth, int srcHeight, int srcPitch, const void* pixels, int dstX, int dstY, int dstW, int dstH)
{
    int windowX;
    int windowY;
    int windowW;
    int windowH;
    if (!fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &windowX, &windowY, &windowW, &windowH)) {
        return;
    }

    bool recreate = pixels != nullptr && (srcWidth != gTextureWidth || srcHeight != gTextureHeight);
    if (recreate) {
        glBindTexture(kTexture2D, gTexture);
        glPixelStorei(kUnpackRowLength, srcPitch / 4);
        glTexImage2D(kTexture2D, 0, kRgba8, srcWidth, srcHeight, 0, kRGBA, kUnsignedByte, nullptr);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        gTextureWidth = srcWidth;
        gTextureHeight = srcHeight;
    }

    glBindTexture(kTexture2D, gTexture);
    if (pixels != nullptr) {
        glPixelStorei(kUnpackRowLength, srcPitch / 4);
        glTexSubImage2D(kTexture2D, 0, 0, 0, srcWidth, srcHeight, kRGBA, kUnsignedByte, pixels);
        glPixelStorei(kUnpackRowLength, 0);
    }

    float nx = static_cast<float>(windowX + dstX * windowW / gLogicalWidth);
    float ny = static_cast<float>(windowY + dstY * windowH / gLogicalHeight);
    float nw = static_cast<float>(dstW * windowW / gLogicalWidth);
    float nh = static_cast<float>(dstH * windowH / gLogicalHeight);

    float rect[4] = {
        2.0f * nx / gWindowWidth - 1.0f,
        2.0f * ny / gWindowHeight - 1.0f,
        2.0f * nw / gWindowWidth,
        2.0f * nh / gWindowHeight,
    };
    glUseProgram(gProgram);
    glActiveTexture(kTexture0);
    glUniform1i(gTextureUniformLocation, 0);
    glUniform4f(gRectUniformLocation, rect[0], rect[1], rect[2], rect[3]);
    glBindVertexArray(gVao);
    glDrawArrays(kTriangles, 0, 6);
}

} // namespace

bool init(SDL_Window* window, int logicalWidth, int logicalHeight)
{
    gWindow = window;
    gLogicalWidth = logicalWidth;
    gLogicalHeight = logicalHeight;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    gContext = SDL_GL_CreateContext(window);
    if (gContext == nullptr) {
        fprintf(stderr, "scene_renderer: SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        fflush(stderr);
        return false;
    }

    if (!glLoad()) {
        fprintf(stderr, "scene_renderer: OpenGL 3.3 entry points are missing\n");
        fflush(stderr);
        shutdown();
        return false;
    }

    // The vanilla SDL_Renderer was created without vsync; keep the same.
    SDL_GL_SetSwapInterval(0);

    unsigned int vertex = compileShader(0x8B31 /* GL_VERTEX_SHADER */, kVertexShader);
    unsigned int fragment = compileShader(0x8B30 /* GL_FRAGMENT_SHADER */, kFragmentShader);
    if (vertex == 0 || fragment == 0) {
        shutdown();
        return false;
    }

    gProgram = glCreateProgram();
    glAttachShader(gProgram, vertex);
    glAttachShader(gProgram, fragment);
    glLinkProgram(gProgram);
    int linked = 0;
    glGetProgramiv(gProgram, 0x8B82 /* GL_LINK_STATUS */, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(gProgram, sizeof(log), nullptr, log);
        fprintf(stderr, "scene_renderer: program link failed: %s\n", log);
        fflush(stderr);
        shutdown();
        return false;
    }

    gTextureUniformLocation = glGetUniformLocation(gProgram, "uTexture");
    gRectUniformLocation = glGetUniformLocation(gProgram, "uRect");

    // Fullscreen quad: two triangles, uv v=0 at the top edge.
    static const float quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &gVao);
    glBindVertexArray(gVao);
    glGenBuffers(1, &gVbo);
    glBindBuffer(kArrayBuffer, gVbo);
    glBufferData(kArrayBuffer, sizeof(quad), quad, kStaticDraw);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406 /* GL_FLOAT */, 0 /* GL_FALSE */, 4 * sizeof(float), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, 4 * sizeof(float), reinterpret_cast<const void*>(2 * sizeof(float)));

    glGenTextures(1, &gTexture);

    // The framebuffer size is physical pixels; SDL_GetWindowSize returns
    // DPI-virtualized points, which letterboxes the image into a corner.
    SDL_GL_GetDrawableSize(window, &gWindowWidth, &gWindowHeight);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    gInitialized = true;
    return true;
}

void shutdown()
{
    gInitialized = false;
    if (gContext != nullptr) {
        destroyStaticObjects();
        destroyStaticHero();
        destroyStaticWalls();
        destroyStaticFloor();
        if (gTexture != 0) {
            glDeleteTextures(1, &gTexture);
            gTexture = 0;
        }
        if (gVbo != 0) {
            glDeleteBuffers(1, &gVbo);
            gVbo = 0;
        }
        if (gVao != 0) {
            glDeleteVertexArrays(1, &gVao);
            gVao = 0;
        }
        if (gProgram != 0) {
            glDeleteProgram(gProgram);
            gProgram = 0;
        }
        SDL_GL_DeleteContext(gContext);
        gContext = nullptr;
    }
    gWindow = nullptr;
    gHeroProjectionLoaded = false;
    gHeroProjection = {};
    gCameraCenterTile = -1;
    gCameraMoved = 0;
}

bool isActive()
{
    return gInitialized;
}

// Eye distance of the game view (units): it sets how strong the perspective
// is. The 25-degree camera's 36.9 was asked milder twice, closer to the game's
// own parallel projection: 52, then 100.
double gameViewDistance()
{
    return 100.0;
}

bool currentGameView(GameView* view)
{
    if (!gHeroProjectionLoaded || gCameraCenterTile < 0) return false;
    // Whole screen pixels keep the texel grid steady while the camera glides.
    int x, y, width, height;
    double screenPerGame = 1.0;
    if (fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &x, &y, &width, &height)) {
        screenPerGame = static_cast<double>(width) / gLogicalWidth;
    }
    *view = {
        { gHeroProjection.squ[0], gHeroProjection.squ[1] },
        { gHeroProjection.sqv[0], gHeroProjection.sqv[1] },
        { gHeroProjection.origin[0], gHeroProjection.origin[1] },
        gHeroProjection.hpx,
        { std::round(gCameraPixel[0] * screenPerGame) / screenPerGame,
            std::round(gCameraPixel[1] * screenPerGame) / screenPerGame },
        gLogicalWidth, gLogicalHeight, gameViewDistance(),
    };
    return true;
}

StaticPreviewCamera staticPreviewCamera(bool withWalls)
{
    GroundPickCamera camera = staticPreviewCameraParameters(withWalls);
    GameView view;
    double eye[3], centerX, centerZ;
    if (withWalls && currentGameView(&view) && gameViewEye(view, eye) && gameViewCenter(view, &centerX, &centerZ)) {
        camera = { { eye[0], eye[1], eye[2] }, { centerX, 0.0, centerZ }, camera.fovY, camera.donorScreenRight };
    }
    return {
        { static_cast<float>(camera.eye[0]), static_cast<float>(camera.eye[1]), static_cast<float>(camera.eye[2]) },
        { static_cast<float>(camera.target[0]), static_cast<float>(camera.target[1]), static_cast<float>(camera.target[2]) },
        static_cast<float>(camera.fovY), 0.1f, 400.0f, camera.donorScreenRight,
    };
}

void staticWallStats(unsigned int* triangles, unsigned int* pages)
{
    if (triangles != nullptr) *triangles = gWallTriangleCount;
    if (pages != nullptr) *pages = static_cast<unsigned int>(gWallBatches.size());
}

bool pickStaticGroundTile(int logicalX, int logicalY, GroundPickResult* result)
{
    if (!gInitialized || !gHeroProjectionLoaded || gWindow == nullptr || result == nullptr) return false;
    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    int windowWidth = 0, windowHeight = 0;
    SDL_GetWindowSize(gWindow, &windowWidth, &windowHeight);
    const GroundPickProjection projection = {
        { gHeroProjection.squ[0], gHeroProjection.squ[1] },
        { gHeroProjection.sqv[0], gHeroProjection.sqv[1] },
        { gHeroProjection.origin[0], gHeroProjection.origin[1] },
    };
    const StaticPreviewCamera staticCamera = staticPreviewCamera(true);
    const GroundPickCamera camera = {
        { staticCamera.eye[0], staticCamera.eye[1], staticCamera.eye[2] },
        { staticCamera.target[0], staticCamera.target[1], staticCamera.target[2] }, staticCamera.fovY,
        staticCamera.donorScreenRight,
    };
    const GroundPickViewport viewport = { gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, windowWidth, windowHeight };
    GameView view;
    if (!currentGameView(&view)) return pickGroundTile(projection, camera, viewport, logicalX, logicalY, result);

    double worldX, worldZ;
    if (logicalX < 0 || logicalY < 0 || logicalX >= gLogicalWidth || logicalY >= gLogicalHeight
        || !gameViewPickGround(view, logicalX + 0.5, logicalY + 0.5, &worldX, &worldZ)) {
        return false;
    }
    const double u = worldX + 50.0;
    const double v = worldZ + 50.0;
    // This preview is the exported 100x100 Kladwtwn square map only.
    if (u < 0.0 || u >= 100.0 || v < 0.0 || v >= 100.0) return false;
    const double artX = projection.origin[0] + u * projection.squ[0] + v * projection.sqv[0];
    const double artY = projection.origin[1] + u * projection.squ[1] + v * projection.sqv[1];
    const int tile = donorHexFromPixel(artX, artY);
    if (tile < 0) return false;
    *result = { tile, worldX, worldZ, u, v, artX, artY };
    return true;
}

bool staticScreenFromGamePixel(double pixelX, double pixelY, double* logicalX, double* logicalY)
{
    GameView view;
    return currentGameView(&view) && gameViewProjectGroundPixel(view, pixelX, pixelY, logicalX, logicalY);
}

int pickStaticObjects(int logicalX, int logicalY, bool (*wanted)(int pickId, void* context), void* context,
    int* pickIds, int capacity)
{
    if (!gInitialized || !gFloorLoaded || !gWallsLoaded || pickIds == nullptr || capacity <= 0) return 0;
    if (logicalX < 0 || logicalY < 0 || logicalX >= gLogicalWidth || logicalY >= gLogicalHeight) return 0;
    GameView view;
    float mvp[16];
    if (!currentGameView(&view) || !gameViewMatrix(view, mvp)) return 0;
    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    int x, y, width, height;
    if (!fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &x, &y, &width, &height)) return 0;
    const int pixelX = x + static_cast<int>((logicalX + 0.5) * width / gLogicalWidth);
    const int pixelY = gWindowHeight - 1 - (y + static_cast<int>((logicalY + 0.5) * height / gLogicalHeight));

    // One pixel of the back buffer between frames (the next frame clears it
    // whole): the cards as drawn, each in its id's colour. Peeling off the
    // nearest and drawing again lists what lies under it, like the engine's
    // intersect list.
    constexpr unsigned int kScissorTest = 0x0C11;
    constexpr unsigned int kDither = 0x0BD0;
    glViewport(x, y, width, height);
    glEnable(kScissorTest);
    glScissor(pixelX, pixelY, 1, 1);
    glDisable(kDither);
    glDisable(kBlend);
    glDisable(kCullFace);
    glEnable(kDepthTest);
    glDepthFunc(kLess);
    glUseProgram(gFloorProgram);
    glUniformMatrix4fv(gFloorMvpLocation, 1, 0, mvp);
    glUniform1i(gFloorTexturedLocation, 1);
    glUniform1i(gFloorTranslucentLocation, 0);
    glUniform1i(gFloorPickLocation, 1);
    glUniform1f(gFloorCutoffLocation, 0.5f);
    glActiveTexture(kTexture0);
    glUniform1i(gFloorTextureLocation, 0);

    int count = 0;
    auto shown = [&](int id) {
        return id >= 0 && std::find(pickIds, pickIds + count, id) == pickIds + count
            && (wanted == nullptr || wanted(id, context));
    };
    auto paint = [&](int id) {
        const unsigned int code = static_cast<unsigned int>(id) + 1;
        glUniform4f(gFloorColorLocation, (code & 0xFF) / 255.0f, ((code >> 8) & 0xFF) / 255.0f,
            ((code >> 16) & 0xFF) / 255.0f, 1.0f);
    };
    while (count < capacity) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glDepthMask(1);
        glClear(kColorBufferBit | kDepthBufferBit);
        if (!gObjectDraws.empty() && gObjectVao != 0) {
            glBindVertexArray(gObjectVao);
            // Flat things lie under the rest; the rest hide by their rows' depth.
            glUniform1i(gFloorDepthAtLocation, 0);
            glDepthMask(0);
            for (const ObjectDraw& draw : gObjectDraws) {
                if (!draw.flat || !shown(draw.pickId)) continue;
                paint(draw.pickId);
                glBindTexture(kTexture2D, draw.texture);
                glDrawArrays(kTriangles, draw.firstVertex, 6);
            }
            glDepthMask(1);
            glUniform1i(gFloorDepthAtLocation, 1);
            for (const ObjectDraw& draw : gObjectDraws) {
                if (draw.flat || !shown(draw.pickId)) continue;
                paint(draw.pickId);
                glBindTexture(kTexture2D, draw.texture);
                glDrawArrays(kTriangles, draw.firstVertex, 6);
            }
        }
        if (gHeroLoaded && shown(gHeroPickId)) {
            glUniform1i(gFloorDepthAtLocation, 1);
            glBindVertexArray(gHeroVao);
            paint(gHeroPickId);
            glBindTexture(kTexture2D, gHeroTexture);
            glDrawArrays(kTriangles, 0, 6);
        }
        unsigned char pixel[4] = {};
        glPixelStorei(kPackAlignment, 1);
        glReadPixels(pixelX, pixelY, 1, 1, kRGBA, kUnsignedByte, pixel);
        glPixelStorei(kPackAlignment, kDefaultPackAlignment);
        const int code = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
        if (code == 0) break;
        pickIds[count++] = code - 1;
    }
    glUniform1i(gFloorPickLocation, 0);
    glUniform1i(gFloorDepthAtLocation, 0);
    glDisable(kScissorTest);
    glDisable(kDepthTest);
    glDepthMask(1);
    glBindVertexArray(0);
    glUseProgram(0);
    return count;
}

void setStaticCameraCenterTile(int tile)
{
    if (tile < 0 || tile >= HEX_GRID_SIZE) {
        gCameraCenterTile = -1;
        return;
    }
    double target[2];
    gameViewTilePixel(tile, target);
    const Uint64 now = SDL_GetPerformanceCounter();
    const bool first = gCameraCenterTile < 0 || gCameraMoved == 0;
    const double seconds = first ? 0.0 : static_cast<double>(now - gCameraMoved) / SDL_GetPerformanceFrequency();
    gameViewGlide(gCameraPixel, target, seconds, first);
    gCameraMoved = now;
    gCameraCenterTile = tile;
}

bool loadStaticProjection(const std::filesystem::path& cacheDirectory, std::string* error)
{
    if (gHeroProjectionLoaded) {
        if (error != nullptr) error->clear();
        return true;
    }
    auto fail = [&](const std::string& message) {
        gHeroProjectionLoaded = false;
        gHeroProjection = {};
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized) return fail("OpenGL renderer is unavailable");

    constexpr size_t kHeaderSize = 12;
    constexpr size_t kValueCount = 10;
    constexpr size_t kPayloadSize = kHeaderSize + kValueCount * sizeof(double);
    std::vector<unsigned char> bytes;
    if (!readBinary(cacheDirectory / "projection.bin", bytes)) {
        return fail("projection.bin is missing, unreadable, or exceeds 64 MB");
    }
    if (bytes.size() != kPayloadSize || bytes[0] != 'F' || bytes[1] != '2'
        || bytes[2] != 'P' || bytes[3] != 'J'
        || readU16(bytes.data() + 4) != 1 || readU16(bytes.data() + 6) != kHeaderSize
        || readU32(bytes.data() + 8) != kValueCount) {
        return fail("projection.bin has an invalid format or length");
    }
    double values[kValueCount];
    for (size_t index = 0; index < kValueCount; index++) {
        values[index] = readF64(bytes.data() + kHeaderSize + index * sizeof(double));
        if (!std::isfinite(values[index])) return fail("projection.bin contains a non-finite value");
    }
    const double determinant = values[0] * values[3] - values[2] * values[1];
    if (std::abs(determinant) < 1e-9
        || std::abs(values[0]) > 1000000.0 || std::abs(values[1]) > 1000000.0
        || std::abs(values[2]) > 1000000.0 || std::abs(values[3]) > 1000000.0
        || std::abs(values[4]) > 1000000.0 || std::abs(values[5]) > 1000000.0
        || values[6] < 1e-3 || values[6] > 4096.0
        || values[7] < 1e-3 || values[7] > 4096.0
        || values[8] < 0.0 || values[8] > 1.0
        || values[9] < 0.0 || values[9] > 1.0) {
        return fail("projection.bin values are out of bounds or not invertible");
    }
    gHeroProjection = { { values[0], values[1] }, { values[2], values[3] },
        { values[4], values[5] }, values[6], values[7], values[8], values[9] };
    gHeroProjectionLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

bool updateStaticHero(const StaticHeroFrame& frame, std::string* error)
{
    auto fail = [&](const std::string& message) {
        destroyStaticHero();
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gWallsLoaded || !gHeroProjectionLoaded) {
        return fail("static wall/hero projection is unavailable");
    }
    if (frame.elevation != 0 || frame.tile < 0 || frame.tile >= 200 * 200
        || frame.rotation < 0 || frame.rotation >= 6) {
        return fail("hero tile, elevation, or rotation is invalid");
    }
    if (frame.width <= 0 || frame.height <= 0 || frame.width > 2048 || frame.height > 2048
        || frame.objectX < -32768 || frame.objectX > 32768
        || frame.objectY < -32768 || frame.objectY > 32768
        || frame.artXOffset < -32768 || frame.artXOffset > 32768
        || frame.artYOffset < -32768 || frame.artYOffset > 32768
        || frame.rgba == nullptr) {
        return fail("hero frame dimensions or pixels are invalid");
    }
    const unsigned long long expectedRgba = static_cast<unsigned long long>(frame.width)
        * static_cast<unsigned long long>(frame.height) * 4;
    if (expectedRgba != frame.rgbaSize) return fail("hero frame RGBA byte count is inconsistent");

    const SpriteCardArt art = { frame.tile, frame.objectX, frame.objectY, frame.artXOffset, frame.artYOffset,
        frame.width, frame.height };
    SpriteCardVertex corners[4];
    double worldU = 0.0;
    double worldV = 0.0;
    if (!standingSpriteCard(cardProjection(), art, corners, &worldU, &worldV)) {
        return fail("hero card anchor or height is not finite");
    }
    const double row = spriteRowPlane(cardProjection(), frame.tile, frame.stack);
    const StaticPreviewCamera camera = staticPreviewCamera(true);
    const DepthVertex topLeft = cardVertex(corners[0], row, camera.eye);
    const DepthVertex bottomLeft = cardVertex(corners[1], row, camera.eye);
    const DepthVertex bottomRight = cardVertex(corners[2], row, camera.eye);
    const DepthVertex topRight = cardVertex(corners[3], row, camera.eye);
    const DepthVertex vertices[] = { topLeft, bottomLeft, bottomRight, topLeft, bottomRight, topRight };

    while (glGetError() != 0) {
    }
    glPixelStorei(kUnpackAlignment, 1);
    if (gHeroVao == 0) glGenVertexArrays(1, &gHeroVao);
    if (gHeroVbo == 0) glGenBuffers(1, &gHeroVbo);
    if (gHeroTexture == 0) glGenTextures(1, &gHeroTexture);
    glBindVertexArray(gHeroVao);
    glBindBuffer(kArrayBuffer, gHeroVbo);
    glBufferData(kArrayBuffer, sizeof(vertices), vertices, kStaticDraw);
    depthVertexLayout();
    glBindTexture(kTexture2D, gHeroTexture);
    glTexImage2D(kTexture2D, 0, kRgba8, frame.width, frame.height, 0, kRGBA, kUnsignedByte, frame.rgba);
    glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    glPixelStorei(kUnpackAlignment, 4);
    glBindVertexArray(0);
    if (glGetError() != 0) return fail("OpenGL failed to upload the hero card");

    gHeroInfo = { frame.tile, frame.fid, frame.frame, frame.rotation, worldU, worldV };
    gHeroPickId = frame.pickId;
    gHeroLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

void clearStaticHero()
{
    destroyStaticHero();
}

bool staticHeroInfo(StaticHeroInfo* info)
{
    if (info == nullptr || !gHeroLoaded) return false;
    *info = gHeroInfo;
    return true;
}

bool staticObjectBaked(int tile, int fid)
{
    const auto found = gBakedObjects.find(bakedObjectKey(tile, fid));
    return found != gBakedObjects.end() && !found->second.lift;
}

bool staticObjectLift(int tile, int fid, float lift[3])
{
    const auto found = gBakedObjects.find(bakedObjectKey(tile, fid));
    if (found == gBakedObjects.end() || !found->second.lift) return false;
    for (int axis = 0; axis < 3; axis++) lift[axis] = found->second.offset[axis];
    return true;
}

bool staticObjectArtCached(int fid, int frame, int rotation)
{
    return gObjectTextures.count(objectArtKey(fid, frame, rotation)) != 0;
}

void beginStaticObjects()
{
    gObjectVertices.clear();
    gObjectDraws.clear();
    // Bound the cache; a frame's texture is uploaded again on demand.
    if (gObjectTextures.size() > kMaxObjectTextures) destroyStaticObjectTextures();
}

bool addStaticObject(const StaticObjectFrame& frame, std::string* error)
{
    auto fail = [&](const char* message) {
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gWallsLoaded || !gHeroProjectionLoaded) return fail("static scene is unavailable");
    if (frame.rotation < 0 || frame.rotation >= 6) return fail("object rotation is invalid");
    const SpriteCardArt art = { frame.tile, frame.objectX, frame.objectY, frame.artXOffset, frame.artYOffset,
        frame.width, frame.height };
    SpriteCardVertex corners[4];
    if (frame.flat) {
        if (!flatSpriteCard(cardProjection(), art, kFlatObjectHeight, corners)) return fail("flat object card is invalid");
    } else if (!standingSpriteCard(cardProjection(), art, corners, nullptr, nullptr)) {
        return fail("object card is invalid");
    }
    const double row = frame.flat ? 0.0 : spriteRowPlane(cardProjection(), frame.tile, frame.stack);
    const StaticPreviewCamera camera = staticPreviewCamera(true);

    const unsigned long long key = objectArtKey(frame.fid, frame.frame, frame.rotation);
    auto cached = gObjectTextures.find(key);
    unsigned int texture = 0;
    if (cached != gObjectTextures.end()) {
        texture = cached->second;
    } else {
        const unsigned long long expectedRgba = static_cast<unsigned long long>(frame.width)
            * static_cast<unsigned long long>(frame.height) * 4;
        if (frame.rgba == nullptr || expectedRgba != frame.rgbaSize) return fail("object frame pixels are missing");
        while (glGetError() != 0) {
        }
        glGenTextures(1, &texture);
        glBindTexture(kTexture2D, texture);
        glPixelStorei(kUnpackAlignment, 1);
        glPixelStorei(kUnpackRowLength, 0);
        glTexImage2D(kTexture2D, 0, kRgba8, frame.width, frame.height, 0, kRGBA, kUnsignedByte, frame.rgba);
        glPixelStorei(kUnpackAlignment, 4);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        if (glGetError() != 0) {
            glDeleteTextures(1, &texture);
            return fail("OpenGL failed to upload an object frame");
        }
        gObjectTextures.emplace(key, texture);
    }

    gObjectDraws.push_back({ texture, static_cast<int>(gObjectVertices.size()), frame.flat,
        { frame.tint[0], frame.tint[1], frame.tint[2] }, frame.glassGray, frame.pickId, frame.pickOnly });
    // A thing standing on a solid moves onto it, and so does its row plane.
    float lift[3] = { 0.0f, 0.0f, 0.0f };
    const bool lifted = staticObjectLift(frame.tile, frame.fid, lift);
    const double liftedRow = row + lift[2];
    const int order[6] = { 0, 1, 2, 0, 2, 3 };
    for (int index : order) {
        SpriteCardVertex corner = corners[index];
        if (lifted) {
            corner.x += lift[0];
            corner.y += lift[1];
            corner.z += lift[2];
        }
        gObjectVertices.push_back(frame.flat
                ? DepthVertex { static_cast<float>(corner.x), static_cast<float>(corner.y), static_cast<float>(corner.z),
                      static_cast<float>(corner.u), static_cast<float>(corner.v), static_cast<float>(corner.x),
                      static_cast<float>(corner.y), static_cast<float>(corner.z) }
                : cardVertex(corner, liftedRow, camera.eye));
    }
    if (error != nullptr) error->clear();
    return true;
}

void endStaticObjects()
{
    if (!gInitialized || gObjectVertices.empty()) return;
    if (gObjectVao == 0) glGenVertexArrays(1, &gObjectVao);
    if (gObjectVbo == 0) glGenBuffers(1, &gObjectVbo);
    glBindVertexArray(gObjectVao);
    glBindBuffer(kArrayBuffer, gObjectVbo);
    glBufferData(kArrayBuffer, static_cast<long long>(gObjectVertices.size() * sizeof(DepthVertex)),
        gObjectVertices.data(), 0x88E8 /* GL_DYNAMIC_DRAW */);
    depthVertexLayout();
    glBindVertexArray(0);
}

void clearStaticObjects()
{
    destroyStaticObjects();
}

unsigned int staticObjectCount()
{
    return static_cast<unsigned int>(gObjectDraws.size());
}

void presentFrame(const unsigned char* rgba, int width, int height, int pitch)
{
    if (!gInitialized) {
        return;
    }

    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(kColorBufferBit);
    glDisable(kBlend);
    drawQuad(width, height, pitch, rgba, 0, 0, gLogicalWidth, gLogicalHeight);
    gSceneOverlayReady = false;
}

void drawSceneOverlay(const unsigned char* rgba, int width, int height, int pitch)
{
    if (!gInitialized || rgba == nullptr || width <= 0 || height <= 0) return;
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    drawOverlay(rgba, width, height, pitch, 0, 0, gLogicalWidth, gLogicalHeight);
    gSceneOverlayReady = true;
}

void drawOverlay(const unsigned char* rgba, int width, int height, int pitch, int dstX, int dstY, int dstW, int dstH)
{
    if (!gInitialized || rgba == nullptr || width <= 0 || height <= 0) {
        return;
    }

    glEnable(kBlend);
    glBlendFunc(1 /* GL_ONE */, kOneMinusSrcAlpha);
    drawQuad(width, height, pitch, rgba, dstX, dstY, dstW, dstH);
    glDisable(kBlend);
}

void swap()
{
    if (!gInitialized) {
        return;
    }
    SDL_GL_SwapWindow(gWindow);
}

bool loadStaticFloor(const std::filesystem::path& cacheDirectory, std::string* error)
{
    if (gFloorLoaded) return true;
    auto fail = [&](const std::string& message) {
        destroyStaticFloor();
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized) return fail("OpenGL renderer is unavailable");

    std::vector<unsigned char> sidecar;
    std::vector<unsigned char> png;
    if (!readBinary(cacheDirectory / "floor.bin", sidecar)) {
        return fail("floor.bin is missing or unreadable");
    }
    if (!readBinary(cacheDirectory / "tiles.png", png)) {
        return fail("tiles.png is missing or unreadable");
    }
    constexpr size_t kHeaderSize = 20;
    constexpr size_t kFloorBytes = 100 * 100 * 2;
    if (sidecar.size() < kHeaderSize + kFloorBytes
        || sidecar[0] != 'F' || sidecar[1] != '2' || sidecar[2] != 'F' || sidecar[3] != 'L') {
        return fail("floor.bin has an invalid header");
    }
    const unsigned short version = readU16(sidecar.data() + 4);
    const unsigned short gridWidth = readU16(sidecar.data() + 6);
    const unsigned short gridHeight = readU16(sidecar.data() + 8);
    const unsigned short tileCount = readU16(sidecar.data() + 10);
    const unsigned short imageWidth = readU16(sidecar.data() + 12);
    const unsigned short imageHeight = readU16(sidecar.data() + 14);
    const unsigned short cellSize = readU16(sidecar.data() + 16);
    const unsigned short cellMargin = readU16(sidecar.data() + 18);
    const size_t expectedSize = kHeaderSize + kFloorBytes + static_cast<size_t>(tileCount) * 4;
    if (version != 1 || gridWidth != 100 || gridHeight != 100 || tileCount == 0
        || cellSize != 88 || cellMargin != 6 || imageWidth == 0 || imageHeight == 0
        || imageWidth > 8192 || imageHeight > 8192
        || sidecar.size() != expectedSize) {
        return fail("floor.bin version, dimensions, or length is unsupported");
    }

    std::vector<unsigned char> rgba;
    unsigned pngWidth = 0;
    unsigned pngHeight = 0;
    const unsigned pngError = lodepng::decode(rgba, pngWidth, pngHeight, png);
    if (pngError != 0) {
        return fail(std::string("tiles.png decode failed: ") + lodepng_error_text(pngError));
    }
    if (pngWidth != imageWidth || pngHeight != imageHeight) {
        return fail("tiles.png dimensions do not match floor.bin");
    }

    std::vector<FloorVertex> vertices;
    vertices.reserve(6 + 10000 * 6);
    auto quad = [&](float x0, float y, float z0, float x1, float z1,
                    float u0, float v0, float u1, float v1) {
        vertices.push_back({ x0, y, z0, u0, v0 });
        vertices.push_back({ x1, y, z0, u1, v0 });
        vertices.push_back({ x1, y, z1, u1, v1 });
        vertices.push_back({ x0, y, z0, u0, v0 });
        vertices.push_back({ x1, y, z1, u1, v1 });
        vertices.push_back({ x0, y, z1, u0, v1 });
    };
    constexpr float kHalfMap = 50.0f;
    quad(-kHalfMap, -0.08f, -kHalfMap, kHalfMap, kHalfMap, 0.0f, 0.0f, 1.0f, 1.0f);
    const unsigned char* floorData = sidecar.data() + kHeaderSize;
    const unsigned char* cellData = floorData + kFloorBytes;
    for (int row = 0; row < 100; row++) {
        for (int col = 0; col < 100; col++) {
            const unsigned short cellIndex = readU16(floorData + (row * 100 + col) * 2);
            if (cellIndex == 0) continue;
            if (cellIndex > tileCount) return fail("floor.bin contains an out-of-range cell index");
            const unsigned char* cell = cellData + (cellIndex - 1) * 4;
            const unsigned short cellX = readU16(cell);
            const unsigned short cellY = readU16(cell + 2);
            if (cellX + cellSize + 2 * cellMargin > imageWidth
                || cellY + cellSize + 2 * cellMargin > imageHeight) {
                return fail("floor.bin contains a tile cell outside the atlas");
            }
            // The whole cell, margin included: tile art overhangs its square,
            // and later squares paint over their neighbours' rim, as the engine
            // and the donor viewer draw them (cut to the square, the rim's
            // transparent texels showed a grid of seams).
            const float grow = static_cast<float>(cellMargin) / cellSize;
            const float u0 = static_cast<float>(cellX) / imageWidth;
            const float v0 = static_cast<float>(cellY) / imageHeight;
            const float u1 = static_cast<float>(cellX + cellSize + 2 * cellMargin) / imageWidth;
            const float v1 = static_cast<float>(cellY + cellSize + 2 * cellMargin) / imageHeight;
            const float x0 = static_cast<float>(col) - kHalfMap;
            const float z0 = static_cast<float>(row) - kHalfMap;
            quad(x0 - grow, 0.0f, z0 - grow, x0 + 1.0f + grow, z0 + 1.0f + grow, u0, v0, u1, v1);
        }
    }

    unsigned int vertexShader = compileShader(0x8B31 /* GL_VERTEX_SHADER */, kFloorVertexShader);
    unsigned int fragmentShader = compileShader(0x8B30 /* GL_FRAGMENT_SHADER */, kFloorFragmentShader);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return fail("static floor shader compilation failed");
    }
    gFloorProgram = glCreateProgram();
    glAttachShader(gFloorProgram, vertexShader);
    glAttachShader(gFloorProgram, fragmentShader);
    glLinkProgram(gFloorProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    int linked = 0;
    glGetProgramiv(gFloorProgram, 0x8B82 /* GL_LINK_STATUS */, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(gFloorProgram, sizeof(log), nullptr, log);
        fprintf(stderr, "scene_renderer: static floor link failed: %s\n", log);
        fflush(stderr);
        return fail("static floor shader link failed");
    }

    glGenVertexArrays(1, &gFloorVao);
    glBindVertexArray(gFloorVao);
    glGenBuffers(1, &gFloorVbo);
    glBindBuffer(kArrayBuffer, gFloorVbo);
    glBufferData(kArrayBuffer, static_cast<long long>(vertices.size() * sizeof(FloorVertex)),
        vertices.data(), kStaticDraw);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
    glGenTextures(1, &gFloorTexture);
    glBindTexture(kTexture2D, gFloorTexture);
    glPixelStorei(kUnpackRowLength, 0);
    glTexImage2D(kTexture2D, 0, kRgba8, pngWidth, pngHeight, 0, kRGBA, kUnsignedByte, rgba.data());
    glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    glBindVertexArray(0);

    gFloorMvpLocation = glGetUniformLocation(gFloorProgram, "uMvp");
    gFloorTextureLocation = glGetUniformLocation(gFloorProgram, "uTexture");
    gFloorTexturedLocation = glGetUniformLocation(gFloorProgram, "uTextured");
    gFloorColorLocation = glGetUniformLocation(gFloorProgram, "uColor");
    gFloorCutoffLocation = glGetUniformLocation(gFloorProgram, "uCutoff");
    gFloorDepthAtLocation = glGetUniformLocation(gFloorProgram, "uDepthAt");
    gFloorTranslucentLocation = glGetUniformLocation(gFloorProgram, "uTranslucent");
    gFloorTintLocation = glGetUniformLocation(gFloorProgram, "uTint");
    gFloorGlassGrayLocation = glGetUniformLocation(gFloorProgram, "uGlassGray");
    gFloorPickLocation = glGetUniformLocation(gFloorProgram, "uPick");
    if (gFloorMvpLocation < 0 || gFloorTextureLocation < 0
        || gFloorTexturedLocation < 0 || gFloorColorLocation < 0 || gFloorCutoffLocation < 0
        || gFloorDepthAtLocation < 0 || gFloorTranslucentLocation < 0 || gFloorTintLocation < 0
        || gFloorGlassGrayLocation < 0 || gFloorPickLocation < 0) {
        return fail("static floor shader uniforms are missing");
    }
    gFloorVertexCount = static_cast<int>(vertices.size());
    gFloorLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

bool loadStaticWalls(const std::filesystem::path& cacheDirectory, std::string* error)
{
    if (gWallsLoaded) return true;
    destroyStaticWalls();
    auto fail = [&](const std::string& message) {
        destroyStaticWalls();
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gFloorLoaded) return fail("static floor renderer is unavailable");

    constexpr size_t kHeaderSize = 20;
    constexpr size_t kBatchSize = 16;
    constexpr size_t kVertexSize = 32;
    constexpr size_t kMaxSidecarBytes = 64 * 1024 * 1024;
    constexpr unsigned int kMaxPages = 16;
    constexpr unsigned int kMaxTriangles = 1000000;
    constexpr unsigned int kMaxImageDimension = 8192;
    std::vector<unsigned char> sidecar;
    if (!readBinary(cacheDirectory / "walls.bin", sidecar) || sidecar.size() > kMaxSidecarBytes) {
        return fail("walls.bin is missing, unreadable, or exceeds 64 MB");
    }
    if (sidecar.size() < kHeaderSize
        || sidecar[0] != 'F' || sidecar[1] != '2' || sidecar[2] != 'W' || sidecar[3] != 'L') {
        return fail("walls.bin has an invalid header");
    }
    const unsigned short version = readU16(sidecar.data() + 4);
    const unsigned short headerSize = readU16(sidecar.data() + 6);
    const unsigned int pageCount = readU32(sidecar.data() + 8);
    const unsigned int batchCount = readU32(sidecar.data() + 12);
    const unsigned int totalTriangles = readU32(sidecar.data() + 16);
    if (version != 2 || headerSize != kHeaderSize || pageCount == 0 || pageCount > kMaxPages
        || batchCount == 0 || batchCount > pageCount || totalTriangles == 0 || totalTriangles > kMaxTriangles) {
        return fail("walls.bin header counts or version are unsupported");
    }

    struct WallBatchData {
        unsigned int page;
        unsigned int triangleCount;
        unsigned int width;
        unsigned int height;
        std::vector<DepthVertex> vertices;
        std::vector<unsigned char> rgba;
    };
    std::vector<WallBatchData> batches;
    batches.reserve(batchCount);
    std::vector<bool> seenPages(pageCount, false);
    size_t offset = kHeaderSize;
    unsigned long long observedTriangles = 0;
    unsigned long long totalDecodedBytes = 0;
    for (unsigned int batchIndex = 0; batchIndex < batchCount; batchIndex++) {
        if (offset > sidecar.size() || sidecar.size() - offset < kBatchSize) {
            return fail("walls.bin has a truncated batch header");
        }
        const unsigned int page = readU32(sidecar.data() + offset);
        const unsigned int width = readU32(sidecar.data() + offset + 4);
        const unsigned int height = readU32(sidecar.data() + offset + 8);
        const unsigned int triangleCount = readU32(sidecar.data() + offset + 12);
        offset += kBatchSize;
        if (page >= pageCount || seenPages[page] || width == 0 || height == 0
            || width > kMaxImageDimension || height > kMaxImageDimension
            || triangleCount == 0 || triangleCount > kMaxTriangles) {
            return fail("walls.bin contains an invalid page batch");
        }
        seenPages[page] = true;
        const unsigned long long vertexCount = static_cast<unsigned long long>(triangleCount) * 3;
        const unsigned long long vertexBytes = vertexCount * kVertexSize;
        if (vertexBytes > sidecar.size() - offset) {
            return fail("walls.bin has truncated triangle data");
        }
        observedTriangles += triangleCount;
        if (observedTriangles > totalTriangles || observedTriangles > kMaxTriangles) {
            return fail("walls.bin triangle counts overflow the header");
        }
        WallBatchData batch;
        batch.page = page;
        batch.triangleCount = triangleCount;
        batch.width = width;
        batch.height = height;
        batch.vertices.resize(static_cast<size_t>(vertexCount));
        for (size_t vertexIndex = 0; vertexIndex < batch.vertices.size(); vertexIndex++) {
            const unsigned char* vertexData = sidecar.data() + offset + vertexIndex * kVertexSize;
            DepthVertex& vertex = batch.vertices[vertexIndex];
            vertex.x = readF32(vertexData);
            vertex.y = readF32(vertexData + 4);
            vertex.z = readF32(vertexData + 8);
            vertex.u = readF32(vertexData + 12);
            vertex.v = readF32(vertexData + 16);
            vertex.depthX = readF32(vertexData + 20);
            vertex.depthY = readF32(vertexData + 24);
            vertex.depthZ = readF32(vertexData + 28);
            if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z)
                || !std::isfinite(vertex.u) || !std::isfinite(vertex.v) || !std::isfinite(vertex.depthX)
                || !std::isfinite(vertex.depthY) || !std::isfinite(vertex.depthZ)
                || vertex.u < 0.0f || vertex.u > 1.0f || vertex.v < 0.0f || vertex.v > 1.0f) {
                return fail("walls.bin contains non-finite positions or out-of-range UVs");
            }
        }
        offset += static_cast<size_t>(vertexBytes);

        char textureName[32];
        snprintf(textureName, sizeof(textureName), "sprites%u.png", page);
        const std::filesystem::path texturePath = cacheDirectory / textureName;
        std::vector<unsigned char> png;
        if (!readBinary(texturePath, png)) {
            return fail(std::string(textureName) + " is missing, unreadable, or exceeds 64 MB");
        }
        if (png.empty()) {
            return fail(std::string(textureName) + " is empty");
        }
        lodepng::State pngState;
        unsigned pngWidth = 0;
        unsigned pngHeight = 0;
        unsigned pngError = lodepng_inspect(&pngWidth, &pngHeight, &pngState, png.data(), png.size());
        if (pngError != 0) {
            return fail(std::string(textureName) + " PNG header is invalid: " + lodepng_error_text(pngError));
        }
        if (pngWidth == 0 || pngHeight == 0 || pngWidth > kMaxImageDimension || pngHeight > kMaxImageDimension
            || pngWidth != width || pngHeight != height) {
            return fail(std::string(textureName) + " dimensions do not match walls.bin batch");
        }
        const unsigned long long decodedBytes = static_cast<unsigned long long>(pngWidth) * pngHeight * 4;
        totalDecodedBytes += decodedBytes;
        if (decodedBytes > 256ULL * 1024 * 1024 || totalDecodedBytes > 512ULL * 1024 * 1024) {
            return fail(std::string(textureName) + " decoded size is too large");
        }
        pngError = lodepng::decode(batch.rgba, pngWidth, pngHeight, pngState, png);
        if (pngError != 0) {
            return fail(std::string(textureName) + " decode failed: " + lodepng_error_text(pngError));
        }
        if (batch.rgba.size() != decodedBytes) {
            return fail(std::string(textureName) + " decoded byte count is inconsistent");
        }
        for (size_t alphaOffset = 3; alphaOffset < batch.rgba.size(); alphaOffset += 4) {
            if (batch.rgba[alphaOffset] != 0 && batch.rgba[alphaOffset] != 255) {
                return fail(std::string(textureName) + " alpha is not binary cutout data");
            }
        }
        batches.push_back(std::move(batch));
    }
    if (offset != sidecar.size() || observedTriangles != totalTriangles) {
        return fail("walls.bin has trailing data or a mismatched triangle count");
    }

    // Objects walls.bin draws; live sprite cards skip them.
    constexpr size_t kBakedHeaderSize = 12;
    constexpr size_t kBakedEntrySize = 24;
    constexpr unsigned int kMaxBakedEntries = 1000000;
    std::vector<unsigned char> baked;
    if (!readBinary(cacheDirectory / "baked.bin", baked)) {
        return fail("baked.bin is missing or unreadable; re-run the scene exporter");
    }
    if (baked.size() < kBakedHeaderSize || baked[0] != 'F' || baked[1] != '2' || baked[2] != 'B' || baked[3] != 'K'
        || readU16(baked.data() + 4) != 2 || readU16(baked.data() + 6) != kBakedHeaderSize) {
        return fail("baked.bin has an invalid header");
    }
    const unsigned int bakedCount = readU32(baked.data() + 8);
    if (bakedCount > kMaxBakedEntries || baked.size() != kBakedHeaderSize + bakedCount * kBakedEntrySize) {
        return fail("baked.bin has a bad length");
    }
    std::unordered_map<unsigned long long, BakedObject> bakedObjects;
    for (unsigned int index = 0; index < bakedCount; index++) {
        const unsigned char* entry = baked.data() + kBakedHeaderSize + index * kBakedEntrySize;
        const int tile = static_cast<int>(readU32(entry));
        const unsigned int mode = readU32(entry + 8);
        const float du = readF32(entry + 12);
        const float dv = readF32(entry + 16);
        const float dz = readF32(entry + 20);
        if (tile < 0 || tile >= HEX_GRID_SIZE) return fail("baked.bin names a tile outside the hex grid");
        if (mode > 1 || !std::isfinite(du) || !std::isfinite(dv) || !std::isfinite(dz)
            || std::abs(du) > 10.0f || std::abs(dv) > 10.0f || std::abs(dz) > 10.0f) {
            return fail("baked.bin has an unknown mode or a bad lift");
        }
        // Lifts are donor (u, v, z); GL is (u, z, v).
        bakedObjects[bakedObjectKey(tile, static_cast<int>(readU32(entry + 4)))] = { mode == 1, { du, dz, dv } };
    }

    while (glGetError() != 0) {
    }
    glPixelStorei(kUnpackAlignment, 1);
    for (const WallBatchData& source : batches) {
        gWallBatches.emplace_back();
        WallBatch& batch = gWallBatches.back();
        batch.page = source.page;
        batch.triangleCount = source.triangleCount;
        glGenVertexArrays(1, &batch.vao);
        glBindVertexArray(batch.vao);
        glGenBuffers(1, &batch.vbo);
        glBindBuffer(kArrayBuffer, batch.vbo);
        glBufferData(kArrayBuffer,
            static_cast<long long>(source.vertices.size() * sizeof(DepthVertex)), source.vertices.data(), kStaticDraw);
        depthVertexLayout();
        glGenTextures(1, &batch.texture);
        glBindTexture(kTexture2D, batch.texture);
        glTexImage2D(kTexture2D, 0, kRgba8, source.width, source.height, 0, kRGBA, kUnsignedByte, source.rgba.data());
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        if (glGetError() != 0) {
            glPixelStorei(kUnpackAlignment, 4);
            return fail("OpenGL failed to upload a wall atlas batch");
        }
    }
    glPixelStorei(kUnpackAlignment, 4);
    glBindVertexArray(0);
    gWallTriangleCount = totalTriangles;
    gBakedObjects = std::move(bakedObjects);
    gWallsLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

void drawStaticPreview(bool withWalls, bool withHero);

void drawStaticFloor()
{
    drawStaticPreview(false, false);
}

void drawStaticWallPreview()
{
    drawStaticPreview(true, false);
}

void drawStaticHeroPreview()
{
    drawStaticPreview(true, true);
}

void drawStaticPreview(bool withWalls, bool withHero)
{
    if (!gInitialized || !gFloorLoaded || (withWalls && !gWallsLoaded)) return;
    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
    glClear(kColorBufferBit | kDepthBufferBit);
    int x, y, width, height;
    if (!fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &x, &y, &width, &height)) return;
    glViewport(x, y, width, height);

    float mvp[16];
    const StaticPreviewCamera camera = staticPreviewCamera(withWalls);
    GameView view;
    if (!withWalls || !currentGameView(&view) || !gameViewMatrix(view, mvp)) {
        makeFloorMvp(static_cast<float>(width) / height, camera, mvp);
    }
    glEnable(kDepthTest);
    glDepthFunc(kLess);
    glDepthMask(1);
    glDisable(kCullFace);
    if (withWalls) {
        glDisable(kBlend);
    } else {
        glEnable(kBlend);
        glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    }
    glUseProgram(gFloorProgram);
    glUniformMatrix4fv(gFloorMvpLocation, 1, 0, mvp);
    glUniform1i(gFloorDepthAtLocation, 0);
    glUniform1i(gFloorTranslucentLocation, 0);
    glUniform1i(gFloorPickLocation, 0);
    glActiveTexture(kTexture0);
    glUniform1i(gFloorTextureLocation, 0);
    glBindVertexArray(gFloorVao);
    glBindTexture(kTexture2D, gFloorTexture);

    // Nothing lies below the floor, so it is painted first and hides nothing
    // (the donor's game view): cards and solids take the depth of their hex
    // row's plane, which can lie under the floor along the view ray.
    glDepthMask(0);
    glUniform1i(gFloorTexturedLocation, 0);
    glUniform1f(gFloorCutoffLocation, 0.01f);
    glUniform4f(gFloorColorLocation, 0.16f, 0.17f, 0.18f, 1.0f);
    glDrawArrays(kTriangles, 0, 6);
    glUniform1i(gFloorTexturedLocation, 1);
    glUniform1f(gFloorCutoffLocation, 0.01f);
    glDrawArrays(kTriangles, 6, gFloorVertexCount - 6);
    glDepthMask(1);

    const bool withObjects = withHero && !gObjectDraws.empty() && gObjectVao != 0;
    if (withObjects) {
        // Flat objects lie on the floor in the engine's draw order: later ones
        // paint over earlier ones, and they hide nothing drawn after them.
        glDepthMask(0);
        glUniform1f(gFloorCutoffLocation, 0.5f);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || !draw.flat || draw.tint[0] >= 0.0f) continue;
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
        glDepthMask(1);
    }

    glUniform1i(gFloorDepthAtLocation, 1);
    if (withWalls) {
        // Wall faces are seen from outside only, as in the donor's game view.
        glEnable(kCullFace);
        glUniform1f(gFloorCutoffLocation, 0.5f);
        for (const WallBatch& batch : gWallBatches) {
            glBindVertexArray(batch.vao);
            glBindTexture(kTexture2D, batch.texture);
            glDrawArrays(kTriangles, 0, static_cast<int>(batch.triangleCount * 3));
        }
        glDisable(kCullFace);
    }
    if (withObjects) {
        glUniform1f(gFloorCutoffLocation, 0.5f);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || draw.flat || draw.tint[0] >= 0.0f) continue;
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
    }
    if (withHero && gHeroLoaded) {
        glUniform1f(gFloorCutoffLocation, 0.5f);
        glBindVertexArray(gHeroVao);
        glBindTexture(kTexture2D, gHeroTexture);
        glDrawArrays(kTriangles, 0, 6);
    }
    if (withObjects) {
        glEnable(kBlend);
        glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
        glDepthMask(0);
        glUniform1i(gFloorTranslucentLocation, 1);
        glUniform1f(gFloorCutoffLocation, 0.5f);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || draw.tint[0] < 0.0f) continue;
            glUniform3f(gFloorTintLocation, draw.tint[0], draw.tint[1], draw.tint[2]);
            glUniform1i(gFloorGlassGrayLocation, draw.glassGray ? 1 : 0);
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
        glUniform1i(gFloorTranslucentLocation, 0);
        glDepthMask(1);
        glDisable(kBlend);
    }
    glUniform1i(gFloorDepthAtLocation, 0);

    glDisable(kBlend);
    glDisable(kDepthTest);
    glDepthMask(1);
    glBindVertexArray(0);
    glUseProgram(0);
}

bool captureStaticFloorPng(const std::filesystem::path& path, int* width, int* height, std::string* error)
{
    return captureStaticPreviewPng(path, false, false, width, height, error);
}

bool captureStaticPreviewPng(const std::filesystem::path& path, bool withWalls, bool withHero,
    int* width, int* height, std::string* error)
{
    auto fail = [&](const std::string& message) {
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gFloorLoaded) return fail("static floor preview is not loaded");
    if (withWalls && !gWallsLoaded) return fail("static wall preview is not loaded");
    if (withHero && !gHeroLoaded) return fail("current hero frame is unavailable");
    std::error_code existsError;
    if (std::filesystem::exists(path, existsError)) return fail("snapshot path already exists");
    if (existsError) return fail("cannot inspect snapshot path");

    int drawableWidth = 0;
    int drawableHeight = 0;
    SDL_GL_GetDrawableSize(gWindow, &drawableWidth, &drawableHeight);
    if (drawableWidth <= 0 || drawableHeight <= 0) return fail("OpenGL drawable has no pixels");

    // Read only our preview render. Do not swap or touch game/map state.
    while (glGetError() != 0) {
    }
    drawStaticPreview(withWalls, withHero);
    if (withHero && gSceneOverlayReady) {
        // With what the game draws over it, as presented.
        glViewport(0, 0, drawableWidth, drawableHeight);
        glEnable(kBlend);
        glBlendFunc(1 /* GL_ONE */, kOneMinusSrcAlpha);
        drawQuad(gTextureWidth, gTextureHeight, gTextureWidth * 4, nullptr, 0, 0, gLogicalWidth, gLogicalHeight);
        glDisable(kBlend);
    }
    if (glGetError() != 0) return fail(withWalls
        ? "static wall draw failed before snapshot" : "static floor draw failed before snapshot");

    const size_t rowBytes = static_cast<size_t>(drawableWidth) * 4;
    std::vector<unsigned char> bottomUp(rowBytes * static_cast<size_t>(drawableHeight));
    std::vector<unsigned char> topDown(bottomUp.size());
    glPixelStorei(kPackAlignment, 1);
    glReadPixels(0, 0, drawableWidth, drawableHeight, kRGBA, kUnsignedByte, bottomUp.data());
    glPixelStorei(kPackAlignment, kDefaultPackAlignment);
    if (glGetError() != 0) return fail("OpenGL backbuffer read failed");
    for (int row = 0; row < drawableHeight; row++) {
        const unsigned char* source = bottomUp.data() + rowBytes * static_cast<size_t>(drawableHeight - 1 - row);
        unsigned char* destination = topDown.data() + rowBytes * static_cast<size_t>(row);
        std::copy(source, source + rowBytes, destination);
    }

    const unsigned encodeError = lodepng::encode(path.string(), topDown,
        static_cast<unsigned>(drawableWidth), static_cast<unsigned>(drawableHeight), LCT_RGBA, 8);
    if (encodeError != 0) {
        std::error_code ec;
        std::filesystem::remove(path, ec); // Remove only output this call may have partially written.
        return fail(std::string("PNG encode failed: ") + lodepng_error_text(encodeError));
    }
    if (width != nullptr) *width = drawableWidth;
    if (height != nullptr) *height = drawableHeight;
    if (error != nullptr) error->clear();
    return true;
}

} // namespace scene_renderer
} // namespace fallout
