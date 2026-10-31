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
// The wall casters' floor copies draw solid: only they set this vertex
// attribute, every other VAO leaves it at 0.
layout(location = 4) in float aShadowSolid;
layout(location = 6) in float aCasterId; // the depth pass: the caster's object row + 1
// walls.bin: the plane its face's depth points span; zero (live cards, whose
// points are on their own lines of sight already): the point as it is.
layout(location = 5) in vec3 aDepthNormal;
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
out float vShadowSolid;
out float vCasterId; // the depth pass writes it into the map's id channel
out float vSelfId; // the screen piece's own object row + 1, for the compare
flat out int vSeeThrough;
flat out float vFullLight;
flat out int vDepthOnPlane;
flat out vec4 vDepthPlane; // normal, and its dot with the plane's points
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vTexcoord = aTexcoord;
    vWorld = aPosition;
    vWorldXZ = aPosition.xz;
    vShadowSolid = aShadowSolid;
    vCasterId = aCasterId;
    vSelfId = aObject + 1.0;
    vSeeThrough = uSeeThrough == 2 ? 1 : 0;
    vFullLight = 0.0;
    if ((uSeeThrough == 1 || uLightMode == 2) && aObject >= 0.0) {
        int row = int(aObject + 0.5);
        vec2 entry = texelFetch(uSeeThroughObjects, ivec2(row % 1024, row / 1024), 0).rg;
        if (uSeeThrough == 1) vSeeThrough = entry.r > 0.5 ? 1 : 0;
        if (uLightMode == 2) vFullLight = entry.g;
    }
    vDepthOnPlane = uDepthAt != 0 && dot(aDepthNormal, aDepthNormal) > 0.5 ? 1 : 0;
    vDepthPlane = vec4(aDepthNormal, dot(aDepthNormal, aDepthAt));
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
in float vShadowSolid;
in float vCasterId;
in float vSelfId;
flat in int vSeeThrough;
flat in float vFullLight;
flat in int vDepthOnPlane;
flat in vec4 vDepthPlane;
uniform mat4 uMvp;
uniform sampler2D uTexture;
uniform int uTextured;
uniform int uCloseupFilter;
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
uniform vec4 uFlyEye; // xyz the free camera's eye, w 1 while it flies (090)
uniform vec2 uLightSqu; // ground (u, v) to game pixels
uniform vec2 uLightSqv;
uniform vec2 uLightOrigin;
// The sun's shadow map (shadow.cc): 1 on, the world-to-map matrix, the map
// itself (unit 5) and its caster ids (unit 6, 090), the sun's strength now,
// the sun's direction (the face test), the AA ring's span in GL units (the
// [3d] shadow_pixel_size setting, the edge softness) and the compare bias in
// map depth, a quarter of one texel's depth (090).
uniform bool uShadowOn;
uniform mat4 uShadowMatrix;
uniform sampler2D uShadowMap;
uniform sampler2D uShadowIds;
uniform float uShadowStrength;
uniform float uShadowDensity; // the [3d] shadow_density setting, scales the darkening
uniform vec3 uShadowLight; // towards the sun, GL (x, y up, z)
uniform float uShadowStep;
uniform float uShadowBias;
uniform float uShadowContact; // a blocker this near the receiver stands on it, map depth (090)
uniform int uShadowDebug; // 1: raw factor, 2: the face test, 3: the coverage term (062 diagnostics)
// 082 round 6: 1 while the depth map renders (the pick colour carries the
// caster's object number for the map's id channel); whose own texels count
// as empty at the compare (the wall draws read vSelfId, the rest take
// uShadowSelfId, 0 = nobody); and the feet point a whole sprite takes its
// shadow factor at, in world (x, z) on the ground.
uniform int uShadowPass;
uniform int uShadowSelfFromVertex;
uniform float uShadowSelfId;
uniform vec2 uShadowAnchor;
// x 1: a low card reads per pixel on the ground at height y; zw a door's
// wall normal (x, z) (090).
uniform vec4 uShadowGround;
// 1: a card takes its whole verdict at the feet point (109): the people,
// whose per-pixel read straddles a shadow edge over several pixels of travel.
uniform float uShadowFeet;
uniform bool uCloudOn;
uniform vec2 uCloudOffset;
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
)" R"(
float cloudHash(vec2 cell) {
    return fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
}
float cloudNoise(vec2 lattice, float wrap) {
    vec2 whole = floor(lattice), part = fract(lattice);
    part = part * part * (3.0 - 2.0 * part);
    float a = cloudHash(mod(whole, wrap)), b = cloudHash(mod(whole + vec2(1.0, 0.0), wrap));
    float c = cloudHash(mod(whole + vec2(0.0, 1.0), wrap)), d = cloudHash(mod(whole + vec2(1.0, 1.0), wrap));
    return mix(mix(a, b, part.x), mix(c, d, part.x), part.y);
}
float cloudAmbient(vec3 w) {
    if (!uCloudOn) return 1.0;
    vec2 t = w.xz + uCloudOffset;
    float cover = cloudNoise(mod(t, 48.0) / 12.0, 4.0) * 0.65
        + cloudNoise(mod(t + 17.0, 48.0) / 6.0, 8.0) * 0.35;
    return 1.0 - uShadowStrength * 0.35 * smoothstep(0.52, 0.74, cover);
}
vec4 pointLights(vec3 w, vec3 n, bool facing, float ambientScale) {
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
    float ambient = uAmbient * ambientScale;
    float light = min(max(ambient, 655.0 / 65536.0 + sum), 1.0);
    vec3 tint = vec3(1.0);
    if (sum > 0.0 && light > ambient) tint = (vec3(ambient) + (light - ambient) * colored / sum) / light;
    // A colour changes the hue, not how bright the engine lights it.
    tint = min(tint / max(dot(tint, vec3(0.299, 0.587, 0.114)), 0.001), vec3(1.6));
    return vec4(tint, light);
}
// The cross of the screen derivatives of vWorld, taken at the top of main()
// while all four pixels of the 2x2 quad still run (090): taken after a
// neighbour's discard (the floor tiles' transparent joints, a window's art
// holes) the derivatives are undefined, NVIDIA gives zero, the normal turned
// NaN and every compare against it failed: a lit pixel inside the shadow on
// every tile joint and around every window.
vec3 gFaceCross;
vec4 sceneLighting() {
    if (uLightMode == 1) return pointLights(vWorld, vec3(0.0, 1.0, 0.0), false, cloudAmbient(vWorld));
    if (uLightMode == 2) {
        if (vFullLight > 0.75) return vec4(1.0);
        vec3 n = normalize(gFaceCross);
        if (dot(n, uTowardCamera) < 0.0) n = -n;
        return pointLights(vWorld, n, true, cloudAmbient(vWorld));
    }
    if (uLightMode == 3) {
        vec3 anchor = vec3(uLightAnchor.x, 0.0, uLightAnchor.y);
        return pointLights(anchor, vec3(0.0, 1.0, 0.0), false, cloudAmbient(anchor));
    }
    if (uLightMode == 4) return vec4(vec3(1.0), uLightValue);
    return vec4(1.0);
}
// The sun's shadow at this fragment: the shadowed share, 0 lit to 1 fully in.
// Crisp by design (082): one even density inside, an edge about one map texel
// soft, no distance melt (the sun never stands lower than 75 degrees, a
// 2-unit wall casts at most 0.54 units of shadow, there is nothing to melt).
// The map samples NEAREST (bilinear depth mixes the caster silhouette with
// the empty far depth: a phantom far blocker ate the contact line and the
// shadow faded from its middle, the owner's round 2); the disk of taps a
// uShadowStep wide (the [3d] shadow_pixel_size setting, the edge softness)
// antialiases the edge.
// A world point in the map: uv and depth, 0..1.
vec3 shadowTex(vec3 w) {
    vec4 s = uShadowMatrix * vec4(w, 1.0);
    return s.xyz / s.w * 0.5 + 0.5;
}
// The face's outward normal from the screen derivatives, towards the camera
// side, as the point lights read it (sceneLighting).
vec3 shadowFaceNormal() {
    vec3 n = normalize(gFaceCross);
    if (dot(n, uTowardCamera) < 0.0) n = -n;
    return n;
}
// The plane the face's art was painted on, towards the camera: in the game
// view a wall's top and a corner's strip carry texels of the front face's
// picture (their depth points lie on it, vDepthPlane), so they take the
// front face's side of the sun. Lit by their own normals the one-pixel caps
// of every wall and fence rail and the corner strips read lit beside their
// shaded wall (090, the owner's 02). Elsewhere: the face's own normal.
vec3 shadowPaintedNormal(vec3 own) {
    if (vDepthOnPlane == 0) return own;
    vec3 n = normalize(vDepthPlane.xyz);
    if (dot(n, uTowardCamera) < 0.0) n = -n;
    return n;
}
// A basis of the face's own plane, so the taps spread across the face and
// never into the body behind it.
void shadowFaceBasis(vec3 n, out vec3 t1, out vec3 t2) {
    vec3 helper = abs(n.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    t1 = normalize(cross(helper, n));
    t2 = cross(n, t1);
}
// The receiver-plane compare (090): how the map depth of the receiver's own
// plane changes per unit of map uv. The map is orthographic, so this is exact
// across the whole face; a face nearly edge-on to the sun is clamped.
vec2 shadowPlaneSlope(vec3 n) {
    vec3 t1, t2;
    shadowFaceBasis(n, t1, t2);
    vec3 a = 0.5 * (uShadowMatrix * vec4(t1, 0.0)).xyz;
    vec3 b = 0.5 * (uShadowMatrix * vec4(t2, 0.0)).xyz;
    float det = a.x * b.y - a.y * b.x;
    if (abs(det) < 1e-12) return vec2(0.0);
    vec2 slope = vec2(a.z * b.y - b.z * a.y, b.z * a.x - a.z * b.x) / det;
    return clamp(slope, vec2(-16.0), vec2(16.0));
}
// One tap's signed depth difference, receiver minus map, in map depth: the
// receiver's depth carried along its own plane to the center of the texel
// the tap lands in, so a caster lying in the receiver's plane compares equal
// whoever wrote it (the acne and hatching of 082). Negative where the tap
// falls outside the box or on a texel the piece itself wrote: an object must
// not shadow itself (082 round 6, mechanism 2).
float shadowTexelDiff(vec3 p, ivec2 texel, vec2 slope, float selfId) {
    vec2 size = vec2(textureSize(uShadowMap, 0));
    if (any(lessThan(texel, ivec2(0))) || any(greaterThanEqual(texel, ivec2(size)))) return -1.0;
    float id = texelFetch(uShadowIds, texel, 0).r * 1024.0;
    if (id > 0.5 && abs(id - selfId) < 0.5) return -1.0;
    float receiver = p.z + dot(slope, (vec2(texel) + 0.5) / size - p.xy);
    return receiver - texelFetch(uShadowMap, texel, 0).r;
}
float shadowTapDiff(vec3 q, vec2 slope, float selfId) {
    vec3 p = shadowTex(q);
    if (p.x < 0.0 || p.x >= 1.0 || p.y < 0.0 || p.y >= 1.0 || p.z >= 1.0) return -1.0;
    return shadowTexelDiff(p, ivec2(p.xy * vec2(textureSize(uShadowMap, 0))), slope, selfId);
}
// Whether a tap is in shadow: a blocker in front of the receiver's plane by
// more than the bias, and on the ground by more than `contact`. Closer, it
// stands on the ground: the ground seen through the stepped transparent foot
// of a wall's sprite lies just behind its face, under the wall, and read the
// face as its blocker (090, a dotted dark line along every wall's foot in
// the evening). A real shadow behind a wall, a crate or a fence rail stands
// far deeper. A wall's face keeps the bias alone: the roof resting on its
// top shades it to the eave.
bool shadowTapHit(vec3 q, vec2 slope, float selfId, float contact) {
    return shadowTapDiff(q, slope, selfId) > max(uShadowBias, contact);
}
// A three-radius disk of 25 taps (1.0, 0.65 turned 22.5 degrees, 0.35
// turned 11.25): fewer rings quantize the edge into flat steps a
// ring-radius apart, the owner reads them as layers and stairs.
const vec2 kShadowTaps[25] = vec2[25](
    vec2(0.0000, 0.0000), vec2(1.0000, 0.0000), vec2(0.7071, 0.7071), vec2(0.0000, 1.0000),
    vec2(-0.7071, 0.7071), vec2(-1.0000, 0.0000), vec2(-0.7071, -0.7071), vec2(-0.0000, -1.0000),
    vec2(0.7071, -0.7071), vec2(0.6005, 0.2487), vec2(0.2487, 0.6005), vec2(-0.2487, 0.6005),
    vec2(-0.6005, 0.2487), vec2(-0.6005, -0.2487), vec2(-0.2487, -0.6005), vec2(0.2487, -0.6005),
    vec2(0.6005, -0.2487), vec2(0.3433, 0.0683), vec2(0.1944, 0.2910), vec2(-0.0683, 0.3433),
    vec2(-0.2910, 0.1944), vec2(-0.3433, -0.0683), vec2(-0.1944, -0.2910), vec2(0.0683, -0.3433),
    vec2(0.2910, -0.1944));
// MSVC caps one raw string literal, so the fragment shader continues as an
// adjacent literal; the compiler concatenates them.
)"
R"(// The 25-tap disk (a three-radius ring, 1.0, 0.65 turned 22.5 degrees, 0.35
// turned 11.25: fewer rings quantize the edge into flat steps a ring-radius
// apart, the owner reads them as layers and stairs) against the map, spread
// in the receiver's own plane. Every tap counts (090): 082's early out on the
// center tap alone lit a whole pixel from one empty texel (holes, stars).
// On the ground a tap counts only when no caster's foot stands between it
// and the center (090): a texel whose blocker stands on the ground, past the
// bias and within uShadowContact. Else the disk reached across a sun-facing
// wall's foot into the shadow behind it and grained the lit ground at its
// base; the outline a wall's top casts on open ground keeps its soft edge.
// Only on the ground (090): on an upright face under an eave the roof's
// edge stands within uShadowContact of the face, the taps towards it were
// dropped, and the few left combed the eave's outline into teeth.
// On an upright face each axis of the disk stretches so that it spans a
// full radius across the map (090): where the sun grazes the face a radius
// laid on it crosses less than a texel, and the map's stair along a
// straight outline read as a comb. At most fourfold.
float shadowAxisStretch(vec3 t) {
    float along = dot(t, uShadowLight);
    return 1.0 / max(sqrt(max(1.0 - along * along, 0.0)), 0.25);
}
float shadowCoverage(vec3 base, vec3 t1, vec3 t2, vec2 slope, float selfId) {
    float contact = uShadowContact;
    float sum = 0.0;
    float count = 0.0;
    bool ground = abs(cross(t1, t2).y) > 0.5;
    vec3 a1 = t1 * (ground ? 1.0 : shadowAxisStretch(t1)) * uShadowStep;
    vec3 a2 = t2 * (ground ? 1.0 : shadowAxisStretch(t2)) * uShadowStep;
    for (int i = 0; i < 25; i++) {
        vec3 q = base + a1 * kShadowTaps[i].x + a2 * kShadowTaps[i].y;
        bool reached = true;
        for (int k = 0; k <= 3 && i > 0 && ground; k++) {
            float on = shadowTapDiff(mix(base, q, float(k) * 0.25), slope, selfId);
            if (on > uShadowBias && on <= contact) {
                reached = false;
                break;
            }
        }
        if (!reached) continue;
        count += 1.0;
        if (shadowTapHit(q, slope, selfId, contact)) sum += 1.0;
    }
    return sum / count;
}
// Towards the eye that sees this fragment: the game's parallel direction, or
// the free camera's own ray (090: a low card under the fly camera followed
// the game's direction and its shadow slid off the ground's around it).
vec3 towardViewer() {
    return uFlyEye.w > 0.5 ? normalize(uFlyEye.xyz - vWorld) : uTowardCamera;
}
// The caster id the map holds where a world point projects, 0 for none.
float shadowCasterId(vec3 q) {
    vec3 m = shadowTex(q);
    return floor(texelFetch(uShadowIds, ivec2(m.xy * vec2(textureSize(uShadowMap, 0))), 0).r * 1024.0 + 0.5);
}
// Whether the eye sees this ground point through a hole in a wall the map
// holds solid (090): a window grate's art holes. The floor seen through them
// lies inside, in the shadow of the roof or the building's body, and dotted
// the lit pane with the grate's holes. The march goes from the point towards
// the eye until the first lit place; the point is seen through a face when
// that place lies on a caster's surface (just behind it a blocker within
// contact, just in front of it the caster itself, not the far ground a
// shadow's edge in the air shows) and the point's own blocker carries no id
// (a roof, a wall's body) while that face does. The ground behind a fence,
// shaded by the fence's own rails and seen between them, keeps its shadow.
// The game view only: under the free camera the march met the edges of low
// casters and lit whole patches of open shadow (24292 h7 fly 557 to 698).
bool seenThroughCaster(vec3 p) {
    vec3 v = towardViewer();
    if (uFlyEye.w > 0.5 || v.y < 0.05) return false;
    float reach = 3.0 / v.y; // a wall's height up the line of sight
    float shaded = 0.0;
    float lit = -1.0;
    for (int k = 1; k <= 24; k++) {
        float s = reach * float(k) / 24.0;
        if (shadowTapDiff(p + v * s, vec2(0.0), 0.0) <= uShadowBias) {
            lit = s;
            break;
        }
        shaded = s;
    }
    if (lit < 0.0) return false;
    for (int k = 0; k < 8; k++) {
        float s = 0.5 * (shaded + lit);
        if (shadowTapDiff(p + v * s, vec2(0.0), 0.0) <= uShadowBias) lit = s;
        else shaded = s;
    }
    float behind = shadowTapDiff(p + v * shaded, vec2(0.0), 0.0);
    float front = shadowTapDiff(p + v * lit, vec2(0.0), 0.0);
    if (behind > uShadowContact || front < -32.0 * uShadowBias) return false;
    return shadowCasterId(p) < 0.5 && shadowCasterId(p + v * shaded) > 0.5;
}
float sunShadow(float selfId) {
    vec3 n = shadowFaceNormal();
    // A face turned away from the sun is in its own shadow: no map lookup,
    // the shaded value (082). This is what striped the walls before: half of
    // the ground-plane taps fell inside the wall's own body and counted as
    // shadowed, changing from one map texel column to the next. The epsilon
    // swallows the cross product's rounding: at noon every vertical face
    // stands at N.L = 0 exactly, and without it the quads split by a
    // rounding sign into a flat shaded half and a lit, map-reading half.
    // The painted plane decides (090, shadowPaintedNormal).
    if (dot(shadowPaintedNormal(n), uShadowLight) <= 0.001) return 1.0;
    // Otherwise the taps lie on the face itself and compare its own plane
    // (090): no lift off the surface, nothing for a neighbour in the same
    // plane to shadow. A face the sun reaches only through its painted plane
    // (the side of a box under a front in the sun) compares on that plane.
    // A face painted with another face's picture (a wall's or a rail's top,
    // a corner strip: its painted plane turned off its own) takes that face's
    // factor where it borders it (090, the owner's decision): the point
    // carried onto the painted plane, the taps spread on it. Lit by its own
    // plane the one-pixel cap read lit on top of a wall in a cast shadow. On
    // an upright plane the point also steps one disk radius down, so the
    // disk lies on the face below its top edge and not half in the air,
    // where a roof's eave dotted it. The cap takes the majority, in or out:
    // a share between read as a half-lit band along the eave over its wall
    // in full shadow.
    vec3 base = vWorld;
    vec3 painted = shadowPaintedNormal(n);
    vec3 t1, t2;
    if (dot(n, painted) < 0.9) {
        vec3 plane = normalize(vDepthPlane.xyz);
        base -= plane * (dot(plane, vWorld) - vDepthPlane.w / length(vDepthPlane.xyz));
        if (abs(plane.y) < 0.1) base.y -= uShadowStep;
        shadowFaceBasis(painted, t1, t2);
        return step(0.5, shadowCoverage(base, t1, t2, shadowPlaneSlope(painted), selfId));
    }
    shadowFaceBasis(n, t1, t2);
    float coverage = shadowCoverage(base, t1, t2, shadowPlaneSlope(n), selfId);
    // On an upright face the disk's edge is a long ramp where the sun
    // grazes it (an eave's outline across a wall): a third as wide, round
    // the same middle (090).
    if (abs(n.y) < 0.5) coverage = clamp((coverage - 0.5) * 3.0 + 0.5, 0.0, 1.0);
    else if (coverage > 0.0 && seenThroughCaster(vWorld)) coverage = 0.0;
    return coverage;
}
// The whole sprite darkens at its feet (082 round 6, deliberately reversing
// 063's per-pixel read): one factor at the card's bottom center, lifted a
// little off the ground, the disk spread in the ground plane. The sprite's
// own number is excluded at the taps, else it would shadow itself at the
// feet; the edge read stays crisp, the sprite darkens over about one texel
// of travel as its feet cross it.
// The card takes its feet's majority, in or out (090): a door or a sign
// stands in its wall's line, the disk's taps that reach into the wall's body
// read shadowed, and a share between left the whole door a fifth or a half
// dark against its wall. The disk is about one texel wide, so a walker still
// darkens within one game pixel of travel.
float sunShadowAtAnchor(float selfId) {
    vec3 base = vec3(uShadowAnchor.x, 0.12, uShadowAnchor.y);
    vec2 slope = shadowPlaneSlope(vec3(0.0, 1.0, 0.0));
    return step(0.5, shadowCoverage(base, vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), slope, selfId));
}
// A low card (straw, grass, rubble; anything lying flat) is ground, not a
// body (090): each pixel reads the shadow where the game's line of sight
// through it meets the ground the card stands on, as the floor does. One
// factor per card broke a corral's shadow into a patchwork of whole cards.
// A card standing in its wall's plane (a door) is part of that wall: on a
// wall turned from the sun it takes the flat shaded value with it (090, the
// noon door lit in its shaded wall at 19265). Anyone else's card has no plane.
bool cardTurned() {
    vec3 w = vec3(uShadowGround.z, 0.0, uShadowGround.w);
    if (dot(w, w) < 0.5) return false;
    if (dot(w, towardViewer()) < 0.0) w = -w;
    return dot(w, uShadowLight) <= 0.001;
}
float sunShadowCard(float selfId) {
    if (cardTurned()) return 1.0;
    if (uShadowGround.x > 0.5) {
        vec3 g = vWorld;
        vec3 toward = towardViewer();
        if (toward.y > 0.01) g -= toward * ((vWorld.y - uShadowGround.y) / toward.y);
        g.y = uShadowGround.y;
        vec2 slope = shadowPlaneSlope(vec3(0.0, 1.0, 0.0));
        return shadowCoverage(g, vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), slope, selfId);
    }
    // The feet-verdict cards keep their whole verdict at the feet (090,
    // 109): the people, and a standing body taller than half a wall. The
    // per-pixel read straddles a shadow edge over several pixels of travel,
    // a walker came out half dark, and the read left lit contour gaps
    // between a figure in shadow and its shadow (082 round 6).
    if (uShadowFeet > 0.5) return sunShadowAtAnchor(selfId);
    // A standing card reads per pixel at its own place and height on its own
    // plane (063's read, now on 090's compare), so a shadow edge crosses the
    // card where the caster crosses it, as it crosses the floor and the
    // walls: the trough beside a fence's shadow stays lit, the barrel inside
    // a shadow darkens to its own share. A billboard has no sun side: no
    // face test, the map decides at every hour. A card that casts is
    // coplanar with its own map texels, and the receiver-plane compare reads
    // them equal: nothing shadows itself.
    vec3 n = normalize(gFaceCross);
    if (dot(n, towardViewer()) < 0.0) n = -n;
    vec3 t1, t2;
    shadowFaceBasis(n, t1, t2);
    float coverage = shadowCoverage(vWorld, t1, t2, shadowPlaneSlope(n), selfId);
    // The upright face's ramp (090): the disk's edge is a long ramp where
    // the sun grazes the plane; a third as wide, round the same middle.
    if (abs(n.y) < 0.5) coverage = clamp((coverage - 0.5) * 3.0 + 0.5, 0.0, 1.0);
    return coverage;
}
// Whose own texels count as empty for this draw: the wall pieces carry their
// object row in the vertices, everything else reads the uniform (0: nobody).
float shadowSelfId() {
    return uShadowSelfFromVertex != 0 ? vSelfId : uShadowSelfId;
}
// The darkening inside a shadow, over the engine's own light: the game's
// painted shadows read about as a half. Every fragment takes its own faded
// share: the floor and the walls at their texel, a card at its pixel's own
// place and height (063): a card no longer darkens whole from its anchor hex
// (hex-chunk stairs on cards, a man darkened where no shadow lies on him),
// and the edge antialiases on cards as it does on the ground. A card reads
// lighter than the ground: a standing figure keeps its read. A face turned
// from the sun takes the flat shaded value, the painted look (062 round 5,
// now by the face test instead of its own depth in the map, 082).
float shadowDarken(float light) {
    float selfId = shadowSelfId();
    if (uShadowDebug == 1 && (uLightMode == 1 || uLightMode == 2 || uLightMode == 3)) {
        // The raw factor as the light: a snapshot then shows the algorithm
        // itself, not the composed scene (062 diagnostics).
        return uLightMode == 3 ? sunShadowCard(selfId) : sunShadow(selfId);
    }
    if (uShadowDebug == 4 && (uLightMode == 1 || uLightMode == 2 || uLightMode == 3)) {
        // The center tap's signed difference, 0.05 of light per 4*uShadowBias
        // (one map texel of depth, 090): 0.5 at zero, every texel a blocker
        // stands in front adds 0.05, every texel behind subtracts.
        vec3 n = shadowFaceNormal();
        if (dot(n, uShadowLight) <= 0.001) return 0.0; // face turned: black
        float diff = shadowTapDiff(vWorld, shadowPlaneSlope(n), selfId);
        return clamp(0.5 + diff / (4.0 * uShadowBias) * 0.05, 0.0, 1.0);
    }
    if (uShadowDebug == 5 && (uLightMode == 1 || uLightMode == 2 || uLightMode == 3)) {
        // The center tap's difference in buckets of 4*uShadowBias (one map
        // texel of depth, 090): 0 lit or outside, then brighter every
        // texel-equivalent the nearest blocker stands in front of the
        // receiver, 0.95 past eight.
        vec3 n = shadowFaceNormal();
        if (dot(n, uShadowLight) <= 0.001) return 0.25;
        float diff = shadowTapDiff(vWorld, shadowPlaneSlope(n), selfId);
        if (diff <= uShadowBias) return 0.0;
        float texels = diff / (4.0 * uShadowBias);
        if (texels <= 1.0) return 0.35;
        if (texels <= 2.0) return 0.5;
        if (texels <= 4.0) return 0.65;
        if (texels <= 8.0) return 0.8;
        return 0.95;
    }
    if (uShadowOn && uShadowStrength > 0.0) {
        if (uLightMode == 1 || uLightMode == 2) {
            // The ground and the walls: crisp shadows, even inside, the flat
            // shaded value on the faces the sun does not hit (082). 0.35 by
            // the owner's eye (082 round 2): 0.425 read too saturated; the
            // [3d] shadow_density setting scales it, 0 turns the cast
            // shadows off.
            light *= 1.0 - uShadowStrength * uShadowDensity * 0.35 * sunShadow(selfId);
        } else if (uLightMode == 3) {
            // A sprite darkens whole at its feet (082 round 6): the old
            // per-pixel read (063) left lit contour gaps between a character
            // in shadow and its shadow. A card lying on the ground darkens
            // as the ground does (090). The ground, a turned card and a
            // per-pixel card read like the walls (0.35), so a shadow edge
            // runs off the floor onto a card without a step; the feet
            // verdict keeps 0.3, a whole body darkens lighter than the
            // ground and keeps its read (109).
            float density = uShadowFeet > 0.5 && uShadowGround.x <= 0.5 && !cardTurned() ? 0.3 : 0.35;
            light *= 1.0 - uShadowStrength * uShadowDensity * density * sunShadowCard(selfId);
        }
    }
    return light;
}
)"
R"(// _intensity_mask_buf_to_buf: inside the egg a nonzero mask value is the
// surface's share in 128ths; 0 outside it and where the surface stays whole.
float eggShare() {
    vec2 game = (gl_FragCoord.xy - uViewport.xy) / uViewport.zw * uLogical;
    game.y = uLogical.y - game.y;
    vec2 e = (game - uEggRect.xy) / uEggRect.zw;
    if (e.x < 0.0 || e.y < 0.0 || e.x >= 1.0 || e.y >= 1.0) return 0.0;
    return min(floor(texture(uEggMask, e).r * 255.0 + 0.5), 128.0);
}
// The depth of the face's depth plane (vDepthPlane) on this fragment's own
// line of sight. The exporter reached the depth points along the game's
// parallel ray; under the view's perspective that lands on another pixel, off
// by more than neighbouring hex rows' planes are apart. Here a solid, a wall
// and a card on one pixel compare their planes there, as a live card's depth
// point already lies on its own line of sight (067: a cabinet and a person of
// one hex row). It is worked out per fragment, not between the face's
// corners, so the faces of one solid tie exactly and the first one drawn
// keeps the pixel.
float planeDepth() {
    vec2 ndc = (gl_FragCoord.xy - uViewport.xy) / uViewport.zw * 2.0 - 1.0;
    vec4 rowX = vec4(uMvp[0][0], uMvp[1][0], uMvp[2][0], uMvp[3][0]);
    vec4 rowY = vec4(uMvp[0][1], uMvp[1][1], uMvp[2][1], uMvp[3][1]);
    vec4 rowZ = vec4(uMvp[0][2], uMvp[1][2], uMvp[2][2], uMvp[3][2]);
    vec4 rowW = vec4(uMvp[0][3], uMvp[1][3], uMvp[2][3], uMvp[3][3]);
    // The point p on the plane whose clip x and y over w are this pixel's:
    // a.p = 0, b.p = 0, normal.p = offset.
    vec4 a = rowX - ndc.x * rowW;
    vec4 b = rowY - ndc.y * rowW;
    mat3 m = transpose(mat3(a.xyz, b.xyz, vDepthPlane.xyz));
    float det = determinant(m);
    if (abs(det) < 1e-12) return gl_FragCoord.z;
    vec3 p = inverse(m) * vec3(-a.w, -b.w, vDepthPlane.w);
    return dot(rowZ, vec4(p, 1.0)) / dot(rowW, vec4(p, 1.0)) * 0.5 + 0.5;
}
// The depth pass closes the small holes in the art (090): a transparent
// texel with opaque art on both sides within kShadowArtGap texels, along a
// row, a column or a diagonal, casts. The pinholes and one or two texel
// slits of fence boards and foliage let the sun through as lit dots and
// threads inside their shadows; a gap wider than that (between two pickets,
// a lattice) stays open.
const int kShadowArtGap = 2;
bool shadowArtSolid() {
    vec2 texelSize = 1.0 / vec2(textureSize(uTexture, 0));
    vec2 steps[4] = vec2[4](vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(1.0, -1.0));
    for (int i = 0; i < 4; i++) {
        bool a = false, b = false;
        for (int k = 1; k <= kShadowArtGap; k++) {
            vec2 d = steps[i] * texelSize * float(k);
            a = a || texture(uTexture, vTexcoord + d).a >= uCutoff;
            b = b || texture(uTexture, vTexcoord - d).a >= uCutoff;
        }
        if (a && b) return true;
    }
    return false;
}
void main() {
    gFaceCross = cross(dFdx(vWorld), dFdy(vWorld));
    gl_FragDepth = vDepthOnPlane != 0 ? planeDepth() : gl_FragCoord.z;
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
        vec2 closeupPixel = vec2(0.0);
        float closeupFootprint = 1.0;
        if (uCloseupFilter != 0 && uPick == 0 && uTranslucent == 0) {
            closeupPixel = vTexcoord * vec2(textureSize(uTexture, 0)) - vec2(0.5);
            closeupFootprint = max(length(dFdx(closeupPixel)), length(dFdy(closeupPixel)));
        }
        // In the depth pass the attribute sits above 0.5 for the solid
        // texels: the floor copies entirely, and the bottom band of every
        // caster at its own place - the art's antialiased bottom edge would
        // leave those texels empty in the map and the ground by the wall lit
        // (082 round 5). The scene's own draws leave the attribute at 0 and
        // discard as always.
        if (vShadowSolid < 0.5 && texel.a < uCutoff && (uShadowPass == 0 || !shadowArtSolid())) discard;
        // Smooth magnified native art only in the fly view. Keep the nearest
        // texel's alpha: it carries material IDs, transparency and pick data.
        if (uCloseupFilter != 0 && uPick == 0 && uTranslucent == 0) {
            ivec2 dims = textureSize(uTexture, 0);
            vec2 pixel = closeupPixel;
            // Affect only strongly magnified art (over 3 screen pixels per
            // source texel); ordinary fly-camera views retain their texture.
            float amount = 0.65 * (1.0 - smoothstep(0.15, 0.30, closeupFootprint));
            if (amount > 0.0) {
                ivec2 base = ivec2(floor(pixel));
                vec2 f = fract(pixel);
                vec3 sum = vec3(0.0);
                float total = 0.0;
                for (int y = 0; y < 2; ++y) for (int x = 0; x < 2; ++x) {
                    ivec2 at = clamp(base + ivec2(x, y), ivec2(0), dims - ivec2(1));
                    vec4 sampleColor = texelFetch(uTexture, at, 0);
                    float weight = (x == 0 ? 1.0 - f.x : f.x) * (y == 0 ? 1.0 - f.y : f.y);
                    // Do not wash sharp art details across a high-contrast edge.
                    weight *= 1.0 - smoothstep(0.12, 0.45, length(sampleColor.rgb - texel.rgb));
                    if (sampleColor.a >= uCutoff) { sum += sampleColor.rgb * weight; total += weight; }
                }
                if (total > 0.0) texel.rgb = mix(texel.rgb, sum / total, amount);
            }
        }
        if (uPick != 0) {
            // The depth pass writes the caster's object number into the map's
            // id channel (082 round 6, mechanism 2); the mouse pick keeps its
            // flat id colour.
            outColor = uShadowPass != 0 ? vec4(vCasterId / 1024.0, 0.0, 0.0, 1.0) : uColor;
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
            float mark = texel.a * 255.0;
            bool calibratedSign = vFullLight > 0.2 && vFullLight < 0.3;
            bool signTube = (calibratedSign || (vFullLight > 0.4 && vFullLight < 0.6))
                && texel.r >= 0.5 && texel.r > texel.b * 1.5 && texel.r >= texel.g;
            bool muzzleCore = mark > 245.5 && mark < 247.5;
            bool calibratedCore = (mark > 239.5 && mark < 247.5) || (calibratedSign && signTube);
            bool effectCore = calibratedCore || (mark > 247.5 && mark < 250.5) || signTube;
            bool fullBright = abs(mark - 254.0) < 0.5 || effectCore;
            outColor.a = 1.0;
            // _dark_trans_buf_to_buf: the engine darkens a texel by its light in
            // 128 steps, on its 5-bit colour rounded down (setIntensityTableColor).
            vec4 lit = fullBright ? vec4(1.0) : sceneLighting();
            if (!effectCore) lit.a = shadowDarken(lit.a);
            if (uShadowDebug == 2) {
                // The face test itself: white where the sun hits the face and
                // the map is read, black where the face takes the flat
                // shaded value (082 diagnostics). Same epsilon as the gate.
                outEmissive = vec4(0.0);
                // Red: no usable normal here (090).
                if (!(dot(gFaceCross, gFaceCross) > 1e-20)) {
                    outColor = vec4(1.0, 0.0, 0.0, 1.0);
                    return;
                }
                outColor = vec4(vec3(dot(shadowPaintedNormal(shadowFaceNormal()), uShadowLight) > 0.001 ? 1.0 : 0.0), 1.0);
                return;
            }
            if (uShadowDebug == 6 && (uLightMode == 1 || uLightMode == 2)) {
                // Who casts the shadow here (090): the center tap's caster id
                // as a colour, black where the tap reads lit, gray for a
                // blocker that carries no id.
                outEmissive = vec4(0.0);
                vec3 n = shadowFaceNormal();
                if (dot(shadowPaintedNormal(n), uShadowLight) <= 0.001) {
                    outColor = vec4(0.0, 0.0, 1.0, 1.0);
                    return;
                }
                float diff = shadowTapDiff(vWorld, shadowPlaneSlope(n), shadowSelfId());
                vec3 p = shadowTex(vWorld);
                float id = texelFetch(uShadowIds, ivec2(p.xy * vec2(textureSize(uShadowMap, 0))), 0).r * 1024.0;
                vec3 c = id < 0.5 ? vec3(0.5) : fract(sin(vec3(id) * vec3(12.9898, 78.233, 37.719)) * 43758.5453);
                outColor = vec4(diff > uShadowBias ? c : vec3(0.0), 1.0);
                return;
            }
            if (uShadowDebug == 3) {
                // The raw coverage: the map against the face-plane taps, no
                // face gate (062 diagnostics).
                outEmissive = vec4(0.0);
                vec3 n = shadowFaceNormal();
                vec3 t1, t2;
                shadowFaceBasis(n, t1, t2);
                outColor = vec4(vec3(shadowCoverage(vWorld, t1, t2, shadowPlaneSlope(n), shadowSelfId())), 1.0);
                return;
            }
            if (uShadowDebug == 1) {
                // The pure factor, flat gray: no art, no engine light steps
                // (062 diagnostics). What takes no sun shadow (roofs, the
                // unlit draws) reads magenta: the seam metric leaves it out
                // (090, tools/shadow_seams.py). A face turned from the sun
                // (the flat shaded value, no cast shadow) reads blue: lit
                // ground seen between two shaded fence rails is light, not a
                // hole in a shadow. A standing body (a person, a tree: a
                // card with the whole verdict at its feet, not a door in its
                // wall) reads green: the metric leaves its own pixels out,
                // its outline against a lit wall is not a seam, and still
                // counts the ground round it (090). Every other standing
                // card reads per pixel since 109 and stays in the factor
                // read.
                outEmissive = vec4(0.0);
                if (uLightMode == 3 && uShadowGround.x < 0.5
                    && dot(uShadowGround.zw, uShadowGround.zw) < 0.5 && uShadowFeet > 0.5) {
                    outColor = vec4(0.0, 1.0, 0.0, 1.0);
                    return;
                }
                bool receiver = uLightMode == 1 || uLightMode == 2 || uLightMode == 3;
                bool turned = (uLightMode == 1 || uLightMode == 2)
                    ? dot(shadowPaintedNormal(shadowFaceNormal()), uShadowLight) <= 0.001
                    : uLightMode == 3 && cardTurned();
                outColor = !receiver ? vec4(1.0, 0.0, 1.0, 1.0) : turned ? vec4(0.0, 0.0, 1.0, 1.0) : vec4(vec3(lit.a), 1.0);
                return;
            }
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
            // under its own light); the glow blurs this buffer. The alpha
            // carries which texels are the marked ones: the wide halo spreads
            // only those, so daylight surfaces never feed it.
            bool marked = (mark > 251.5 && mark < 254.5) || effectCore;
            vec3 emission = marked ? texel.rgb * outColor.a : vec3(0.0);
            vec3 bright = outColor.rgb * smoothstep(0.88, 0.98, max(max(outColor.r, outColor.g), outColor.b));
            vec3 glow = max(emission, bright);
            // White/cyan monitor pixels look brighter than equally strong red
            // embers. Compensate their luminance, bounded to half strength;
            // warm fire and green slime keep their original contribution.
            float peak = max(max(glow.r, glow.g), glow.b);
            float luminance = dot(glow, vec3(0.2126, 0.7152, 0.0722));
            float coldShare = smoothstep(0.25, 0.65, glow.b / max(peak, 0.0001));
            float compensation = clamp(0.2126 * peak / max(luminance, 0.0001), 0.5, 1.0);
            glow *= mix(1.0, compensation, coldShare);
            // Small noncycling effects need more halo energy than PLASMAB's
            // fully cycling core. This remains behind the single global slider.
            float effectGain = signTube ? 4.0 : (mark < 248.5 ? 8.0 : (mark < 249.5 ? 4.0 : 2.0));
            // The reviewed emission target is RGBA8: it clamps BEFORE blur.
            // Preserve that clamp before scaling to common strength 2; simply
            // multiplying the old gain by the ratio would change its colors.
            if (calibratedCore) {
                float referenceGain = muzzleCore ? 1.0 : (signTube ? 4.0 : (mark < 240.5 ? 8.0 : (mark < 241.5 ? 1.0
                    : (mark < 242.5 ? 8.0 : 4.0))));
                float chosenRatio = muzzleCore ? (mark > 246.5 ? 0.45 : 0.25) : (signTube ? 0.2 : (mark < 240.5 ? 0.35 : (mark < 241.5 ? 0.75
                    : (mark < 243.5 ? 0.5 : (mark < 244.5 ? 0.35 : 0.2)))));
                glow = clamp(glow * referenceGain, 0.0, 1.0) * chosenRatio;
            } else if (effectCore) {
                glow *= effectGain;
            }
            outEmissive = vec4(glow, marked ? 1.0 : 0.0);
            // Roofs blend over the scene last. Their opaque pixels must also
            // erase the emission below; alpha zero would leave a hidden sign's
            // halo visible through the roof in the second render target.
            if (uLightMode == 4) outEmissive = vec4(0.0, 0.0, 0.0, outColor.a);
        }
    }
}
)";

} // namespace

