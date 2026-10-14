#pragma once

// Synthetic-scene renderer for G1/G2: linear color + device depth + true
// motion vectors from previous/current camera matrices, plus the final
// linear->sRGB present pass. Modes per spec section 4:
//   native  - present pass shows the color target directly, no NGX
//   carrier - NGX DLAA runs, present shows the NGX output
//   neural  - same as carrier; the RenoDX add-on intercepts the evaluate
//             and runs the neural feature 18 pass

#include "camera.h"
#include "gpu.h"
#include "neural.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>

namespace f2native {

enum class RenderMode { Native, Carrier, Neural };

struct SelftestResult {
    bool allPassed = true;
    std::string details; // human-readable PASS/FAIL lines
};

class Render {
public:
    void initialize(Gpu& gpu, RenderMode mode, CameraKind cameraKind, uint32_t width,
                    uint32_t height);
    void resize(uint32_t width, uint32_t height);
    // Records and submits one frame at timeline position tSeconds.
    // Returns true when NGX history must reset at this frame.
    bool renderFrame(double tSeconds, Route route, bool& resetHistory);
    void waitGpu() { gpu_->waitGpu(); }
    void shutdown();

    // Numerical contract checks (spec section 7); runs its own frames.
    SelftestResult runSelftest();

    NeuralPass& neural() { return neural_; }
    uint64_t framesRendered() const { return framesRendered_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

private:
    void createSizedResources(uint32_t width, uint32_t height);
    void createPipelines();
    void createSceneGeometry();
    void createProceduralTextures();
    void throwIfFailed(HRESULT hr, const char* what) const;
    // Copies one pixel of the resource into a readback buffer and returns it
    // as 4 float32 channels (half-float formats decoded; R32_FLOAT in [0]).
    std::vector<float> readbackPixel(ID3D12Resource* res, D3D12_RESOURCE_STATES& state,
                                     D3D12_RESOURCE_STATES copyableState, UINT x, UINT y,
                                     DXGI_FORMAT viewFormat);
    struct Gpu* gpu_ = nullptr;
    RenderMode mode_ = RenderMode::Native;
    CameraKind cameraKind_ = CameraKind::Free;
    Camera camera_;
    NeuralPass neural_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint64_t framesRendered_ = 0;
    D3D12_RESOURCE_STATES stateColor_ = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES stateMotion_ = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES stateDepth_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    D3D12_RESOURCE_STATES bbStates_[2] = {D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON};

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_; // color + motion
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_; // shader-visible: tex + color/motion/depth/output
    Microsoft::WRL::ComPtr<ID3D12Resource> color_;
    Microsoft::WRL::ComPtr<ID3D12Resource> motion_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depth_;
    Microsoft::WRL::ComPtr<ID3D12Resource> output_;
    Microsoft::WRL::ComPtr<ID3D12Resource> checkerTex_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cardTex_;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadScratch_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingUploads_;
    Microsoft::WRL::ComPtr<ID3D12Resource> frameCB_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> gbufferRoot_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> presentRoot_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> gbufferPso_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> presentPso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> sceneVB_;
    D3D12_VERTEX_BUFFER_VIEW sceneView_ = {};
    D3D12_GPU_DESCRIPTOR_HANDLE checkerSrvGpu_ = {};
    D3D12_GPU_DESCRIPTOR_HANDLE cardSrvGpu_ = {};
    D3D12_GPU_DESCRIPTOR_HANDLE colorSrvGpu_ = {};
    D3D12_GPU_DESCRIPTOR_HANDLE outputSrvGpu_ = {};
    uint8_t* frameCBMapped_ = nullptr;
    UINT srvDescriptorSize_ = 0;
};

} // namespace f2native
