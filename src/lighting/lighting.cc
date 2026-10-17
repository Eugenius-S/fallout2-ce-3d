#include "lighting.h"

#include <cmath>

#include "../settings.h"

namespace fallout {
namespace lighting {

namespace {

constexpr float kPi = 3.14159265358979f;

int gWindowWidth = 640;
int gWindowHeight = 480;

inline float pitchRadians()
{
    return settings.lighting.pitch * kPi / 180.0f;
}

} // namespace

void init(int windowWidth, int windowHeight)
{
    gWindowWidth = windowWidth;
    gWindowHeight = windowHeight;
}

bool isPerspective()
{
    return settings.lighting.camera != 0;
}

void projectElevated(float x, float y, float h, float* outX, float* outY, float* outDepth)
{
    const float cx = gWindowWidth / 2.0f;
    const float cy = gWindowHeight / 2.0f;
    const float d = static_cast<float>(settings.lighting.distance);
    const float sp = std::sin(pitchRadians());
    const float cp = std::cos(pitchRadians());

    float zc = d - (y - cy) * cp - h * sp;
    if (zc < 1.0f) {
        zc = 1.0f;
    }

    *outX = cx + d * (x - cx) / zc;
    *outY = cy + d * ((y - cy) * sp - h * cp) / zc;

    if (outDepth != nullptr) {
        *outDepth = zc;
    }
}

void projectGround(float x, float y, float* outX, float* outY)
{
    projectElevated(x, y, 0.0f, outX, outY);
}

void inverseGround(float x, float y, float* outX, float* outY)
{
    const float cx = gWindowWidth / 2.0f;
    const float cy = gWindowHeight / 2.0f;
    const float d = static_cast<float>(settings.lighting.distance);
    const float sp = std::sin(pitchRadians());
    const float cp = std::cos(pitchRadians());

    const float b = y - cy;
    float zc = d * d * sp / (d * sp + b * cp);
    if (zc < 1.0f) {
        zc = 1.0f;
    }

    *outX = cx + zc * (x - cx) / d;
    *outY = cy + zc * b / (d * sp);
}

int farMargin()
{
    int top = 0;
    int sides = 0;
    renderMargins(&top, &sides);
    return top;
}

void renderMargins(int* top, int* sides)
{
    *top = 0;
    *sides = 0;
    if (!isPerspective()) {
        return;
    }

    const float cx = gWindowWidth / 2.0f;
    const float cy = gWindowHeight / 2.0f;
    const float d = static_cast<float>(settings.lighting.distance);
    const float sp = std::sin(pitchRadians());
    const float cp = std::cos(pitchRadians());

    // Vanilla y that projects to the window top sits above it by
    // extTop = (cy*d)/(d*sp - cy*cp) - cy. If the horizon falls inside the
    // window the denominator flips; the void above it cannot be covered by
    // more tiles.
    const float denom = d * sp - cy * cp;
    if (denom <= 0.0f) {
        return;
    }

    const float extTop = (cy * d) / denom - cy;
    // Far rows are minified, so they pull content from beyond the window's
    // left/right edges as well.
    const float depthTop = d + (cy + extTop) * cp;
    const float extSide = cx * (depthTop / d - 1.0f);

    *top = static_cast<int>(std::ceil(extTop)) + 16;
    *sides = static_cast<int>(std::ceil(extSide)) + 16;
}

} // namespace lighting
} // namespace fallout
