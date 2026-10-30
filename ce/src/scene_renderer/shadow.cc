#include "shadow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../dev_output.h"
#include "../scene/scene_lights.h"
#include "../settings.h"
#include "../devtools/devtools.h"
#include "gl_api.h"
#include "scene_internal.h"

namespace fallout {
unsigned int gameTimeGetTime();
namespace scene_renderer {
namespace internal {

namespace {

// 8192: the box spans the whole map plus the caster pad, about 123 GL units,
// so one texel is 0.015 units, under one game pixel at the camera (hpx is
// about 51): the NEAREST stair is invisible and the edge reads about one
// map texel soft, the owner's crisp (082). 4096 left the stair at 1.5 game
// pixels, visible on diagonal edges.
constexpr unsigned int kShadowMapSize = 8192;
constexpr unsigned int kFramebuffer = 0x8D40;
constexpr unsigned int kDepthAttachment = 0x8D00;
constexpr unsigned int kDepthComponent24 = 0x81A6;
constexpr unsigned int kFramebufferComplete = 0x8CD5;
constexpr unsigned int kDepthComponent = 0x1902;
constexpr unsigned int kUnsignedInt = 0x1405;
constexpr unsigned int kDrawBuffersNone = 0; // GL_NONE
constexpr unsigned int kColorAttachment0 = 0x8CE0;
constexpr unsigned int kColorAttachment1 = 0x8CE1;
constexpr unsigned int kR16 = 0x822D;
constexpr unsigned int kR32F = 0x822E;
constexpr unsigned int kUnsignedShort = 0x1403;
constexpr unsigned int kFloat = 0x1406;
// The caster ids travel in the id channel normalized to this scale: the map
// has at most 1024 wall object rows, 16 fixed-point bits carry them exactly.
constexpr float kIdScale = 1024.0f;
// How far the eye sits from the map center, GL units; the ortho depth range
// follows from it.
constexpr double kEyeDistance = 220.0;

unsigned int gShadowMapTexture = 0;
unsigned int gShadowIdTexture = 0;
unsigned int gShadowFbo = 0;
// The receivers read the filled map: the depth after the gap-fill pass, and
// the caster id that travels with it (082 round 6, mechanisms 1 and 2). The
// depth stays a full float (090): 16 bits put the coplanar compare on
// quantization noise.
unsigned int gShadowFilledTexture = 0;
unsigned int gShadowFilledIdTexture = 0;
unsigned int gShadowDilateFbo = 0;
unsigned int gShadowDilateProgram = 0;
unsigned int gShadowDilateVao = 0;
int gShadowDilateDepthLocation = -1;
int gShadowDilateIdLocation = -1;

// Mechanism 1 (082 round 6): one fullscreen pass copies the depth map into
// the filled map, filling an empty texel when occupied texels stand on both
// sides within 4 texels along one of the four axes (the two diagonals
// included). The value is the farther of the two, closer to the ground; its
// id travels along. The outer contour never grows: a fill needs both sides,
// so holes wider than 8 texels (fence lattices, true openings) stay. 4, not
// 082's 2: the map's texels halved in size when the casters began to fill
// the whole map (090), the filled gap keeps its width on the ground.
constexpr const char* kDilateVertexShader = R"(#version 330 core
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr const char* kDilateFragmentShader = R"(#version 330 core
uniform sampler2D uDepthMap; // the raw depth (unit 0)
uniform sampler2D uIdMap; // the raw caster ids (unit 1)
layout(location = 0) out vec4 outDepth; // r: the filled depth
layout(location = 1) out vec4 outId; // r: the caster id / 1024
void main() {
    ivec2 size = textureSize(uDepthMap, 0);
    ivec2 p = ivec2(gl_FragCoord.xy);
    float depth = texelFetch(uDepthMap, p, 0).r;
    float id = texelFetch(uIdMap, p, 0).r;
    if (depth >= 0.999) {
        ivec2 steps[4] = ivec2[4](ivec2(1, 0), ivec2(0, 1), ivec2(1, 1), ivec2(1, -1));
        float best = -1.0;
        float bestId = 0.0;
        for (int i = 0; i < 4; i++) {
            // The nearest occupied texel within 4 on each side (090): a probe
            // at exactly 4 stepped over a fence rail thinner than that and
            // left the gap between two rails' shadows open, a dotted lit line.
            float da = 1.0, db = 1.0;
            ivec2 a = p, b = p;
            for (int k = 1; k <= 4 && da >= 0.999; k++) {
                a = clamp(p + steps[i] * k, ivec2(0), size - 1);
                da = texelFetch(uDepthMap, a, 0).r;
            }
            for (int k = 1; k <= 4 && db >= 0.999; k++) {
                b = clamp(p - steps[i] * k, ivec2(0), size - 1);
                db = texelFetch(uDepthMap, b, 0).r;
            }
            if (da >= 0.999 || db >= 0.999) continue;
            float d = max(da, db);
            if (d > best) {
                best = d;
                bestId = da > db ? texelFetch(uIdMap, a, 0).r : texelFetch(uIdMap, b, 0).r;
            }
        }
        if (best > 0.0) {
            depth = best;
            id = bestId;
        }
    }
    outDepth = vec4(depth, 0.0, 0.0, 1.0);
    outId = vec4(id, 0.0, 0.0, 1.0);
}
)";

unsigned int gWallCasterVao = 0;
std::vector<WallCasterRange> gWallCasterRanges;
unsigned int gRoofCasterVao = 0;
int gRoofCasterVertices = 0;
float gMaxCasterHeight = 3.2f;
// The last frame's shadow pass, for the dev perf log.
bool gShadowRendered = false;
double gShadowSeconds = 0.0;

// The sun's shadow matrix: world GL (x, y up, z) to the depth map's clip
// space, -1..1 like any projection (the receivers map it to 0..1),
// column-major for glUniformMatrix4fv. Until 090 it gave 0..1 itself, so the
// casters filled only the map's upper right quarter: a 4096 map in effect.
// The eye sits at sunDirection * kEyeDistance and looks at the map center.
// *texel takes one map texel's width on the ground plane's larger axis, GL
// units.
void shadowMatrix(const float direction[3], float pad, float* out, float* texel)
{
    const double dir[3] = { direction[0], direction[1], direction[2] };
    const double zAxis[3] = { -dir[0], -dir[1], -dir[2] };
    // The light's right from the ground's up (0, 1, 0): up x zAxis.
    double xAxis[3] = { -zAxis[2], 0.0, zAxis[0] };
    double length = std::sqrt(xAxis[0] * xAxis[0] + xAxis[2] * xAxis[2]);
    if (length < 1e-6) {
        xAxis[0] = 1.0;
        xAxis[2] = 0.0;
        length = 1.0;
    }
    xAxis[0] /= length;
    xAxis[2] /= length;
    const double yAxis[3] = {
        zAxis[1] * xAxis[2] - zAxis[2] * xAxis[1],
        zAxis[2] * xAxis[0] - zAxis[0] * xAxis[2],
        zAxis[0] * xAxis[1] - zAxis[1] * xAxis[0],
    };
    // Half-span of the padded ground square [-half, half]^2 seen along the
    // light's right and up axes.
    const double half = 50.0 + pad;
    const double spanX = half * (std::fabs(xAxis[0]) + std::fabs(xAxis[2]));
    const double spanY = half * (std::fabs(yAxis[0]) + std::fabs(yAxis[2]));
    const double near = 1.0;
    const double far = 2.0 * kEyeDistance + 2.0 * gMaxCasterHeight + 20.0;
    const double eye[3] = { dir[0] * kEyeDistance, dir[1] * kEyeDistance, dir[2] * kEyeDistance };
    const double toEye[3] = {
        xAxis[0] * eye[0] + xAxis[1] * eye[1] + xAxis[2] * eye[2],
        yAxis[0] * eye[0] + yAxis[1] * eye[1] + yAxis[2] * eye[2],
        zAxis[0] * eye[0] + zAxis[1] * eye[1] + zAxis[2] * eye[2],
    };
    const double uvScaleX = 1.0 / (2.0 * spanX);
    const double uvScaleY = 1.0 / (2.0 * spanY);
    const double depthScale = 1.0 / (far - near);
    // Column-major 4x4: out[column * 4 + row], the columns are the world
    // x, y, z coefficients and the biases.
    out[0] = static_cast<float>(uvScaleX * xAxis[0]);
    out[1] = static_cast<float>(uvScaleY * yAxis[0]);
    out[2] = static_cast<float>(depthScale * zAxis[0]);
    out[3] = 0.0f;
    out[4] = static_cast<float>(uvScaleX * xAxis[1]);
    out[5] = static_cast<float>(uvScaleY * yAxis[1]);
    out[6] = static_cast<float>(depthScale * zAxis[1]);
    out[7] = 0.0f;
    out[8] = static_cast<float>(uvScaleX * xAxis[2]);
    out[9] = static_cast<float>(uvScaleY * yAxis[2]);
    out[10] = static_cast<float>(depthScale * zAxis[2]);
    out[11] = 0.0f;
    out[12] = static_cast<float>(0.5 - uvScaleX * toEye[0]);
    out[13] = static_cast<float>(0.5 - uvScaleY * toEye[1]);
    out[14] = static_cast<float>(-depthScale * (toEye[2] + near));
    out[15] = 1.0f;
    // 0..1 to clip space: x' = 2x - 1 on the three rows.
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) out[column * 4 + row] *= 2.0f;
        out[12 + row] = 2.0f * out[12 + row] - 1.0f;
    }
    *texel = static_cast<float>(2.0 * std::max(spanX, spanY) / kShadowMapSize);
}

void ensureResources()
{
    if (gShadowMapTexture == 0) {
        glGenTextures(1, &gShadowMapTexture);
        glBindTexture(kTexture2D, gShadowMapTexture);
        glTexImage2D(kTexture2D, 0, kDepthComponent24, kShadowMapSize, kShadowMapSize, 0,
            kDepthComponent, kUnsignedInt, nullptr);
        // NEAREST, deliberately: bilinear depth mixes the caster's silhouette
        // with the empty far depth, so the contact line reads a phantom far
        // blocker and the shadow fades from its own middle instead of its
        // object (the owner's round 2), and the interpolation breaks into
        // contour layers inside. The disk of taps does the smoothing.
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    }
    if (gShadowIdTexture == 0) {
        glGenTextures(1, &gShadowIdTexture);
        glBindTexture(kTexture2D, gShadowIdTexture);
        glTexImage2D(kTexture2D, 0, kR16, kShadowMapSize, kShadowMapSize, 0, kRed, kUnsignedShort, nullptr);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    }
    if (gShadowFilledTexture == 0) {
        glGenTextures(1, &gShadowFilledTexture);
        glBindTexture(kTexture2D, gShadowFilledTexture);
        glTexImage2D(kTexture2D, 0, kR32F, kShadowMapSize, kShadowMapSize, 0, kRed, kFloat, nullptr);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    }
    if (gShadowFilledIdTexture == 0) {
        glGenTextures(1, &gShadowFilledIdTexture);
        glBindTexture(kTexture2D, gShadowFilledIdTexture);
        glTexImage2D(kTexture2D, 0, kR16, kShadowMapSize, kShadowMapSize, 0, kRed, kUnsignedShort, nullptr);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    }
    if (gShadowFbo == 0) {
        glGenFramebuffers(1, &gShadowFbo);
        glBindFramebuffer(kFramebuffer, gShadowFbo);
        glFramebufferTexture2D(kFramebuffer, kDepthAttachment, kTexture2D, gShadowMapTexture, 0);
        glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gShadowIdTexture, 0);
        const unsigned int color = kColorAttachment0;
        glDrawBuffers(1, &color);
        if (glCheckFramebufferStatus(kFramebuffer) != kFramebufferComplete) {
            fprintf(stderr, "scene_renderer: the shadow framebuffer is incomplete; shadows stay off\n");
            fflush(stderr);
            glDeleteFramebuffers(1, &gShadowFbo);
            gShadowFbo = 0;
        }
    }
    if (gShadowFbo != 0 && gShadowDilateFbo == 0) {
        glGenFramebuffers(1, &gShadowDilateFbo);
        glBindFramebuffer(kFramebuffer, gShadowDilateFbo);
        glFramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, gShadowFilledTexture, 0);
        glFramebufferTexture2D(kFramebuffer, kColorAttachment1, kTexture2D, gShadowFilledIdTexture, 0);
        const unsigned int colors[2] = { kColorAttachment0, kColorAttachment1 };
        glDrawBuffers(2, colors);
        if (glCheckFramebufferStatus(kFramebuffer) != kFramebufferComplete) {
            fprintf(stderr, "scene_renderer: the shadow fill framebuffer is incomplete; shadows stay off\n");
            fflush(stderr);
            glDeleteFramebuffers(1, &gShadowDilateFbo);
            gShadowDilateFbo = 0;
            glDeleteFramebuffers(1, &gShadowFbo);
            gShadowFbo = 0;
        }
    }
    if (gShadowFbo != 0 && gShadowDilateProgram == 0) {
        unsigned int vertex = compileShader(0x8B31 /* GL_VERTEX_SHADER */, kDilateVertexShader);
        unsigned int fragment = compileShader(0x8B30 /* GL_FRAGMENT_SHADER */, kDilateFragmentShader);
        if (vertex != 0 && fragment != 0) {
            gShadowDilateProgram = glCreateProgram();
            glAttachShader(gShadowDilateProgram, vertex);
            glAttachShader(gShadowDilateProgram, fragment);
            glLinkProgram(gShadowDilateProgram);
            int linked = 0;
            glGetProgramiv(gShadowDilateProgram, 0x8B82 /* GL_LINK_STATUS */, &linked);
            if (linked == 0) {
                fprintf(stderr, "scene_renderer: the shadow fill program failed to link; shadows stay off\n");
                fflush(stderr);
                glDeleteProgram(gShadowDilateProgram);
                gShadowDilateProgram = 0;
            } else {
                gShadowDilateDepthLocation = glGetUniformLocation(gShadowDilateProgram, "uDepthMap");
                gShadowDilateIdLocation = glGetUniformLocation(gShadowDilateProgram, "uIdMap");
                glGenVertexArrays(1, &gShadowDilateVao);
            }
        } else {
            fprintf(stderr, "scene_renderer: the shadow fill shader failed to compile; shadows stay off\n");
            fflush(stderr);
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (gShadowDilateProgram == 0) {
            glDeleteFramebuffers(1, &gShadowDilateFbo);
            gShadowDilateFbo = 0;
            glDeleteFramebuffers(1, &gShadowFbo);
            gShadowFbo = 0;
        }
    }
    glBindTexture(kTexture2D, 0);
}

} // namespace

