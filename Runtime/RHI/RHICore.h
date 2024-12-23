#pragma once
#include "Core/Core.h"

#define RHI_REMAINING_MIP_LEVELS uint32(~0)
#define RHI_REMAINING_ARRAY_SLICES uint32(~0)
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

// Geometry Shading Support
extern RHI_API bool GRHISupportsGeometryShaders;

// RenderTargetArrayIndex from vertex-shader Support
extern RHI_API bool GRHISupportRenderTargetArrayIndexFromVertexShader;

// View-Instancing
extern RHI_API bool   GRHISupportsViewInstancing;
extern RHI_API uint32 GRHIMaxViewInstanceCount;

// Hardware RayTracing
extern RHI_API bool            GRHISupportsRayTracing;
extern RHI_API ERayTracingTier GRHIRayTracingTier;
extern RHI_API uint32          GRHIRayTracingMaxRecursionDepth;

// Hardware Variable Rate Shading
extern RHI_API bool             GRHISupportsVRS;
extern RHI_API EShadingRateTier GRHIShadingRateTier;
extern RHI_API uint32           GRHIShadingRateImageTileSize;

// Draw-Indirect
extern RHI_API bool   GRHISupportDrawIndirect;
extern RHI_API bool   GRHISupportMultiDrawIndirect;
extern RHI_API uint32 GRHIMaxDrawIndirectCount;
