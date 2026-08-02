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

/** Use this value to specify all plane slices of a texture (color, or both depth and stencil) */
#define RHI_ALL_PLANE_SLICES (uint32(~0))

/** Use this value to specify a buffer range that extends to the end of the buffer */
#define RHI_WHOLE_SIZE (uint64(~0))

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

enum class EShaderModel : uint8
{
    Unknown = 0,
    SM_6_0  = 3,
    SM_6_1  = 4,
    SM_6_2  = 5,
    SM_6_3  = 6,
    SM_6_4  = 7,
    SM_6_5  = 8,
    SM_6_6  = 9,
    SM_6_7  = 10,
    SM_6_8  = 11,
    SM_6_9  = 12,
    SM_6_10 = 13,
};

NODISCARD constexpr const CHAR* ToString(EShaderModel ShaderModel)
{
    switch (ShaderModel)
    {
        case EShaderModel::SM_6_0:  return "SM_6_0";
        case EShaderModel::SM_6_1:  return "SM_6_1";
        case EShaderModel::SM_6_2:  return "SM_6_2";
        case EShaderModel::SM_6_3:  return "SM_6_3";
        case EShaderModel::SM_6_4:  return "SM_6_4";
        case EShaderModel::SM_6_5:  return "SM_6_5";
        case EShaderModel::SM_6_6:  return "SM_6_6";
        case EShaderModel::SM_6_7:  return "SM_6_7";
        case EShaderModel::SM_6_8:  return "SM_6_8";
        case EShaderModel::SM_6_9:  return "SM_6_9";
        case EShaderModel::SM_6_10: return "SM_6_10";

        default: return "Unknown";
    }
}

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
