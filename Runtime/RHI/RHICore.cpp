#include "RHI/RHICore.h"

// Geometry Shading Support
RHI_API bool RHIDeviceInfo::SupportsGeometryShaders = false;

// RenderTargetArrayIndex from vertex-shader Support
RHI_API bool RHIDeviceInfo::SupportRenderTargetArrayIndexFromVertexShader = false;

// View-Instancing
RHI_API bool RHIDeviceInfo::SupportsViewInstancing = false;
RHI_API uint32 RHIDeviceInfo::MaxViewInstanceCount = 0;

// Hardware RayTracing
RHI_API bool RHIDeviceInfo::SupportsRayTracing = false;
RHI_API ERayTracingTier RHIDeviceInfo::RayTracingTier = ERayTracingTier::NotSupported;
RHI_API uint32 RHIDeviceInfo::RayTracingMaxRecursionDepth = 0;

// Hardware Variable Rate Shading
RHI_API bool RHIDeviceInfo::SupportsVRS = false;
RHI_API EShadingRateTier RHIDeviceInfo::ShadingRateTier = EShadingRateTier::NotSupported;
RHI_API uint32 RHIDeviceInfo::ShadingRateImageTileSize = 0;

// Draw-Indirect
RHI_API bool RHIDeviceInfo::SupportDrawIndirect = true;
RHI_API bool RHIDeviceInfo::SupportMultiDrawIndirect = false;
RHI_API uint32 RHIDeviceInfo::MaxDrawIndirectCount = 0;

// Statistics
RHI_API FAtomicUInt64 RHIStatistics::NumDrawCalls = 0;
RHI_API FAtomicUInt64 RHIStatistics::NumDispatchCalls = 0;
RHI_API FAtomicUInt64 RHIStatistics::NumCommands = 0;