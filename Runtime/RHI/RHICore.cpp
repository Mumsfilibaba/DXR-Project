#include "RHICore.h"

// Geometry Shading Support
RHI_API bool GRHISupportsGeometryShaders = false;

// View-Instancing
RHI_API bool   GRHISupportsViewInstancing = false;
RHI_API uint32 GRHIMaxViewInstanceCount   = 0;

// Hardware RayTracing
RHI_API bool            GRHISupportsRayTracing          = false;
RHI_API ERayTracingTier GRHIRayTracingTier              = ERayTracingTier::NotSupported;
RHI_API uint32          GRHIRayTracingMaxRecursionDepth = 0;

// Hardware Variable Rate Shading
RHI_API bool             GRHISupportsVRS              = false;
RHI_API EShadingRateTier GRHIShadingRateTier          = EShadingRateTier::NotSupported;
RHI_API uint32           GRHIShadingRateImageTileSize = 0;

// Draw-Indirect
RHI_API bool   GRHISupportDrawIndirect      = true;
RHI_API bool   GRHISupportMultiDrawIndirect = false;
RHI_API uint32 GRHIMaxDrawIndirectCount     = 0;
