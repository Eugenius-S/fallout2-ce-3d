#include "render.h"
#include "projection.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace f2native {

namespace {

// DIAG: probe root signature created early in initialize.
Microsoft::WRL::ComPtr<ID3D12RootSignature> g_diagRootSig;

constexpr wchar_t kSceneShader[] = L"shaders\\scene.hlsl";
constexpr wchar_t kPresentShader[] = L"shaders\\present.hlsl";

struct SVertex {
    float pos[3];
    float uv[2];
    float col[3];
};

struct FrameCBData {
    XMFLOAT4X4 viewProj;
    XMFLOAT4X4 prevViewProj;
    float renderSize[2];
    uint32_t objectId;
    float pad[2];
};
static_assert(sizeof(FrameCBData) <= 256, "CB slot is 256 bytes");

std::vector<uint8_t> makeCheckerTexture() {
    constexpr UINT kSize = 256;
    std::vector<uint8_t> tex(kSize * kSize * 4);
    for (UINT y = 0; y < kSize; ++y) {
        for (UINT x = 0; x < kSize; ++x) {
            const bool cell = ((x / 32) + (y / 32)) % 2 == 0;
            const uint8_t v = cell ? 235 : 140;
            uint8_t* px = &tex[(y * kSize + x) * 4];
            px[0] = px[1] = px[2] = v;
            px[3] = 255;
        }
    }
    return tex;
}

std::vector<uint8_t> makeCardTexture() {
    constexpr UINT kSize = 128;
    std::vector<uint8_t> tex(kSize * kSize * 4);
    for (UINT y = 0; y < kSize; ++y) {
        for (UINT x = 0; x < kSize; ++x) {
            uint8_t* px = &tex[(y * kSize + x) * 4];
            px[0] = 242; // warm card color; the hole is alpha-tested in the shader
            px[1] = 235;
            px[2] = 204;
            px[3] = 255;
        }
    }
    return tex;
}

void appendQuad(std::vector<SVertex>& vb, const float p0[3], const float p1[3],
                const float p2[3], const float p3[3], const float uv[4][2],
                const float col[3]) {
    const SVertex quad[6] = {
        {{p0[0], p0[1], p0[2]}, {uv[0][0], uv[0][1]}, {col[0], col[1], col[2]}},
        {{p1[0], p1[1], p1[2]}, {uv[1][0], uv[1][1]}, {col[0], col[1], col[2]}},
        {{p2[0], p2[1], p2[2]}, {uv[2][0], uv[2][1]}, {col[0], col[1], col[2]}},
        {{p0[0], p0[1], p0[2]}, {uv[0][0], uv[0][1]}, {col[0], col[1], col[2]}},
        {{p2[0], p2[1], p2[2]}, {uv[2][0], uv[2][1]}, {col[0], col[1], col[2]}},
        {{p3[0], p3[1], p3[2]}, {uv[3][0], uv[3][1]}, {col[0], col[1], col[2]}},
    };
    vb.insert(vb.end(), quad, quad + 6);
}

} // namespace

void Render::initialize(Gpu& gpu, RenderMode mode, CameraKind cameraKind, uint32_t width,
                        uint32_t height) {
    gpu_ = &gpu;
    mode_ = mode;
    cameraKind_ = cameraKind;
    width_ = width;
    height_ = height;
    std::fprintf(stderr, "[init] render.initialize entry removed reason: 0x%08lX\n",
                 static_cast<unsigned long>(gpu.device()->GetDeviceRemovedReason()));

    camera_.configure(cameraKind_, static_cast<float>(width) / static_cast<float>(height));

    camera_.configure(cameraKind_, static_cast<float>(width) / static_cast<float>(height));

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 8;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    throwIfFailed(gpu.device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap_)),
                  "CreateDescriptorHeap(SRV)");
    srvDescriptorSize_ =
        gpu.device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // DIAG probe: creates a representative root signature; the first stage
    // after which this fails marks the poisoning point.
    auto diagRootSig = [&](const char* stage) {
        D3D12_DESCRIPTOR_RANGE r = {};
        r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        r.NumDescriptors = 1;
        D3D12_ROOT_PARAMETER p[2] = {};
        p[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        p[0].Descriptor.ShaderRegister = 0;
        p[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        p[1].DescriptorTable.NumDescriptorRanges = 1;
        p[1].DescriptorTable.pDescriptorRanges = &r;
        D3D12_STATIC_SAMPLER_DESC s = {};
        s.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        s.AddressU = s.AddressV = s.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        s.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        s.ShaderRegister = 0;
        s.MaxLOD = D3D12_FLOAT32_MAX;
        D3D12_ROOT_SIGNATURE_DESC rd = {};
        rd.NumParameters = 2;
        rd.pParameters = p;
        rd.NumStaticSamplers = 1;
        rd.pStaticSamplers = &s;
        rd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ComPtr<ID3DBlob> blob;
        ComPtr<ID3D12RootSignature> rs;
        HRESULT hrS = D3D12SerializeRootSignature(&rd, D3D_ROOT_SIGNATURE_VERSION_1, &blob,
                                                  nullptr);
        HRESULT hrC = S_OK;
        if (SUCCEEDED(hrS)) {
            hrC = gpu.device()->CreateRootSignature(0, blob->GetBufferPointer(),
                                                    blob->GetBufferSize(),
                                                    IID_PPV_ARGS(&rs));
        }
        std::fprintf(stderr, "[init][probe %s] serialize=0x%08lX create=0x%08lX removed=0x%08lX\n",
                     stage, static_cast<unsigned long>(hrS),
                     static_cast<unsigned long>(hrC),
                     static_cast<unsigned long>(gpu.device()->GetDeviceRemovedReason()));
        if (std::string(stage) == "A0 entry") {
            g_diagRootSig = rs; // kept for the recording experiment
        }
    };

    diagRootSig("A0 entry");
    createSizedResources(width, height);
    diagRootSig("A1 after-sized");
    gpu.beginFrame();
    diagRootSig("A2 after-beginframe");
    createProceduralTextures();
    diagRootSig("A3 after-textures");
    gpu.flushInit();
    diagRootSig("A4 after-flush-skipped");
    pendingUploads_.clear(); // GPU has executed the copies; scratches may die
    diagRootSig("A5 after-clear-scratch");

    createPipelines();
    diagRootSig("A6 after-pipelines");
    createSceneGeometry();
    diagRootSig("A7 after-geometry");

    // One 256-byte CB slot per draw call (objectId differs), 4 draw calls.
    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = 256 * 4;
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.Format = DXGI_FORMAT_UNKNOWN;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    throwIfFailed(gpu.device()->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&frameCB_)),
                  "CreateCommittedResource(frameCB)");
    D3D12_RANGE noRead = {0, 0};
    throwIfFailed(frameCB_->Map(0, &noRead, reinterpret_cast<void**>(&frameCBMapped_)),
                  "frameCB Map");

    // NGX runtime for carrier/neural modes (spec: native must not init NGX).
    if (mode_ != RenderMode::Native) {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::filesystem::path dir(exePath);
        neural_.initialize(gpu.device(), dir.parent_path());
    }
}

