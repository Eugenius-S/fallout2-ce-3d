#pragma once

#include <windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

namespace f2native {

// Minimal D3D12 renderer: device + direct queue + flip-discard swapchain +
// one colored triangle. G0 skeleton; the scene/neural layers build on the
// same device and queue ownership model.
class Gpu {
public:
    void initialize(HWND hwnd, uint32_t width, uint32_t height);
    void resize(uint32_t width, uint32_t height);
    void renderFrame(); // clear + triangle + present, one fence wait
    void waitGpu();
    void shutdown();

private:
    void createSwapchainBuffers();
    void createPipeline();
    void createTriangle();
    void throwIfFailed(HRESULT hr, const char* what) const;

    HWND hwnd_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> backbuffers_[2];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
    Microsoft::WRL::ComPtr<ID3D12Resource> triangle_;
    D3D12_VERTEX_BUFFER_VIEW triangleView_ = {};
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    HANDLE fenceEvent_ = nullptr;
    UINT64 fenceValue_ = 0;
    UINT rtvDescriptorSize_ = 0;
    bool windowVisible_ = true;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
    void dumpDebugMessages();
    uint64_t framesDone_ = 0;
};

} // namespace f2native
