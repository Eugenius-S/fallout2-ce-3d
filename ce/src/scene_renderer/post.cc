#include "post.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "gl_api.h"
#include "scene_internal.h"
#include "../settings.h"

namespace fallout {
namespace scene_renderer {

namespace {

constexpr unsigned int kFramebuffer = 0x8D40;
constexpr unsigned int kColorAttachment0 = 0x8CE0;
constexpr unsigned int kDepthAttachment = 0x8D00;
constexpr unsigned int kRenderbuffer = 0x8D41;
constexpr unsigned int kDepthComponent24 = 0x81A6;
constexpr unsigned int kFragShader = 0x8B30;
constexpr unsigned int kVertexShader = 0x8B31;
constexpr unsigned int kCompileStatus = 0x8B81;
constexpr unsigned int kLinkStatus = 0x8B82;
constexpr unsigned int kRgba8 = 0x8058;
constexpr unsigned int kLinear = 0x2601;
constexpr unsigned int kTriangles = 0x0004;
constexpr unsigned int kArrayBuffer = 0x8892;
constexpr unsigned int kStaticDraw = 0x88E4;
constexpr unsigned int kFloat = 0x1406;

constexpr const char* kPostVertexShader = R"(#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexcoord;
out vec2 vTexcoord;
void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexcoord = aTexcoord;
}
)";

constexpr const char* kBrightFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
out vec4 outColor;
void main() {
    vec3 c = texture(uTexture, vTexcoord).rgb;
    float luma = dot(c, vec3(0.299, 0.587, 0.114));
    float k = smoothstep(0.55, 0.85, luma);
    outColor = vec4(c * k, 1.0);
}
)";

constexpr const char* kBlurFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
uniform vec2 uDirection;
out vec4 outColor;
void main() {
    vec3 sum = texture(uTexture, vTexcoord).rgb * 0.227027;
    sum += texture(uTexture, vTexcoord + uDirection * 1.3846).rgb * 0.316216;
    sum += texture(uTexture, vTexcoord - uDirection * 1.3846).rgb * 0.316216;
    sum += texture(uTexture, vTexcoord + uDirection * 3.2308).rgb * 0.070270;
    sum += texture(uTexture, vTexcoord - uDirection * 3.2308).rgb * 0.070270;
    outColor = vec4(sum, 1.0);
}
)";

constexpr const char* kAddFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
uniform float uStrength;
out vec4 outColor;
void main() {
    outColor = vec4(texture(uTexture, vTexcoord).rgb * uStrength, 1.0);
}
)";

bool gInitialized = false;
bool gDisabled = false;
unsigned int gProgramBright = 0;
unsigned int gProgramBlur = 0;
unsigned int gProgramAdd = 0;
unsigned int gQuadVao = 0;
unsigned int gQuadVbo = 0;
unsigned int gSceneFbo = 0;
unsigned int gSceneTexture = 0;
unsigned int gSceneDepth = 0;
unsigned int gGlowA = 0;
unsigned int gGlowB = 0;
unsigned int gGlowFboA = 0;
unsigned int gGlowFboB = 0;
int gGlowWidth = 0;
int gGlowHeight = 0;

unsigned int compile(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int compiled = 0;
    glGetShaderiv(shader, kCompileStatus, reinterpret_cast<int*>(&compiled));
    if (!compiled) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "post: shader compile failed: %s\n", log);
        fflush(stderr);
        return 0;
    }
    return shader;
}

unsigned int makeProgram(const char* fragment)
{
    unsigned int v = compile(kVertexShader, kPostVertexShader);
    unsigned int f = compile(kFragShader, fragment);
    if (v == 0 || f == 0) {
        glDeleteShader(v);
        glDeleteShader(f);
        return 0;
    }
    unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    int linked = 0;
    glGetProgramiv(p, kLinkStatus, reinterpret_cast<int*>(&linked));
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        fprintf(stderr, "post: program link failed: %s\n", log);
        fflush(stderr);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

unsigned int colorTexture(int width, int height)
{
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(kTexture2D, texture);
    glTexImage2D(kTexture2D, 0, kRgba8, width, height, 0, kRGBA, kUnsignedByte, nullptr);
    glTexParameteri(kTexture2D, kTextureMinFilter, kLinear);
    glTexParameteri(kTexture2D, kTextureMagFilter, kLinear);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    return texture;
}

void destroyTargets()
{
    if (gSceneTexture != 0) { glDeleteTextures(1, &gSceneTexture); gSceneTexture = 0; }
    if (gSceneDepth != 0) { glDeleteRenderbuffers(1, &gSceneDepth); gSceneDepth = 0; }
    if (gSceneFbo != 0) { glDeleteFramebuffers(1, &gSceneFbo); gSceneFbo = 0; }
    if (gGlowA != 0) { glDeleteTextures(1, &gGlowA); gGlowA = 0; }
    if (gGlowB != 0) { glDeleteTextures(1, &gGlowB); gGlowB = 0; }
    if (gGlowFboA != 0) { glDeleteFramebuffers(1, &gGlowFboA); gGlowFboA = 0; }
    if (gGlowFboB != 0) { glDeleteFramebuffers(1, &gGlowFboB); gGlowFboB = 0; }
    gGlowWidth = 0;
    gGlowHeight = 0;
}

// Draws a fullscreen quad sampling `texture` with the given program; dx/dy
// are the blur direction in texels.
void drawPostQuad(unsigned int texture, unsigned int program, float dx, float dy)
{
    glUseProgram(program);
    glActiveTexture(kTexture0);
    glBindTexture(kTexture2D, texture);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(program, "uDirection"), dx, dy);
    glBindVertexArray(gQuadVao);
    glDrawArrays(kTriangles, 0, 6);
}

} // namespace

