#include <stdio.h>

#include "gl_api.h"
#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {
namespace internal {

namespace {

constexpr const char* kSceneVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec3 aDepthAt; // the point whose depth this vertex takes
layout(location = 3) in float aObject; // walls.bin: the object row it shows, -1: none
uniform mat4 uMvp;
uniform int uDepthAt;
// Whether the egg cuts what is drawn: 0 no, 1 the walls whose object row is
// set in uSeeThroughObjects (the hero stands behind them), 2 the whole draw.
uniform int uSeeThrough;
uniform sampler2D uSeeThroughObjects; // per object row: R see-through, G light
uniform int uLightMode;
out vec2 vTexcoord;
out vec3 vWorld;
flat out int vSeeThrough;
flat out float vObjectLight;
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vTexcoord = aTexcoord;
    vWorld = aPosition;
    vSeeThrough = uSeeThrough == 2 ? 1 : 0;
    vObjectLight = -1.0;
    if ((uSeeThrough == 1 || uLightMode == 2) && aObject >= 0.0) {
        int row = int(aObject + 0.5);
        vec2 entry = texelFetch(uSeeThroughObjects, ivec2(row % 1024, row / 1024), 0).rg;
        if (uSeeThrough == 1) vSeeThrough = entry.r > 0.5 ? 1 : 0;
        if (uLightMode == 2) vObjectLight = entry.g;
    }
    if (uDepthAt != 0) {
        vec4 q = uMvp * vec4(aDepthAt, 1.0);
        gl_Position.z = q.z / q.w * gl_Position.w;
    }
}
)";

constexpr const char* kSceneFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
in vec3 vWorld;
flat in int vSeeThrough;
flat in float vObjectLight;
uniform sampler2D uTexture;
uniform int uTextured;
uniform vec4 uColor;
uniform float uCutoff;
uniform int uTranslucent; // the engine's translucent blend: steam, glass, energy, ...
uniform vec3 uTint;
uniform int uGlassGray;
uniform int uPick; // the pick pass: every texel shown is the object's id colour
uniform int uEgg; // the see-through circle around the hero
uniform sampler2D uEggMask;
uniform vec4 uEggRect; // game logical pixels: left, top, width, height
uniform vec4 uViewport; // GL viewport of the game picture
uniform vec2 uLogical; // the game picture's size in game logical pixels
// The egg over walls the hero stands behind: 1 leaves out what it shows
// through, 2 draws only that, blended over what lies behind.
uniform int uEggPass;
uniform int uLightMode; // 0 unlit, 1 by the hex under the fragment, 2 by its object's hex, 3 uLightValue
uniform sampler2D uLightMap; // the game's light per hex, 200 x 200
uniform float uLightValue;
uniform vec2 uLightSqu; // ground (u, v) to game pixels
uniform vec2 uLightSqv;
uniform vec2 uLightOrigin;
out vec4 outColor;
// The light of the ground under GL point w, between the hexes around it as the
// engine spreads it over a floor tile. The hex grid's odd columns sit half a
// step lower; their average place is close enough for light.
float lightAtGround(vec3 w) {
    vec2 px = uLightOrigin + (w.x + 50.0) * uLightSqu + (w.z + 50.0) * uLightSqv;
    float hy = ((px.x - 20.0) + 4.0 * (px.y - 11.0)) / 64.0;
    float a = ((px.x - 20.0) - 16.0 * hy) / 24.0;
    return texture(uLightMap, vec2((199.0 - a + 0.5) / 200.0, (hy + 0.5) / 200.0)).r;
}
float sceneLight() {
    if (uLightMode == 1) return lightAtGround(vWorld);
    if (uLightMode == 2) return vObjectLight >= 0.0 ? vObjectLight : lightAtGround(vWorld);
    if (uLightMode == 3) return uLightValue;
    return 1.0;
}
// _intensity_mask_buf_to_buf: inside the egg a nonzero mask value is the
// surface's share in 128ths; 0 outside it and where the surface stays whole.
float eggShare() {
    vec2 game = (gl_FragCoord.xy - uViewport.xy) / uViewport.zw * uLogical;
    game.y = uLogical.y - game.y;
    vec2 e = (game - uEggRect.xy) / uEggRect.zw;
    if (e.x < 0.0 || e.y < 0.0 || e.x >= 1.0 || e.y >= 1.0) return 0.0;
    return min(floor(texture(uEggMask, e).r * 255.0 + 0.5), 128.0);
}
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
            // Level 0 leaves what lies behind unchanged; then the engine darkens
            // the result by the object's light, what lies behind included:
            // L * (tint * a + behind * (1 - a)).
            float a = min(gray, 7.0) / 7.0;
            float light = min(floor(sceneLight() * 128.0 + 0.0001), 128.0) / 128.0;
            outColor = vec4(uTint * a * light, 1.0 - light * (1.0 - a));
        } else {
            outColor = texel;
            // A sprite's animated colours (alpha 254) stay at full light.
            bool fullBright = abs(texel.a * 255.0 - 254.0) < 0.5;
            if (fullBright) outColor.a = 1.0;
            // _dark_trans_buf_to_buf: the engine darkens a texel by its light in
            // 128 steps, on its 5-bit colour rounded down (setIntensityTableColor).
            float steps = fullBright ? 128.0 : floor(sceneLight() * 128.0 + 0.0001);
            if (steps < 128.0) outColor.rgb = floor(floor(outColor.rgb * 31.0 + 0.5) * steps / 128.0) / 31.0;
            if (uEgg != 0) {
                float m = eggShare();
                if (m > 0.0) outColor.a *= m / 128.0;
            }
            if (uEggPass != 0) {
                float m = vSeeThrough != 0 ? eggShare() : 0.0;
                if (uEggPass == 1) {
                    if (m > 0.0) discard;
                } else {
                    if (m <= 0.0) discard;
                    outColor.a *= m / 128.0;
                }
            }
        }
    }
}
)";

} // namespace

