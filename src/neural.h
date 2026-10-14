#pragma once

// NeuralPass: in-process NGX DLAA carrier for the experimental DLSS5 neural
// pass (RenoDX add-on intercepts the evaluate and runs feature 18).
// No IPC, no second process. Interface per plan section 1.

#include <d3d12.h>
#include <filesystem>
#include <cstdint>

struct NVSDK_NGX_Handle;
struct NVSDK_NGX_Parameter;

namespace f2native {

struct FrameInputs {
    ID3D12Resource* color;        // R16G16B16A16_FLOAT, linear, render target input
    ID3D12Resource* output;       // R16G16B16A16_FLOAT, UAV result
    ID3D12Resource* depth;        // R32_FLOAT view of the D32_TYPELESS depth
    ID3D12Resource* motion;       // R16G16_FLOAT, pixels, prev - current
    uint32_t width;
    uint32_t height;
    bool resetHistory;
};

class NeuralPass {
public:
    void initialize(ID3D12Device* device, const std::filesystem::path& runtimeDir);
    void create(ID3D12GraphicsCommandList* commands, uint32_t width, uint32_t height);
    void evaluate(ID3D12GraphicsCommandList* commands, const FrameInputs& frame);
    void shutdown(); // after GPU fence, before device destruction

    bool ready() const { return feature_ != nullptr; }
    uint64_t evaluations() const { return evaluations_; }

private:
    ID3D12Device* device_ = nullptr;
    NVSDK_NGX_Parameter* params_ = nullptr;
    NVSDK_NGX_Handle* feature_ = nullptr;
    bool initialized_ = false;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint64_t evaluations_ = 0;
};

} // namespace f2native