unsigned int gSceneProgram = 0;
int gSceneMvpLocation = -1;
int gSceneTextureLocation = -1;
int gSceneTexturedLocation = -1;
int gSceneCloseupFilterLocation = -1;
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
int gSceneFlyEyeLocation = -1;
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
int gSceneShadowIdsLocation = -1;
int gSceneShadowStrengthLocation = -1;
int gSceneShadowDensityLocation = -1;
int gSceneShadowLightLocation = -1;
int gSceneShadowStepLocation = -1;
int gSceneShadowDebugLocation = -1;
int gSceneShadowBiasLocation = -1;
int gSceneShadowContactLocation = -1;
int gSceneShadowPassLocation = -1;
int gSceneShadowSelfFromVertexLocation = -1;
int gSceneShadowSelfIdLocation = -1;
int gSceneShadowAnchorLocation = -1;
int gSceneShadowGroundLocation = -1;
int gSceneShadowFeetLocation = -1;
int gSceneCloudOnLocation = -1;
int gSceneCloudOffsetLocation = -1;

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
    gSceneCloseupFilterLocation = glGetUniformLocation(gSceneProgram, "uCloseupFilter");
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
    gSceneFlyEyeLocation = glGetUniformLocation(gSceneProgram, "uFlyEye");
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
    gSceneShadowIdsLocation = glGetUniformLocation(gSceneProgram, "uShadowIds");
    gSceneShadowStrengthLocation = glGetUniformLocation(gSceneProgram, "uShadowStrength");
    gSceneShadowDensityLocation = glGetUniformLocation(gSceneProgram, "uShadowDensity");
    gSceneShadowLightLocation = glGetUniformLocation(gSceneProgram, "uShadowLight");
    gSceneShadowStepLocation = glGetUniformLocation(gSceneProgram, "uShadowStep");
    gSceneShadowDebugLocation = glGetUniformLocation(gSceneProgram, "uShadowDebug");
    gSceneShadowBiasLocation = glGetUniformLocation(gSceneProgram, "uShadowBias");
    gSceneShadowContactLocation = glGetUniformLocation(gSceneProgram, "uShadowContact");
    gSceneShadowPassLocation = glGetUniformLocation(gSceneProgram, "uShadowPass");
    gSceneShadowSelfFromVertexLocation = glGetUniformLocation(gSceneProgram, "uShadowSelfFromVertex");
    gSceneShadowSelfIdLocation = glGetUniformLocation(gSceneProgram, "uShadowSelfId");
    gSceneShadowAnchorLocation = glGetUniformLocation(gSceneProgram, "uShadowAnchor");
    gSceneShadowGroundLocation = glGetUniformLocation(gSceneProgram, "uShadowGround");
    gSceneShadowFeetLocation = glGetUniformLocation(gSceneProgram, "uShadowFeet");
    gSceneCloudOnLocation = glGetUniformLocation(gSceneProgram, "uCloudOn");
    gSceneCloudOffsetLocation = glGetUniformLocation(gSceneProgram, "uCloudOffset");
    if (gSceneMvpLocation < 0 || gSceneTextureLocation < 0
        || gSceneTexturedLocation < 0 || gSceneCloseupFilterLocation < 0 || gSceneColorLocation < 0 || gSceneCutoffLocation < 0
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
        || gSceneCloudOnLocation < 0 || gSceneCloudOffsetLocation < 0
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
    gSceneCloseupFilterLocation = -1;
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
