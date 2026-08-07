#pragma once
#include "Core/Core.h"
#include "Core/Templates/Bits.h"

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

/** Use this value to address every subresource at once (sampler feedback transcode) */
#define RHI_ALL_SUBRESOURCES (uint32(~0))

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

/** Maximum number of control points in a single tessellation patch */
#define RHI_MAX_PATCH_CONTROL_POINTS (32)

/** Maximum sample positions in one description: a 2x2 pixel grid at 16 samples per pixel */
#define RHI_MAX_SAMPLE_POSITIONS (64)

// -------------------------------------------------------------------------------------------
// MSAA Sample Counts
// -------------------------------------------------------------------------------------------

#define RHI_SAMPLE_COUNT_1  (1)
#define RHI_SAMPLE_COUNT_2  (2)
#define RHI_SAMPLE_COUNT_4  (4)
#define RHI_SAMPLE_COUNT_8  (8)
#define RHI_SAMPLE_COUNT_16 (16)
#define RHI_SAMPLE_COUNT_32 (32)
#define RHI_SAMPLE_COUNT_64 (64)

/** Highest sample count any backend reports; a mask only ever holds bits 1 through this value */
#define RHI_MAX_SAMPLE_COUNT (RHI_SAMPLE_COUNT_64)

/** Every bit a well-formed sample-count mask may contain */
#define RHI_ALL_SAMPLE_COUNTS (uint32((RHI_MAX_SAMPLE_COUNT << 1) - 1))

NODISCARD constexpr bool IsSampleCountSupported(uint32 SampleCountMask, uint32 SampleCount)
{
    return SampleCount != 0 && (SampleCount & (SampleCount - 1)) == 0 && (SampleCountMask & SampleCount) != 0;
}

NODISCARD constexpr uint32 GetMaxSampleCount(uint32 SampleCountMask)
{
    const uint32 ValidBits = SampleCountMask & RHI_ALL_SAMPLE_COUNTS;
    return ValidBits != 0 ? (1u << Bits::MostSignificant<uint32>(ValidBits)) : 0u;
}

NODISCARD constexpr uint32 ClampSampleCountToMask(uint32 SampleCountMask, uint32 DesiredSampleCount)
{
    const uint32 Desired = DesiredSampleCount != 0 ? DesiredSampleCount : uint32(RHI_SAMPLE_COUNT_1);

    // Keep only the counts that are no greater than the desired one, then take the highest of those.
    const uint32 Candidates = Desired >= RHI_MAX_SAMPLE_COUNT ? RHI_ALL_SAMPLE_COUNTS : ((Desired << 1) - 1);
    return GetMaxSampleCount(SampleCountMask & Candidates);
}

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

enum class ESamplerFeedbackTier : uint8
{
    NotSupported = 0,

    Tier0_9 = 1,
    Tier1_0 = 2,
};

NODISCARD constexpr const CHAR* ToString(ESamplerFeedbackTier SamplerFeedbackTier)
{
    switch (SamplerFeedbackTier)
    {
        case ESamplerFeedbackTier::NotSupported: return "NotSupported";
        case ESamplerFeedbackTier::Tier0_9:      return "Tier0_9";
        case ESamplerFeedbackTier::Tier1_0:      return "Tier1_0";

        default: return "Unknown";
    }
}

enum class ESamplePositionsTier : uint8
{
    NotSupported = 0,

    /** One set of positions shared by every pixel */
    Tier1 = 1,

    /** Per-pixel positions within a 2x2 quad */
    Tier2 = 2,
};

NODISCARD constexpr const CHAR* ToString(ESamplePositionsTier SamplePositionsTier)
{
    switch (SamplePositionsTier)
    {
        case ESamplePositionsTier::NotSupported: return "NotSupported";
        case ESamplePositionsTier::Tier1:        return "Tier1";
        case ESamplePositionsTier::Tier2:        return "Tier2";

        default: return "Unknown";
    }
}
