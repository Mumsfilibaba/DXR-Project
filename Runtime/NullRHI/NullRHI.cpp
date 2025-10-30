#include "NullRHI.h"
#include "RHI/RHICore.h"

IMPLEMENT_ENGINE_MODULE(FNullRHIModule, NullRHI);

FRHI* FNullRHIModule::CreateRHI()
{
    return new FNullRHI();
}

FNullRHI::FNullRHI()
    : FRHI(ERHIType::Null)
    , CommandContext(new FNullRHICommandContext())
{
    // -------------------------------------------------------------------------------------------
    // Shader / Pipeline Features
    // -------------------------------------------------------------------------------------------
    RHIDeviceFeatureSupport::bSupportsGeometryShaders                       = true;
    RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = true;

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------
    RHIDeviceFeatureSupport::bSupportsViewInstancing = false;
    RHIDeviceFeatureSupport::MaxViewInstanceCount    = 1;

    // -------------------------------------------------------------------------------------------
    // Hardware Ray Tracing
    // -------------------------------------------------------------------------------------------
    RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
    RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
    RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------
    RHIDeviceFeatureSupport::bSupportsVRS             = false;
    RHIDeviceFeatureSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
    RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;

    // -------------------------------------------------------------------------------------------
    // Draw Indirect
    // -------------------------------------------------------------------------------------------
    RHIDeviceFeatureSupport::bSupportDrawIndirect      = true;
    RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = false; 
    RHIDeviceFeatureSupport::MaxDrawIndirectCount      = 1;

    // -------------------------------------------------------------------------------------------
    // Texture / Image Limits
    // -------------------------------------------------------------------------------------------
    RHIDeviceFeatureSupport::MaxTexture1DSize        = 8192;
    RHIDeviceFeatureSupport::MaxTexture1DArrayLayers = 256;
    RHIDeviceFeatureSupport::MaxTexture2DSize        = 8192;
    RHIDeviceFeatureSupport::MaxTexture2DArrayLayers = 256;
    RHIDeviceFeatureSupport::MaxTexture3DWidth       = 2048;
    RHIDeviceFeatureSupport::MaxTexture3DHeight      = 2048;
    RHIDeviceFeatureSupport::MaxTexture3DDepth       = 2048;
    RHIDeviceFeatureSupport::MaxCubeTextureSize      = 8192;
    RHIDeviceFeatureSupport::MaxCubeArrayCount       = 256 / RHI_NUM_CUBE_FACES; // 42 cubes

    // -------------------------------------------------------------------------------------------
    // Buffer / Memory Limits
    // -------------------------------------------------------------------------------------------
    RHIDeviceFeatureSupport::MaxBufferSize              = uint64(~0);
    RHIDeviceFeatureSupport::MaxConstantBufferSize      = 64 * 1024; // 64 KB
    RHIDeviceFeatureSupport::MaxStorageBufferSize       = uint64(~0);
    RHIDeviceFeatureSupport::StructuredBufferMinStride  = 4;
    RHIDeviceFeatureSupport::StructuredBufferMaxStride  = 2048;
    RHIDeviceFeatureSupport::RawBufferRequiredAlignment = 4;
}

FNullRHI::~FNullRHI()
{
    SAFE_DELETE(CommandContext);
}
