#include "RHI/RHI.h"
#include "D3D11RHI/D3D11RHI.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11Capabilities.h"
#include "D3D11RHI/D3D11Loader.h"

D3D11RHI_API D3D_FEATURE_LEVEL GD3D11FeatureLevel                         = D3D_FEATURE_LEVEL_11_0;
D3D11RHI_API bool              GD3D11SupportsFences                       = false;
D3D11RHI_API bool              GD3D11SupportsUAVsInAllStages              = false;
D3D11RHI_API bool              GD3D11SupportsPartialConstantBufferUpdates = false;
D3D11RHI_API uint32            GD3D11MaxUnorderedAccessViews              = D3D11_MAX_UNORDERED_ACCESS_VIEWS_FL11_0;
D3D11RHI_API bool              GD3D11SupportsComposition                  = false;

static const CHAR* ToString(D3D_FEATURE_LEVEL FeatureLevel)
{
    switch (FeatureLevel)
    {
        case D3D_FEATURE_LEVEL_11_0: return "11_0";
        case D3D_FEATURE_LEVEL_11_1: return "11_1";
        default:                     return "Unknown";
    }
}

void DumpD3D11Capabilities()
{
    const auto YesNo = [](bool bBoolean) -> const CHAR*
    {
        return bBoolean ? "Yes" : "No";
    };

    D3D11_INFO("---------------------------------- D3D11 Feature Support ----------------------------------");
    D3D11_INFO("Feature Level                  : %s", ToString(GD3D11FeatureLevel));
    D3D11_INFO("Fences (ID3D11Fence)           : %s", YesNo(GD3D11SupportsFences));
    D3D11_INFO("UAVs in all stages             : %s", YesNo(GD3D11SupportsUAVsInAllStages));
    D3D11_INFO("Partial constant buffer update : %s", YesNo(GD3D11SupportsPartialConstantBufferUpdates));
    D3D11_INFO("Max UAV slots                  : %u", GD3D11MaxUnorderedAccessViews);
    D3D11_INFO("DirectComposition              : %s", YesNo(GD3D11SupportsComposition));
    D3D11_INFO("--------------------------------------------------------------------------------------------");
}

bool FD3D11DeviceRHI::InitializeDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // D3D11 Specific Features
    // -------------------------------------------------------------------------------------------

    GD3D11FeatureLevel                         = Device->GetFeatureLevel();
    GD3D11SupportsFences                       = Device->GetD3D11Device5() != nullptr && Device->GetD3D11Context4() != nullptr;
    GD3D11SupportsUAVsInAllStages              = GD3D11FeatureLevel >= D3D_FEATURE_LEVEL_11_1;
    GD3D11SupportsPartialConstantBufferUpdates = Device->GetD3D11Context1() != nullptr;
    GD3D11MaxUnorderedAccessViews              = GD3D11SupportsUAVsInAllStages ? D3D11_MAX_UNORDERED_ACCESS_VIEWS : D3D11_MAX_UNORDERED_ACCESS_VIEWS_FL11_0;
#if D3D11_ENABLE_COMPOSITION
    GD3D11SupportsComposition                  = D3D11::DCompositionCreateDevice != nullptr;