void Render::throwIfFailed(HRESULT hr, const char* what) const {
    if (FAILED(hr)) {
        char msg[320];
        if (hr == DXGI_ERROR_DEVICE_REMOVED && gpu_ && gpu_->device()) {
            gpu_->dumpDebugMessages(); // validation output first
            const HRESULT reason = gpu_->device()->GetDeviceRemovedReason();
            std::snprintf(msg, sizeof(msg), "%s failed: 0x%08lX (device removed reason: 0x%08lX)",
                          what, static_cast<unsigned long>(hr),
                          static_cast<unsigned long>(reason));
        } else {
            std::snprintf(msg, sizeof(msg), "%s failed: 0x%08lX", what,
                          static_cast<unsigned long>(hr));
        }
        throw std::runtime_error(msg);
    }
}

void Render::createSizedResources(uint32_t width, uint32_t height) {
    Gpu& gpu = *gpu_;
    ID3D12Device* dev = gpu.device();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    throwIfFailed(dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)),
                  "CreateDescriptorHeap(gbuffer RTV)");
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    throwIfFailed(dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_)),
                  "CreateDescriptorHeap(DSV)");

    auto makeTarget = [&](DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
                          ID3D12Resource** out) {
        D3D12_RESOURCE_DESC d = {};
        d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width = width;
        d.Height = height;
        d.DepthOrArraySize = 1;
        d.MipLevels = 1;
        d.Format = format;
        d.SampleDesc.Count = 1;
        d.Flags = flags;
        D3D12_HEAP_PROPERTIES heap = {};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        // Depth-stencil resources must not be created in COMMON (they cannot
        // decay to it); everything else starts COMMON and gets promoted.
        const D3D12_RESOURCE_STATES initialState =
            (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
                ? D3D12_RESOURCE_STATE_DEPTH_WRITE
                : D3D12_RESOURCE_STATE_COMMON;
        throwIfFailed(dev->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &d,
                                                   initialState, nullptr,
                                                   IID_PPV_ARGS(out)),
                      "CreateCommittedResource(gbuffer target)");
    };

    makeTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
               color_.ReleaseAndGetAddressOf());
    std::fprintf(stderr, "[init] color target ok, removed=0x%08lX\n",
                 static_cast<unsigned long>(dev->GetDeviceRemovedReason()));
    makeTarget(DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
               motion_.ReleaseAndGetAddressOf());
    std::fprintf(stderr, "[init] motion target ok, removed=0x%08lX\n",
                 static_cast<unsigned long>(dev->GetDeviceRemovedReason()));
    makeTarget(DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
               depth_.ReleaseAndGetAddressOf());
    std::fprintf(stderr, "[init] depth target ok, removed=0x%08lX\n",
                 static_cast<unsigned long>(dev->GetDeviceRemovedReason()));
    makeTarget(DXGI_FORMAT_R16G16B16A16_FLOAT,
               D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                   D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
               output_.ReleaseAndGetAddressOf());
    std::fprintf(stderr, "[init] output target ok, removed=0x%08lX\n",
                 static_cast<unsigned long>(dev->GetDeviceRemovedReason()));

    // Views
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    dev->CreateRenderTargetView(color_.Get(), &rtvDesc, rtv);
    std::fprintf(stderr, "[init] rtv color ok, removed=0x%08lX\n",
                 static_cast<unsigned long>(dev->GetDeviceRemovedReason()));
    rtvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
    rtv.ptr += gpu.device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dev->CreateRenderTargetView(motion_.Get(), &rtvDesc, rtv);
    std::fprintf(stderr, "[init] rtv motion ok, removed=0x%08lX\n",
                 static_cast<unsigned long>(dev->GetDeviceRemovedReason()));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dev->CreateDepthStencilView(depth_.Get(), &dsvDesc,
                                dsvHeap_->GetCPUDescriptorHandleForHeapStart());
    std::fprintf(stderr, "[init] dsv depth ok, removed=0x%08lX\n",
                 static_cast<unsigned long>(dev->GetDeviceRemovedReason()));

    D3D12_CPU_DESCRIPTOR_HANDLE srv = srvHeap_->GetCPUDescriptorHandleForHeapStart();
    // [0] checker and [1] card SRVs are filled in createProceduralTextures,
    // after those textures exist.
    auto makeSrvAt = [&](ID3D12Resource* res, DXGI_FORMAT format, UINT slot) {
        D3D12_CPU_DESCRIPTOR_HANDLE h = srv;
        h.ptr += static_cast<SIZE_T>(slot) * srvDescriptorSize_;
        std::fprintf(stderr, "[init] srv slot=%u size=%u h.ptr=0x%012llx start=0x%012llx\n", slot,
                     srvDescriptorSize_, static_cast<unsigned long long>(h.ptr),
                     static_cast<unsigned long long>(srv.ptr));
        dev->CreateShaderResourceView(res, nullptr, h); // default desc from resource
        std::fprintf(stderr, "[init] srv slot=%u ok, removed=0x%08lX\n", slot,
                     static_cast<unsigned long>(dev->GetDeviceRemovedReason()));
    };
    makeSrvAt(color_.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, 2);
    colorSrvGpu_ = srvHeap_->GetGPUDescriptorHandleForHeapStart();
    colorSrvGpu_.ptr += srvDescriptorSize_ * 2;
    makeSrvAt(output_.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, 3);
    outputSrvGpu_ = colorSrvGpu_;
    outputSrvGpu_.ptr += srvDescriptorSize_;
}

