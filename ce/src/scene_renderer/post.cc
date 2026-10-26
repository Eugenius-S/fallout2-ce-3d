#include "post.h"

#include "../dev_output.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "gl_api.h"
#include "scene_internal.h"
#include "../settings.h"

namespace fallout {
namespace scene_renderer {

namespace {

// The framebuffer entry points and the constants the shared list has none of.
constexpr unsigned int kFramebuffer = 0x8D40;
constexpr unsigned int kColorAttachment0 = 0x8CE0;
constexpr unsigned int kColorAttachment1 = 0x8CE1;
constexpr unsigned int kDepthAttachment = 0x8D00;
constexpr unsigned int kRenderbuffer = 0x8D41;
constexpr unsigned int kDepthComponent24 = 0x81A6;
constexpr unsigned int kFragShader = 0x8B30;
constexpr unsigned int kVertexShader = 0x8B31;
constexpr unsigned int kCompileStatus = 0x8B81;
constexpr unsigned int kLinkStatus = 0x8B82;
constexpr unsigned int kFloat = 0x1406;

using internal::kArrayBuffer;
using internal::kBlend;
using internal::kClampToEdge;
using internal::kDepthTest;
using internal::kLinear;
using internal::kOne;
using internal::kOneMinusSrcAlpha;
using internal::kRGBA;
using internal::kRgba8;
using internal::kSrcAlpha;
using internal::kStaticDraw;
using internal::kTexture0;
constexpr unsigned int kTexture1 = 0x84C1;
using internal::kTexture2D;
using internal::kTextureMagFilter;
using internal::kTextureMinFilter;
using internal::kTextureWrapS;
using internal::kTextureWrapT;
using internal::kTriangles;
using internal::kUnsignedByte;

constexpr const char* kPostVertexShader = R"(#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexcoord;
out vec2 vTexcoord;
void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexcoord = aTexcoord;
}
)";

constexpr const char* kBlurFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
uniform vec2 uDirection;
uniform float uBright; // unused by the blur, kept for one shared quad draw
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
uniform float uNeutralShare;
out vec4 outColor;
void main() {
    vec3 glow = texture(uTexture, vTexcoord).rgb;
    // Suppress the shared white component after blur: otherwise adjacent
    // colored source pixels can mix back into a large white blob. Keep the
    // chromatic remainder and the original scene picture unchanged.
    float neutral = min(min(glow.r, glow.g), glow.b);
    glow -= vec3(neutral * (1.0 - uNeutralShare));
    outColor = vec4(glow * uStrength, 1.0);
}
)";

// The wide halo comes only from what shines by itself: the emissive buffer's
// alpha holds 1 on its marked texels (the palette's cycling colours) and 0 on
// the bright-pass ones, so daylight surfaces never feed it.
constexpr const char* kMarksFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
uniform vec2 uDirection;
uniform float uBright;
out vec4 outColor;
void main() {
    vec4 texel = texture(uTexture, vTexcoord);
    outColor = vec4(texel.rgb * texel.a, 1.0);
}
)";

bool gInitialized = false;
bool gDisabled = false;
unsigned int gProgramBlur = 0;
unsigned int gProgramAdd = 0;
unsigned int gProgramMarks = 0;
unsigned int gQuadVao = 0;
unsigned int gQuadVbo = 0;
unsigned int gSceneFbo = 0;
unsigned int gSceneTexture = 0;
unsigned int gEmissiveTexture = 0;
unsigned int gSceneDepth = 0;
unsigned int gGlowA = 0;
unsigned int gGlowB = 0;
unsigned int gGlowFboA = 0;
unsigned int gGlowFboB = 0;
unsigned int gGlowA8 = 0;
unsigned int gGlowB8 = 0;
unsigned int gGlowFboA8 = 0;
unsigned int gGlowFboB8 = 0;
int gTargetsWidth = 0;
int gTargetsHeight = 0;

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
    if (gEmissiveTexture != 0) { glDeleteTextures(1, &gEmissiveTexture); gEmissiveTexture = 0; }
    if (gSceneDepth != 0) { glDeleteRenderbuffers(1, &gSceneDepth); gSceneDepth = 0; }
    if (gSceneFbo != 0) { glDeleteFramebuffers(1, &gSceneFbo); gSceneFbo = 0; }
    if (gGlowA != 0) { glDeleteTextures(1, &gGlowA); gGlowA = 0; }
    if (gGlowB != 0) { glDeleteTextures(1, &gGlowB); gGlowB = 0; }
    if (gGlowFboA != 0) { glDeleteFramebuffers(1, &gGlowFboA); gGlowFboA = 0; }
    if (gGlowFboB != 0) { glDeleteFramebuffers(1, &gGlowFboB); gGlowFboB = 0; }
    if (gGlowA8 != 0) { glDeleteTextures(1, &gGlowA8); gGlowA8 = 0; }
    if (gGlowB8 != 0) { glDeleteTextures(1, &gGlowB8); gGlowB8 = 0; }
    if (gGlowFboA8 != 0) { glDeleteFramebuffers(1, &gGlowFboA8); gGlowFboA8 = 0; }
    if (gGlowFboB8 != 0) { glDeleteFramebuffers(1, &gGlowFboB8); gGlowFboB8 = 0; }
    gTargetsWidth = 0;
    gTargetsHeight = 0;
}

