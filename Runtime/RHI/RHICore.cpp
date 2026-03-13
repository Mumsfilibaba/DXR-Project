#include "RHI/RHICore.h"

// -------------------------------------------------------------------------------------------
// Feature Support
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// Shader / Pipeline
// -------------------------------------------------------------------------------------------
RHI_API bool   RHIDeviceFeatureSupport::bSupportsGeometryShaders                       = true;
RHI_API bool   RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = true;

// -------------------------------------------------------------------------------------------
// View Instancing
// -------------------------------------------------------------------------------------------
RHI_API bool   RHIDeviceFeatureSupport::bSupportsViewInstancing = false;
RHI_API uint32 RHIDeviceFeatureSupport::MaxViewInstanceCount    = 1;

// -------------------------------------------------------------------------------------------
// Hardware Ray Tracing
// -------------------------------------------------------------------------------------------
RHI_API bool             RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
RHI_API ERayTracingTier  RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
RHI_API uint32           RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading
// -------------------------------------------------------------------------------------------
RHI_API bool             RHIDeviceFeatureSupport::bSupportsVRS             = false;
RHI_API EShadingRateTier RHIDeviceFeatureSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
RHI_API uint32           RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;

// -------------------------------------------------------------------------------------------
// Draw Indirect
// -------------------------------------------------------------------------------------------
RHI_API bool   RHIDeviceFeatureSupport::bSupportDrawIndirect      = true;
RHI_API bool   RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = false;
RHI_API uint32 RHIDeviceFeatureSupport::MaxDrawIndirectCount      = 1;

// -------------------------------------------------------------------------------------------
// Texture / Image Limits
// -------------------------------------------------------------------------------------------
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture1DSize        = 8192;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture1DArrayLayers = 256;

RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture2DSize        = 8192;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture2DArrayLayers = 256;

RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture3DWidth       = 2048;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture3DHeight      = 2048;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture3DDepth       = 2048;

RHI_API uint32 RHIDeviceFeatureSupport::MaxCubeTextureSize      = 8192;
RHI_API uint32 RHIDeviceFeatureSupport::MaxCubeArrayCount       = 256 / RHI_NUM_CUBE_FACES; // 42

// -------------------------------------------------------------------------------------------
// Buffer / Memory Limits
// -------------------------------------------------------------------------------------------
RHI_API uint64 RHIDeviceFeatureSupport::MaxBufferSize              = uint64(~0);
RHI_API uint32 RHIDeviceFeatureSupport::MaxConstantBufferSize      = 64 * 1024; // 64 KB
RHI_API uint64 RHIDeviceFeatureSupport::MaxStorageBufferSize       = uint64(~0);
RHI_API uint32 RHIDeviceFeatureSupport::StructuredBufferMinStride  = 4;
RHI_API uint32 RHIDeviceFeatureSupport::StructuredBufferMaxStride  = 2048;
RHI_API uint32 RHIDeviceFeatureSupport::RawBufferRequiredAlignment = 4;

RHI_API bool RHIDeviceFeatureSupport::bSupportsDynamicDepthBias = false;
RHI_API bool RHIDeviceFeatureSupport::bSupportsStreamOutput     = false;

// -------------------------------------------------------------------------------------------
// Statistics
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// Command Submission Metrics
// -------------------------------------------------------------------------------------------
RHI_API FAtomicUInt64 RHIStatistics::NumDrawCalls     = { 0 };
RHI_API FAtomicUInt64 RHIStatistics::NumDispatchCalls = { 0 };
RHI_API FAtomicUInt64 RHIStatistics::NumCommands      = { 0 };