void Render::createProceduralTextures() {
    Gpu& gpu = *gpu_;
    ID3D12Device* dev = gpu.device();
    ID3D12GraphicsCommandList* list = gpu.list();

    auto uploadTexture = [&](const std::vector<uint8_t>& pixels, UINT size,
                             ID3D12Resource** out) {
        D3D12_RESOURCE_DESC d = {};
        d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width = size;
        d.Height = size;
        d.DepthOrArraySize = 1;
        d.MipLevels = 1;
        d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d.SampleDesc.Count = 1;
        D3D12_HEAP_PROPERTIES heap = {};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        throwIfFailed(dev->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &d,
                                                   D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                   IID_PPV_ARGS(out)),
                      "CreateCommittedResource(texture)");
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
        UINT64 total = 0;
        dev->GetCopyableFootprints(&d, 0, 1, 0, &footprint, nullptr, nullptr, &total);
        D3D12_HEAP_PROPERTIES upHeap = {};
        upHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = total;
        bufDesc.Height = 1;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1;
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> scratch;
        throwIfFailed(dev->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                   IID_PPV_ARGS(&scratch)),
                      "CreateCommittedResource(texture upload)");
        void* mapped = nullptr;
        D3D12_RANGE noRead = {0, 0};
        throwIfFailed(scratch->Map(0, &noRead, &mapped), "texture upload Map");
        for (UINT row = 0; row < size; ++row) {
            std::memcpy(static_cast<uint8_t*>(mapped) + footprint.Footprint.RowPitch * row,
                        pixels.data() + size * 4 * row, size * 4);
        }
        scratch->Unmap(0, nullptr);
        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = scratch.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = footprint;
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = *out;
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        D3D12_RESOURCE_BARRIER toShader = {};
        toShader.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toShader.Transition.pResource = *out;
        toShader.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        toShader.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        toShader.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        list->ResourceBarrier(1, &toShader);
        uploadScratch_ = scratch; // keeps alive until GPU finishes (fence per frame)
        pendingUploads_.push_back(scratch); // both scratches must outlive flushInit
    };

    std::fprintf(stderr, "[init] uploading checker texture\n");
    uploadTexture(makeCheckerTexture(), 256, checkerTex_.ReleaseAndGetAddressOf());
    std::fprintf(stderr, "[init] checker OK\n");
    uploadTexture(makeCardTexture(), 128, cardTex_.ReleaseAndGetAddressOf());
    std::fprintf(stderr, "[init] card OK\n");

    // SRVs for the now-existing textures: [0] checker, [1] card.
    D3D12_CPU_DESCRIPTOR_HANDLE srv = srvHeap_->GetCPUDescriptorHandleForHeapStart();
    auto makeSrv = [&](ID3D12Resource* res, DXGI_FORMAT format, UINT slot) {
        D3D12_CPU_DESCRIPTOR_HANDLE h = srv;
        h.ptr += static_cast<SIZE_T>(slot) * srvDescriptorSize_;
        D3D12_SHADER_RESOURCE_VIEW_DESC s = {};
        s.Format = format;
        s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        s.Texture2D.MostDetailedMip = 0;
        s.Texture2D.MipLevels = 1;
        s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        dev->CreateShaderResourceView(res, &s, h);
    };
    makeSrv(checkerTex_.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    checkerSrvGpu_ = srvHeap_->GetGPUDescriptorHandleForHeapStart();
    makeSrv(cardTex_.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, 1);
    cardSrvGpu_ = checkerSrvGpu_;
    cardSrvGpu_.ptr += srvDescriptorSize_;
}

