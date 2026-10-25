#include "context.h"

#include <stdio.h>

#include "gl_api.h"
#include "post.h"
#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {

namespace internal {

SDL_Window* gWindow = nullptr;
SDL_GLContext gContext = nullptr;
bool gInitialized = false;
int gTextureWidth = 0;
int gTextureHeight = 0;
int gLogicalWidth = 0;
int gLogicalHeight = 0;
int gWindowWidth = 0;
int gWindowHeight = 0;
bool gSceneOverlayReady = false;

} // namespace internal

using namespace internal;

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

unsigned int gProgram = 0;
unsigned int gVao = 0;
unsigned int gVbo = 0;
unsigned int gTexture = 0;
unsigned int gSharpTextTexture = 0;
int gSharpTextTextureWidth = 0;
int gSharpTextTextureHeight = 0;
int gTextureUniformLocation = -1;
int gRectUniformLocation = -1;

} // namespace

namespace internal {

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

void drawTextureQuad(unsigned int texture, int dstX, int dstY, int dstW, int dstH)
{
    int windowX;
    int windowY;
    int windowW;
    int windowH;
    if (!fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &windowX, &windowY, &windowW, &windowH)) {
        return;
    }

    glActiveTexture(kTexture0);
    glBindTexture(kTexture2D, texture);

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

} // namespace internal

namespace {

void contextLog(const char* message)
{
    fprintf(stderr, "scene_renderer: %s\n", message);
    fflush(stderr);
    if (FILE* log = fopen("scene-context.log", "a")) {
        fprintf(log, "scene_renderer: %s\n", message);
        fclose(log);
    }
}

// ARB_debug_output is optional on 4.1; no 4.3-only entry points are required.
void contextDebugMessage(unsigned int, unsigned int type, unsigned int id, unsigned int,
    int, const char* message, const void*)
{
    if (type != 0x824C /* ERROR_ARB */ && type != 0x824D /* DEPRECATED_BEHAVIOR_ARB */) return;
    char line[1024];
    snprintf(line, sizeof(line), "GL debug error/deprecation id=%u: %s", id, message);
    contextLog(line);
}

} // namespace

