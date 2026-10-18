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
#include <utility>
#include <vector>

#include <lodepng.h>

#include "gl_api.h"
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
uniform mat4 uMvp;
out vec2 vTexcoord;
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vTexcoord = aTexcoord;
}
)";

constexpr const char* kFloorFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
uniform int uTextured;
uniform vec4 uColor;
uniform float uCutoff;
out vec4 outColor;
void main() {
    if (uTextured == 0) {
        outColor = uColor;
    } else {
        vec4 texel = texture(uTexture, vTexcoord);
        if (texel.a < uCutoff) discard;
        outColor = texel;
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
constexpr unsigned int kLineLoop = 0x0002;
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
constexpr unsigned int kLinear = 0x2601;
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
HeroProjection gHeroProjection;
StaticHeroInfo gHeroInfo = {};
int gGroundCursorTile = -1;
unsigned int gCursorVao = 0;
unsigned int gCursorVbo = 0;

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

void drawQuad(int srcWidth, int srcHeight, int srcPitch, const void* pixels, int dstX, int dstY, int dstW, int dstH)
{
    int windowX;
    int windowY;
    int windowW;
    int windowH;
    if (!fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &windowX, &windowY, &windowW, &windowH)) {
        return;
    }

    bool recreate = srcWidth != gTextureWidth || srcHeight != gTextureHeight;
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
    glPixelStorei(kUnpackRowLength, srcPitch / 4);
    glTexSubImage2D(kTexture2D, 0, 0, 0, srcWidth, srcHeight, kRGBA, kUnsignedByte, pixels);

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
        destroyStaticHero();
        destroyStaticWalls();
        destroyStaticFloor();
        if (gCursorVbo != 0) { glDeleteBuffers(1, &gCursorVbo); gCursorVbo = 0; }
        if (gCursorVao != 0) { glDeleteVertexArrays(1, &gCursorVao); gCursorVao = 0; }
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
    gGroundCursorTile = -1;
}

bool isActive()
{
    return gInitialized;
}

StaticPreviewCamera staticPreviewCamera(bool withWalls)
{
    const GroundPickCamera camera = staticPreviewCameraParameters(withWalls);
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
    return pickGroundTile(projection, camera, viewport, logicalX, logicalY, result);
}

void setStaticGroundCursor(int tile)
{
    gGroundCursorTile = tile >= 0 && tile < HEX_GRID_SIZE ? tile : -1;
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

    const int hx = frame.tile % 200;
    const int hy = frame.tile / 200;
    const int a = 199 - hx;
    const double hexX = 48.0 * (a >> 1) + 32.0 * (a & 1) + 16.0 * hy + 16.0;
    const double hexY = -12.0 * (a >> 1) + 12.0 * hy + 8.0;
    const double dx = hexX + frame.objectX - gHeroProjection.origin[0];
    const double dy = hexY + frame.objectY - gHeroProjection.origin[1];
    const double determinant = gHeroProjection.squ[0] * gHeroProjection.sqv[1]
        - gHeroProjection.sqv[0] * gHeroProjection.squ[1];
    const double worldU = (gHeroProjection.sqv[1] * dx - gHeroProjection.sqv[0] * dy) / determinant - 50.0;
    const double worldV = (gHeroProjection.squ[0] * dy - gHeroProjection.squ[1] * dx) / determinant - 50.0;
    if (!std::isfinite(worldU) || !std::isfinite(worldV)) return fail("hero world anchor is not finite");

    const double top = static_cast<double>(frame.artYOffset) - (frame.height - 1);
    const double bottom = top + frame.height;
    double up0;
    double up1;
    if (bottom <= 0.0) {
        up0 = -bottom / gHeroProjection.hpx;
        up1 = -top / gHeroProjection.hpx;
    } else {
        const double farRow = std::min(bottom, std::max(0.0, top));
        up0 = 0.0;
        up1 = (farRow - top + 0.5 * (bottom - farRow)) / gHeroProjection.hpx;
    }
    const StaticPreviewCamera camera = staticPreviewCamera(true);
    const GroundPickCamera basisCamera = {
        { camera.eye[0], camera.eye[1], camera.eye[2] },
        { camera.target[0], camera.target[1], camera.target[2] }, camera.fovY, camera.donorScreenRight,
    };
    double forward[3], screenRight[3], screenUp[3];
    if (!previewCameraBasis(basisCamera, forward, screenRight, screenUp)
        || !std::isfinite(up0) || !std::isfinite(up1)) {
        return fail("hero card camera basis or height is invalid");
    }
    const double rightX = screenRight[0];
    const double rightZ = screenRight[2];
    const double leftPixels = static_cast<double>(frame.artXOffset) - frame.width / 2;
    const double rightPixels = leftPixels + frame.width;
    const double leftDistance = leftPixels / gHeroProjection.bbPx;
    const double rightDistance = rightPixels / gHeroProjection.bbPx;
    auto vertex = [&](double horizontal, double height, double u, double v) {
        const double positionX = worldU + rightX * horizontal;
        const double positionZ = worldV + rightZ * horizontal;
        return FloorVertex { static_cast<float>(positionX), static_cast<float>(height),
            static_cast<float>(positionZ), static_cast<float>(u), static_cast<float>(v) };
    };
    const FloorVertex topLeft = vertex(leftDistance, up1, 0.0, 0.0);
    const FloorVertex bottomLeft = vertex(leftDistance, up0, 0.0, 1.0);
    const FloorVertex bottomRight = vertex(rightDistance, up0, 1.0, 1.0);
    const FloorVertex topRight = vertex(rightDistance, up1, 1.0, 0.0);
    const FloorVertex vertices[] = { topLeft, bottomLeft, bottomRight, topLeft, bottomRight, topRight };

    while (glGetError() != 0) {
    }
    glPixelStorei(kUnpackAlignment, 1);
    if (gHeroVao == 0) glGenVertexArrays(1, &gHeroVao);
    if (gHeroVbo == 0) glGenBuffers(1, &gHeroVbo);
    if (gHeroTexture == 0) glGenTextures(1, &gHeroTexture);
    glBindVertexArray(gHeroVao);
    glBindBuffer(kArrayBuffer, gHeroVbo);
    glBufferData(kArrayBuffer, sizeof(vertices), vertices, kStaticDraw);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
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
}

void drawOverlay(const unsigned char* rgba, int width, int height, int pitch, int dstX, int dstY, int dstW, int dstH)
{
    if (!gInitialized || rgba == nullptr || width <= 0 || height <= 0) {
        return;
    }

    glEnable(kBlend);
    glBlendFunc(1 /* GL_ONE */, 3 /* GL_ONE_MINUS_SRC_ALPHA */);
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
            const unsigned short contentX = cellX + cellMargin;
            const unsigned short contentY = cellY + cellMargin;
            const float u0 = static_cast<float>(contentX) / imageWidth;
            const float v0 = static_cast<float>(contentY) / imageHeight;
            const float u1 = static_cast<float>(contentX + cellSize) / imageWidth;
            const float v1 = static_cast<float>(contentY + cellSize) / imageHeight;
            const float x0 = static_cast<float>(col) - kHalfMap;
            const float z0 = static_cast<float>(row) - kHalfMap;
            quad(x0, 0.0f, z0, x0 + 1.0f, z0 + 1.0f, u0, v0, u1, v1);
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
    glTexParameteri(kTexture2D, kTextureMinFilter, kLinear);
    glTexParameteri(kTexture2D, kTextureMagFilter, kLinear);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    glBindVertexArray(0);

    gFloorMvpLocation = glGetUniformLocation(gFloorProgram, "uMvp");
    gFloorTextureLocation = glGetUniformLocation(gFloorProgram, "uTexture");
    gFloorTexturedLocation = glGetUniformLocation(gFloorProgram, "uTextured");
    gFloorColorLocation = glGetUniformLocation(gFloorProgram, "uColor");
    gFloorCutoffLocation = glGetUniformLocation(gFloorProgram, "uCutoff");
    if (gFloorMvpLocation < 0 || gFloorTextureLocation < 0
        || gFloorTexturedLocation < 0 || gFloorColorLocation < 0 || gFloorCutoffLocation < 0) {
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
    constexpr size_t kVertexSize = 20;
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
    if (version != 1 || headerSize != kHeaderSize || pageCount == 0 || pageCount > kMaxPages
        || batchCount == 0 || batchCount > pageCount || totalTriangles == 0 || totalTriangles > kMaxTriangles) {
        return fail("walls.bin header counts or version are unsupported");
    }

    struct WallBatchData {
        unsigned int page;
        unsigned int triangleCount;
        unsigned int width;
        unsigned int height;
        std::vector<FloorVertex> vertices;
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
            FloorVertex& vertex = batch.vertices[vertexIndex];
            vertex.x = readF32(vertexData);
            vertex.y = readF32(vertexData + 4);
            vertex.z = readF32(vertexData + 8);
            vertex.u = readF32(vertexData + 12);
            vertex.v = readF32(vertexData + 16);
            if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z)
                || !std::isfinite(vertex.u) || !std::isfinite(vertex.v)
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
            static_cast<long long>(source.vertices.size() * sizeof(FloorVertex)), source.vertices.data(), kStaticDraw);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
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
    makeFloorMvp(static_cast<float>(width) / height, camera, mvp);
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
    glActiveTexture(kTexture0);
    glUniform1i(gFloorTextureLocation, 0);
    glBindVertexArray(gFloorVao);
    glBindTexture(kTexture2D, gFloorTexture);

    glUniform1i(gFloorTexturedLocation, 0);
    glUniform1f(gFloorCutoffLocation, 0.01f);
    glUniform4f(gFloorColorLocation, 0.16f, 0.17f, 0.18f, 1.0f);
    glDrawArrays(kTriangles, 0, 6);
    glUniform1i(gFloorTexturedLocation, 1);
    glUniform1f(gFloorCutoffLocation, 0.01f);
    glDrawArrays(kTriangles, 6, gFloorVertexCount - 6);

    if (withWalls) {
        glUniform1f(gFloorCutoffLocation, 0.5f);
        for (const WallBatch& batch : gWallBatches) {
            glBindVertexArray(batch.vao);
            glBindTexture(kTexture2D, batch.texture);
            glDrawArrays(kTriangles, 0, static_cast<int>(batch.triangleCount * 3));
        }
    }
    if (withHero && gHeroLoaded) {
        glUniform1f(gFloorCutoffLocation, 0.5f);
        glBindVertexArray(gHeroVao);
        glBindTexture(kTexture2D, gHeroTexture);
        glDrawArrays(kTriangles, 0, 6);
    }

    if (gGroundCursorTile >= 0) {
        auto pixelCenter = [](int tile, double* x, double* y) {
            const int hx = tile % 200, hy = tile / 200, a = 199 - hx;
            *x = 48.0 * (a >> 1) + 32.0 * (a & 1) + 16.0 * hy + 16.0;
            *y = -12.0 * (a >> 1) + 12.0 * hy + 8.0;
        };
        auto toWorld = [&](double px, double py, float* wx, float* wz) {
            const double det = gHeroProjection.squ[0] * gHeroProjection.sqv[1] - gHeroProjection.sqv[0] * gHeroProjection.squ[1];
            const double dx = px - gHeroProjection.origin[0], dy = py - gHeroProjection.origin[1];
            const double u = (gHeroProjection.sqv[1] * dx - gHeroProjection.sqv[0] * dy) / det;
            const double v = (gHeroProjection.squ[0] * dy - gHeroProjection.squ[1] * dx) / det;
            *wx = static_cast<float>(u - 50.0); *wz = static_cast<float>(v - 50.0);
        };
        double centerX, centerY;
        pixelCenter(gGroundCursorTile, &centerX, &centerY);
        FloorVertex outline[6] = {};
        bool validOutline = true;
        for (int i = 0; i < 6; i++) {
            const int a = tileGetTileInDirection(gGroundCursorTile, static_cast<Rotation>(i), 1);
            const int b = tileGetTileInDirection(gGroundCursorTile, static_cast<Rotation>((i + 1) % 6), 1);
            if (!hexGridTileIsValid(a) || !hexGridTileIsValid(b)) { validOutline = false; break; }
            double ax, ay, bx, by;
            pixelCenter(a, &ax, &ay); pixelCenter(b, &bx, &by);
            toWorld((centerX + ax + bx) / 3.0, (centerY + ay + by) / 3.0, &outline[i].x, &outline[i].z);
            outline[i].y = 0.035f;
        }
        if (validOutline) {
            if (gCursorVao == 0) glGenVertexArrays(1, &gCursorVao);
            if (gCursorVbo == 0) glGenBuffers(1, &gCursorVbo);
            glBindVertexArray(gCursorVao);
            glBindBuffer(kArrayBuffer, gCursorVbo);
            glBufferData(kArrayBuffer, sizeof(outline), outline, kStaticDraw);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, 0x1406, 0, sizeof(FloorVertex), reinterpret_cast<const void*>(0));
            glUniform1i(gFloorTexturedLocation, 0);
            glUniform4f(gFloorColorLocation, 1.0f, 0.85f, 0.1f, 1.0f);
            glDrawArrays(kLineLoop, 0, 6);
        }
    }

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
