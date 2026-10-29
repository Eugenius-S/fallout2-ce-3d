#include "sky.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "camera.h"
#include "gl_api.h"
#include "scene_internal.h"
#include "../scene/scene_lights.h"
#include "../scene/scene_view.h"
#include "../scripts.h"

namespace fallout {
namespace scene_renderer {
namespace {

constexpr unsigned int kVertexShader = 0x8B31;
constexpr unsigned int kFragmentShader = 0x8B30;
constexpr unsigned int kCompileStatus = 0x8B81;
constexpr unsigned int kLinkStatus = 0x8B82;
unsigned int gProgram = 0;
unsigned int gVao = 0;
bool gFailed = false;

// Adapted from Apolotary's SKY_FS in fallout2-first-person/viewer/render.js.
// The donor's Sustainable Use License applies; this shader is modified for
// Fallout CE 3D's camera, sun direction, and continuously blended game time.
constexpr const char* kVertex = R"(#version 330 core
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";
constexpr const char* kFragment = R"(#version 330 core
out vec4 outColor;
uniform vec3 uForward;
uniform vec3 uRight;
uniform vec3 uUp;
uniform vec3 uSun;
uniform vec3 uZenith;
uniform vec3 uHorizon;
uniform vec3 uMesa;
uniform float uStars;
uniform float uGlow;
uniform int uUnderground;
uniform vec2 uResolution;
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
void main() {
    if (uUnderground != 0) { outColor = vec4(0.012, 0.012, 0.016, 1.0); return; }
    vec2 ndc = gl_FragCoord.xy / uResolution * 2.0 - 1.0;
    vec3 ray = normalize(uForward + ndc.x * uRight + ndc.y * uUp);
    float elevation = ray.y;
    float h = smoothstep(-0.06, 0.52, elevation);
    vec3 color = mix(uHorizon, uZenith, h);
    float haze = exp(-abs(elevation) * 15.0);
    color = mix(color, uHorizon, haze * 0.32);
    float sun = pow(max(dot(ray, normalize(uSun)), 0.0), 90.0) * uGlow;
    color += vec3(1.0, 0.66, 0.36) * sun;
    float stars = step(0.997, hash(floor(ray.xz * 850.0))) * smoothstep(0.02, 0.35, elevation) * uStars;
    color += vec3(stars);
    float ridge = sin(atan(ray.z, ray.x) * 11.0 + sin(atan(ray.z, ray.x) * 3.0) * 2.0) * 0.035 - 0.11;
    float mesa = (1.0 - smoothstep(ridge, ridge + 0.012, elevation)) * step(-0.2, elevation);
    color = mix(color, uMesa, mesa * (1.0 - h));
    outColor = vec4(color, 1.0);
}
)";

unsigned int compile(unsigned int type, const char* source)
{
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int ok = 0;
    glGetShaderiv(shader, kCompileStatus, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "scene sky: shader compile failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool init()
{
    if (gProgram != 0) return true;
    if (gFailed) return false;
    const unsigned int vs = compile(kVertexShader, kVertex);
    const unsigned int fs = compile(kFragmentShader, kFragment);
    if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); gFailed = true; return false; }
    gProgram = glCreateProgram();
    glAttachShader(gProgram, vs);
    glAttachShader(gProgram, fs);
    glLinkProgram(gProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int ok = 0;
    glGetProgramiv(gProgram, kLinkStatus, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetProgramInfoLog(gProgram, sizeof(log), nullptr, log);
        fprintf(stderr, "scene sky: shader link failed: %s\n", log);
        glDeleteProgram(gProgram);
        gProgram = 0;
        gFailed = true;
        return false;
    }
    glGenVertexArrays(1, &gVao);
    return true;
}

} // namespace

void drawSceneSky(float aspect, int width, int height, bool flying)
{
    // Keep the original fixed game camera byte-for-byte; the sky belongs only
    // to the free-flight or deliberately turned 3D camera.
    if (!flying && !scene::cameraTurned()) return;
    if (width <= 0 || height <= 0 || !init()) return;
    StaticFlyCamera camera{};
    if (flying) {
        if (!staticFlyCamera(&camera)) return;
    } else if (!staticGameViewCamera(&camera)) return;
    float forward[3], right[3];
    staticFlyCameraAxes(camera, forward, right);
    const float fov = std::clamp(camera.fovY > 0.0f ? camera.fovY : 55.0f, 1.0f, 150.0f) * 0.00872664626f;
    const float scale = std::tan(fov);
    for (float& value : right) value *= scale * aspect;
    float up[3] = {
        forward[1] * right[2] - forward[2] * right[1],
        forward[2] * right[0] - forward[0] * right[2],
        forward[0] * right[1] - forward[1] * right[0],
    };
    for (float& value : up) value *= aspect > 0.0f ? scale / (scale * aspect) : 1.0f;
    const unsigned int time = gameTimeGetTime() % (24u * 36000u);
    const float hour = time / 36000.0f;
    const auto smooth = [](float a, float b, float x) { float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f); return t * t * (3.0f - 2.0f * t); };
    const float day = smooth(6.0f, 10.0f, hour) * (1.0f - smooth(14.0f, 18.5f, hour));
    const float twilight = std::max(1.0f - smooth(4.2f, 8.0f, hour), smooth(16.0f, 20.0f, hour));
    const float night = std::clamp(1.0f - day - twilight * 0.55f, 0.0f, 1.0f);
    const float zenith[3] = { 0.035f + 0.235f * day + 0.12f * twilight, 0.06f + 0.37f * day + 0.11f * twilight, 0.15f + 0.51f * day + 0.21f * twilight };
    const float horizon[3] = { 0.13f + 0.67f * day + 0.75f * twilight, 0.19f + 0.52f * day + 0.31f * twilight, 0.33f + 0.23f * day + 0.03f * twilight };
    const float mesa[3] = { 0.03f + 0.39f * day + 0.17f * twilight, 0.04f + 0.23f * day + 0.07f * twilight, 0.09f + 0.11f * day + 0.04f * twilight };
    const scene::Sun& sun = scene::currentSun();
    glDisable(internal::kDepthTest);
    glDepthMask(0);
    glUseProgram(gProgram);
    glUniform3f(glGetUniformLocation(gProgram, "uForward"), forward[0], forward[1], forward[2]);
    glUniform3f(glGetUniformLocation(gProgram, "uRight"), right[0], right[1], right[2]);
    glUniform3f(glGetUniformLocation(gProgram, "uUp"), up[0], up[1], up[2]);
    glUniform3f(glGetUniformLocation(gProgram, "uSun"), sun.direction[0], sun.direction[1], sun.direction[2]);
    glUniform3f(glGetUniformLocation(gProgram, "uZenith"), zenith[0], zenith[1], zenith[2]);
    glUniform3f(glGetUniformLocation(gProgram, "uHorizon"), horizon[0], horizon[1], horizon[2]);
    glUniform3f(glGetUniformLocation(gProgram, "uMesa"), mesa[0], mesa[1], mesa[2]);
    glUniform1f(glGetUniformLocation(gProgram, "uStars"), night);
    glUniform1f(glGetUniformLocation(gProgram, "uGlow"), 0.35f + twilight * 0.35f + sun.strength * 0.3f);
    glUniform1i(glGetUniformLocation(gProgram, "uUnderground"), scene::sceneDaylight() ? 0 : 1);
    glUniform2f(glGetUniformLocation(gProgram, "uResolution"), static_cast<float>(width), static_cast<float>(height));
    glBindVertexArray(gVao);
    glDrawArrays(internal::kTriangles, 0, 3);
    glDepthMask(1);
    glEnable(internal::kDepthTest);
}

void destroySceneSky()
{
    if (gProgram && internal::gContext) glDeleteProgram(gProgram);
    if (gVao && internal::gContext) glDeleteVertexArrays(1, &gVao);
    gProgram = gVao = 0;
    gFailed = false;
}

} // namespace scene_renderer
} // namespace fallout
