#pragma once
#include "Core/Core.h"

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
};

NODISCARD constexpr const CHAR* ToString(ERayTracingTier RayTracingTier)
{
    switch (RayTracingTier)
    {
        case ERayTracingTier::NotSupported: return "NotSupported";
        case ERayTracingTier::Tier1:        return "Tier1";
        case ERayTracingTier::Tier1_1:      return "Tier1_1";
        
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

struct RHIDeviceFeatureSupport
{
    // -------------------------------------------------------------------------------------------
    // Shader / Pipeline Features
    // -------------------------------------------------------------------------------------------

    /** Whether the device supports geometry shaders */
    static RHI_API bool bSupportsGeometryShaders;

    /** Whether SV_RenderTargetArrayIndex is supported from the vertex shader stage */
    static RHI_API bool bSupportRenderTargetArrayIndexFromVertexShader;

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------

    /** Whether view instancing is supported */
    static RHI_API bool bSupportsViewInstancing;

    /** Maximum number of view instances supported */
    static RHI_API uint32 MaxViewInstanceCount;

    // -------------------------------------------------------------------------------------------
    // Hardware Ray Tracing
    // -------------------------------------------------------------------------------------------

    /** Whether hardware-accelerated ray tracing is supported */
    static RHI_API bool bSupportsRayTracing;

    /** Ray tracing tier support (e.g., Tier 1.0, 1.1, etc.) */
    static RHI_API ERayTracingTier RayTracingTier;

    /** Maximum recursion depth supported for ray tracing pipelines */
    static RHI_API uint32 RayTracingMaxRecursionDepth;

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------

    /** Whether hardware Variable Rate Shading is supported */
    static RHI_API bool bSupportsVRS;

    /** Shading rate tier (Tier1, Tier2, etc.) */
    static RHI_API EShadingRateTier ShadingRateTier;

    /** Shading rate image tile size (e.g., 16x16) */
    static RHI_API uint32 ShadingRateImageTileSize;

    // -------------------------------------------------------------------------------------------
    // Draw Indirect
    // -------------------------------------------------------------------------------------------

    /** Whether indirect draw calls are supported */
    static RHI_API bool bSupportDrawIndirect;

    /** Whether multi-draw indirect is supported */
    static RHI_API bool bSupportMultiDrawIndirect;

    /** Maximum number of draws per indirect call */
    static RHI_API uint32 MaxDrawIndirectCount;

    // -------------------------------------------------------------------------------------------
    // Texture / Image Limits
    // -------------------------------------------------------------------------------------------

    // --- 1D Textures ---

    /** Maximum width of a 1D texture */
    static RHI_API uint32 MaxTexture1DSize;

    /** Maximum number of array layers for 1D textures */
    static RHI_API uint32 MaxTexture1DArrayLayers;


    // --- 2D Textures ---

    /** Maximum width or height of a 2D texture */
    static RHI_API uint32 MaxTexture2DSize;

    /** Maximum number of array layers for 2D textures */
    static RHI_API uint32 MaxTexture2DArrayLayers;


    // --- 3D Textures ---

    /** Maximum 3D texture width */
    static RHI_API uint32 MaxTexture3DWidth;

    /** Maximum 3D texture height */
    static RHI_API uint32 MaxTexture3DHeight;

    /** Maximum 3D texture depth */
    static RHI_API uint32 MaxTexture3DDepth;


    // --- Cube Textures ---

    /** Maximum cube map face resolution */
    static RHI_API uint32 MaxCubeTextureSize;

    /** Maximum number of cube maps in a cube texture array (arrayLayers / 6) */
    static RHI_API uint32 MaxCubeArrayCount;

    // -------------------------------------------------------------------------------------------
    // Buffer / Memory Limits
    // -------------------------------------------------------------------------------------------

    /** Maximum buffer size in bytes */
    static RHI_API uint64 MaxBufferSize;

    // --- Constant / Uniform Buffers ---

    /** Maximum size of a constant/uniform buffer binding */
    static RHI_API uint32 MaxConstantBufferSize;

    // --- Storage / Structured Buffers ---

    /** Maximum size of a storage buffer binding */
    static RHI_API uint64 MaxStorageBufferSize;

    /** Minimum stride for structured buffers (bytes) */
    static RHI_API uint32 StructuredBufferMinStride;

    /** Maximum stride for structured buffers (bytes) */
    static RHI_API uint32 StructuredBufferMaxStride;

    /** Required alignment for raw/byte-address buffers (SRV/UAV) */
    static RHI_API uint32 RawBufferRequiredAlignment;

    // -------------------------------------------------------------------------------------------
    // Dynamic State
    // -------------------------------------------------------------------------------------------

    /** Whether dynamic depth bias (RSSetDepthBias) is supported */
    static RHI_API bool bSupportsDynamicDepthBias;

    /** Whether stream output / transform feedback is supported */
    static RHI_API bool bSupportsStreamOutput;

    // -------------------------------------------------------------------------------------------
    // Query Support
    // -------------------------------------------------------------------------------------------

    /** Whether GPU timestamp queries are supported */
    static RHI_API bool bSupportsTimestampQueries;

    /** Whether pipeline statistics queries are supported */
    static RHI_API bool bSupportsPipelineStatisticsQueries;

    /** Whether the backend can filter out GPU idle bubbles from timestamp results */
    static RHI_API bool bSupportsGPUTimestampBubblesRemoval;
};

// -------------------------------------------------------------------------------------------
// Statistics (atomic)
// -------------------------------------------------------------------------------------------

struct RHIStatistics
{
    // -------------------------------------------------------------------------------------------
    // Command Submission Metrics
    // -------------------------------------------------------------------------------------------

    /** Total number of graphics draw calls submitted to the GPU */
    static RHI_API FAtomicUInt64 NumDrawCalls;

    /** Total number of compute dispatch calls submitted to the GPU */
    static RHI_API FAtomicUInt64 NumDispatchCalls;

    /** Total number of commands recorded to command lists/command buffers */
    static RHI_API FAtomicUInt64 NumCommands;
};
