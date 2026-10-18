#include "scene_gl.h"

#include <algorithm>
#include <stdio.h>

#include "gl_api.h"

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
}

bool isActive()
{
    return gInitialized;
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

} // namespace scene_renderer
} // namespace fallout
