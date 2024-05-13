#include "RHICore.h"

// Hardware RayTracing
RHI_API bool            GRHISupportsRayTracing          = false;
RHI_API ERayTracingTier GRHIRayTracingTier              = ERayTracingTier::NotSupported;
RHI_API uint32          GRHIRayTracingMaxRecursionDepth = 0;

// Hardware Variable Rate Shading
RHI_API bool             GRHISupportsVRS              = false;
RHI_API EShadingRateTier GRHIShadingRateTier          = EShadingRateTier::NotSupported;
RHI_API uint32           GRHIShadingRateImageTileSize = 0;

// Geometry Shading Support
RHI_API bool GRHISupportsGeometryShaders = false;
