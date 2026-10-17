#include "neural.h"

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs.h>
#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_helpers_d3d.h>

#include <cstdio>
#include <stdexcept>
#include <string>

namespace f2native {

namespace {

// This prototype's own project UUID (spec section 5) - not another app's ID.
constexpr char kProjectId[] = "d91e6822-7691-4ad8-a81b-597ed04e49ef";

std::string hex(NVSDK_NGX_Result r) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned>(r));
    return buf;
}

void throwIfNgxFailed(NVSDK_NGX_Result r, const char* what) {
    if (NVSDK_NGX_FAILED(r)) {
        throw std::runtime_error(std::string(what) + " failed: " + hex(r));
    }
}

} // namespace

void NeuralPass::initialize(ID3D12Device* device, const std::filesystem::path& runtimeDir) {
    device_ = device;

    NVSDK_NGX_FeatureCommonInfo info = {};
    // The NGX runtime dlls (nvngx_dlss.dll, nvngx_dlssnr.dll) sit next to the exe.
    const std::wstring path = runtimeDir.wstring();
    const wchar_t* paths[1] = {path.c_str()};
    info.PathListInfo.Path = const_cast<wchar_t**>(paths);
    info.PathListInfo.Length = 1;

    NVSDK_NGX_Result r = NVSDK_NGX_D3D12_Init_with_ProjectID(
        kProjectId, NVSDK_NGX_ENGINE_TYPE_CUSTOM, "1.0", path.c_str(), device, &info,
        NVSDK_NGX_Version_API);
    throwIfNgxFailed(r, "NVSDK_NGX_D3D12_Init_with_ProjectID");
    initialized_ = true;

    r = NVSDK_NGX_D3D12_AllocateParameters(&params_);
    throwIfNgxFailed(r, "NVSDK_NGX_D3D12_AllocateParameters");
}

void NeuralPass::create(ID3D12GraphicsCommandList* commands, uint32_t width, uint32_t height) {
    NVSDK_NGX_DLSS_Create_Params cp = {};
    cp.Feature.InWidth = width;
    cp.Feature.InHeight = height;
    cp.Feature.InTargetWidth = width;  // DLAA: render size == output size
    cp.Feature.InTargetHeight = height;
    cp.Feature.InPerfQualityValue = NVSDK_NGX_PerfQuality_Value_DLAA;
    // Spec section 5: linear float input described by IsHDR, own motion at
    // render resolution, game owns exposure. No jitter in the first version.
    cp.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
                              NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
                              NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
    cp.InEnableOutputSubrects = false;

    NVSDK_NGX_Result r = NGX_D3D12_CREATE_DLSS_EXT(commands, 1, 1, &feature_, params_, &cp);
    throwIfNgxFailed(r, "NGX_D3D12_CREATE_DLSS_EXT");
    width_ = width;
    height_ = height;
}

void NeuralPass::evaluate(ID3D12GraphicsCommandList* commands, const FrameInputs& frame) {
    if (!feature_) throw std::runtime_error("NeuralPass::evaluate before create");

    NVSDK_NGX_D3D12_DLSS_Eval_Params ep = {};
    ep.Feature.pInColor = frame.color;
    ep.Feature.pInOutput = frame.output;
    ep.pInDepth = frame.depth;
    ep.pInMotionVectors = frame.motion;
    ep.InJitterOffsetX = 0.0f;
    ep.InJitterOffsetY = 0.0f;
    ep.InRenderSubrectDimensions.Width = frame.width;
    ep.InRenderSubrectDimensions.Height = frame.height;
    ep.InReset = frame.resetHistory ? 1u : 0u;
    ep.InMVScaleX = 1.0f;
    ep.InMVScaleY = 1.0f;
    ep.InPreExposure = 1.0f;
    ep.InExposureScale = 1.0f;

    NVSDK_NGX_Result r = NGX_D3D12_EVALUATE_DLSS_EXT(commands, feature_, params_, &ep);
    throwIfNgxFailed(r, "NGX_D3D12_EVALUATE_DLSS_EXT");
    ++evaluations_;
}

void NeuralPass::releaseFeature() {
    if (feature_ != nullptr) {
        NVSDK_NGX_D3D12_ReleaseFeature(feature_);
        feature_ = nullptr;
    }
}

void NeuralPass::shutdown() {
    if (feature_ != nullptr) {
        NVSDK_NGX_D3D12_ReleaseFeature(feature_);
        feature_ = nullptr;
    }
    if (params_ != nullptr) {
        NVSDK_NGX_D3D12_DestroyParameters(params_);
        params_ = nullptr;
    }
    initialized_ = false;
}

} // namespace f2native
