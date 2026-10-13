// CPU reference for the motion-vector contract (spec section 7).
//
//   currentUV = (0.6, 0.5), previousUV = (0.5, 0.5) at 1280x720
//   motion = (previousUV - currentUV) * resolution = (-128, 0)
//
// The GPU HLSL in shaders/scene.hlsl must reproduce exactly this formula; this
// test pins the expected numbers so a shader-side deviation is detectable via
// the readback tests added in G1.

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace {

struct Vec2 {
    float x;
    float y;
};

// Spec section 5: motion is previous minus current, in pixels of render resolution.
Vec2 motionPixels(Vec2 currentUV, Vec2 previousUV, uint32_t width, uint32_t height) {
    return {(previousUV.x - currentUV.x) * static_cast<float>(width),
            (previousUV.y - currentUV.y) * static_cast<float>(height)};
}

int failures = 0;

void expectNear(const char* what, float actual, float expected, float tolerance) {
    if (std::fabs(actual - expected) > tolerance) {
        std::printf("FAIL %s: expected %.6f, got %.6f (tolerance %.6f)\n", what, expected,
                    actual, tolerance);
        ++failures;
    } else {
        std::printf("OK   %s = %.6f\n", what, actual);
    }
}

} // namespace

int main() {
    const Vec2 motion = motionPixels({0.6f, 0.5f}, {0.5f, 0.5f}, 1280, 720);
    expectNear("motion.x (spec 7 sample)", motion.x, -128.0f, 1e-4f);
    expectNear("motion.y (spec 7 sample)", motion.y, 0.0f, 1e-4f);

    const Vec2 zero = motionPixels({0.25f, 0.75f}, {0.25f, 0.75f}, 1280, 720);
    expectNear("static camera motion.x", zero.x, 0.0f, 1e-6f);
    expectNear("static camera motion.y", zero.y, 0.0f, 1e-6f);

    if (failures == 0) {
        std::printf("contracts OK\n");
        return 0;
    }
    std::printf("contracts FAILED: %d case(s)\n", failures);
    return 4; // failed gate per plan section 1
}
