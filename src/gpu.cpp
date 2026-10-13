#include "gpu.h"

#include <d3dcompiler.h>
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace f2native {

namespace {

constexpr char kVertexShader[] = R"(
struct VSIn  { float3 pos : POSITION; float3 col : COLOR; };
struct VSOut { float4 pos : SV_POSITION; float3 col : COLOR; };
VSOut vsMain(VSIn i) {
    VSOut o;
    o.pos = float4(i.pos, 1.0);
    o.col = i.col;
    return o;
}
)";

constexpr char kPixelShader[] = R"(
struct VSOut { float4 pos : SV_POSITION; float3 col : COLOR; };
float4 psMain(VSOut i) : SV_Target {
    return float4(i.col, 1.0);
}
)";

struct Vertex {
    float pos[3];
    float col[3];
};

} // namespace

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

    // Debug layer stays on in every build of this prototype: the info queue
    // diagnostics are the primary GPU error channel.
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
    }
    UINT dxgiFlags = 0;
#ifdef NDEBUG
    dxgiFlags = 0; // factory debug flag needs the SDK layer installed; device layer is enough
#else
    dxgiFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    throwIfFailed(CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2");

    ComPtr<IDXGIAdapter1> adapter;
    throwIfFailed(factory_->EnumAdapters1(0, &adapter), "EnumAdapters1(0)");
    throwIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_)),
                  "D3D12CreateDevice");
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_)))) {
        infoQueue_->SetMessageCountLimit(64);
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
    createPipeline();
    createTriangle();

    throwIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                  IID_PPV_ARGS(&allocator_)),
                  "CreateCommandAllocator");
    throwIfFailed(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator_.Get(),
                                             pipeline_.Get(), IID_PPV_ARGS(&commandList_)),
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

void Gpu::createPipeline() {
    D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> serialized;
    ComPtr<ID3DBlob> errors;
    if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized,
                                           &errors))) {
        throw std::runtime_error(static_cast<const char*>(errors ? errors->GetBufferPointer()
                                                                 : "D3D12SerializeRootSignature failed"));
    }
    throwIfFailed(device_->CreateRootSignature(0, serialized->GetBufferPointer(),
                                               serialized->GetBufferSize(),
                                               IID_PPV_ARGS(&rootSignature_)),
                  "CreateRootSignature");

    ComPtr<ID3DBlob> vs;
    ComPtr<ID3DBlob> ps;
    const HRESULT vsHr = D3DCompile(kVertexShader, strlen(kVertexShader), nullptr, nullptr,
                                    nullptr, "vsMain", "vs_5_0", 0, 0, &vs, &errors);
    if (FAILED(vsHr)) {
        const char* detail = errors ? static_cast<const char*>(errors->GetBufferPointer()) : "";
        char msg[512];
        std::snprintf(msg, sizeof(msg), "D3DCompile(vs_5_0) failed: 0x%08lX: %.400s",
                      static_cast<unsigned long>(vsHr), detail);
        throw std::runtime_error(msg);
    }
    errors.Reset();
    const HRESULT psHr = D3DCompile(kPixelShader, strlen(kPixelShader), nullptr, nullptr,
                                    nullptr, "psMain", "ps_5_0", 0, 0, &ps, &errors);
    if (FAILED(psHr)) {
        const char* detail = errors ? static_cast<const char*>(errors->GetBufferPointer()) : "";
        char msg[512];
        std::snprintf(msg, sizeof(msg), "D3DCompile(ps_5_0) failed: 0x%08lX: %.400s",
                      static_cast<unsigned long>(psHr), detail);
        throw std::runtime_error(msg);
    }

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputLayout, 2};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    throwIfFailed(device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_)),
                  "CreateGraphicsPipelineState");
}

void Gpu::createTriangle() {
    constexpr Vertex kVerts[3] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.2f, 0.2f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.2f, 1.0f, 0.2f}},
        {{ 0.0f,  0.5f, 0.0f}, {0.3f, 0.4f, 1.0f}},
    };
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = sizeof(kVerts);
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    throwIfFailed(device_->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                   IID_PPV_ARGS(&triangle_)),
                  "CreateCommittedResource(vertex buffer)");
    void* mapped = nullptr;
    D3D12_RANGE readRange = {0, 0};
    throwIfFailed(triangle_->Map(0, &readRange, &mapped), "vertex Map");
    std::memcpy(mapped, kVerts, sizeof(kVerts));
    triangle_->Unmap(0, nullptr);
    triangleView_.BufferLocation = triangle_->GetGPUVirtualAddress();
    triangleView_.StrideInBytes = sizeof(Vertex);
    triangleView_.SizeInBytes = sizeof(kVerts);
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