unsigned int gSceneProgram = 0;
int gSceneMvpLocation = -1;
int gSceneTextureLocation = -1;
int gSceneTexturedLocation = -1;
int gSceneColorLocation = -1;
int gSceneCutoffLocation = -1;
int gSceneDepthAtLocation = -1;
int gSceneTranslucentLocation = -1;
int gSceneTintLocation = -1;
int gSceneGlassGrayLocation = -1;
int gScenePickLocation = -1;
int gSceneEggLocation = -1;
int gSceneEggMaskLocation = -1;
int gSceneEggRectLocation = -1;
int gSceneViewportLocation = -1;
int gSceneLogicalLocation = -1;
int gSceneLightModeLocation = -1;
int gSceneLightMapLocation = -1;
int gSceneLightValueLocation = -1;
int gSceneLightSquLocation = -1;
int gSceneLightSqvLocation = -1;
int gSceneLightOriginLocation = -1;
int gSceneSeeThroughLocation = -1;
int gSceneSeeThroughObjectsLocation = -1;
int gSceneEggPassLocation = -1;

bool createSceneShader(std::string* error)
{
    unsigned int vertexShader = compileShader(0x8B31 /* GL_VERTEX_SHADER */, kSceneVertexShader);
    unsigned int fragmentShader = compileShader(0x8B30 /* GL_FRAGMENT_SHADER */, kSceneFragmentShader);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        *error = "static floor shader compilation failed";
        return false;
    }
    gSceneProgram = glCreateProgram();
    glAttachShader(gSceneProgram, vertexShader);
    glAttachShader(gSceneProgram, fragmentShader);
    glLinkProgram(gSceneProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    int linked = 0;
    glGetProgramiv(gSceneProgram, 0x8B82 /* GL_LINK_STATUS */, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(gSceneProgram, sizeof(log), nullptr, log);
        fprintf(stderr, "scene_renderer: static floor link failed: %s\n", log);
        fflush(stderr);
        *error = "static floor shader link failed";
        return false;
    }

    gSceneMvpLocation = glGetUniformLocation(gSceneProgram, "uMvp");
    gSceneTextureLocation = glGetUniformLocation(gSceneProgram, "uTexture");
    gSceneTexturedLocation = glGetUniformLocation(gSceneProgram, "uTextured");
    gSceneColorLocation = glGetUniformLocation(gSceneProgram, "uColor");
    gSceneCutoffLocation = glGetUniformLocation(gSceneProgram, "uCutoff");
    gSceneDepthAtLocation = glGetUniformLocation(gSceneProgram, "uDepthAt");
    gSceneTranslucentLocation = glGetUniformLocation(gSceneProgram, "uTranslucent");
    gSceneTintLocation = glGetUniformLocation(gSceneProgram, "uTint");
    gSceneGlassGrayLocation = glGetUniformLocation(gSceneProgram, "uGlassGray");
    gScenePickLocation = glGetUniformLocation(gSceneProgram, "uPick");
    gSceneEggLocation = glGetUniformLocation(gSceneProgram, "uEgg");
    gSceneEggMaskLocation = glGetUniformLocation(gSceneProgram, "uEggMask");
    gSceneEggRectLocation = glGetUniformLocation(gSceneProgram, "uEggRect");
    gSceneViewportLocation = glGetUniformLocation(gSceneProgram, "uViewport");
    gSceneLogicalLocation = glGetUniformLocation(gSceneProgram, "uLogical");
    gSceneLightModeLocation = glGetUniformLocation(gSceneProgram, "uLightMode");
    gSceneLightMapLocation = glGetUniformLocation(gSceneProgram, "uLightMap");
    gSceneLightValueLocation = glGetUniformLocation(gSceneProgram, "uLightValue");
    gSceneLightSquLocation = glGetUniformLocation(gSceneProgram, "uLightSqu");
    gSceneLightSqvLocation = glGetUniformLocation(gSceneProgram, "uLightSqv");
    gSceneLightOriginLocation = glGetUniformLocation(gSceneProgram, "uLightOrigin");
    gSceneSeeThroughLocation = glGetUniformLocation(gSceneProgram, "uSeeThrough");
    gSceneSeeThroughObjectsLocation = glGetUniformLocation(gSceneProgram, "uSeeThroughObjects");
    gSceneEggPassLocation = glGetUniformLocation(gSceneProgram, "uEggPass");
    if (gSceneMvpLocation < 0 || gSceneTextureLocation < 0
        || gSceneTexturedLocation < 0 || gSceneColorLocation < 0 || gSceneCutoffLocation < 0
        || gSceneDepthAtLocation < 0 || gSceneTranslucentLocation < 0 || gSceneTintLocation < 0
        || gSceneGlassGrayLocation < 0 || gScenePickLocation < 0 || gSceneEggLocation < 0
        || gSceneEggMaskLocation < 0 || gSceneEggRectLocation < 0 || gSceneViewportLocation < 0
        || gSceneLogicalLocation < 0 || gSceneSeeThroughLocation < 0 || gSceneSeeThroughObjectsLocation < 0
        || gSceneEggPassLocation < 0 || gSceneLightModeLocation < 0 || gSceneLightMapLocation < 0
        || gSceneLightValueLocation < 0 || gSceneLightSquLocation < 0 || gSceneLightSqvLocation < 0
        || gSceneLightOriginLocation < 0) {
        *error = "static floor shader uniforms are missing";
        return false;
    }
    return true;
}

void destroySceneShader()
{
    if (gContext != nullptr && gSceneProgram != 0) glDeleteProgram(gSceneProgram);
    gSceneProgram = 0;
    gSceneMvpLocation = -1;
    gSceneTextureLocation = -1;
    gSceneTexturedLocation = -1;
    gSceneColorLocation = -1;
    gSceneCutoffLocation = -1;
}

} // namespace internal
} // namespace scene_renderer
} // namespace fallout
