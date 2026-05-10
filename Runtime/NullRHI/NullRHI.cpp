#include "NullRHI.h"
#include "RHI/RHICore.h"

IMPLEMENT_ENGINE_MODULE(FNullModuleRHI, NullRHI);

FRHIDevice* FNullModuleRHI::CreateDevice()
{
    return new FNullDeviceRHI();
}

FNullDeviceRHI::FNullDeviceRHI()
    : FRHIDevice()
    , CommandContext(new FNullRHICommandContext())
{
    // -------------------------------------------------------------------------------------------
    // Swap-Chain Defaults
    // -------------------------------------------------------------------------------------------
    
    RHI::DefaultSwapChainFormat = EFormat::B8G8R8A8_Unorm;

    // -------------------------------------------------------------------------------------------
    // Shader / Pipeline Features
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsGeometryShaders                       = true;
    RHI::bSupportRenderTargetArrayIndexFromVertexShader = true;

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsViewInstancing = false;
    RHI::MaxViewInstanceCount    = 1;

    // -------------------------------------------------------------------------------------------
    // Hardware Ray Tracing - advertise Tier 1.1 so the engine exercises RT paths.
    // NullRHI returns valid stub RT shaders / PSO / scene / geometry.
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsRayTracing         = true;
    RHI::RayTracingTier              = ERayTracingTier::Tier1_1;
    RHI::RayTracingMaxRecursionDepth = 31;

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsVRS             = false;
    RHI::ShadingRateTier          = EShadingRateTier::NotSupported;
    RHI::ShadingRateImageTileSize = 0;

    // -------------------------------------------------------------------------------------------
    // Draw Indirect
    // -------------------------------------------------------------------------------------------

    RHI::bSupportDrawIndirect      = true;
    RHI::bSupportMultiDrawIndirect = true;
    RHI::MaxDrawIndirectCount      = uint32(~0u);

    // -------------------------------------------------------------------------------------------
    // Texture / Image Limits
    // -------------------------------------------------------------------------------------------

    RHI::MaxTexture1DSize        = 16384;
    RHI::MaxTexture1DArrayLayers = 2048;
    RHI::MaxTexture2DSize        = 16384;
    RHI::MaxTexture2DArrayLayers = 2048;
    RHI::MaxTexture3DWidth       = 2048;
    RHI::MaxTexture3DHeight      = 2048;
    RHI::MaxTexture3DDepth       = 2048;
    RHI::MaxCubeTextureSize      = 16384;
    RHI::MaxCubeArrayCount       = 2048 / RHI_NUM_CUBE_FACES;

    // -------------------------------------------------------------------------------------------
    // Buffer / Memory Limits
    // -------------------------------------------------------------------------------------------

    RHI::MaxBufferSize              = uint64(~0ull);
    RHI::MaxConstantBufferSize      = 64 * 1024; // 64 KB
    RHI::MaxStorageBufferSize       = uint64(~0ull);
    RHI::StructuredBufferMinStride  = 0;
    RHI::StructuredBufferMaxStride  = uint32(~0u);
    RHI::RawBufferRequiredAlignment = 4;

    // -------------------------------------------------------------------------------------------
    // Dynamic State / Query Support
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsDynamicDepthBias           = true;
    RHI::bSupportsStreamOutput               = true;
    RHI::bSupportsTimestampQueries           = true;
    RHI::bSupportsPipelineStatisticsQueries  = true;
    RHI::bSupportsGPUTimestampBubblesRemoval = true;
}

FNullDeviceRHI::~FNullDeviceRHI()
{
    SAFE_DELETE(CommandContext);
}

ERHIType FNullDeviceRHI::GetRHIType() const
{
    return ERHIType::Null;
}