void Render::createPipelines() {
    std::fprintf(stderr, "[init] createPipelines enter\n");
    Gpu& gpu = *gpu_;
    ID3D12Device* dev = gpu.device();

    // G-buffer root signature: CBV b0, SRV table t0, static linear sampler.
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    D3D12_ROOT_PARAMETER gParams[2] = {};
    gParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    gParams[0].Descriptor.ShaderRegister = 0;
    gParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    gParams[1].DescriptorTable.NumDescriptorRanges = 1;
    gParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    sampler.ShaderRegister = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    D3D12_ROOT_SIGNATURE_DESC gRootDesc = {};
    gRootDesc.NumParameters = 2;
    gRootDesc.pParameters = gParams;
    gRootDesc.NumStaticSamplers = 1;
    gRootDesc.pStaticSamplers = &sampler;
    gRootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> serialized;
    ComPtr<ID3DBlob> errors;
    throwIfFailed(D3D12SerializeRootSignature(&gRootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              &serialized, &errors),
                  "D3D12SerializeRootSignature(gbuffer)");
    throwIfFailed(dev->CreateRootSignature(0, serialized->GetBufferPointer(),
                                           serialized->GetBufferSize(),
                                           IID_PPV_ARGS(&gbufferRoot_)),
                  "CreateRootSignature(gbuffer)");

    // Present root signature: SRV table t0 + static sampler.
    D3D12_ROOT_PARAMETER pParams[1] = {};
    pParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    pParams[0].DescriptorTable.NumDescriptorRanges = 1;
    pParams[0].DescriptorTable.pDescriptorRanges = &srvRange;
    D3D12_ROOT_SIGNATURE_DESC pRootDesc = {};
    pRootDesc.NumParameters = 1;
    pRootDesc.pParameters = pParams;
    pRootDesc.NumStaticSamplers = 1;
    pRootDesc.pStaticSamplers = &sampler;
    throwIfFailed(D3D12SerializeRootSignature(&pRootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              &serialized, &errors),
                  "D3D12SerializeRootSignature(present)");
    throwIfFailed(dev->CreateRootSignature(0, serialized->GetBufferPointer(),
                                           serialized->GetBufferSize(),
                                           IID_PPV_ARGS(&presentRoot_)),
                  "CreateRootSignature(present)");

    // Shaders
    const UINT compileFlags = 0;
    auto compileFromFile = [&](const wchar_t* file, const char* entry, const char* target,
                               ComPtr<ID3DBlob>& blob) {
        ComPtr<ID3DBlob> shaderErrors;
        const HRESULT hr =
            D3DCompileFromFile(file, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target,
                               compileFlags, 0, &blob, &shaderErrors);
        if (FAILED(hr)) {
            const char* detail = shaderErrors
                                     ? static_cast<const char*>(shaderErrors->GetBufferPointer())
                                     : "";
            char head[160];
            std::snprintf(head, sizeof(head), "D3DCompileFromFile(%ls %s) failed: 0x%08lX", file,
                          entry, static_cast<unsigned long>(hr));
            throw std::runtime_error(std::string(head) + "\n" + detail);
        }
    };
    ComPtr<ID3DBlob> vs, ps;
    compileFromFile(kSceneShader, "vsMain", "vs_5_0", vs);
    compileFromFile(kSceneShader, "psMain", "ps_5_0", ps);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"UV",       0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.InputLayout = {inputLayout, 3};
    pso.pRootSignature = gbufferRoot_.Get();
    pso.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    pso.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.DepthStencilState.DepthEnable = TRUE;
    pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 2;
    pso.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    pso.RTVFormats[1] = DXGI_FORMAT_R16G16_FLOAT;
    pso.SampleDesc.Count = 1;
    throwIfFailed(dev->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&gbufferPso_)),
                  "CreateGraphicsPipelineState(gbuffer)");

    errors.Reset();
    compileFromFile(kPresentShader, "vsMain", "vs_5_0", vs);
    compileFromFile(kPresentShader, "psMain", "ps_5_0", ps);
    pso = {};
    pso.pRootSignature = presentRoot_.Get();
    pso.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    pso.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.SampleDesc.Count = 1;
    throwIfFailed(dev->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&presentPso_)),
                  "CreateGraphicsPipelineState(present)");
}

