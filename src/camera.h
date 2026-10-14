#pragma once

#include <DirectXMath.h>

namespace f2native {

enum class Route { Static, Pan };
enum class CameraKind { Free, Iso };

// Perspective camera with previous-frame state for per-pixel motion vectors.
// The pan route translates the eye along screen-right by 4 floor units over
// 10 seconds and back over the next 10 (spec G2 test route); a camera cut is
// an instantaneous jump that must set the NGX reset flag.
class Camera {
public:
    void configure(CameraKind kind, float aspectRatio);

    // Moves to the pose for timeline position tSeconds. Returns true when the
    // frame at this pose must reset NGX history (first frame, camera cut,
    // projection change).
    bool poseAt(double tSeconds, Route route);

    DirectX::XMMATRIX view() const { return view_; }
    DirectX::XMMATRIX proj() const { return proj_; }
    DirectX::XMMATRIX viewProj() const { return viewProj_; }
    DirectX::XMMATRIX prevViewProj() const { return prevViewProj_; }
    DirectX::XMFLOAT3 eye() const { return eye_; }

private:
    void lookAt(DirectX::XMFLOAT3 eye, DirectX::XMFLOAT3 target);

    CameraKind kind_ = CameraKind::Free;
    float aspect_ = 16.0f / 9.0f;
    DirectX::XMMATRIX view_ = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX proj_ = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX viewProj_ = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX prevViewProj_ = DirectX::XMMatrixIdentity();
    DirectX::XMFLOAT3 eye_ = {};
    bool firstPose_ = true;
};

} // namespace f2native
