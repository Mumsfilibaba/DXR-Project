#include "RHI/RHICore.h"

// Geometry Shading Support
RHI_API bool FRHIDeviceInfo::SupportsGeometryShaders = false;

// RenderTargetArrayIndex from vertex-shader Support
RHI_API bool FRHIDeviceInfo::SupportRenderTargetArrayIndexFromVertexShader = false;

// View-Instancing
RHI_API bool   FRHIDeviceInfo::SupportsViewInstancing = false;
RHI_API uint32 FRHIDeviceInfo::MaxViewInstanceCount   = 0;

// Hardware RayTracing
RHI_API bool            FRHIDeviceInfo::SupportsRayTracing          = false;
RHI_API ERayTracingTier FRHIDeviceInfo::RayTracingTier              = ERayTracingTier::NotSupported;
RHI_API uint32          FRHIDeviceInfo::RayTracingMaxRecursionDepth = 0;

// Hardware Variable Rate Shading
RHI_API bool             FRHIDeviceInfo::SupportsVRS              = false;
RHI_API EShadingRateTier FRHIDeviceInfo::ShadingRateTier          = EShadingRateTier::NotSupported;
RHI_API uint32           FRHIDeviceInfo::ShadingRateImageTileSize = 0;

// Draw-Indirect
RHI_API bool   FRHIDeviceInfo::SupportDrawIndirect      = true;
RHI_API bool   FRHIDeviceInfo::SupportMultiDrawIndirect = false;
RHI_API uint32 FRHIDeviceInfo::MaxDrawIndirectCount     = 0;
