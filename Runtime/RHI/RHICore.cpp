#include "RHICore.h"

// Hardware RayTracing support
ERayTracingTier FHardwareSupport::RayTracingTier = ERayTracingTier::NotSupported;
bool            FHardwareSupport::bRayTracing = false;
uint32          FHardwareSupport::MaxRecursionDepth = 0;

// Hardware Variable Rate Shading Support
bool             FHardwareSupport::bVariableShadingRate = false;
EShadingRateTier FHardwareSupport::ShadingRateTier = EShadingRateTier::NotSupported;
uint32           FHardwareSupport::ShadingRateImageTileSize = 0;
