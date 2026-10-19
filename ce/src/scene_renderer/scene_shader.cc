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
uniform mat4 uMvp;
uniform int uDepthAt;
out vec2 vTexcoord;
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vTexcoord = aTexcoord;
    if (uDepthAt != 0) {
        vec4 q = uMvp * vec4(aDepthAt, 1.0);
        gl_Position.z = q.z / q.w * gl_Position.w;
    }
}
)";

constexpr const char* kSceneFragmentShader = R"(#version 330 core
in vec2 vTexcoord;
uniform sampler2D uTexture;
uniform int uTextured;
uniform vec4 uColor;
uniform float uCutoff;
uniform int uTranslucent; // the engine's translucent blend: steam, glass, energy, ...
uniform vec3 uTint;
uniform int uGlassGray;
uniform int uPick; // the pick pass: every texel shown is the object's id colour
out vec4 outColor;
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
            if (gray < 1.0) discard;
            outColor = vec4(uTint, min(gray, 7.0) / 7.0);
        } else {
            outColor = texel;
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
    if (gSceneMvpLocation < 0 || gSceneTextureLocation < 0
        || gSceneTexturedLocation < 0 || gSceneColorLocation < 0 || gSceneCutoffLocation < 0
        || gSceneDepthAtLocation < 0 || gSceneTranslucentLocation < 0 || gSceneTintLocation < 0
        || gSceneGlassGrayLocation < 0 || gScenePickLocation < 0) {
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
