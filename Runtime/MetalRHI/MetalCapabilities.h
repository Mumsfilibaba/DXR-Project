#pragma once
#include "Core/Core.h"
#include "MetalRHI/MetalConfiguration.h"
#include <Metal/Metal.h>

// -------------------------------------------------------------------------------------------
// Metal Device Feature Support
// -------------------------------------------------------------------------------------------

struct FMetalFeatureSet
{
    MTLArgumentBuffersTier ArgumentBuffersTier;
    uint8                  MaxVertexAmplificationCount;
    uint8                  SupportedSampleCounts;
    bool                   bAppleGPU;
    bool                   bMetal3;
    bool                   bUnifiedMemory;
    bool                   bDepth24Stencil8;
    bool                   bStageBoundaryTimestamps;
    bool                   bDrawBoundaryTimestamps;
    bool                   bDispatchBoundaryTimestamps;
    bool                   bBlitBoundaryTimestamps;
    bool                   bResidencySets;
    bool                   bRayTracing;
    bool                   bRayTracingFromRender;
    bool                   bHardwareRayTracing;
};

extern METALRHI_API FMetalFeatureSet GMetalFeatures;

extern METALRHI_API MTLGPUFamily            GMetalHighestGPUFamily;
extern METALRHI_API MTLReadWriteTextureTier GMetalReadWriteTextureTier;
extern METALRHI_API bool                    GMetalSupportsMeshShaders;
extern METALRHI_API uint64                  GMetalMaxBufferLength;
extern METALRHI_API uint32                  GMetalMaxThreadsPerThreadgroup;
extern METALRHI_API uint32                  GMetalMaxTextureArrayLayers;
extern METALRHI_API uint32                  GMetalMaxTexture2DSize;
extern METALRHI_API uint32                  GMetalMaxTexture3DSize;
extern METALRHI_API bool                    GMetalSupportsCounterSampling;
extern METALRHI_API bool                    GMetalSupportsTimestampQueries;
extern METALRHI_API bool                    GMetalSupportsStatisticQueries;
extern METALRHI_API bool                    GMetalSupportsDepthBoundsTest;
extern METALRHI_API bool                    GMetalSupportsSamplerLODBias;
extern METALRHI_API bool                    GMetalSupportsBCTextureCompression;
extern METALRHI_API bool                    GMetalSupportsBindless;
extern METALRHI_API bool                    GMetalSupportsProgrammableSamplePositions;
extern METALRHI_API bool                    GMetalSupportsStencilResolve;

namespace MetalRHI
{
#if METAL_ASSUME_APPLE_GPU
    FORCEINLINE constexpr bool IsAppleGPU()                       { return true; }
    FORCEINLINE constexpr bool IsTileBasedGPU()                   { return true; }
    FORCEINLINE constexpr bool HasUnifiedMemory()                 { return true; }
    FORCEINLINE constexpr bool SupportsMetal3()                   { return true; }
    FORCEINLINE constexpr bool SamplesTimestampsAtStageBoundary() { return true; }
    FORCEINLINE constexpr bool SupportsDepth24Stencil8()          { return false; }
#else
    FORCEINLINE bool IsAppleGPU()                       { return GMetalFeatures.bAppleGPU; }
    FORCEINLINE bool IsTileBasedGPU()                   { return GMetalFeatures.bAppleGPU; }
    FORCEINLINE bool HasUnifiedMemory()                 { return GMetalFeatures.bUnifiedMemory; }
    FORCEINLINE bool SupportsMetal3()                   { return GMetalFeatures.bMetal3; }
    FORCEINLINE bool SamplesTimestampsAtStageBoundary() { return GMetalFeatures.bStageBoundaryTimestamps; }
    FORCEINLINE bool SupportsDepth24Stencil8()          { return GMetalFeatures.bDepth24Stencil8; }
#endif

    FORCEINLINE MTLRenderStages GetRenderStages()
    {
        return GMetalSupportsMeshShaders
            ? (MTLRenderStageVertex | MTLRenderStageFragment | MTLRenderStageObject | MTLRenderStageMesh)
            : (MTLRenderStageVertex | MTLRenderStageFragment);
    }
}

// -------------------------------------------------------------------------------------------
// Metal Capability Logging
// -------------------------------------------------------------------------------------------

extern METALRHI_API void DumpMetalCapabilities();
