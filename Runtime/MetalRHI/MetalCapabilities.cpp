#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "MetalRHI/MetalCore.h"
#include "MetalRHI/MetalCapabilities.h"

static FAutoConsoleCommand CCmdMetalDumpCapsCommand(
    TEXT("MetalRHI.DumpCaps"),
    TEXT("Logs the backend-native Metal capability table (all GMetal* globals)"),
    FConsoleCommandDelegate::CreateLambda([](FStringView)
    {
        DumpMetalCapabilities();
    }));

METALRHI_API FMetalFeatureSet GMetalFeatures =
{
    .ArgumentBuffersTier         = MTLArgumentBuffersTier1,
    .MaxVertexAmplificationCount = 1,
    .SupportedSampleCounts       = 1,
};

METALRHI_API MTLGPUFamily            GMetalHighestGPUFamily                    = MTLGPUFamilyApple1;
METALRHI_API MTLReadWriteTextureTier GMetalReadWriteTextureTier                = MTLReadWriteTextureTierNone;
METALRHI_API bool                    GMetalSupportsMeshShaders                 = false;
METALRHI_API uint64                  GMetalMaxBufferLength                     = 0;
METALRHI_API uint32                  GMetalMaxThreadsPerThreadgroup            = 0;
METALRHI_API uint32                  GMetalMaxTextureArrayLayers               = 0;
METALRHI_API uint32                  GMetalMaxTexture2DSize                    = 0;
METALRHI_API uint32                  GMetalMaxTexture3DSize                    = 0;
METALRHI_API bool                    GMetalSupportsCounterSampling             = false;
METALRHI_API bool                    GMetalSupportsTimestampQueries            = false;
METALRHI_API bool                    GMetalSupportsStatisticQueries            = false;
METALRHI_API bool                    GMetalSupportsDepthBoundsTest             = false;
METALRHI_API bool                    GMetalSupportsSamplerLODBias              = false;
METALRHI_API bool                    GMetalSupportsBCTextureCompression        = false;
METALRHI_API bool                    GMetalSupportsBindless                    = false;
METALRHI_API bool                    GMetalSupportsProgrammableSamplePositions = false;
METALRHI_API bool                    GMetalSupportsStencilResolve              = false;

static const CHAR_T* GetGPUFamilyName(MTLGPUFamily Family)
{
    switch (Family)
    {
        case MTLGPUFamilyApple9:  return TEXT("Apple9");
        case MTLGPUFamilyApple8:  return TEXT("Apple8");
        case MTLGPUFamilyApple7:  return TEXT("Apple7");
        case MTLGPUFamilyApple6:  return TEXT("Apple6");
        case MTLGPUFamilyApple5:  return TEXT("Apple5");
        case MTLGPUFamilyApple4:  return TEXT("Apple4");
        case MTLGPUFamilyApple3:  return TEXT("Apple3");
        case MTLGPUFamilyApple2:  return TEXT("Apple2");
        case MTLGPUFamilyApple1:  return TEXT("Apple1");
        case MTLGPUFamilyMac2:    return TEXT("Mac2");
        case MTLGPUFamilyCommon3: return TEXT("Common3");
        case MTLGPUFamilyCommon2: return TEXT("Common2");
        case MTLGPUFamilyCommon1: return TEXT("Common1");
        default:                  return TEXT("Unknown");
    }
}

static const CHAR_T* GetRayTracingSupportName()
{
    if (!GMetalFeatures.bRayTracing)
    {
        return TEXT("No");
    }

    if (GMetalFeatures.bHardwareRayTracing)
    {
        return TEXT("Hardware, compute and render");
    }

    return GMetalFeatures.bRayTracingFromRender ? TEXT("Compute emulation, compute and render") : TEXT("Compute emulation, compute only");
}