// Draws a fullscreen quad sampling `texture` with the given program; dx/dy
// are the blur direction in texels, bright 1.0 or the bright-pass threshold.
void drawPostQuad(unsigned int texture, unsigned int program, float dx, float dy, float strength, float bright = 1.0f, float neutralShare = 1.0f)
{
    glUseProgram(program);
    glActiveTexture(kTexture0);
    glBindTexture(kTexture2D, texture);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(program, "uDirection"), dx, dy);
    glUniform1f(glGetUniformLocation(program, "uStrength"), strength);
    glUniform1f(glGetUniformLocation(program, "uBright"), bright);
    glUniform1f(glGetUniformLocation(program, "uNeutralShare"), neutralShare);
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

    gProgramBlur = makeProgram(kBlurFragmentShader);
    gProgramAdd = makeProgram(kAddFragmentShader);
    gProgramMarks = makeProgram(kMarksFragmentShader);
    if (gProgramBlur == 0 || gProgramAdd == 0 || gProgramMarks == 0) {
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
    glBindVertexArray(0);

    gInitialized = true;
    resizeGlow();
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

void shutdownGlow()
{
    destroyTargets();
    if (gProgramBlur != 0) { glDeleteProgram(gProgramBlur); gProgramBlur = 0; }
    if (gProgramAdd != 0) { glDeleteProgram(gProgramAdd); gProgramAdd = 0; }
    if (gProgramMarks != 0) { glDeleteProgram(gProgramMarks); gProgramMarks = 0; }
    if (gQuadVbo != 0) { glDeleteBuffers(1, &gQuadVbo); gQuadVbo = 0; }
    if (gQuadVao != 0) { glDeleteVertexArrays(1, &gQuadVao); gQuadVao = 0; }
    gInitialized = false;
}

void resizeGlow()
{
    if (!gInitialized) return;
    int width = 0;
    int height = 0;
    SDL_GL_GetDrawableSize(internal::gWindow, &width, &height);
    if (width == gTargetsWidth && height == gTargetsHeight) return;
    destroyTargets();
    // The scene renders here in full size: colour for the picture, a second
    // attachment for what its texels shine by themselves.
    gSceneTexture = colorTexture(width, height);
    gEmissiveTexture = colorTexture(width, height);
    glGenRenderbuffers(1, &gSceneDepth);
    glBindRenderbuffer(kRenderbuffer, gSceneDepth);
    glRenderbufferStorage(kRenderbuffer, kDepthComponent24, width, height);
    glGenFramebuffers(1, &gSceneFbo);
    glBindFramebuffer(kFramebuffer, gSceneFbo);
    glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gSceneTexture, 0);
    glFramebufferTexture2D(kFramebuffer, kColorAttachment1, kTexture2D, gEmissiveTexture, 0);
    glFramebufferRenderbuffer(kFramebuffer, kDepthAttachment, kRenderbuffer, gSceneDepth);
    const unsigned int attachments[2] = { kColorAttachment0, kColorAttachment1 };
    glDrawBuffers(2, attachments);
    // The bleed is blurred at a quarter of the window, wide enough at this size.
    int glowWidth = std::max(width / 4, 1);
    int glowHeight = std::max(height / 4, 1);
    gGlowA = colorTexture(glowWidth, glowHeight);
    gGlowB = colorTexture(glowWidth, glowHeight);
    glGenFramebuffers(1, &gGlowFboA);
    glBindFramebuffer(kFramebuffer, gGlowFboA);
    glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gGlowA, 0);
    glDrawBuffers(1, &kColorAttachment0);
    glGenFramebuffers(1, &gGlowFboB);
    glBindFramebuffer(kFramebuffer, gGlowFboB);
    glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gGlowB, 0);
    glDrawBuffers(1, &kColorAttachment0);
    // The wide halo blurs an eighth of the window: twice the quarter's reach
    // per round, so a few rounds spill it far over the surroundings.
    int wideWidth = std::max(width / 8, 1);
    int wideHeight = std::max(height / 8, 1);
    gGlowA8 = colorTexture(wideWidth, wideHeight);
    gGlowB8 = colorTexture(wideWidth, wideHeight);
    glGenFramebuffers(1, &gGlowFboA8);
    glBindFramebuffer(kFramebuffer, gGlowFboA8);
    glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gGlowA8, 0);
    glDrawBuffers(1, &kColorAttachment0);
    glGenFramebuffers(1, &gGlowFboB8);
    glBindFramebuffer(kFramebuffer, gGlowFboB8);
    glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gGlowB8, 0);
    glDrawBuffers(1, &kColorAttachment0);
    glBindFramebuffer(kFramebuffer, 0);
    gTargetsWidth = width;
    gTargetsHeight = height;
}

