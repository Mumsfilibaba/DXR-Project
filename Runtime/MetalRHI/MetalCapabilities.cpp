#include "Core/Misc/ConsoleManager.h"
#include "MetalRHI/MetalCapabilities.h"

static FAutoConsoleCommand CCmdMetalDumpCapsCommand(
    "MetalRHI.DumpCaps",
    "Logs the backend-native Metal capability table (all GMetal* globals)",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        DumpMetalCapabilities();
    }));

METALRHI_API MTLGPUFamily            GMetalHighestGPUFamily                  = MTLGPUFamilyApple1;
METALRHI_API MTLArgumentBuffersTier  GMetalArgumentBuffersTier               = MTLArgumentBuffersTier1;
METALRHI_API MTLReadWriteTextureTier GMetalReadWriteTextureTier              = MTLReadWriteTextureTierNone;
METALRHI_API bool                    GMetalSupportsRayTracing                = false;
METALRHI_API bool                    GMetalSupportsRayTracingFromRender      = false;
METALRHI_API bool                    GMetalSupportsMeshShaders               = false;
METALRHI_API bool                    GMetalSupportsUnifiedMemory             = false;
METALRHI_API uint64                  GMetalMaxBufferLength                   = 0;
METALRHI_API uint32                  GMetalMaxThreadsPerThreadgroup          = 0;
METALRHI_API uint32                  GMetalMaxVertexAmplificationCount       = 1;
METALRHI_API uint32                  GMetalMaxTextureArrayLayers             = 0;
METALRHI_API uint32                  GMetalMaxTexture2DSize                  = 0;
METALRHI_API bool                    GMetalSupportsCounterSampling           = false;
METALRHI_API bool                    GMetalSupportsTimestampQueries          = false;
METALRHI_API bool                    GMetalSupportsTimestampStageBoundary    = false;
METALRHI_API bool                    GMetalSupportsTimestampDrawBoundary     = false;
METALRHI_API bool                    GMetalSupportsTimestampDispatchBoundary = false;
METALRHI_API bool                    GMetalSupportsTimestampBlitBoundary     = false;
METALRHI_API bool                    GMetalSupportsBCTextureCompression      = false;

void DumpMetalCapabilities()
{
    const auto YesNo = [](bool bValue) -> const CHAR*
    {
        return bValue ? "Yes" : "No";
    };

    LOG_INFO("[MetalRHI] --------------------------- Metal Capabilities (native) ---------------------------");
    LOG_INFO("[MetalRHI]   Highest GPU Family                     : %d", static_cast<int32>(GMetalHighestGPUFamily));
    LOG_INFO("[MetalRHI]   Argument Buffers Tier                  : %d", static_cast<int32>(GMetalArgumentBuffersTier));
    LOG_INFO("[MetalRHI]   Read-Write Texture Tier                : %d", static_cast<int32>(GMetalReadWriteTextureTier));
    LOG_INFO("[MetalRHI]   Unified Memory                         : %s", YesNo(GMetalSupportsUnifiedMemory));
    LOG_INFO("[MetalRHI]   Ray Tracing                            : %s", YesNo(GMetalSupportsRayTracing));
    LOG_INFO("[MetalRHI]   Ray Tracing From Render                : %s", YesNo(GMetalSupportsRayTracingFromRender));
    LOG_INFO("[MetalRHI]   Mesh Shaders                           : %s", YesNo(GMetalSupportsMeshShaders));
    LOG_INFO("[MetalRHI]   Counter Sampling                       : %s", YesNo(GMetalSupportsCounterSampling));
    LOG_INFO("[MetalRHI]   Timestamp Queries                      : %s", YesNo(GMetalSupportsTimestampQueries));
    LOG_INFO("[MetalRHI]   Timestamp AtStageBoundary              : %s", YesNo(GMetalSupportsTimestampStageBoundary));
    LOG_INFO("[MetalRHI]   Timestamp AtDrawBoundary               : %s", YesNo(GMetalSupportsTimestampDrawBoundary));
    LOG_INFO("[MetalRHI]   Timestamp AtDispatchBoundary           : %s", YesNo(GMetalSupportsTimestampDispatchBoundary));
    LOG_INFO("[MetalRHI]   Timestamp AtBlitBoundary               : %s", YesNo(GMetalSupportsTimestampBlitBoundary));
    LOG_INFO("[MetalRHI]   BC Texture Compression                 : %s", YesNo(GMetalSupportsBCTextureCompression));
    LOG_INFO("[MetalRHI]   Max Buffer Length                      : %llu", static_cast<unsigned long long>(GMetalMaxBufferLength));
    LOG_INFO("[MetalRHI]   Max Threads Per Threadgroup            : %u", GMetalMaxThreadsPerThreadgroup);
    LOG_INFO("[MetalRHI]   Max Vertex Amplification Count         : %u", GMetalMaxVertexAmplificationCount);
    LOG_INFO("[MetalRHI]   Max Texture Array Layers               : %u", GMetalMaxTextureArrayLayers);
    LOG_INFO("[MetalRHI]   Max Texture 2D Size                    : %u", GMetalMaxTexture2DSize);
    LOG_INFO("[MetalRHI] ----------------------------------------------------------------------------------");
}