void Gpu::renderFrame() {
    if (!windowVisible_) return;
    const UINT bbIndex = swapchain_->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(bbIndex) * rtvDescriptorSize_;

    throwIfFailed(allocator_->Reset(), "allocator Reset");
    throwIfFailed(commandList_->Reset(allocator_.Get(), pipeline_.Get()), "commandList Reset");
    commandList_->SetGraphicsRootSignature(rootSignature_.Get());

    D3D12_RESOURCE_BARRIER toRender = {};
    toRender.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRender.Transition.pResource = backbuffers_[bbIndex].Get();
    toRender.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRender.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toRender.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &toRender);

    commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    const float clearColor[] = {0.05f, 0.06f, 0.10f, 1.0f};
    commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    D3D12_VIEWPORT viewport = {0.0f, 0.0f, static_cast<float>(width_),
                               static_cast<float>(height_), 0.0f, 1.0f};
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
    commandList_->RSSetViewports(1, &viewport);
    commandList_->RSSetScissorRects(1, &scissor);

    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->IASetVertexBuffers(0, 1, &triangleView_);
    commandList_->DrawInstanced(3, 1, 0, 0);

    D3D12_RESOURCE_BARRIER toPresent = toRender;
    toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList_->ResourceBarrier(1, &toPresent);

    // One-shot G0 diagnostic: read back frame 5 and print sample pixels, so
    // "draw produces nothing" is distinguishable from "present shows nothing".
    ComPtr<ID3D12Resource> readback;
    bool didReadback = false;
    if (++framesDone_ == 5) {
        didReadback = true;
        D3D12_RESOURCE_BARRIER toCopy = toRender;
        toCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        commandList_->ResourceBarrier(1, &toCopy);

        D3D12_HEAP_PROPERTIES readbackHeap = {};
        readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC rbDesc = {};
        rbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rbDesc.Width = 4096 * 4 * 3; // three rows at 16 KiB stride
        rbDesc.Height = 1;
        rbDesc.DepthOrArraySize = 1;
        rbDesc.MipLevels = 1;
        rbDesc.Format = DXGI_FORMAT_UNKNOWN;
        rbDesc.SampleDesc.Count = 1;
        rbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        throwIfFailed(device_->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE,
                                                       &rbDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                       nullptr, IID_PPV_ARGS(&readback)),
                      "CreateCommittedResource(readback)");
        const UINT rows[3] = {height_ / 4, height_ / 2, height_ * 3 / 4};
        for (UINT r = 0; r < 3; ++r) {
            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource = backbuffers_[bbIndex].Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_TEXTURE_COPY_LOCATION dst = {};
            dst.pResource = readback.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dst.PlacedFootprint.Offset = r * 4096 * 4;
            dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            dst.PlacedFootprint.Footprint.Width = 1280;
            dst.PlacedFootprint.Footprint.Height = 1;
            dst.PlacedFootprint.Footprint.Depth = 1;
            dst.PlacedFootprint.Footprint.RowPitch = 4096 * 4;
            D3D12_BOX box = {0, rows[r], 0, 1280, rows[r] + 1, 1};
            commandList_->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);
        }

        D3D12_RESOURCE_BARRIER fromCopy = toRender;
        fromCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        fromCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList_->ResourceBarrier(1, &fromCopy);
    }

    throwIfFailed(commandList_->Close(), "commandList Close");
    ID3D12CommandList* lists[] = {commandList_.Get()};
    queue_->ExecuteCommandLists(1, lists);
    throwIfFailed(swapchain_->Present(1, 0), "Present");

    fenceValue_++;
    throwIfFailed(queue_->Signal(fence_.Get(), fenceValue_), "fence Signal");
    if (fence_->GetCompletedValue() < fenceValue_) {
        throwIfFailed(fence_->SetEventOnCompletion(fenceValue_, fenceEvent_), "SetEventOnCompletion");
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
    if (didReadback) {
        D3D12_RANGE read = {0, 4096 * 4 * 3};
        void* mapped = nullptr;
        if (SUCCEEDED(readback->Map(0, &read, &mapped))) {
            const auto* base = static_cast<const uint8_t*>(mapped);
            const UINT rows[3] = {height_ / 4, height_ / 2, height_ * 3 / 4};
            for (UINT r = 0; r < 3; ++r) {
                const auto* pixels = base + r * 4096 * 4;
                UINT firstNonClear = UINT_MAX;
                UINT lastNonClear = 0;
                for (UINT x = 0; x < 1280; ++x) {
                    const uint8_t red = pixels[x * 4];
                    if (red > 20) { // clear red is 13; triangle edges are >=~51
                        if (firstNonClear == UINT_MAX) firstNonClear = x;
                        lastNonClear = x;
                    }
                }
                std::fprintf(stderr, "[grid] row=%u nonclear x=[%u..%u]\n", rows[r],
                             firstNonClear == UINT_MAX ? 0u : firstNonClear, lastNonClear);
            }
            const D3D12_RANGE written = {0, 0};
            readback->Unmap(0, &written);
        }
    }
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

void Gpu::shutdown() {
    waitGpu();
    if (fenceEvent_) {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

} // namespace f2native
