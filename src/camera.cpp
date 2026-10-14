#include "camera.h"

#include <cmath>

using namespace DirectX;

namespace f2native {

void Camera::configure(CameraKind kind, float aspectRatio) {
    kind_ = kind;
    aspect_ = aspectRatio;
    // Floor tiles are 1 world unit (spec: pan moves 4 floor units).
    proj_ = XMMatrixPerspectiveFovLH(XM_PIDIV4 /*50 degrees ish*/, aspectRatio, 0.1f, 100.0f);
}

void Camera::lookAt(XMFLOAT3 eye, XMFLOAT3 target) {
    eye_ = eye;
    const XMVECTOR e = XMLoadFloat3(&eye);
    const XMVECTOR t = XMLoadFloat3(&target);
    view_ = XMMatrixLookAtLH(e, t, XMVectorSet(0, 1, 0, 0));
    viewProj_ = view_ * proj_;
}

bool Camera::poseAt(double tSeconds, Route route) {
    bool reset = firstPose_;
    XMFLOAT3 prevEye = eye_;

    double phase = 0.0;
    if (route == Route::Pan) {
        // 4 units right over 10 s, back over the next 10, repeat.
        constexpr double kPeriod = 20.0;
        const double u = tSeconds / kPeriod;
        phase = u - std::floor(u); // 0..1
        // triangle wave 0..1..0
        phase = phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
    }
    const double offset = 4.0 * phase;

    if (kind_ == CameraKind::Iso) {
        // Upstream-style high oblique view; still a real perspective camera.
        lookAt(XMFLOAT3(static_cast<float>(offset) + 0.0f, 6.5f, 6.0f), XMFLOAT3(0.0f, 0.4f, 0.0f));
    } else {
        lookAt(XMFLOAT3(static_cast<float>(offset), 2.2f, 4.5f), XMFLOAT3(0.0f, 0.6f, 0.0f));
    }

    if (!firstPose_) {
        // A camera cut is a discontinuity: eye jumped more than a pan step could.
        const float dx = eye_.x - prevEye.x;
        const float dy = eye_.y - prevEye.y;
        const float dz = eye_.z - prevEye.z;
        if (dx * dx + dy * dy + dz * dz > 1.5f * 1.5f) reset = true;
    }
    if (!reset) prevViewProj_ = viewProj_; // previous frame's matrices
    else prevViewProj_ = viewProj_;        // reset frame: history re-seeded, matrices equal
    firstPose_ = false;
    return reset;
}

} // namespace f2native
