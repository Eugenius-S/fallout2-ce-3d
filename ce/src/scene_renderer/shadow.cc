#include "shadow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../scene/scene_lights.h"
#include "../settings.h"
#include "gl_api.h"
#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {
namespace internal {

namespace {

constexpr unsigned int kShadowMapSize = 4096;
constexpr unsigned int kFramebuffer = 0x8D40;
constexpr unsigned int kDepthAttachment = 0x8D00;
constexpr unsigned int kDepthComponent24 = 0x81A6;
constexpr unsigned int kFramebufferComplete = 0x8CD5;
constexpr unsigned int kDepthComponent = 0x1902;
constexpr unsigned int kUnsignedInt = 0x1405;
constexpr unsigned int kDrawBuffersNone = 0; // GL_NONE
// How far the eye sits from the map center, GL units; the ortho depth range
// follows from it.
constexpr double kEyeDistance = 220.0;

unsigned int gShadowMapTexture = 0;
unsigned int gShadowFbo = 0;

unsigned int gWallCasterVao = 0;
std::vector<WallCasterRange> gWallCasterRanges;
unsigned int gRoofCasterVao = 0;
int gRoofCasterVertices = 0;
float gMaxCasterHeight = 3.2f;
// The last frame's shadow pass, for the dev perf log.
bool gShadowRendered = false;
double gShadowSeconds = 0.0;

// The sun's shadow matrix: world GL (x, y up, z) to the depth map's uv and
// depth in 0..1, column-major for glUniformMatrix4fv. The eye sits at
// sunDirection * kEyeDistance and looks at the map center.
void shadowMatrix(const float direction[3], float pad, float* out)
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
}

void ensureResources()
{
    if (gShadowMapTexture == 0) {
        glGenTextures(1, &gShadowMapTexture);
        glBindTexture(kTexture2D, gShadowMapTexture);
        glTexImage2D(kTexture2D, 0, kDepthComponent24, kShadowMapSize, kShadowMapSize, 0,
            kDepthComponent, kUnsignedInt, nullptr);
        // Bilinear depth: the shader's soft compare turns the interpolation
        // into edge antialiasing, and the melt's single blocker depth reads
        // smooth instead of per-texel steps (062).
        glTexParameteri(kTexture2D, kTextureMinFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureMagFilter, kLinear);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    }
    if (gShadowFbo == 0) {
        glGenFramebuffers(1, &gShadowFbo);
        glBindFramebuffer(kFramebuffer, gShadowFbo);
        glFramebufferTexture2D(kFramebuffer, kDepthAttachment, kTexture2D, gShadowMapTexture, 0);
        const unsigned int none = kDrawBuffersNone;
        glDrawBuffers(1, &none);
        if (glCheckFramebufferStatus(kFramebuffer) != kFramebufferComplete) {
            fprintf(stderr, "scene_renderer: the shadow framebuffer is incomplete; shadows stay off\n");
            fflush(stderr);
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
        if (gShadowFbo != 0) glDeleteFramebuffers(1, &gShadowFbo);
    }
    gShadowMapTexture = 0;
    gShadowFbo = 0;
    gWallCasterVao = 0;
    gWallCasterRanges.clear();
    gRoofCasterVao = 0;
    gRoofCasterVertices = 0;
    gMaxCasterHeight = 3.2f;
}

bool bindStaticShadows(const float* sceneMvp)
{
    gShadowRendered = false;
    if (!settings.scene3d.shadows || sceneMvp == nullptr) return false;
    const scene::Sun& sun = scene::currentSun();
    if (!sun.up || sun.strength <= 0.0f) return false;
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
    shadowMatrix(sun.direction, pad, matrix);

    // The AA ring: uShadowStep GL units per [3d] shadow_pixel_size game
    // pixel (hpx is the projection's pixels per GL unit), but at least two
    // depth map texels wide: the nine taps must straddle texel boundaries or
    // the edge still reads a stair (the owner's rounds 6 and 7; 1.25 texels
    // were not enough). The compare bias covers the ring's span and the
    // depth map's slope error; it is set in the map's 0..1 depth, whose
    // range the matrix uses. Set before the passes: the casters draw with
    // them.
    const double hpx = gHeroProjection.hpx > 0.0 ? gHeroProjection.hpx : 32.0;
    const float step = static_cast<float>(settings.scene3d.shadow_pixel_size / hpx);
    const double near = 1.0;
    const double far = 2.0 * kEyeDistance + 2.0 * gMaxCasterHeight + 20.0;
    const float texel = static_cast<float>(2.0 * (50.0 + pad) / kShadowMapSize);
    const float ring = std::max(step, texel * 2.0f);
    glUniform1i(gSceneShadowOnLocation, 1);
    glUniformMatrix4fv(gSceneShadowMatrixLocation, 1, 0, matrix);
    glUniform1f(gSceneShadowStrengthLocation, sun.strength);
    glUniform1f(gSceneShadowStepLocation, ring);
    glUniform1f(gSceneShadowBiasLocation, static_cast<float>(ring * 1.5 / (far - near)));
    // The map's depth span in GL units: the shader fades the shadow with the
    // distance the sun's ray travels past its caster. uShadowGround scales
    // that ray distance to the ground distance, so the rim's hexes hold at
    // every sun height (062).
    glUniform1f(gSceneShadowRangeLocation, static_cast<float>(far - near));
    const float groundScale = static_cast<float>(
        std::sqrt(std::max(1.0 - static_cast<double>(sun.direction[1]) * sun.direction[1], 0.0)));
    glUniform1f(gSceneShadowGroundLocation, groundScale);
    glActiveTexture(kTexture0 + 5);
    glBindTexture(kTexture2D, gShadowMapTexture);
    glUniform1i(gSceneShadowMapLocation, 5);
    glActiveTexture(kTexture0);

    // The casters draw with the scene program itself, in its pick mode (a
    // flat colour, no lighting math) with the shadow matrix as uMvp. Textured
    // with a cutoff, so the art's transparent holes (a fence's gaps, a
    // window's glass) cast no shadow (063); the discard sits in the shader's
    // shared textured path, before the pick branch.
    glBindFramebuffer(kFramebuffer, gShadowFbo);
    glViewport(0, 0, kShadowMapSize, kShadowMapSize);
    glClear(kDepthBufferBit);
    glDisable(kBlend);
    glDisable(kCullFace); // a caster's back and end faces write depth too
    glEnable(kDepthTest);
    glDepthFunc(kLess);
    glDepthMask(1);
    glColorMask(0, 0, 0, 0);
    glUniformMatrix4fv(gSceneMvpLocation, 1, 0, matrix);
    glUniform1i(gScenePickLocation, 1);
    glUniform1i(gSceneTexturedLocation, 1);
    glUniform1f(gSceneCutoffLocation, 0.5f);
    glActiveTexture(kTexture0);
    if (gWallCasterVao != 0 && !gWallCasterRanges.empty()) {
        glBindVertexArray(gWallCasterVao);
        for (const WallCasterRange& range : gWallCasterRanges) {
            if (range.count <= 0 || range.texture == 0) continue;
            glBindTexture(kTexture2D, range.texture);
            glDrawArrays(kTriangles, range.first, range.count);
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
