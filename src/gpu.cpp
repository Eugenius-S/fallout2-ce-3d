#include "gpu.h"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace f2native {

void Gpu::throwIfFailed(HRESULT hr, const char* what) const {
    if (FAILED(hr)) {
        char msg[160];
        std::snprintf(msg, sizeof(msg), "%s failed: 0x%08lX", what, static_cast<unsigned long>(hr));
        throw std::runtime_error(msg);
    }
}

void Gpu::initialize(HWND hwnd, uint32_t width, uint32_t height) {
    hwnd_ = hwnd;
    width_ = width;
    height_ = height;

    // DIAG: debug layer can be disabled via F2NATIVE_NO_DEBUGLAYER=1 - on this
    // Insider build the layer fail-fasts inside Clear calls.
    char noDbg[8] = {};
    size_t noDbgSz = 0;
    getenv_s(&noDbgSz, noDbg, sizeof(noDbg), "F2NATIVE_NO_DEBUGLAYER");
    ComPtr<ID3D12Debug> debug;
    const HRESULT dbgHr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    std::fprintf(stderr, "[gpu] D3D12GetDebugInterface: 0x%08lX\n",
                 static_cast<unsigned long>(dbgHr));
    if (SUCCEEDED(dbgHr) && !(noDbgSz > 0 && noDbg[0] == '1')) {
        debug->EnableDebugLayer();
    }

    throwIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2");

    ComPtr<IDXGIAdapter1> adapter;
    {
        // DIAG: --warp env var switches to the software adapter for bisection.
        char useWarp[8] = {};
        size_t sz = 0;
        getenv_s(&sz, useWarp, sizeof(useWarp), "F2NATIVE_WARP");
        const bool warp = sz > 0 && useWarp[0] == '1';
        if (warp) {
            ComPtr<IDXGIFactory4> f4;
            throwIfFailed(factory_.As(&f4), "factory As(IDXGIFactory4)");
            throwIfFailed(f4->EnumWarpAdapter(IID_PPV_ARGS(&adapter)), "EnumWarpAdapter");
            std::fprintf(stderr, "[gpu] using WARP software adapter\n");
        } else {
            throwIfFailed(factory_->EnumAdapters1(0, &adapter), "EnumAdapters1(0)");
        }
    }
    throwIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_)),
                  "D3D12CreateDevice");
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_)))) {
        infoQueue_->SetMessageCountLimit(128);
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    throwIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue_)),
                  "CreateCommandQueue");

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = width;
    scDesc.Height = height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> swapchain1;
    throwIfFailed(factory_->CreateSwapChainForHwnd(queue_.Get(), hwnd, &scDesc, nullptr, nullptr,
                                                   &swapchain1),
                  "CreateSwapChainForHwnd");
    throwIfFailed(swapchain1.As(&swapchain_), "swapchain As(IDXGISwapChain3)");
    factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    throwIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)),
                  "CreateDescriptorHeap(RTV)");
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    createSwapchainBuffers();

    throwIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                  IID_PPV_ARGS(&allocator_)),
                  "CreateCommandAllocator");
    throwIfFailed(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator_.Get(),
                                             nullptr, IID_PPV_ARGS(&commandList_)),
                  "CreateCommandList");
    throwIfFailed(commandList_->Close(), "commandList initial Close");

    throwIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)),
                  "CreateFence");
    fenceEvent_ = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (!fenceEvent_) throw std::runtime_error("CreateEventEx failed");
}

void Gpu::createSwapchainBuffers() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; ++i) {
        throwIfFailed(swapchain_->GetBuffer(i, IID_PPV_ARGS(&backbuffers_[i])), "GetBuffer");
        device_->CreateRenderTargetView(backbuffers_[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize_;
    }
}

void Gpu::resize(uint32_t width, uint32_t height) {
    if (!swapchain_ || (width == width_ && height == height_)) return;
    waitGpu();
    for (auto& bb : backbuffers_) bb.Reset();
    width_ = width;
    height_ = height;
    throwIfFailed(swapchain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0),
                  "ResizeBuffers");
    createSwapchainBuffers();
}

bool Gpu::beginFrame() {
    if (!windowVisible_) return false;
    throwIfFailed(allocator_->Reset(), "allocator Reset");
    throwIfFailed(commandList_->Reset(allocator_.Get(), nullptr), "commandList Reset");
    return true;
}

void Gpu::submitFrame() {
    throwIfFailed(commandList_->Close(), "commandList Close");
    ID3D12CommandList* lists[] = {commandList_.Get()};
    queue_->ExecuteCommandLists(1, lists);
    throwIfFailed(swapchain_->Present(vsync_ ? 1 : 0, 0), "Present");

    fenceValue_++;
    throwIfFailed(queue_->Signal(fence_.Get(), fenceValue_), "fence Signal");
    if (fence_->GetCompletedValue() < fenceValue_) {
        throwIfFailed(fence_->SetEventOnCompletion(fenceValue_, fenceEvent_), "SetEventOnCompletion");
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
    dumpDebugMessages();
}

void Gpu::flushInit() {
    // Close, execute and wait WITHOUT presenting; then restore the pristine
    // closed-list state the per-frame lifecycle expects. DIAG: skip the
    // execute entirely - an executed zero-command list is the current
    // device-removal suspect.
    throwIfFailed(commandList_->Close(), "commandList Close(flushInit)");
    ID3D12CommandList* lists[] = {commandList_.Get()};
    queue_->ExecuteCommandLists(1, lists);
    fenceValue_++;
    throwIfFailed(queue_->Signal(fence_.Get(), fenceValue_), "fence Signal(flushInit)");
    if (fence_->GetCompletedValue() < fenceValue_) {
        throwIfFailed(fence_->SetEventOnCompletion(fenceValue_, fenceEvent_), "SetEventOnCompletion");
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
    throwIfFailed(allocator_->Reset(), "allocator Reset(flushInit)");
    throwIfFailed(commandList_->Reset(allocator_.Get(), nullptr), "commandList Reset(flushInit)");
    throwIfFailed(commandList_->Close(), "commandList Close(flushInit, pristine)");
    dumpDebugMessages();
}

void Gpu::waitGpu() {
    if (!queue_) return;
    fenceValue_++;
    throwIfFailed(queue_->Signal(fence_.Get(), fenceValue_), "fence Signal(waitGpu)");
    if (fence_->GetCompletedValue() < fenceValue_) {
        throwIfFailed(fence_->SetEventOnCompletion(fenceValue_, fenceEvent_), "SetEventOnCompletion");
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
    dumpDebugMessages();
}

void Gpu::dumpDebugMessages() {
    if (!infoQueue_) return;
    const UINT64 count = infoQueue_->GetNumStoredMessages();
    for (UINT64 i = 0; i < count; ++i) {
        SIZE_T size = 0;
        infoQueue_->GetMessage(i, nullptr, &size);
        std::vector<char> buffer(size);
        D3D12_MESSAGE* msg = reinterpret_cast<D3D12_MESSAGE*>(buffer.data());
        if (SUCCEEDED(infoQueue_->GetMessage(i, msg, &size))) {
            std::fprintf(stderr, "[d3d12][%.3s] %s\n",
                         msg->Severity == D3D12_MESSAGE_SEVERITY_ERROR ? "ERR" : "msg",
                         msg->pDescription);
        }
    }
    infoQueue_->ClearStoredMessages();
}

void Gpu::shutdown() {
    waitGpu();
    if (fenceEvent_) {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

} // namespace f2native