bool initGlow(std::string* error)
{
    if (gInitialized) {
        if (error != nullptr) error->clear();
        return true;
    }

    gProgramBright = makeProgram(kBrightFragmentShader);
    gProgramBlur = makeProgram(kBlurFragmentShader);
    gProgramAdd = makeProgram(kAddFragmentShader);
    if (gProgramBright == 0 || gProgramBlur == 0 || gProgramAdd == 0) {
        if (error != nullptr) *error = "the glow programs failed to build";
        return false;
    }

    static const float quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
    };
    glGenVertexArrays(1, &gQuadVao);
    glBindVertexArray(gQuadVao);
    glGenBuffers(1, &gQuadVbo);
    glBindBuffer(kArrayBuffer, gQuadVbo);
    glBufferData(kArrayBuffer, sizeof(quad), quad, kStaticDraw);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, kFloat, 0, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, kFloat, 0, 4 * sizeof(float),
        reinterpret_cast<const void*>(2 * sizeof(float)));

    resizeGlow();
    gInitialized = true;
    if (error != nullptr) error->clear();
    return true;
}

bool glowActive()
{
    return gInitialized && !gDisabled && settings.scene3d.glow;
}

void setGlowDisabled()
{
    gDisabled = true;
}

void resizeGlow()
{
    if (!gInitialized) return;
    SDL_GL_GetDrawableSize(gWindow, &gDrawableWidth, &gDrawableHeight);
    destroyTargets();
    gSceneTexture = colorTexture(gDrawableWidth, gDrawableHeight);
    glGenRenderbuffers(1, &gSceneDepth);
    glBindRenderbuffer(kRenderbuffer, gSceneDepth);
    glRenderbufferStorage(kRenderbuffer, kDepthComponent24, gDrawableWidth, gDrawableHeight);
    glGenFramebuffers(1, &gSceneFbo);
    glBindFramebuffer(kFramebuffer, gSceneFbo);
    glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gSceneTexture, 0);
    glFramebufferRenderbuffer(kFramebuffer, kDepthAttachment, kRenderbuffer, gSceneDepth);
    gGlowWidth = std::max(gDrawableWidth / 4, 1);
    gGlowHeight = std::max(gDrawableHeight / 4, 1);
    gGlowA = colorTexture(gGlowWidth, gGlowHeight);
    gGlowB = colorTexture(gGlowWidth, gGlowHeight);
    glBindFramebuffer(kFramebuffer, 0);
}

bool beginSceneCapture()
{
    if (!gInitialized) return false;
    SDL_GL_GetDrawableSize(gWindow, &gDrawableWidth, &gDrawableHeight);
    resizeGlow();
    glBindFramebuffer(kFramebuffer, gSceneFbo);
    glViewport(0, 0, gDrawableWidth, gDrawableHeight);
    return true;
}

void compositeGlow()
{
    if (!gInitialized) return;

    // The bright pass downsamples the scene into glow A.
    glBindFramebuffer(kFramebuffer, gGlowFboA);
    glViewport(0, 0, gGlowWidth, gGlowHeight);
    drawPostQuad(gSceneTexture, gProgramBright, 0.0f, 0.0f);
    // Two blur passes: horizontal into B, vertical back into A.
    glBindFramebuffer(kFramebuffer, gGlowFboB);
    glViewport(0, 0, gGlowWidth, gGlowHeight);
    drawPostQuad(gGlowA, gProgramBlur, 1.0f / gGlowWidth, 0.0f);
    glBindFramebuffer(kFramebuffer, gGlowFboA);
    glViewport(0, 0, gGlowWidth, gGlowHeight);
    drawPostQuad(gGlowB, gProgramBlur, 0.0f, 1.0f / gGlowHeight);

    // Composite the scene and the glow over the window.
    glBindFramebuffer(kFramebuffer, 0);
    SDL_GL_GetDrawableSize(gWindow, &gDrawableWidth, &gDrawableHeight);
    glViewport(0, 0, gDrawableWidth, gDrawableHeight);
    glDisable(kDepthTest);
    glEnable(kBlend);
    glBlendFunc(kOne, kOneMinusSrcAlpha);
    drawPostQuad(gSceneTexture, gProgramAdd, 0.0f, 0.0f);
    glBlendFunc(kOne, kOne);
    drawPostQuad(gGlowA, gProgramAdd, 0.0f, 0.0f);
    glDisable(kBlend);
}

} // namespace scene_renderer
} // namespace fallout
