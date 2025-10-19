#include "RHI/RHICore.h"

// ==== Feature Support (conservative, engine-safe defaults) ====

// Shader / Pipeline
RHI_API bool   RHIDeviceFeatureSupport::bSupportsGeometryShaders                       = true;
RHI_API bool   RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = true;

// View Instancing (disabled by default)
RHI_API bool   RHIDeviceFeatureSupport::bSupportsViewInstancing = false;
RHI_API uint32 RHIDeviceFeatureSupport::MaxViewInstanceCount    = 1;

// Hardware Ray Tracing (disabled by default)
RHI_API bool             RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
RHI_API ERayTracingTier  RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
RHI_API uint32           RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;

// Variable Rate Shading (disabled by default)
RHI_API bool             RHIDeviceFeatureSupport::bSupportsVRS             = false;
RHI_API EShadingRateTier RHIDeviceFeatureSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
RHI_API uint32           RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;

// Draw-Indirect (basic only by default)
RHI_API bool   RHIDeviceFeatureSupport::bSupportDrawIndirect      = true;
RHI_API bool   RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = false;
RHI_API uint32 RHIDeviceFeatureSupport::MaxDrawIndirectCount      = 1;

// Texture / Image limits (D3D11-class safe)
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture1DSize        = 8192;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture1DArrayLayers = 256;

RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture2DSize        = 8192;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture2DArrayLayers = 256;

RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture3DWidth       = 2048;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture3DHeight      = 2048;
RHI_API uint32 RHIDeviceFeatureSupport::MaxTexture3DDepth       = 2048;

RHI_API uint32 RHIDeviceFeatureSupport::MaxCubeTextureSize      = 8192;
RHI_API uint32 RHIDeviceFeatureSupport::MaxCubeArrayCount       = 256 / RHI_NUM_CUBE_FACES; // 42

// ==== Statistics (atomic) ====
RHI_API FAtomicUInt64 RHIStatistics::NumDrawCalls     = { 0 };
RHI_API FAtomicUInt64 RHIStatistics::NumDispatchCalls = { 0 };
RHI_API FAtomicUInt64 RHIStatistics::NumCommands      = { 0 };