void Render::createSceneGeometry() {
    std::fprintf(stderr, "[init] createSceneGeometry enter\n");
    Gpu& gpu = *gpu_;
    ID3D12Device* dev = gpu.device();
    std::vector<SVertex> vb;

    // Floor: y=0, 12x12 units, uv 0..6 (2 checker cells per unit).
    {
        const float c[3] = {0.85f, 0.88f, 0.95f};
        const float p0[3] = {-6, 0, 6}, p1[3] = {6, 0, 6}, p2[3] = {6, 0, -6}, p3[3] = {-6, 0, -6};
        const float uv[4][2] = {{0, 6}, {6, 6}, {6, 0}, {0, 0}};
        appendQuad(vb, p0, p1, p2, p3, uv, c);
    }
    // Box A: 1x1x1 at (-1, 0.5, -1), 5 visible faces (skip bottom).
    {
        const float c[3] = {0.95f, 0.35f, 0.3f};
        const float x0 = -1.5f, x1 = -0.5f, y0 = 0.0f, y1 = 1.0f, z0 = -1.5f, z1 = -0.5f;
        const float u[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        const float fz0[3] = {x0, y0, z1}, fz1[3] = {x1, y0, z1}, fz2[3] = {x1, y1, z1}, fz3[3] = {x0, y1, z1};
        appendQuad(vb, fz0, fz1, fz2, fz3, u, c);
        const float bz0[3] = {x1, y0, z0}, bz1[3] = {x0, y0, z0}, bz2[3] = {x0, y1, z0}, bz3[3] = {x1, y1, z0};
        appendQuad(vb, bz0, bz1, bz2, bz3, u, c);
        const float lx0[3] = {x0, y0, z0}, lx1[3] = {x0, y0, z1}, lx2[3] = {x0, y1, z1}, lx3[3] = {x0, y1, z0};
        appendQuad(vb, lx0, lx1, lx2, lx3, u, c);
        const float rx0[3] = {x1, y0, z1}, rx1[3] = {x1, y0, z0}, rx2[3] = {x1, y1, z0}, rx3[3] = {x1, y1, z1};
        appendQuad(vb, rx0, rx1, rx2, rx3, u, c);
        const float ty0[3] = {x0, y1, z1}, ty1[3] = {x1, y1, z1}, ty2[3] = {x1, y1, z0}, ty3[3] = {x0, y1, z0};
        appendQuad(vb, ty0, ty1, ty2, ty3, u, c);
    }
    // Box B: 1.5x0.75x1 at (1.2, 0.375, -2), front face only for the overlap test.
    {
        const float c[3] = {0.3f, 0.85f, 0.45f};
        const float x0 = 0.45f, x1 = 1.95f, y0 = 0.0f, y1 = 0.75f, z1 = -1.5f;
        const float u[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        const float f0[3] = {x0, y0, z1}, f1[3] = {x1, y0, z1}, f2[3] = {x1, y1, z1}, f3[3] = {x0, y1, z1};
        appendQuad(vb, f0, f1, f2, f3, u, c);
        const float l0[3] = {x0, y0, -2.0f}, l1[3] = {x0, y0, z1}, l2[3] = {x0, y1, z1}, l3[3] = {x0, y1, -2.0f};
        appendQuad(vb, l0, l1, l2, l3, u, c);
        const float t0[3] = {x0, y1, z1}, t1[3] = {x1, y1, z1}, t2[3] = {x1, y1, -2.0f}, t3[3] = {x0, y1, -2.0f};
        appendQuad(vb, t0, t1, t2, t3, u, c);
    }
    // Alpha card: 1.4x1.4 at z=1.2 facing the camera, central hole in shader.
    {
        const float c[3] = {0.95f, 0.92f, 0.8f};
        const float p0[3] = {-0.7f, 0.2f, 1.2f}, p1[3] = {0.7f, 0.2f, 1.2f};
        const float p2[3] = {0.7f, 1.6f, 1.2f}, p3[3] = {-0.7f, 1.6f, 1.2f};
        const float uv[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        appendQuad(vb, p0, p1, p2, p3, uv, c);
    }

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = vb.size() * sizeof(SVertex);
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    throwIfFailed(dev->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&sceneVB_)),
                  "CreateCommittedResource(sceneVB)");
    void* mapped = nullptr;
    D3D12_RANGE noRead = {0, 0};
    throwIfFailed(sceneVB_->Map(0, &noRead, &mapped), "sceneVB Map");
    std::memcpy(mapped, vb.data(), vb.size() * sizeof(SVertex));
    sceneVB_->Unmap(0, nullptr);
    sceneView_.BufferLocation = sceneVB_->GetGPUVirtualAddress();
    sceneView_.StrideInBytes = sizeof(SVertex);
    sceneView_.SizeInBytes = static_cast<UINT>(vb.size() * sizeof(SVertex));
    std::fprintf(stderr, "[init] scene geometry done (%zu verts)\n", vb.size());
}

void Render::resize(uint32_t width, uint32_t height) {
    if (!gpu_) return; // WM_SIZE can fire before initialize (window creation)
    if (width == width_ && height == height_) return;
    gpu_->waitGpu();
    color_.Reset();
    motion_.Reset();
    depth_.Reset();
    output_.Reset();
    width_ = width;
    height_ = height;
    bbStates_[0] = bbStates_[1] = D3D12_RESOURCE_STATE_COMMON;
    createSizedResources(width, height);
    camera_.configure(cameraKind_, static_cast<float>(width) / static_cast<float>(height));
    // The NGX feature is size-bound: release and rebuild it for the new size.
    // The first evaluate afterwards must reset history.
    if (mode_ != RenderMode::Native && neural_.ready()) {
        neural_.releaseFeature();
        gpu_->beginFrame();
        neural_.create(gpu_->list(), width_, height_);
        gpu_->submitFrame();
        forceReset_ = true;
    }
}

bool Render::renderFrame(double tSeconds, Route route, bool& resetHistory) {
    Gpu& gpu = *gpu_;
    std::fprintf(stderr, "[frame %llu] enter\n", static_cast<unsigned long long>(framesRendered_));
    if (!gpu.beginFrame()) return false;
    ID3D12GraphicsCommandList* list = gpu.list();

    resetHistory = camera_.poseAt(tSeconds, route);
    std::fprintf(stderr, "[frame] pose ok\n");
    resetHistory = resetHistory || framesRendered_ == 0 || forceReset_;
    forceReset_ = false;

    // Write the four CB slots (one per draw).
    FrameCBData cb = {};
    XMStoreFloat4x4(&cb.viewProj, XMMatrixTranspose(camera_.viewProj()));
    XMStoreFloat4x4(&cb.prevViewProj, XMMatrixTranspose(camera_.prevViewProj()));
    cb.renderSize[0] = static_cast<float>(width_);
    cb.renderSize[1] = static_cast<float>(height_);
    const UINT drawVertexOffsets[4] = {0, 6, 36, 54};
    const UINT drawVertexCounts[4] = {6, 30, 18, 6};
    for (uint32_t obj = 0; obj < 4; ++obj) {
        cb.objectId = obj;
        std::memcpy(frameCBMapped_ + obj * 256, &cb, sizeof(cb));
    }
    std::fprintf(stderr, "[frame] cb written\n");

    // G-buffer pass
    D3D12_RESOURCE_BARRIER bar = {};
    bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] = {rtvHeap_->GetCPUDescriptorHandleForHeapStart()};
    rtvs[1] = rtvs[0];
    rtvs[1].ptr += gpu.device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    {
    if (stateColor_ != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        bar.Transition.pResource = color_.Get();
        bar.Transition.StateBefore = stateColor_;
        bar.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        list->ResourceBarrier(1, &bar);
        stateColor_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (stateMotion_ != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        bar.Transition.pResource = motion_.Get();
        bar.Transition.StateBefore = stateMotion_;
        bar.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        list->ResourceBarrier(1, &bar);
        stateMotion_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (stateDepth_ != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
        bar.Transition.pResource = depth_.Get();
        bar.Transition.StateBefore = stateDepth_;
        bar.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        list->ResourceBarrier(1, &bar);
        stateDepth_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    }

    {
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    std::fprintf(stderr, "[frame] barriers ok\n");
    list->OMSetRenderTargets(2, rtvs, FALSE, &dsvHandle);
    std::fprintf(stderr, "[frame] omset ok\n");
    const float clearColor[4] = {0.02f, 0.03f, 0.06f, 1.0f}; // "sky"
    const float clearMotion[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    list->ClearRenderTargetView(rtvs[0], clearColor, 0, nullptr);
    list->ClearRenderTargetView(rtvs[1], clearMotion, 0, nullptr);
    list->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    gpu.dumpDebugMessages(); // DIAG: read validation messages before the fail-fast
    std::fprintf(stderr, "[frame] clear ok\n");
    }
    D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(width_), static_cast<float>(height_), 0, 1};
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
    list->RSSetViewports(1, &viewport);
    list->RSSetScissorRects(1, &scissor);
    std::fprintf(stderr, "[frame] viewport ok\n");
    {
    list->SetGraphicsRootSignature(gbufferRoot_.Get());
    list->SetPipelineState(gbufferPso_.Get());
    ID3D12DescriptorHeap* heaps[1] = {srvHeap_.Get()};
    list->SetDescriptorHeaps(1, heaps);
    const D3D12_GPU_VIRTUAL_ADDRESS cbBase = frameCB_->GetGPUVirtualAddress();
    list->SetGraphicsRootDescriptorTable(1, checkerSrvGpu_);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->IASetVertexBuffers(0, 1, &sceneView_);
    for (uint32_t obj = 0; obj < 4; ++obj) {
        if (obj == 1 || obj == 2) {
            list->SetGraphicsRootDescriptorTable(1, checkerSrvGpu_);
        }
        list->SetGraphicsRootConstantBufferView(0, cbBase + obj * 256);
        list->DrawInstanced(drawVertexCounts[obj], 1, drawVertexOffsets[obj], 0);
    }
    }
    std::fprintf(stderr, "[frame] gbuffer drawn\n");

    // Neural pass (carrier/neural): decline everything to COMMON, let NGX drive.
    if (mode_ != RenderMode::Native) {
        if (stateColor_ != D3D12_RESOURCE_STATE_COMMON) {
            bar.Transition.pResource = color_.Get();
            bar.Transition.StateBefore = stateColor_;
            bar.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
            list->ResourceBarrier(1, &bar);
            stateColor_ = D3D12_RESOURCE_STATE_COMMON;
        }
        if (stateMotion_ != D3D12_RESOURCE_STATE_COMMON) {
            bar.Transition.pResource = motion_.Get();
            bar.Transition.StateBefore = stateMotion_;
            bar.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
            list->ResourceBarrier(1, &bar);
            stateMotion_ = D3D12_RESOURCE_STATE_COMMON;
        }
        if (stateDepth_ != D3D12_RESOURCE_STATE_COMMON) {
            bar.Transition.pResource = depth_.Get();
            bar.Transition.StateBefore = stateDepth_;
            bar.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
            list->ResourceBarrier(1, &bar);
            stateDepth_ = D3D12_RESOURCE_STATE_COMMON;
        }
        if (framesRendered_ == 0 && !neural_.ready()) {
            neural_.create(list, width_, height_);
        }
        FrameInputs inputs = {};
        inputs.color = color_.Get();
        inputs.output = output_.Get();
        inputs.depth = depth_.Get();
        inputs.motion = motion_.Get();
        inputs.width = width_;
        inputs.height = height_;
        inputs.resetHistory = resetHistory;
        neural_.evaluate(list, inputs);
    }

    // Present pass
    {
    if (mode_ == RenderMode::Native) {
        if (stateColor_ != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
            bar.Transition.pResource = color_.Get();
            bar.Transition.StateBefore = stateColor_;
            bar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            list->ResourceBarrier(1, &bar);
            stateColor_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
    }
    const UINT bbIndex = gpu.backbufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE bbRtv = gpu.backbufferRtv(bbIndex);
    const float bbClear[4] = {0.02f, 0.03f, 0.06f, 1.0f};
    ID3D12DescriptorHeap* heaps[1] = {srvHeap_.Get()};
    if (bbStates_[bbIndex] != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        D3D12_RESOURCE_BARRIER bbBar = {};
        bbBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        bbBar.Transition.pResource = gpu.backbufferResource(bbIndex);
        bbBar.Transition.StateBefore = bbStates_[bbIndex];
        bbBar.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        bbBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        list->ResourceBarrier(1, &bbBar);
        bbStates_[bbIndex] = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    list->OMSetRenderTargets(1, &bbRtv, FALSE, nullptr);
    list->ClearRenderTargetView(bbRtv, bbClear, 0, nullptr);
    list->RSSetViewports(1, &viewport);
    list->RSSetScissorRects(1, &scissor);
    list->SetGraphicsRootSignature(presentRoot_.Get());
    list->SetPipelineState(presentPso_.Get());
    list->SetDescriptorHeaps(1, heaps);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    if (mode_ == RenderMode::Native) {
        list->SetGraphicsRootDescriptorTable(0, colorSrvGpu_);
    } else {
        list->SetGraphicsRootDescriptorTable(0, outputSrvGpu_);
    }
    list->DrawInstanced(3, 1, 0, 0);
    {
        D3D12_RESOURCE_BARRIER bbToPresent = {};
        bbToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        bbToPresent.Transition.pResource = gpu.backbufferResource(bbIndex);
        bbToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        bbToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        bbToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        list->ResourceBarrier(1, &bbToPresent);
        bbStates_[bbIndex] = D3D12_RESOURCE_STATE_PRESENT;
    }

    }

    gpu.submitFrame();
    ++framesRendered_;
    return true;
}

void Render::shutdown() {
    gpu_->waitGpu();
    if (frameCBMapped_) {
        frameCB_->Unmap(0, nullptr);
        frameCBMapped_ = nullptr;
    }
    if (mode_ != RenderMode::Native) neural_.shutdown();
}

} // namespace f2native

namespace f2native {

std::vector<float> Render::readbackPixel(ID3D12Resource* res, D3D12_RESOURCE_STATES& state,
                                         D3D12_RESOURCE_STATES copyableState, UINT x, UINT y,
                                         DXGI_FORMAT viewFormat) {
    Gpu& gpu = *gpu_;
    if (!gpu.beginFrame()) return {};
    ID3D12GraphicsCommandList* list = gpu.list();

    D3D12_RESOURCE_BARRIER bar = {};
    bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    bar.Transition.pResource = res;
    bar.Transition.StateBefore = state;
    bar.Transition.StateAfter = copyableState;
    bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    list->ResourceBarrier(1, &bar);

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC rb = {};
    rb.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rb.Width = 256;
    rb.Height = 1;
    rb.DepthOrArraySize = 1;
    rb.MipLevels = 1;
    rb.Format = DXGI_FORMAT_UNKNOWN;
    rb.SampleDesc.Count = 1;
    rb.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> readback;
    throwIfFailed(gpu.device()->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &rb,
                                                        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                        IID_PPV_ARGS(&readback)),
                  "CreateCommittedResource(pixel readback)");
    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = res;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = readback.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Footprint.Format = viewFormat;
    dst.PlacedFootprint.Footprint.Width = 1;
    dst.PlacedFootprint.Footprint.Height = 1;
    dst.PlacedFootprint.Footprint.Depth = 1;
    dst.PlacedFootprint.Footprint.RowPitch = 256;
    D3D12_BOX box = {x, y, 0, x + 1, y + 1, 1};
    list->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);

    bar.Transition.StateBefore = copyableState;
    bar.Transition.StateAfter = state;
    list->ResourceBarrier(1, &bar);
    gpu.submitFrame();

    D3D12_RANGE readRange = {0, 64};
    void* mapped = nullptr;
    std::vector<float> out(4, 0.0f);
    if (SUCCEEDED(readback->Map(0, &readRange, &mapped))) {
        if (viewFormat == DXGI_FORMAT_R32_FLOAT) {
            out[0] = *static_cast<const float*>(mapped);
        } else {
            const uint16_t* halves = static_cast<const uint16_t*>(mapped);
            for (int ch = 0; ch < 4; ++ch) {
                out[ch] = XMConvertHalfToFloat(halves[ch]);
            }
        }
        const D3D12_RANGE written = {0, 0};
        readback->Unmap(0, &written);
    }
    return out;
}

SelftestResult Render::runSelftest() {
    SelftestResult result;
    auto check = [&](bool ok, const std::string& name, const std::string& detail) {
        result.details += (ok ? "PASS " : "FAIL ") + name + (detail.empty() ? "" : " | " + detail) + "\n";
        if (!ok) result.allPassed = false;
    };

    // 1. Static camera: motion must be ~0 everywhere on geometry.
    bool r = false;
    renderFrame(1.0, Route::Static, r);
    const XMMATRIX staticViewProj = camera_.viewProj(); // pose 1.0 = offset 0
    renderFrame(1.1, Route::Static, r);
    {
        auto m = readbackPixel(motion_.Get(), stateMotion_,
                               D3D12_RESOURCE_STATE_COPY_SOURCE, 640, 600,
                               DXGI_FORMAT_R16G16_FLOAT);
        const float mx = m[0], my = m[1];
        check(std::fabs(mx) <= 0.25f && std::fabs(my) <= 0.25f, "static motion ~0",
              "motion=(" + std::to_string(mx) + ", " + std::to_string(my) + ")");
    }

    // 2. Pan motion at the world origin: GPU motion must match the CPU
    //    projection of (0,0,0) through prev/curr matrices within 0.25 px.
    renderFrame(5.0, Route::Pan, r);
    renderFrame(5.2, Route::Pan, r);
    {
        // True correspondence: unproject the pixel to the z=0 plane to find the
        // floor point it shows NOW, then compare its projection delta prev->curr
        // with the GPU motion buffer.
        const LONG px = 900, py = 500; // guaranteed floor, away from card/boxes
        const float ndcX = (px + 0.5f) / width_ * 2.0f - 1.0f;
        const float ndcY = 1.0f - (py + 0.5f) / height_ * 2.0f;
        const XMMATRIX invView = XMMatrixInverse(nullptr, camera_.view());
        const XMMATRIX invProj = XMMatrixInverse(nullptr, camera_.proj());
        const XMMATRIX toWorld = invProj * invView; // row-vector: undo proj first, then view
        const XMVECTOR nearW = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), toWorld);
        const XMVECTOR farW = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), toWorld);
        const XMFLOAT3 eyePos = camera_.eye();
        const XMVECTOR eyeV = XMLoadFloat3(&eyePos);
        const XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(farW, nearW));
        const float dz = XMVectorGetZ(dir);
        if (std::fabs(dz) < 1e-6f) throw std::runtime_error("pan ray parallel to floor");
        const float tHit = -XMVectorGetZ(eyeV) / dz;
        const float qx = XMVectorGetX(eyeV) + XMVectorGetX(dir) * tHit;
        const float qy = XMVectorGetY(eyeV) + XMVectorGetY(dir) * tHit;
        auto project = [&](float u, float v, float z, XMMATRIX vp) {
            const XMVECTOR clip = XMVector4Transform(XMVectorSet(u, v, z, 1.0f), vp);
            const float x = XMVectorGetX(clip), y = XMVectorGetY(clip), w = XMVectorGetW(clip);
            return XMFLOAT2{x / w * 0.5f + 0.5f, 0.5f - y / w * 0.5f};
        };
        const XMFLOAT2 curr = project(qx, qy, 0.0f, camera_.viewProj());
        const XMFLOAT2 prev = project(qx, qy, 0.0f, camera_.prevViewProj());
        const float expectX = (prev.x - curr.x) * width_;
        const float expectY = (prev.y - curr.y) * height_;
        // DIAG: field comparison GPU vs CPU over a 5x5 grid.
        for (int gy = 0; gy < 5; ++gy) {
            for (int gx = 0; gx < 5; ++gx) {
                const LONG sx = 160 + gx * 240, sy = 180 + gy * 100;
                const float snx = (sx + 0.5f) / width_ * 2.0f - 1.0f;
                const float sny = 1.0f - (sy + 0.5f) / height_ * 2.0f;
                const XMVECTOR nW = XMVector3TransformCoord(XMVectorSet(snx, sny, 0.0f, 1.0f), toWorld);
                const XMVECTOR fW = XMVector3TransformCoord(XMVectorSet(snx, sny, 1.0f, 1.0f), toWorld);
                const XMVECTOR d2 = XMVector3Normalize(XMVectorSubtract(fW, nW));
                const float dz2 = XMVectorGetZ(d2);
                if (std::fabs(dz2) < 1e-6f) continue;
                const float t2 = -XMVectorGetZ(eyeV) / dz2;
                const float qu = XMVectorGetX(eyeV) + XMVectorGetX(d2) * t2;
                const float qv = XMVectorGetY(eyeV) + XMVectorGetY(d2) * t2;
                if (qu < -5.9f || qu > 5.9f || qv < -5.9f || qv > 5.9f) continue; // outside floor
                const XMFLOAT2 c2 = project(qu, qv, 0.0f, camera_.viewProj());
                const XMFLOAT2 p2 = project(qu, qv, 0.0f, camera_.prevViewProj());
                auto mv = readbackPixel(motion_.Get(), stateMotion_,
                                        D3D12_RESOURCE_STATE_COPY_SOURCE,
                                        static_cast<UINT>(sx), static_cast<UINT>(sy),
                                        DXGI_FORMAT_R16G16_FLOAT);
                const XMFLOAT2 s2 = project(qu, qv, 0.0f, staticViewProj);
                std::fprintf(stderr, "[field] (%u,%u) gpu=(%.3f,%.3f) cpu5=(%.3f,%.3f) cpuS=(%.3f,%.3f)\n",
                             sx, sy, mv[0], mv[1],
                             (p2.x - c2.x) * width_, (p2.y - c2.y) * height_,
                             (s2.x - c2.x) * width_, (s2.y - c2.y) * height_);
            }
        }

        auto m = readbackPixel(motion_.Get(), stateMotion_,
                               D3D12_RESOURCE_STATE_COPY_SOURCE,
                               static_cast<UINT>(px), static_cast<UINT>(py),
                               DXGI_FORMAT_R16G16_FLOAT);
        const float dx = std::fabs(m[0] - expectX);
        const float dy = std::fabs(m[1] - expectY);
        check((std::fabs(m[0]) + std::fabs(m[1])) > 1.0f && dx <= 0.25f && dy <= 0.25f,
                  "pan motion matches CPU and is nonzero",
              "gpu=(" + std::to_string(m[0]) + ", " + std::to_string(m[1]) + ") cpu=(" +
                  std::to_string(expectX) + ", " + std::to_string(expectY) + ") at (" +
                  std::to_string(px) + ", " + std::to_string(py) + ") floor=(" +
                  std::to_string(qx) + ", " + std::to_string(qy) + ")");
    }

    // 3. Depth: geometry depth < 1 (near), sky depth == 1 (far).
    {
        auto dFloor = readbackPixel(depth_.Get(), stateDepth_,
                                    D3D12_RESOURCE_STATE_COPY_SOURCE, 640, 600,
                                    DXGI_FORMAT_R32_FLOAT);
        auto dSky = readbackPixel(depth_.Get(), stateDepth_,
                                  D3D12_RESOURCE_STATE_COPY_SOURCE, 100, 100,
                                  DXGI_FORMAT_R32_FLOAT);
        const float nearD = dFloor[0], farD = dSky[0];
        check(nearD > 0.0f && nearD < 1.0f && farD == 1.0f, "depth near/far convention",
              "floor=" + std::to_string(nearD) + " sky=" + std::to_string(farD));
    }

    // 4. Alpha hole: the card center is discarded; the pixel must differ from
    //    the card's solid color (something behind is visible).
    {
        auto hole = readbackPixel(color_.Get(), stateColor_,
                                  D3D12_RESOURCE_STATE_COPY_SOURCE, 640, 430,
                                  DXGI_FORMAT_R16G16B16A16_FLOAT);
        auto solid = readbackPixel(color_.Get(), stateColor_,
                                   D3D12_RESOURCE_STATE_COPY_SOURCE, 640, 300,
                                   DXGI_FORMAT_R16G16B16A16_FLOAT);
        auto dist = [](const std::vector<float>& a, const std::vector<float>& b) {
            const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        };
        const float d = dist(hole, solid);
        check(d > 0.15f, "alpha card hole shows the scene behind",
              "color distance hole-vs-solid = " + std::to_string(d));
    }

    // 5. Projection port vs the upstream JS reference (node tools/projection-ref.js).
    {
        Proj proj;
        const bool ok = Proj::fromJsonString(
            "{\"squ\":[-48,12],\"sqv\":[32,24],\"origin\":[4784,-1190],\"hpx\":51.06858,"
            "\"roofPx\":96.0,\"bbPx\":57.24334,\"rowTilt\":0.0005,\"stack\":0.0001}", proj);
        check(ok, "proj parsed from scene constants", ok ? "" : "parse failed");
        if (ok) {
            auto close = [](double a, double b) { return std::fabs(a - b) <= 1e-9; };
            double px[2];
            proj.groundToPx(0, 0, 0, px);
            check(close(px[0], 4784.0) && close(px[1], -1190.0), "groundToPx 0,0,0",
                  std::to_string(px[0]) + "," + std::to_string(px[1]));
            proj.groundToPx(10, 5, 0, px);
            check(close(px[0], 4464.0) && close(px[1], -950.0), "groundToPx 10,5,0",
                  std::to_string(px[0]) + "," + std::to_string(px[1]));
            proj.groundToPx(37, 21, 0, px);
            check(close(px[0], 3680.0) && close(px[1], -242.0), "groundToPx 37,21,0",
                  std::to_string(px[0]) + "," + std::to_string(px[1]));
            check(hexFromPx(600, 400) == 6997, "hexFromPx 600,400",
                  std::to_string(hexFromPx(600, 400)));
            check(hexFromPx(1234, 567) == 10784, "hexFromPx 1234,567",
                  std::to_string(hexFromPx(1234, 567)));
            double hp[2];
            hexPx(25075, hp);
            check(close(hp[0], 4992.0) && close(hp[1], 764.0), "hexPx 25075",
                  std::to_string(hp[0]) + "," + std::to_string(hp[1]));
            check(close(rowPlane(proj, 10, 5), 4.9475), "rowPlane 10,5",
                  std::to_string(rowPlane(proj, 10, 5)));
            const SpriteCard c = card(proj, 875, 3586, 37, 17, -8);
            check(close(c.shift, 4.5) && close(c.lean, 0.169010216) && close(c.z1, 0.156652094) &&
                      close(c.up1, 0.244768897),
                  "card sprite0",
                  std::to_string(c.shift) + "," + std::to_string(c.lean));
            check(close(proj.roofZ, 1.879825129) && close(proj.groundPerPx, 0.037557826),
                  "roofZ/groundPerPx",
                  std::to_string(proj.roofZ) + " / " + std::to_string(proj.groundPerPx));
        }
    }

    return result;
}

} // namespace f2native
