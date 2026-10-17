#pragma once

#include <windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

namespace f2native {

// Device/queue/swapchain owner plus the per-frame command lifecycle.
// Scene rendering lives in Render; NGX lives in NeuralPass.
class Gpu {
public:
    void initialize(HWND hwnd, uint32_t width, uint32_t height);
    void resize(uint32_t width, uint32_t height);
    bool beginFrame();          // false when the window is hidden; resets list
    void submitFrame();         // close, execute, present, fence wait
    void flushInit();           // close + execute + wait, for init-time uploads
    void waitGpu();
    void dumpDebugMessages();
    void shutdown();
    void setVsync(bool on) { vsync_ = on; }

    ID3D12Device* device() const { return device_.Get(); }
    ID3D12GraphicsCommandList* list() const { return commandList_.Get(); }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    UINT backbufferIndex() const { return swapchain_->GetCurrentBackBufferIndex(); }
    ID3D12Resource* backbufferResource(UINT index) const { return backbuffers_[index].Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE backbufferRtv(UINT index) const {
        D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        h.ptr += static_cast<SIZE_T>(index) * rtvDescriptorSize_;
        return h;
    }

private:
    void createSwapchainBuffers();
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
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
    HANDLE fenceEvent_ = nullptr;
    UINT64 fenceValue_ = 0;
    UINT rtvDescriptorSize_ = 0;
    bool windowVisible_ = true;
    bool vsync_ = true;
};

} // namespace f2native
