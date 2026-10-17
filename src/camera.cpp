#include "camera.h"

#include <cmath>

using namespace DirectX;

namespace f2native {

void Camera::configure(CameraKind kind, float aspectRatio) {
    kind_ = kind;
    aspect_ = aspectRatio;
    if (kind_ == CameraKind::Iso) {
        // Orthographic: the game's own camera is a parallel projection
        // (geom.js: px(u,v,z) = origin + u*squ + v*sqv - (0, hpx*z)).
        // The view height covers ~24 floor units of the map.
        proj_ = XMMatrixOrthographicLH(24.0f * aspectRatio, 24.0f, 0.1f, 200.0f);
    } else {
        proj_ = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 100.0f);
    }
}

void Camera::lookAt(XMFLOAT3 eye, XMFLOAT3 target) {
    eye_ = eye;
    const XMVECTOR e = XMLoadFloat3(&eye);
    const XMVECTOR t = XMLoadFloat3(&target);
    view_ = XMMatrixLookAtLH(e, t, XMVectorSet(0, 1, 0, 0));
    viewProj_ = view_ * proj_;
}

bool Camera::poseAt(double tSeconds, Route route) {
    // THE FIX (verifier point 1): the previous matrices must be captured BEFORE
    // lookAt overwrites viewProj_. Previously prevViewProj_ was assigned the
    // current matrix, so the neural pass received near-zero motion vectors.
    const XMMATRIX previousViewProj = viewProj_;
    const XMFLOAT3 previousEye = eye_;

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
        // The game's camera ray (Proj::towardCamera): straight along the
        // projection's view axis, high above the map centre.
        lookAt(XMFLOAT3(static_cast<float>(offset) + 0.0f, 9.0f, 6.0f),
               XMFLOAT3(0.0f, 0.4f, 0.0f));
    } else {
        lookAt(XMFLOAT3(static_cast<float>(offset), 2.2f, 4.5f),
               XMFLOAT3(0.0f, 0.6f, 0.0f));
    }

    bool reset = firstPose_;
    if (!firstPose_) {
        // A camera cut is a discontinuity: the eye jumped farther than a pan
        // step could ever move it.
        const float dx = eye_.x - previousEye.x;
        const float dy = eye_.y - previousEye.y;
        const float dz = eye_.z - previousEye.z;
        if (dx * dx + dy * dy + dz * dz > 1.5f * 1.5f) reset = true;
    }
    prevViewProj_ = reset ? viewProj_ : previousViewProj;
    firstPose_ = false;
    return reset;
}

} // namespace f2native