bool beginSceneCapture()
{
    if (gDisabled || !settings.scene3d.glow) return false;
    if (!gInitialized) {
        std::string error;
        if (!initGlow(&error)) {
            fprintf(stderr, "post: glow off: %s\n", error.c_str());
            fflush(stderr);
            if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
                fprintf(log, "post: glow off: %s\n", error.c_str());
                fclose(log);
            }
            setGlowDisabled();
            return false;
        }
    }
    resizeGlow();
    if (gSceneFbo == 0) return false;
    glBindFramebuffer(kFramebuffer, gSceneFbo);
    // Re-assert both attachments: a lost draw-buffer state would silently
    // drop every emissive write and the glow would stay off.
    const unsigned int attachments[2] = { kColorAttachment0, kColorAttachment1 };
    glDrawBuffers(2, attachments);
    glViewport(0, 0, gTargetsWidth, gTargetsHeight);
    return true;
}

void compositeGlow()
{
    if (!gInitialized || gSceneFbo == 0) return;

    // Two scales of bleed. The near glow keeps the quarter's one round, tight
    // around the source as before; the wide halo is the marked texels only,
    // blurred at an eighth of the window twice, spilling far over whatever is
    // next to the source (the floor and walls included).
    int glowWidth = std::max(gTargetsWidth / 4, 1);
    int glowHeight = std::max(gTargetsHeight / 4, 1);
    glBindFramebuffer(kFramebuffer, gGlowFboB);
    glViewport(0, 0, glowWidth, glowHeight);
    drawPostQuad(gEmissiveTexture, gProgramBlur, 1.0f / glowWidth, 0.0f, 1.0f);
    glBindFramebuffer(kFramebuffer, gGlowFboA);
    glViewport(0, 0, glowWidth, glowHeight);
    drawPostQuad(gGlowB, gProgramBlur, 0.0f, 1.0f / glowHeight, 1.0f);

    int wideWidth = std::max(gTargetsWidth / 8, 1);
    int wideHeight = std::max(gTargetsHeight / 8, 1);
    glBindFramebuffer(kFramebuffer, gGlowFboA8);
    glViewport(0, 0, wideWidth, wideHeight);
    drawPostQuad(gEmissiveTexture, gProgramMarks, 0.0f, 0.0f, 1.0f);
    for (int pass = 0; pass < 2; pass++) {
        glBindFramebuffer(kFramebuffer, gGlowFboB8);
        glViewport(0, 0, wideWidth, wideHeight);
        drawPostQuad(gGlowA8, gProgramBlur, 1.0f / wideWidth, 0.0f, 1.0f);
        glBindFramebuffer(kFramebuffer, gGlowFboA8);
        glViewport(0, 0, wideWidth, wideHeight);
        drawPostQuad(gGlowB8, gProgramBlur, 0.0f, 1.0f / wideHeight, 1.0f);
    }

    // The picture, then its two bleeds added over it, over the window: the
    // quarter one lighter, the wide halo carries the spill.
    glBindFramebuffer(kFramebuffer, 0);
    glViewport(0, 0, gTargetsWidth, gTargetsHeight);
    glDisable(kDepthTest);
    glDepthMask(1);
    glEnable(kBlend);
    glBlendFunc(kOne, kOneMinusSrcAlpha);
    drawPostQuad(gSceneTexture, gProgramAdd, 0.0f, 0.0f, 1.0f);
    const double strength = std::max(settings.scene3d.glow_strength, 0.0);
    glBlendFunc(kOne, kOne);
    drawPostQuad(gGlowA, gProgramAdd, 0.0f, 0.0f, static_cast<float>(0.55 * strength), 1.0f, 0.15f);
    drawPostQuad(gGlowA8, gProgramAdd, 0.0f, 0.0f, static_cast<float>(1.5 * strength), 1.0f, 0.15f);
    glDisable(kBlend);
    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace scene_renderer
} // namespace fallout
