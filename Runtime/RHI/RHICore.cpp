#include "RHI/RHICore.h"
#include "RHI/RHITypes.h"

// -------------------------------------------------------------------------------------------
// Feature Support
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// Shader / Pipeline
// -------------------------------------------------------------------------------------------

RHI_API bool RHI::bSupportsGeometryShaders                       = true;
RHI_API bool RHI::bSupportRenderTargetArrayIndexFromVertexShader = true;

// -------------------------------------------------------------------------------------------
// View Instancing
// -------------------------------------------------------------------------------------------

RHI_API bool   RHI::bSupportsViewInstancing = false;
RHI_API uint32 RHI::MaxViewInstanceCount    = 1;

// -------------------------------------------------------------------------------------------
// Hardware Ray Tracing
// -------------------------------------------------------------------------------------------

RHI_API bool            RHI::bSupportsRayTracing         = false;
RHI_API ERayTracingTier RHI::RayTracingTier              = ERayTracingTier::NotSupported;
RHI_API uint32          RHI::RayTracingMaxRecursionDepth = 0;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading
// -------------------------------------------------------------------------------------------

RHI_API bool             RHI::bSupportsVRS             = false;
RHI_API EShadingRateTier RHI::ShadingRateTier          = EShadingRateTier::NotSupported;
RHI_API uint32           RHI::ShadingRateImageTileSize = 0;

// -------------------------------------------------------------------------------------------
// Draw Indirect
// -------------------------------------------------------------------------------------------

RHI_API bool   RHI::bSupportDrawIndirect      = true;
RHI_API bool   RHI::bSupportMultiDrawIndirect = false;
RHI_API uint32 RHI::MaxDrawIndirectCount      = 1;

// -------------------------------------------------------------------------------------------
// Texture / Image Limits
// -------------------------------------------------------------------------------------------

RHI_API uint32 RHI::MaxTexture1DSize        = 8192;
RHI_API uint32 RHI::MaxTexture1DArrayLayers = 256;

RHI_API uint32 RHI::MaxTexture2DSize        = 8192;
RHI_API uint32 RHI::MaxTexture2DArrayLayers = 256;

RHI_API uint32 RHI::MaxTexture3DWidth       = 2048;
RHI_API uint32 RHI::MaxTexture3DHeight      = 2048;
RHI_API uint32 RHI::MaxTexture3DDepth       = 2048;

RHI_API uint32 RHI::MaxCubeTextureSize      = 8192;
RHI_API uint32 RHI::MaxCubeArrayCount       = 256 / RHI_NUM_CUBE_FACES; // 42

// -------------------------------------------------------------------------------------------
// Buffer / Memory Limits
// -------------------------------------------------------------------------------------------

RHI_API uint64 RHI::MaxBufferSize              = uint64(~0);
RHI_API uint32 RHI::MaxConstantBufferSize      = 64 * 1024; // 64 KB
RHI_API uint64 RHI::MaxStorageBufferSize       = uint64(~0);
RHI_API uint32 RHI::StructuredBufferMinStride  = 4;
RHI_API uint32 RHI::StructuredBufferMaxStride  = 2048;
RHI_API uint32 RHI::RawBufferRequiredAlignment = 4;

RHI_API bool RHI::bSupportsDynamicDepthBias = false;
RHI_API bool RHI::bSupportsStreamOutput     = false;

// -------------------------------------------------------------------------------------------
// Query Support
// -------------------------------------------------------------------------------------------

RHI_API bool RHI::bSupportsTimestampQueries           = false;
RHI_API bool RHI::bSupportsPipelineStatisticsQueries  = false;
RHI_API bool RHI::bSupportsGPUTimestampBubblesRemoval = false;

// -------------------------------------------------------------------------------------------
// Swap-Chain Defaults
// -------------------------------------------------------------------------------------------

RHI_API EFormat RHI::DefaultSwapChainFormat = EFormat::Unknown;
