#pragma once
#include "Core/Core.h"

#define RHI_REMAINING_MIP_LEVELS uint32(~0)
#define RHI_REMAINING_ARRAY_SLICES uint32(~0)
#define RHI_NUM_CUBE_FACES (6)

enum class ERayTracingTier : uint8
{
    NotSupported = 0,
    Tier1        = 1,
    Tier1_1      = 2,
};

inline const CHAR* ToString(ERayTracingTier Tier)
{
    switch (Tier)
    {
        case ERayTracingTier::NotSupported: return "NotSupported";
        case ERayTracingTier::Tier1:        return "Tier1";
        case ERayTracingTier::Tier1_1:      return "Tier1_1";
        default:                            return "Unknown";
    }
}

enum class EShadingRateTier : uint8
{
    NotSupported = 0,
    Tier1        = 1,
    Tier2        = 2,
};

inline const CHAR* ToString(EShadingRateTier Tier)
{
    switch (Tier)
    {
        case EShadingRateTier::NotSupported: return "NotSupported";
        case EShadingRateTier::Tier1:        return "Tier1";
        case EShadingRateTier::Tier2:        return "Tier2";
        default:                             return "Unknown";
    }
}

struct FHardwareLimits
{
    inline static constexpr uint32 MAX_RENDER_TARGETS = 8;
    
    inline static constexpr uint32 MAX_LOCAL_SHADER_BINDINGS = 4;
    
    inline static constexpr uint32 MAX_SHADER_CONSTANTS = 32;

    inline static constexpr uint32 MAX_VERTEX_BUFFERS = 32;
};

struct FHardwareSupport
{
    // Hardware RayTracing
    extern RHI_API static bool            bRayTracing;
    extern RHI_API static ERayTracingTier RayTracingTier;
    extern RHI_API static uint32          MaxRecursionDepth;

    // Hardware Variable Rate Shading
    extern RHI_API static bool             bVariableShadingRate;
    extern RHI_API static EShadingRateTier ShadingRateTier;
    extern RHI_API static uint32           ShadingRateImageTileSize;
};