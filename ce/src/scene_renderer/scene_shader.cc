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
uniform sampler2D uSeeThroughObjects; // per object row: R see-through, G full light
uniform int uLightMode;
out vec2 vTexcoord;
out vec3 vWorld;
out vec2 vWorldXZ;
flat out int vSeeThrough;
flat out float vFullLight;
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vTexcoord = aTexcoord;
    vWorld = aPosition;
    vWorldXZ = aPosition.xz;
    vSeeThrough = uSeeThrough == 2 ? 1 : 0;
    vFullLight = 0.0;
    if ((uSeeThrough == 1 || uLightMode == 2) && aObject >= 0.0) {
        int row = int(aObject + 0.5);
        vec2 entry = texelFetch(uSeeThroughObjects, ivec2(row % 1024, row / 1024), 0).rg;
        if (uSeeThrough == 1) vSeeThrough = entry.r > 0.5 ? 1 : 0;
        if (uLightMode == 2) vFullLight = entry.g;
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
in vec2 vWorldXZ;
flat in int vSeeThrough;
flat in float vFullLight;
uniform sampler2D uTexture;
uniform int uTextured;
uniform vec4 uColor;
uniform float uCutoff;
uniform int uTranslucent; // the engine's translucent blend: steam, glass, energy, ...
uniform vec3 uTint;
uniform int uGlassGray;
uniform int uPick; // the pick pass: every texel shown is the object's id colour
uniform int uClipEnabled;
uniform vec3 uClipPlane0;
uniform vec3 uClipPlane1;
uniform vec3 uClipPlane2;
uniform vec3 uClipPlane3;
uniform int uEgg; // the see-through circle around the hero
uniform sampler2D uEggMask;
uniform vec4 uEggRect; // game logical pixels: left, top, width, height
uniform vec4 uViewport; // GL viewport of the game picture
uniform vec2 uLogical; // the game picture's size in game logical pixels
// The egg over walls the hero stands behind: 1 leaves out what it shows
// through, 2 draws only that, blended over what lies behind.
uniform int uEggPass;
// 0 unlit, 1 the floor at the fragment, 2 walls at the fragment, by the lights
// they face, 3 a sprite at its hex (uLightAnchor), 4 uLightValue, 5 the hex
// shadow pass (046).
uniform int uLightMode;
uniform float uLightValue;
uniform vec2 uLightAnchor; // GL ground point (x, z) of a sprite's hex
uniform float uAmbient; // the map's ambient light, at least the engine's base
uniform int uLightCount;
uniform vec4 uLightPos[32]; // GL point it shines from; w: its reach in hexes (lightDistance + 1)
uniform vec4 uLightColor[32]; // rgb, the brightest channel 1; a: its intensity
uniform vec2 uLightWindow[32]; // first hex column and row of its 21 x 21 reach window
uniform sampler2D uLightReach; // the hexes each light reaches, a window per light
// The many small lights, per hex as the engine adds them up: rgb their colour
// times a, a their light (1 full), 200 x 200.
uniform sampler2D uLightMap;
uniform float uHexUnit; // GL length of one hex step on the ground
uniform vec3 uTowardCamera; // GL direction towards the game's camera
uniform vec2 uLightSqu; // ground (u, v) to game pixels
uniform vec2 uLightSqv;
uniform vec2 uLightOrigin;
// The sun's shadow map (shadow.cc): 1 on, the world-to-map matrix, the map
// itself (unit 5), the sun's strength now, the lookup snap in GL units (the
// [3d] shadow_pixel_size grid) and the compare bias in map depth.
uniform bool uShadowOn;
uniform mat4 uShadowMatrix;
uniform sampler2D uShadowMap;
uniform float uShadowStrength;
uniform float uShadowStep;
uniform float uShadowBias;
uniform float uShadowRange; // the map's depth span in GL units
// A sprite card's own share of the shadow at its anchor, faded over ~150 ms
// on the CPU from the hex pass (046); the floor and the walls test per
// fragment instead.
uniform float uSpriteShadow;
out vec4 outColor;
// What a texel shines by itself for the glow (task 022): the colour a
// full-bright texel keeps, premultiplied by its shown coverage; black
// elsewhere. Only read when the glow renders the scene into its own
// framebuffer; with none bound the value is dropped.
layout(location = 1) out vec4 outEmissive;
// The hex (column, row) under GL point w, between hex centers. The hex grid's
// odd columns sit half a step lower; their average place is close enough for light.
vec2 hexAt(vec3 w) {
    vec2 px = uLightOrigin + (w.x + 50.0) * uLightSqu + (w.z + 50.0) * uLightSqv;
    float hy = ((px.x - 20.0) + 4.0 * (px.y - 11.0)) / 64.0;
    float a = ((px.x - 20.0) - 16.0 * hy) / 24.0;
    return vec2(199.0 - a, hy);
}
// The game's light at GL point w and its colour: rgb the tint, a the light
// (1 full). As the engine adds it up: at least the ambient level, else its
// base plus every light that reaches the hex (walls stop it, as the engine
// spreads it), fading over its distance. The small lights come per hex,
// between hex centers; the big ones smooth, from their own point, and a wall
// (`facing`, normal n) takes more of one it faces.
vec4 pointLights(vec3 w, vec3 n, bool facing) {
    vec2 hex = hexAt(w);
    vec4 small = texture(uLightMap, (hex + 0.5) / 200.0);
    float sum = small.a;
    vec3 colored = small.rgb;
    for (int i = 0; i < uLightCount; i++) {
        vec2 local = hex - uLightWindow[i];
        if (local.x < 1.0 || local.y < 1.0 || local.x > 19.0 || local.y > 19.0) continue;
        float reached = texture(uLightReach, vec2((local.x + 0.5) / 21.0, (float(i) * 21.0 + local.y + 0.5) / 672.0)).r;
        if (reached <= 0.0) continue;
        vec3 toLight = uLightPos[i].xyz - w;
        float hexes = length(toLight.xz) / uHexUnit;
        float span = uLightPos[i].w;
        // obj_adjust_light: ring k gets I - k (I - base) / span; between rings.
        float ring = hexes / span;
        if (ring >= 1.0) continue;
        float c = mix(uLightColor[i].a, 655.0 / 65536.0, ring) * reached;
        if (facing) c *= 0.55 + 0.65 * max(dot(n, normalize(toLight)), 0.0);
        sum += c;
        colored += c * uLightColor[i].rgb;
    }
    float light = min(max(uAmbient, 655.0 / 65536.0 + sum), 1.0);
    vec3 tint = vec3(1.0);
    if (sum > 0.0 && light > uAmbient) tint = (vec3(uAmbient) + (light - uAmbient) * colored / sum) / light;
    // A colour changes the hue, not how bright the engine lights it.
    tint = min(tint / max(dot(tint, vec3(0.299, 0.587, 0.114)), 0.001), vec3(1.6));
    return vec4(tint, light);
}
vec4 sceneLighting() {
    if (uLightMode == 1) return pointLights(vWorld, vec3(0.0, 1.0, 0.0), false);
    if (uLightMode == 2) {
        if (vFullLight > 0.5) return vec4(1.0);
        vec3 n = normalize(cross(dFdx(vWorld), dFdy(vWorld)));
        if (dot(n, uTowardCamera) < 0.0) n = -n;
        return pointLights(vWorld, n, true);
    }
    if (uLightMode == 3) return pointLights(vec3(uLightAnchor.x, 0.0, uLightAnchor.y), vec3(0.0, 1.0, 0.0), false);
    if (uLightMode == 4) return vec4(vec3(1.0), uLightValue);
    return vec4(1.0);
}
// The sun's shadow at this fragment: the shadowed share, 0 lit to 1 fully in.
// The lookup snaps to a world grid of uShadowStep units (the [3d]
// shadow_pixel_size game pixels), so the edge stays put while the camera and
// the sun move. Four taps a step across keep the core sharp and round the
// snap's stairs off. The shadow is strongest at its caster and fades to
// nothing with the distance the sun's ray travels from it (the map depth the
// caster wrote vs the fragment's own): a wall keeps its dark contact line, a
// fence's shadow melts along its length, and a roof's far stain across a yard
// is gone.
float shadowPast(vec2 xz, float y) {
    vec2 snapped = floor(xz / uShadowStep + 0.5) * uShadowStep;
    vec4 s = uShadowMatrix * vec4(snapped.x, y, snapped.y, 1.0);
    vec3 p = s.xyz / s.w * 0.5 + 0.5;
    if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z >= 1.0) return -1.0;
    float diff = p.z - (texture(uShadowMap, p.xy).r + uShadowBias);
    return diff > 0.0 ? diff : -1.0;
}
float sunShadow() {
    float sum = 0.0;
    for (int i = 0; i < 4; i++) {
        vec2 offset = vec2(float(i & 1), float(i >> 1)) * 2.0 - 1.0;
        float past = shadowPast(vWorld.xz + offset * uShadowStep, vWorld.y);
        if (past >= 0.0) sum += 1.0 - smoothstep(0.0, 5.0, past * uShadowRange);
    }
    return sum * 0.25;
}
// The darkening inside a shadow, over the engine's own light: the game's
// painted shadows read about as a half. The floor and the walls take the
// fragment's faded share; a sprite takes its anchor's faded share (046);
// mode 5 is the hex pass itself, the pure factor the fade chases.
float shadowDarken(float light) {
    if (uShadowOn && uShadowStrength > 0.0) {
        if (uLightMode == 1 || uLightMode == 2) {
            light *= 1.0 - uShadowStrength * 0.5 * sunShadow();
        } else if (uLightMode == 3) {
            light *= 1.0 - uShadowStrength * 0.5 * uSpriteShadow;
        } else if (uLightMode == 5) {
            light *= 1.0 - sunShadow();
        }
    }
    return light;
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
    if (uClipEnabled != 0) {
        if (dot(uClipPlane0.xy, vWorldXZ) + uClipPlane0.z < -0.00001
            || dot(uClipPlane1.xy, vWorldXZ) + uClipPlane1.z < -0.00001
            || dot(uClipPlane2.xy, vWorldXZ) + uClipPlane2.z < -0.00001
            || dot(uClipPlane3.xy, vWorldXZ) + uClipPlane3.z < -0.00001) discard;
    }
    if (uTextured == 0) {
        outColor = uColor;
        outEmissive = vec4(0.0);
    } else {
        vec4 texel = texture(uTexture, vTexcoord);
        if (texel.a < uCutoff) discard;
        if (uPick != 0) {
            outColor = uColor;
            outEmissive = vec4(0.0);
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
            float light = min(floor(shadowDarken(sceneLighting().a) * 128.0 + 0.0001), 128.0) / 128.0;
            outColor = vec4(uTint * a * light, 1.0 - light * (1.0 - a));
            outEmissive = vec4(0.0);
        } else {
            outColor = texel;
            // A sprite's animated colours (alpha 254, the exporter's sprite
            // marker) stay at full light; alpha 252 only glows, the tiles and
            // walls darken as everything does.
            bool fullBright = abs(texel.a * 255.0 - 254.0) < 0.5;
            outColor.a = 1.0;
            // _dark_trans_buf_to_buf: the engine darkens a texel by its light in
            // 128 steps, on its 5-bit colour rounded down (setIntensityTableColor).
            vec4 lit = fullBright ? vec4(1.0) : sceneLighting();
            lit.a = shadowDarken(lit.a);
            float steps = floor(lit.a * 128.0 + 0.0001);
            if (steps < 128.0) outColor.rgb = floor(floor(outColor.rgb * 31.0 + 0.5) * steps / 128.0) / 31.0;
            outColor.rgb *= lit.rgb;
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
            // What shines by itself: the marked texels (252, 254) always, plus
            // whatever the lit picture shows bright (a fire, a slime pool
            // under its own light); the glow blurs this buffer.
            float mark = texel.a * 255.0;
            vec3 emission = mark > 251.5 && mark < 254.5 ? texel.rgb * outColor.a : vec3(0.0);
            vec3 bright = outColor.rgb * smoothstep(0.85, 0.97, max(max(outColor.r, outColor.g), outColor.b));
            outEmissive = vec4(max(emission, bright), 1.0);
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
int gSceneClipEnabledLocation = -1;
int gSceneClipPlaneLocations[4] = { -1, -1, -1, -1 };
int gSceneEggLocation = -1;
int gSceneEggMaskLocation = -1;
int gSceneEggRectLocation = -1;
int gSceneViewportLocation = -1;
int gSceneLogicalLocation = -1;
int gSceneLightModeLocation = -1;
int gSceneLightReachLocation = -1;
int gSceneLightMapLocation = -1;
int gSceneLightValueLocation = -1;
int gSceneLightSquLocation = -1;
int gSceneLightSqvLocation = -1;
int gSceneLightOriginLocation = -1;
int gSceneLightAnchorLocation = -1;
int gSceneAmbientLocation = -1;
int gSceneHexUnitLocation = -1;
int gSceneTowardCameraLocation = -1;
int gSceneLightCountLocation = -1;
int gSceneLightPosLocation = -1;
int gSceneLightColorLocation = -1;
int gSceneLightWindowLocation = -1;
int gSceneSeeThroughLocation = -1;
int gSceneSeeThroughObjectsLocation = -1;
int gSceneEggPassLocation = -1;
int gSceneShadowOnLocation = -1;
int gSceneShadowMatrixLocation = -1;
int gSceneShadowMapLocation = -1;
int gSceneShadowStrengthLocation = -1;
int gSceneShadowStepLocation = -1;
int gSceneShadowBiasLocation = -1;
int gSceneSpriteShadowLocation = -1;
int gSceneShadowRangeLocation = -1;

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
    gSceneClipEnabledLocation = glGetUniformLocation(gSceneProgram, "uClipEnabled");
    gSceneClipPlaneLocations[0] = glGetUniformLocation(gSceneProgram, "uClipPlane0");
    gSceneClipPlaneLocations[1] = glGetUniformLocation(gSceneProgram, "uClipPlane1");
    gSceneClipPlaneLocations[2] = glGetUniformLocation(gSceneProgram, "uClipPlane2");
    gSceneClipPlaneLocations[3] = glGetUniformLocation(gSceneProgram, "uClipPlane3");
    gSceneEggLocation = glGetUniformLocation(gSceneProgram, "uEgg");
    gSceneEggMaskLocation = glGetUniformLocation(gSceneProgram, "uEggMask");
    gSceneEggRectLocation = glGetUniformLocation(gSceneProgram, "uEggRect");
    gSceneViewportLocation = glGetUniformLocation(gSceneProgram, "uViewport");
    gSceneLogicalLocation = glGetUniformLocation(gSceneProgram, "uLogical");
    gSceneLightModeLocation = glGetUniformLocation(gSceneProgram, "uLightMode");
    gSceneLightReachLocation = glGetUniformLocation(gSceneProgram, "uLightReach");
    gSceneLightMapLocation = glGetUniformLocation(gSceneProgram, "uLightMap");
    gSceneLightValueLocation = glGetUniformLocation(gSceneProgram, "uLightValue");
    gSceneLightSquLocation = glGetUniformLocation(gSceneProgram, "uLightSqu");
    gSceneLightSqvLocation = glGetUniformLocation(gSceneProgram, "uLightSqv");
    gSceneLightOriginLocation = glGetUniformLocation(gSceneProgram, "uLightOrigin");
    gSceneLightAnchorLocation = glGetUniformLocation(gSceneProgram, "uLightAnchor");
    gSceneAmbientLocation = glGetUniformLocation(gSceneProgram, "uAmbient");
    gSceneHexUnitLocation = glGetUniformLocation(gSceneProgram, "uHexUnit");
    gSceneTowardCameraLocation = glGetUniformLocation(gSceneProgram, "uTowardCamera");
    gSceneLightCountLocation = glGetUniformLocation(gSceneProgram, "uLightCount");
    gSceneLightPosLocation = glGetUniformLocation(gSceneProgram, "uLightPos");
    gSceneLightColorLocation = glGetUniformLocation(gSceneProgram, "uLightColor");
    gSceneLightWindowLocation = glGetUniformLocation(gSceneProgram, "uLightWindow");
    gSceneSeeThroughLocation = glGetUniformLocation(gSceneProgram, "uSeeThrough");
    gSceneSeeThroughObjectsLocation = glGetUniformLocation(gSceneProgram, "uSeeThroughObjects");
    gSceneEggPassLocation = glGetUniformLocation(gSceneProgram, "uEggPass");
    gSceneShadowOnLocation = glGetUniformLocation(gSceneProgram, "uShadowOn");
    gSceneShadowMatrixLocation = glGetUniformLocation(gSceneProgram, "uShadowMatrix");
    gSceneShadowMapLocation = glGetUniformLocation(gSceneProgram, "uShadowMap");
    gSceneShadowStrengthLocation = glGetUniformLocation(gSceneProgram, "uShadowStrength");
    gSceneShadowStepLocation = glGetUniformLocation(gSceneProgram, "uShadowStep");
    gSceneShadowBiasLocation = glGetUniformLocation(gSceneProgram, "uShadowBias");
    gSceneSpriteShadowLocation = glGetUniformLocation(gSceneProgram, "uSpriteShadow");
    gSceneShadowRangeLocation = glGetUniformLocation(gSceneProgram, "uShadowRange");
    if (gSceneMvpLocation < 0 || gSceneTextureLocation < 0
        || gSceneTexturedLocation < 0 || gSceneColorLocation < 0 || gSceneCutoffLocation < 0
        || gSceneDepthAtLocation < 0 || gSceneTranslucentLocation < 0 || gSceneTintLocation < 0
        || gSceneGlassGrayLocation < 0 || gScenePickLocation < 0 || gSceneEggLocation < 0
        || gSceneEggMaskLocation < 0 || gSceneEggRectLocation < 0 || gSceneViewportLocation < 0
        || gSceneLogicalLocation < 0 || gSceneSeeThroughLocation < 0 || gSceneSeeThroughObjectsLocation < 0
        || gSceneEggPassLocation < 0 || gSceneLightModeLocation < 0 || gSceneLightReachLocation < 0
        || gSceneLightMapLocation < 0
        || gSceneLightValueLocation < 0 || gSceneLightSquLocation < 0 || gSceneLightSqvLocation < 0
        || gSceneLightOriginLocation < 0 || gSceneLightAnchorLocation < 0 || gSceneAmbientLocation < 0
        || gSceneHexUnitLocation < 0 || gSceneTowardCameraLocation < 0 || gSceneLightCountLocation < 0
        || gSceneLightPosLocation < 0 || gSceneLightColorLocation < 0 || gSceneLightWindowLocation < 0
        || gSceneClipEnabledLocation < 0
        || gSceneClipPlaneLocations[0] < 0 || gSceneClipPlaneLocations[1] < 0
        || gSceneClipPlaneLocations[2] < 0 || gSceneClipPlaneLocations[3] < 0) {
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
    gSceneLightModeLocation = -1;
    gSceneLightMapLocation = -1;
    gSceneLightValueLocation = -1;
    gSceneLightSquLocation = -1;
    gSceneLightSqvLocation = -1;
    gSceneLightOriginLocation = -1;
    gSceneClipEnabledLocation = -1;
    for (int index = 0; index < 4; index++) gSceneClipPlaneLocations[index] = -1;
}

} // namespace internal
} // namespace scene_renderer
} // namespace fallout
