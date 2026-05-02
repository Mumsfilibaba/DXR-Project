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
    // Hardware Ray Tracing
    // -------------------------------------------------------------------------------------------
    RHI::bSupportsRayTracing         = false;
    RHI::RayTracingTier              = ERayTracingTier::NotSupported;
    RHI::RayTracingMaxRecursionDepth = 0;

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
    RHI::bSupportMultiDrawIndirect = false; 
    RHI::MaxDrawIndirectCount      = 1;

    // -------------------------------------------------------------------------------------------
    // Texture / Image Limits
    // -------------------------------------------------------------------------------------------
    RHI::MaxTexture1DSize        = 8192;
    RHI::MaxTexture1DArrayLayers = 256;
    RHI::MaxTexture2DSize        = 8192;
    RHI::MaxTexture2DArrayLayers = 256;
    RHI::MaxTexture3DWidth       = 2048;
    RHI::MaxTexture3DHeight      = 2048;
    RHI::MaxTexture3DDepth       = 2048;
    RHI::MaxCubeTextureSize      = 8192;
    RHI::MaxCubeArrayCount       = 256 / RHI_NUM_CUBE_FACES; // 42 cubes

    // -------------------------------------------------------------------------------------------
    // Buffer / Memory Limits
    // -------------------------------------------------------------------------------------------
    RHI::MaxBufferSize              = uint64(~0);
    RHI::MaxConstantBufferSize      = 64 * 1024; // 64 KB
    RHI::MaxStorageBufferSize       = uint64(~0);
    RHI::StructuredBufferMinStride  = 4;
    RHI::StructuredBufferMaxStride  = 2048;
    RHI::RawBufferRequiredAlignment = 4;
}

FNullRHI::~FNullRHI()
{
    SAFE_DELETE(CommandContext);
}