#endif

    // -------------------------------------------------------------------------------------------
    // Swap-Chain Defaults
    // -------------------------------------------------------------------------------------------

    RHI::DefaultSwapChainFormat        = EFormat::R8G8B8A8_Unorm;
    RHI::bSupportsTransparentSwapChain = GD3D11SupportsComposition;

    // -------------------------------------------------------------------------------------------
    // Shader / Pipeline Features
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsGeometryShaders                       = true;
    RHI::bSupportsTessellation                          = true;
    RHI::MaxPatchControlPoints                          = RHI_MAX_PATCH_CONTROL_POINTS;
    RHI::bSupportRenderTargetArrayIndexFromVertexShader = false;
    RHI::MaxShaderModel                                 = EShaderModel::SM_5_0;
    RHI::bSupportsBindless                              = false;

    // -------------------------------------------------------------------------------------------
    // Features D3D11 does not have
    // -------------------------------------------------------------------------------------------

    RHI::MaxViewInstanceCount                 = 1;
    RHI::bSupportsViewInstancing              = false;
    RHI::RayTracingTier                       = ERayTracingTier::NotSupported;
    RHI::RayTracingMaxRecursionDepth          = 0;
    RHI::bSupportsRayTracing                  = false;
    RHI::bSupportsInlineRayTracing            = false;
    RHI::bSupportsRayTracingPipelineAdditions = false;
    RHI::bSupportsDispatchRaysIndirect        = false;
    RHI::ShadingRateTier                      = EShadingRateTier::NotSupported;
    RHI::ShadingRateImageTileSize             = 0;
    RHI::bSupportsVRS                         = false;
    RHI::SamplerFeedbackTier                  = ESamplerFeedbackTier::NotSupported;
    RHI::bSupportsSamplerFeedback             = false;
    RHI::SamplePositionsTier                  = ESamplePositionsTier::NotSupported;
    RHI::bSupportsProgrammableSamplePositions = false;
    RHI::bSupportsDepthBoundsTest             = false;

    // -------------------------------------------------------------------------------------------
    // Draw Indirect
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsDrawIndirect               = true;
    RHI::bSupportsDrawIndirectCount          = false;
    RHI::bSupportsDispatchIndirect           = true;
    RHI::bSupportsDispatchMeshIndirect       = false;
    RHI::bSupportsDispatchMeshIndirectCount  = false;
    RHI::MaxDrawIndirectCommandCount         = uint32(~0u);
    RHI::MaxDispatchMeshIndirectCommandCount = 0;

    // -------------------------------------------------------------------------------------------
    // Texture / Image Limits (Feature Level 11_0)
    // -------------------------------------------------------------------------------------------

    RHI::MaxTexture1DSize        = D3D11_MAX_TEXTURE1D_SIZE;
    RHI::MaxTexture1DArrayLayers = D3D11_MAX_TEXTURE1D_ARRAY_SLICES;
    RHI::MaxTexture2DSize        = D3D11_MAX_TEXTURE2D_SIZE;
    RHI::MaxTexture2DArrayLayers = D3D11_MAX_TEXTURE2D_ARRAY_SLICES;
    RHI::MaxTexture3DWidth       = D3D11_MAX_TEXTURE3D_SIZE;
    RHI::MaxTexture3DHeight      = D3D11_MAX_TEXTURE3D_SIZE;
    RHI::MaxTexture3DDepth       = D3D11_MAX_TEXTURE3D_SIZE;
    RHI::MaxCubeTextureSize      = D3D11_MAX_TEXTURECUBE_SIZE;
    RHI::MaxCubeArrayCount       = D3D11_MAX_TEXTURE2D_ARRAY_SLICES / RHI_NUM_CUBE_FACES;

    // -------------------------------------------------------------------------------------------
    // Buffer / Memory Limits
    // -------------------------------------------------------------------------------------------

    RHI::MaxBufferSize              = uint64(D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM) * 1024ull * 1024ull;
    RHI::MaxConstantBufferSize      = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * D3D11_CONSTANT_BUFFER_ELEMENT_SIZE;
    RHI::MaxStorageBufferSize       = uint64(D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM) * 1024ull * 1024ull;
    RHI::StructuredBufferMinStride  = 4;
    RHI::StructuredBufferMaxStride  = 2048;
    RHI::RawBufferRequiredAlignment = 4;

    // -------------------------------------------------------------------------------------------
    // Dynamic State / Query Support
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsDynamicDepthBias           = true;
    RHI::bSupportsStreamOutput               = true;
    RHI::bSupportsTimestampQueries           = true;
    RHI::bSupportsPipelineStatisticsQueries  = true;
    RHI::bSupportsGPUTimestampBubblesRemoval = false;

    DumpD3D11Capabilities();
    return true;
}