bool init(SDL_Window* window, int logicalWidth, int logicalHeight)
{
    gWindow = window;
    gLogicalWidth = logicalWidth;
    gLogicalHeight = logicalHeight;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    const bool debugContext = SDL_getenv("F2CE_GL_DEBUG") != nullptr;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
        | (debugContext ? SDL_GL_CONTEXT_DEBUG_FLAG : 0));
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    const auto fallback = [](const char* reason) {
        contextLog(reason);
        shutdown();
        // Do not make the vanilla SDL renderer inherit our failed core request.
        SDL_GL_ResetAttributes();
        return false;
    };

    gContext = SDL_GL_CreateContext(window);
    if (gContext == nullptr) {
        fprintf(stderr, "scene_renderer: SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        fflush(stderr);
        return fallback("OpenGL 4.1 core unavailable; using the 2D renderer");
    }

    if (!glLoad()) {
        return fallback("OpenGL 4.1 core entry points missing; using the 2D renderer");
    }

    int major = 0, minor = 0, profile = 0, flags = 0;
    glGetIntegerv(0x821B /* MAJOR_VERSION */, &major);
    glGetIntegerv(0x821C /* MINOR_VERSION */, &minor);
    glGetIntegerv(0x9126 /* CONTEXT_PROFILE_MASK */, &profile);
    glGetIntegerv(0x821E /* CONTEXT_FLAGS */, &flags);
    if (glGetError() != 0 || major < 4 || (major == 4 && minor < 1)
        || (profile & 1 /* CORE_PROFILE_BIT */) == 0 || (flags & 1 /* FORWARD_COMPATIBLE_BIT */) == 0) {
        return fallback("Driver did not provide forward-compatible 4.1 core; using the 2D renderer");
    }
    char identity[512];
    snprintf(identity, sizeof(identity), "OpenGL %s; renderer=%s; profile=0x%x flags=0x%x",
        reinterpret_cast<const char*>(glGetString(0x1F02 /* VERSION */)),
        reinterpret_cast<const char*>(glGetString(0x1F01 /* RENDERER */)), profile, flags);
    contextLog(identity);
    if (debugContext && SDL_GL_ExtensionSupported("GL_ARB_debug_output")) {
        using DebugCallback = decltype(&contextDebugMessage);
        using SetDebugCallback = void (*)(DebugCallback, const void*);
        const auto install = reinterpret_cast<SetDebugCallback>(SDL_GL_GetProcAddress("glDebugMessageCallbackARB"));
        if (install != nullptr) {
            glEnable(0x8242 /* DEBUG_OUTPUT_SYNCHRONOUS_ARB */);
            install(contextDebugMessage, nullptr);
            contextLog("ARB_debug_output error/deprecation callback enabled");
        }
    }

    // The vanilla SDL_Renderer was created without vsync; keep the same.
    SDL_GL_SetSwapInterval(0);

    unsigned int vertex = compileShader(0x8B31 /* GL_VERTEX_SHADER */, kVertexShader);
    unsigned int fragment = compileShader(0x8B30 /* GL_FRAGMENT_SHADER */, kFragmentShader);
    if (vertex == 0 || fragment == 0) {
        return fallback("OpenGL shader initialization failed; using the 2D renderer");
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
        return fallback("OpenGL program initialization failed; using the 2D renderer");
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
    glGenTextures(1, &gSharpTextTexture);

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
        if (gSharpTextTexture != 0) {
            glDeleteTextures(1, &gSharpTextTexture);
            gSharpTextTexture = 0;
            gSharpTextTextureWidth = 0;
            gSharpTextTextureHeight = 0;
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
        shutdownGlow();
        SDL_GL_DeleteContext(gContext);
        gContext = nullptr;
    }
    gWindow = nullptr;
    resetCamera();
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

bool sharpTextOutputSize(int* width, int* height)
{
    if (!gInitialized || width == nullptr || height == nullptr) return false;
    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    int x = 0;
    int y = 0;
    return fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &x, &y, width, height);
}

bool uploadSharpTextOverlay(const unsigned char* rgba, int width, int height, int pitch)
{
    if (!gInitialized || rgba == nullptr || width <= 0 || height <= 0 || pitch < width * 4
        || pitch % 4 != 0 || gSharpTextTexture == 0) return false;
    while (glGetError() != 0) {
    }
    glActiveTexture(kTexture0);
    if (width != gSharpTextTextureWidth || height != gSharpTextTextureHeight) {
        glBindTexture(kTexture2D, gSharpTextTexture);
        glTexImage2D(kTexture2D, 0, kRgba8, width, height, 0, kRGBA, kUnsignedByte, nullptr);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        gSharpTextTextureWidth = width;
        gSharpTextTextureHeight = height;
    }
    glBindTexture(kTexture2D, gSharpTextTexture);
    glPixelStorei(kUnpackRowLength, pitch / 4);
    glTexSubImage2D(kTexture2D, 0, 0, 0, width, height, kRGBA, kUnsignedByte, rgba);
    glPixelStorei(kUnpackRowLength, 0);
    return glGetError() == 0;
}

bool drawSharpTextOverlay()
{
    if (!gInitialized || gSharpTextTexture == 0 || gSharpTextTextureWidth <= 0 || gSharpTextTextureHeight <= 0) return false;
    while (glGetError() != 0) {
    }
    glEnable(kBlend);
    glBlendFunc(1 /* GL_ONE */, kOneMinusSrcAlpha);
    drawTextureQuad(gSharpTextTexture, 0, 0, gLogicalWidth, gLogicalHeight);
    glDisable(kBlend);
    return glGetError() == 0;
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
