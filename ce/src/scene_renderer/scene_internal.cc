#include "scene_internal.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdio.h>

#include "gl_api.h"

namespace fallout {
namespace scene_renderer {
namespace internal {

DepthVertex cardVertex(const SpriteCardVertex& corner, double row, const float eye[3])
{
    const double ray[3] = { corner.x - eye[0], corner.y - eye[1], corner.z - eye[2] };
    const double t = std::abs(ray[2]) > 1e-9 ? (row - corner.z) / ray[2] : 0.0;
    return { static_cast<float>(corner.x), static_cast<float>(corner.y), static_cast<float>(corner.z),
        static_cast<float>(corner.u), static_cast<float>(corner.v),
        static_cast<float>(corner.x + ray[0] * t), static_cast<float>(corner.y + ray[1] * t),
        static_cast<float>(row) };
}

void depthVertexLayout()
{
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406 /* GL_FLOAT */, 0, sizeof(DepthVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 0x1406, 0, sizeof(DepthVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, 0x1406, 0, sizeof(DepthVertex), reinterpret_cast<const void*>(5 * sizeof(float)));
}

bool readBinary(const std::filesystem::path& path, std::vector<unsigned char>& bytes)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) return false;
    const std::streamoff size = stream.tellg();
    if (size < 0 || size > 64 * 1024 * 1024) return false;
    bytes.resize(static_cast<size_t>(size));
    stream.seekg(0, std::ios::beg);
    return bytes.empty() || static_cast<bool>(stream.read(reinterpret_cast<char*>(bytes.data()), size));
}

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int compiled = 0;
    glGetShaderiv(shader, 0x8B81 /* GL_COMPILE_STATUS */, &compiled);
    if (!compiled) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "scene_renderer: shader compile failed: %s\n", log);
        fflush(stderr);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool fitRect(int imageWidth, int imageHeight, int windowWidth, int windowHeight, int* outX, int* outY, int* outW, int* outH)
{
    if (imageWidth <= 0 || imageHeight <= 0 || windowWidth <= 0 || windowHeight <= 0) {
        return false;
    }

    // SDL_RenderSetLogicalSize equivalent: scale to fit, center, letterbox.
    int w = windowWidth;
    int h = windowWidth * imageHeight / imageWidth;
    if (h > windowHeight) {
        h = windowHeight;
        w = windowHeight * imageWidth / imageHeight;
    }
    *outW = std::max(w, 1);
    *outH = std::max(h, 1);
    *outX = (windowWidth - *outW) / 2;
    *outY = (windowHeight - *outH) / 2;
    return true;
}

} // namespace internal
} // namespace scene_renderer
} // namespace fallout
