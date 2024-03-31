#include "RHICore.h"

// Hardware RayTracing support
ERayTracingTier FHardwareSupport::RayTracingTier    = ERayTracingTier::NotSupported;
bool            FHardwareSupport::bRayTracing       = false;
uint32          FHardwareSupport::MaxRecursionDepth = 0;

// Hardware Variable Rate Shading Support
EShadingRateTier FHardwareSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
uint32           FHardwareSupport::ShadingRateImageTileSize = 0;