void setShadowWallCasters(unsigned int vao, float maxCasterHeight, std::vector<WallCasterRange> ranges)
{
    gWallCasterVao = vao;
    gWallCasterRanges = std::move(ranges);
    if (maxCasterHeight > 0.0f) gMaxCasterHeight = maxCasterHeight;
}

void setShadowRoofCasters(unsigned int vao, int vertexCount)
{
    gRoofCasterVao = vao;
    gRoofCasterVertices = vertexCount;
}

void destroyStaticShadows()
{
    if (gContext != nullptr) {
        if (gShadowMapTexture != 0) glDeleteTextures(1, &gShadowMapTexture);
        if (gShadowIdTexture != 0) glDeleteTextures(1, &gShadowIdTexture);
        if (gShadowFilledTexture != 0) glDeleteTextures(1, &gShadowFilledTexture);
        if (gShadowFilledIdTexture != 0) glDeleteTextures(1, &gShadowFilledIdTexture);
        if (gShadowFbo != 0) glDeleteFramebuffers(1, &gShadowFbo);
        if (gShadowDilateFbo != 0) glDeleteFramebuffers(1, &gShadowDilateFbo);
        if (gShadowDilateProgram != 0) glDeleteProgram(gShadowDilateProgram);
        if (gShadowDilateVao != 0) glDeleteVertexArrays(1, &gShadowDilateVao);
    }
    gShadowMapTexture = 0;
    gShadowIdTexture = 0;
    gShadowFilledTexture = 0;
    gShadowFilledIdTexture = 0;
    gShadowFbo = 0;
    gShadowDilateFbo = 0;
    gShadowDilateProgram = 0;
    gShadowDilateVao = 0;
    gWallCasterVao = 0;
    gWallCasterRanges.clear();
    gRoofCasterVao = 0;
    gRoofCasterVertices = 0;
    gMaxCasterHeight = 3.2f;
}