void DumpMetalCapabilities()
{
    const auto YesNo = [](bool bValue) -> const CHAR_T*
    {
        return bValue ? TEXT("Yes") : TEXT("No");
    };

    LOG_INFO("[MetalRHI] --------------------------- Metal Capabilities (native) ---------------------------");
    LOG_INFO("[MetalRHI]   Highest GPU Family                     : %s%s", GetGPUFamilyName(GMetalHighestGPUFamily), GMetalFeatures.bMetal3 ? TEXT(" (Metal3)") : TEXT(""));
    LOG_INFO("[MetalRHI]   Argument Buffers Tier                  : %s", GMetalFeatures.ArgumentBuffersTier == MTLArgumentBuffersTier2 ? TEXT("Tier2") : TEXT("Tier1"));
    LOG_INFO("[MetalRHI]   Read-Write Texture Tier                : %d", static_cast<int32>(GMetalReadWriteTextureTier));
    LOG_INFO("[MetalRHI]   Apple GPU                              : %s", YesNo(GMetalFeatures.bAppleGPU));
    LOG_INFO("[MetalRHI]   Unified Memory                         : %s", YesNo(GMetalFeatures.bUnifiedMemory));
    LOG_INFO("[MetalRHI]   Residency Sets                         : %s", YesNo(GMetalFeatures.bResidencySets));
    LOG_INFO("[MetalRHI]   Ray Tracing (GPU)                      : %s", GetRayTracingSupportName());
    LOG_INFO("[MetalRHI]   Ray Tracing (exposed to renderer)      : %s", YesNo(RHI::bSupportsRayTracing));
    LOG_INFO("[MetalRHI]   Mesh Shaders                           : %s", YesNo(GMetalSupportsMeshShaders));
    LOG_INFO("[MetalRHI]   Counter Sampling                       : %s", YesNo(GMetalSupportsCounterSampling));
    LOG_INFO("[MetalRHI]   Timestamp Queries                      : %s", YesNo(GMetalSupportsTimestampQueries));
    LOG_INFO("[MetalRHI]   Timestamp AtStageBoundary              : %s", YesNo(GMetalFeatures.bStageBoundaryTimestamps));
    LOG_INFO("[MetalRHI]   Timestamp AtDrawBoundary               : %s", YesNo(GMetalFeatures.bDrawBoundaryTimestamps));
    LOG_INFO("[MetalRHI]   Timestamp AtDispatchBoundary           : %s", YesNo(GMetalFeatures.bDispatchBoundaryTimestamps));
    LOG_INFO("[MetalRHI]   Timestamp AtBlitBoundary               : %s", YesNo(GMetalFeatures.bBlitBoundaryTimestamps));
    LOG_INFO("[MetalRHI]   Statistic Queries                      : %s", YesNo(GMetalSupportsStatisticQueries));
    LOG_INFO("[MetalRHI]   Depth Bounds Test                      : %s", YesNo(GMetalSupportsDepthBoundsTest));
    LOG_INFO("[MetalRHI]   Sampler LOD Bias                       : %s", YesNo(GMetalSupportsSamplerLODBias));
    LOG_INFO("[MetalRHI]   BC Texture Compression                 : %s", YesNo(GMetalSupportsBCTextureCompression));
    LOG_INFO("[MetalRHI]   Depth24 Stencil8                       : %s", YesNo(GMetalFeatures.bDepth24Stencil8));
    LOG_INFO("[MetalRHI]   Bindless Descriptors                   : %s", YesNo(GMetalSupportsBindless));
    LOG_INFO("[MetalRHI]   Programmable Sample Positions          : %s", YesNo(GMetalSupportsProgrammableSamplePositions));
    LOG_INFO("[MetalRHI]   MSAA Stencil Resolve                   : %s", YesNo(GMetalSupportsStencilResolve));
    LOG_INFO("[MetalRHI]   Supported Sample Counts                : 0x%X", static_cast<uint32>(GMetalFeatures.SupportedSampleCounts));
    LOG_INFO("[MetalRHI]   Max Buffer Length                      : %llu", static_cast<unsigned long long>(GMetalMaxBufferLength));
    LOG_INFO("[MetalRHI]   Max Threads Per Threadgroup            : %u", GMetalMaxThreadsPerThreadgroup);
    LOG_INFO("[MetalRHI]   Max Vertex Amplification Count         : %u", static_cast<uint32>(GMetalFeatures.MaxVertexAmplificationCount));
    LOG_INFO("[MetalRHI]   Max Texture Array Layers               : %u", GMetalMaxTextureArrayLayers);
    LOG_INFO("[MetalRHI]   Max Texture 2D Size                    : %u", GMetalMaxTexture2DSize);
    LOG_INFO("[MetalRHI]   Max Texture 3D Size                    : %u", GMetalMaxTexture3DSize);
    LOG_INFO("[MetalRHI] ----------------------------------------------------------------------------------");
}
