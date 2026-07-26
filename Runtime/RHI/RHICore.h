#pragma once
#include "Core/Core.h"

enum class EFormat : uint8;

// -------------------------------------------------------------------------------------------
// Resource Range Macros
// -------------------------------------------------------------------------------------------

/** Use this value to specify "all remaining" mip levels in resource views */
#define RHI_REMAINING_MIP_LEVELS (uint32(~0))

/** Use this value to specify "all remaining" array slices in resource views */
#define RHI_REMAINING_ARRAY_SLICES (uint32(~0))

/** Alias for RHI_REMAINING_MIP_LEVELS (full mip chain) */
#define RHI_ALL_MIP_LEVELS (uint32(~0))

/** Alias for RHI_REMAINING_ARRAY_SLICES (all array layers) */
#define RHI_ALL_ARRAY_SLICES (uint32(~0))

/** Number of faces in a cube map texture */
#define RHI_NUM_CUBE_FACES (6)

// -------------------------------------------------------------------------------------------
// Fixed Hardware / API Binding Limits
// -------------------------------------------------------------------------------------------

/** Maximum number of simultaneous render targets supported */
#define RHI_MAX_RENDER_TARGETS (8)

/** Maximum number of local shader bindings (e.g., root constants or local root parameters) */
#define RHI_MAX_LOCAL_SHADER_BINDINGS (4)

/** Maximum number of shader constants that can be bound directly */
#define RHI_MAX_SHADER_CONSTANTS (32)

/** Maximum number of vertex buffers that can be bound at once */
#define RHI_MAX_VERTEX_BUFFERS (32)

enum class ERayTracingTier : uint8
{
    NotSupported = 0,

    Tier1   = 1,
    Tier1_1 = 2,
    Tier1_2 = 3,
    Tier2_0 = 4,
};

NODISCARD constexpr const CHAR* ToString(ERayTracingTier RayTracingTier)
{
    switch (RayTracingTier)
    {
        case ERayTracingTier::NotSupported: return "NotSupported";
        case ERayTracingTier::Tier1:        return "Tier1";
        case ERayTracingTier::Tier1_1:      return "Tier1_1";
        case ERayTracingTier::Tier1_2:      return "Tier1_2";
        case ERayTracingTier::Tier2_0:      return "Tier2_0";
        
        default: return "Unknown";
    }
}

enum class EShadingRateTier : uint8
{
    NotSupported = 0,

    Tier1 = 1,
    Tier2 = 2,
};

NODISCARD constexpr const CHAR* ToString(EShadingRateTier ShadingRateTier)
{
    switch (ShadingRateTier)
    {
        case EShadingRateTier::NotSupported: return "NotSupported";
        case EShadingRateTier::Tier1:        return "Tier1";
        case EShadingRateTier::Tier2:        return "Tier2";
        
        default: return "Unknown";
    }
}
