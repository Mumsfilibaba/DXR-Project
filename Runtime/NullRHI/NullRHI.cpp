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
    // Shader / pipeline features
    RHIDeviceFeatureSupport::bSupportsGeometryShaders                       = true;   // Most GPUs support this
    RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = true;   // Common D3D12-era feature

    // View Instancing
    RHIDeviceFeatureSupport::bSupportsViewInstancing = false;  // Disabled for NullRHI
    RHIDeviceFeatureSupport::MaxViewInstanceCount    = 1;

    // Hardware Ray Tracing
    RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
    RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
    RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;

    // Variable Rate Shading
    RHIDeviceFeatureSupport::bSupportsVRS             = false;
    RHIDeviceFeatureSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
    RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;

    // Draw-Indirect
    RHIDeviceFeatureSupport::bSupportDrawIndirect      = true;   // Basic indirect draws are safe to assume
    RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = false;  // Keep conservative
    RHIDeviceFeatureSupport::MaxDrawIndirectCount      = 1;      // Only 1 indirect call supported in NullRHI

    // Texture limits
    // Conservative limits roughly matching the D3D11 minimum spec
    RHIDeviceFeatureSupport::MaxTexture1DSize        = 8192;
    RHIDeviceFeatureSupport::MaxTexture1DArrayLayers = 256;

    RHIDeviceFeatureSupport::MaxTexture2DSize        = 8192;
    RHIDeviceFeatureSupport::MaxTexture2DArrayLayers = 256;

    RHIDeviceFeatureSupport::MaxTexture3DWidth  = 2048;
    RHIDeviceFeatureSupport::MaxTexture3DHeight = 2048;
    RHIDeviceFeatureSupport::MaxTexture3DDepth  = 2048;

    RHIDeviceFeatureSupport::MaxCubeTextureSize = 8192;
    RHIDeviceFeatureSupport::MaxCubeArrayCount  = 256 / 6;  // 42 cubes
}

FNullRHI::~FNullRHI()
{
    SAFE_DELETE(CommandContext);
}
