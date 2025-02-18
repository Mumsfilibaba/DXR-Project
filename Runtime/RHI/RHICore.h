#pragma once
#include "Core/Core.h"

#define RHI_REMAINING_MIP_LEVELS uint32(~0)
#define RHI_REMAINING_ARRAY_SLICES uint32(~0)
#define RHI_ALL_MIP_LEVELS uint32(~0)
#define RHI_ALL_ARRAY_SLICES uint32(~0)
#define RHI_NUM_CUBE_FACES (6)

#define RHI_MAX_RENDER_TARGETS (8)
#define RHI_MAX_LOCAL_SHADER_BINDINGS (4)
#define RHI_MAX_SHADER_CONSTANTS (32)
#define RHI_MAX_VERTEX_BUFFERS (32)

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

struct FRHIDeviceInfo
{
    // Geometry Shading Support
    static RHI_API bool SupportsGeometryShaders;

    // RenderTargetArrayIndex from vertex-shader Support
    static RHI_API bool SupportRenderTargetArrayIndexFromVertexShader;

    // View-Instancing
    static RHI_API bool   SupportsViewInstancing;
    static RHI_API uint32 MaxViewInstanceCount;

    // Hardware RayTracing
    static RHI_API bool            SupportsRayTracing;
    static RHI_API ERayTracingTier RayTracingTier;
    static RHI_API uint32          RayTracingMaxRecursionDepth;

    // Hardware Variable Rate Shading
    static RHI_API bool             SupportsVRS;
    static RHI_API EShadingRateTier ShadingRateTier;
    static RHI_API uint32           ShadingRateImageTileSize;

    // Draw-Indirect
    static RHI_API bool   SupportDrawIndirect;
    static RHI_API bool   SupportMultiDrawIndirect;
    static RHI_API uint32 MaxDrawIndirectCount;
};