bool bindStaticShadows(const float* sceneMvp)
{
    gShadowRendered = false;
    glUniform1i(gSceneCloudOnLocation, 0);
    if (sceneMvp == nullptr) return false;
    const scene::Sun& sun = scene::currentSun();
    if (!sun.up || sun.strength <= 0.0f) return false;
    glUniform1i(gSceneCloudOnLocation, settings.scene3d.cloud_shadows ? 1 : 0);
    glUniform1f(gSceneShadowStrengthLocation, sun.strength);
    if (settings.scene3d.cloud_shadows) {
        const double phase = devtools::cloudPhaseRequested();
        const double ticks = phase >= 0.0 ? phase * 600.0 : static_cast<double>(gameTimeGetTime());
        glUniform2f(gSceneCloudOffsetLocation,
            static_cast<float>(std::fmod(ticks * 0.8 / 600.0, 48.0)),
            static_cast<float>(std::fmod(ticks * 0.6 / 600.0, 48.0)));
    }
    if (!settings.scene3d.shadows) return false;
    // The binding is saved before ensureResources: creating the shadow
    // framebuffer binds it, and the scene must draw where it did.
    int savedFbo = 0;
    glGetIntegerv(0x8CA6 /* GL_DRAW_FRAMEBUFFER_BINDING */, &savedFbo);
    int savedViewport[4] = {};
    glGetIntegerv(0x0BA2 /* GL_VIEWPORT */, savedViewport);
    ensureResources();
    unsigned int roofVao = 0;
    int roofVertices = 0;
    roofShadowCasters(&roofVao, &roofVertices);
    if (gShadowFbo == 0 || (gWallCasterVao == 0 && roofVao == 0)) {
        glBindFramebuffer(kFramebuffer, static_cast<unsigned int>(savedFbo));
        glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
        return false;
    }
    gRoofCasterVao = roofVao;
    gRoofCasterVertices = roofVertices;
    const Uint64 started = SDL_GetPerformanceCounter();
    // The sun leans casters across the ground: the box grows with the shadow
    // length, held inside limits so the depth map keeps its resolution.
    const double height = std::max(static_cast<double>(sun.direction[1]), 0.05);
    const float pad = static_cast<float>(std::clamp(gMaxCasterHeight / height + 8.0, 8.0, 96.0));
    float matrix[16];
    float texel = 0.0f;
    shadowMatrix(sun.direction, pad, matrix, &texel);

    // The AA ring: uShadowStep GL units per [3d] shadow_pixel_size game
    // pixel (hpx is the projection's pixels per GL unit), but at least one
    // depth map texel wide, so the taps straddle texel boundaries and
    // antialias the NEAREST stair. The compare bias, a quarter texel of
    // depth: the receiver compares its own plane carried to the texel center
    // (090), so a coplanar caster compares equal to float precision and the
    // bias only has to clear the rounding; a quarter texel is under half a
    // game pixel of light gap at a wall's foot. Set before the passes: the
    // casters draw with them.
    const double hpx = gHeroProjection.hpx > 0.0 ? gHeroProjection.hpx : 32.0;
    const float step = static_cast<float>(settings.scene3d.shadow_pixel_size / hpx);
    const double near = 1.0;
    const double far = 2.0 * kEyeDistance + 2.0 * gMaxCasterHeight + 20.0;
    const float ring = std::max(step, texel);
    glUniform1i(gSceneShadowOnLocation, 1);
    glUniformMatrix4fv(gSceneShadowMatrixLocation, 1, 0, matrix);
    glUniform1f(gSceneShadowStrengthLocation, sun.strength);
    glUniform1f(gSceneShadowDensityLocation, static_cast<float>(settings.scene3d.shadow_density));
    glUniform3f(gSceneShadowLightLocation, sun.direction[0], sun.direction[1], sun.direction[2]);
    glUniform1f(gSceneShadowStepLocation, ring);
    glUniform1f(gSceneShadowBiasLocation, 0.25f * texel / static_cast<float>(far - near));
    // Contact (090): a blocker within kShadowContact texels of depth of the
    // receiver stands on it and casts nothing there; the ground seen through
    // the stepped transparent foot of a wall's sprite lies just behind its
    // face, under the wall, and read the face as its blocker.
    constexpr double kShadowContact = 16.0;
    glUniform1f(gSceneShadowContactLocation, static_cast<float>(kShadowContact * texel / (far - near)));
    glUniform1i(gSceneShadowDebugLocation, devtools::shadowDebugRequested());
    glActiveTexture(kTexture0 + 5);
    glBindTexture(kTexture2D, gShadowFilledTexture);
    glUniform1i(gSceneShadowMapLocation, 5);
    glActiveTexture(kTexture0 + 6);
    glBindTexture(kTexture2D, gShadowFilledIdTexture);
    glUniform1i(gSceneShadowIdsLocation, 6);
    glActiveTexture(kTexture0);

    // The casters draw with the scene program itself, in its pick mode (a
    // flat colour, no lighting math) with the shadow matrix as uMvp. Textured
    // with a cutoff, so the art's transparent holes (a fence's gaps, a
    // window's glass) cast no shadow (063); the discard sits in the shader's
    // shared textured path, before the pick branch.
    glBindFramebuffer(kFramebuffer, gShadowFbo);
    glViewport(0, 0, kShadowMapSize, kShadowMapSize);
    // The map's id channel (082 round 6) clears to 0 = no caster; the clear
    // colour is saved, the game owns it outside this pass.
    float savedClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glGetFloatv(0x0C22 /* GL_COLOR_CLEAR_VALUE */, savedClear);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(kDepthBufferBit | kColorBufferBit);
    glClearColor(savedClear[0], savedClear[1], savedClear[2], savedClear[3]);
    glDisable(kBlend);
    glDisable(kCullFace); // a caster's back and end faces write depth too
    glEnable(kDepthTest);
    glDepthFunc(kLess);
    glDepthMask(1);
    glColorMask(1, 1, 1, 1); // the id channel takes the casters' numbers
    glUniformMatrix4fv(gSceneMvpLocation, 1, 0, matrix);
    glUniform1i(gScenePickLocation, 1);
    glUniform1i(gSceneShadowPassLocation, 1);
    glUniform1i(gSceneTexturedLocation, 1);
    glUniform1f(gSceneCutoffLocation, 0.5f);
    // The scene draws leave their own pass modes in the program: a stale
    // uDepthAt would replace every caster's depth with its depth-anchor
    // point's projection (the caster VAO carries no anchor attribute, so the
    // shader would read (0,0,0) - the map center's depth for everything), a
    // stale uLightMode 2 would fetch the see-through table at row 0 (the
    // caster VAO leaves aObject at 0). The casters draw as plain geometry.
    glUniform1i(gSceneDepthAtLocation, 0);
    glUniform1i(gSceneLightModeLocation, 0);
    glUniform1i(gSceneSeeThroughLocation, 0);
    glActiveTexture(kTexture0);
    if (gWallCasterVao != 0 && !gWallCasterRanges.empty()) {
        glBindVertexArray(gWallCasterVao);
        unsigned int drawnRanges = 0;
        int drawnVertices = 0;
        for (const WallCasterRange& range : gWallCasterRanges) {
            if (range.count <= 0 || range.texture == 0) continue;
            glBindTexture(kTexture2D, range.texture);
            glDrawArrays(kTriangles, range.first, range.count);
            drawnRanges++;
            drawnVertices += range.count;
        }
        static bool passLogged = false;
        if (!passLogged) {
            passLogged = true;
            fprintf(stderr, "SHADOW PASS: wall ranges %u/%zu, vertices %d, roof vertices %d\n",
                drawnRanges, gWallCasterRanges.size(), drawnVertices, gRoofCasterVertices);
            fflush(stderr);
        }
        glBindVertexArray(0);
    }
    glUniform1i(gSceneTexturedLocation, 0);
    glUniform1f(gSceneCutoffLocation, 0.0f);
    if (gRoofCasterVao != 0 && gRoofCasterVertices > 0) {
        glBindVertexArray(gRoofCasterVao);
        glDrawArrays(kTriangles, 0, gRoofCasterVertices);
        glBindVertexArray(0);
    }

    // Mechanism 1 (082 round 6): fill the map's joint gaps, then hand the
    // receivers the filled map (units 5 and 6 were pointed above).
    if (gShadowDilateProgram != 0 && gShadowDilateFbo != 0) {
        glBindFramebuffer(kFramebuffer, gShadowDilateFbo);
        glDisable(kDepthTest);
        glUseProgram(gShadowDilateProgram);
        glActiveTexture(kTexture0);
        glBindTexture(kTexture2D, gShadowMapTexture);
        glUniform1i(gShadowDilateDepthLocation, 0);
        glActiveTexture(kTexture0 + 1);
        glBindTexture(kTexture2D, gShadowIdTexture);
        glUniform1i(gShadowDilateIdLocation, 1);
        glBindVertexArray(gShadowDilateVao);
        glDrawArrays(kTriangles, 0, 3);
        glBindVertexArray(0);
        glBindTexture(kTexture2D, 0);
        glActiveTexture(kTexture0);
        glUseProgram(gSceneProgram);
        glEnable(kDepthTest);
        glBindFramebuffer(kFramebuffer, gShadowFbo);
        glViewport(0, 0, kShadowMapSize, kShadowMapSize);
    }
    glUniform1i(gSceneShadowPassLocation, 0);

    // -ShadowDebug 6: once per run, write the depth map as a PGM, min-pooled
    // to 2048 (dark = a caster wrote near, white = the cleared far depth).
    // The game's own picture tells what the receivers see; this tells what
    // the map itself holds (082 round 6).
    static bool mapDumped = false;
    if (!mapDumped && devtools::shadowDebugRequested() == 6) {
        mapDumped = true;
        std::vector<float> depth(static_cast<size_t>(kShadowMapSize) * kShadowMapSize);
        glReadPixels(0, 0, kShadowMapSize, kShadowMapSize, kDepthComponent, 0x1406 /* GL_FLOAT */,
            depth.data());
        constexpr unsigned int kDumpSize = 2048;
        const unsigned int cell = kShadowMapSize / kDumpSize;
        std::vector<unsigned char> out(static_cast<size_t>(kDumpSize) * kDumpSize);
        for (unsigned int oy = 0; oy < kDumpSize; oy++) {
            for (unsigned int ox = 0; ox < kDumpSize; ox++) {
                float nearest = 1.0f;
                for (unsigned int y = oy * cell; y < (oy + 1) * cell; y += 4) {
                    for (unsigned int x = ox * cell; x < (ox + 1) * cell; x += 4) {
                        nearest = std::min(nearest, depth[static_cast<size_t>(y) * kShadowMapSize + x]);
                    }
                }
                out[static_cast<size_t>(oy) * kDumpSize + ox] =
                    static_cast<unsigned char>(nearest * 255.0f + 0.5f);
            }
        }
        std::FILE* dump = std::fopen(dev_output::outputFile("shadowmap-dump.pgm").string().c_str(), "wb");
        if (dump != nullptr) {
            std::fprintf(dump, "P5\n%u %u\n255\n", kDumpSize, kDumpSize);
            std::fwrite(out.data(), 1, out.size(), dump);
            std::fclose(dump);
        }
    }

    glUniformMatrix4fv(gSceneMvpLocation, 1, 0, sceneMvp);
    glUniform1i(gScenePickLocation, 0);
    glColorMask(1, 1, 1, 1);
    glEnable(kDepthTest);
    glUseProgram(gSceneProgram);
    glBindFramebuffer(kFramebuffer, static_cast<unsigned int>(savedFbo));
    glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
    gShadowSeconds += static_cast<double>(SDL_GetPerformanceCounter() - started)
        / static_cast<double>(SDL_GetPerformanceFrequency());
    gShadowRendered = true;
    return true;
}

void shadowFrameStats(bool* rendered, double* seconds)
{
    *rendered = gShadowRendered;
    *seconds = gShadowSeconds;
    gShadowSeconds = 0.0;
    gShadowRendered = false;
}

} // namespace internal

void shadowFrameStats(bool* rendered, double* seconds)
{
    internal::shadowFrameStats(rendered, seconds);
}

} // namespace scene_renderer
} // namespace fallout
