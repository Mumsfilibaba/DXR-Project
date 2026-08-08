#include "RHI/RHI.h"
#include "RHI/RHITexture.h"
#include "RHI/RayTracing/RHIRayTracingTypes.h"
#include "Core/Misc/ConsoleManager.h"

static FAutoConsoleCommand CCmdDumpRayTracingCapsCommand(
    "RHI.DumpRayTracingCaps",
    "Logs the ray-tracing (DXR 2.0) capability table for the active RHI backend",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        RHI::DumpRayTracingCapabilities();
    }));

static FAutoConsoleCommand CCmdDumpCapsCommand(
    "RHI.DumpCaps",
    "Logs the full device capability table (general + ray tracing) for the active RHI backend",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        RHI::DumpCapabilities();
    }));

static TAutoConsoleVariable<bool> CVarHDRUseDisplayLuminance(
    "RHI.HDR.UseDisplayLuminance",
    "Derive the mastering luminance range from the display instead of the RHI.HDR.*Luminance CVars, "
    "on backends that can report it.",
    true);

static TAutoConsoleVariable<float> CVarHDRMaxMasteringLuminance(
    "RHI.HDR.MaxMasteringLuminance",
    "Maximum mastering-display luminance in nits.",
    1000.0f);

static TAutoConsoleVariable<float> CVarHDRMinMasteringLuminance(
    "RHI.HDR.MinMasteringLuminance",
    "Minimum mastering-display luminance in nits.",
    0.001f);

static TAutoConsoleVariable<float> CVarHDRMaxContentLightLevel(
    "RHI.HDR.MaxContentLightLevel",
    "MaxCLL: brightest single pixel in the content, in nits.",
    1000.0f);

static TAutoConsoleVariable<float> CVarHDRMaxFrameAverageLightLevel(
    "RHI.HDR.MaxFrameAverageLightLevel",
    "MaxFALL: brightest frame average across the content, in nits.",
    400.0f);

// -------------------------------------------------------------------------------------------
// Feature Support
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// Shader / Pipeline
// -------------------------------------------------------------------------------------------

RHI_API bool         RHI::bSupportsGeometryShaders                       = true;
RHI_API bool         RHI::bSupportsTessellation                          = false;
RHI_API uint32       RHI::MaxPatchControlPoints                          = 0;
RHI_API bool         RHI::bSupportRenderTargetArrayIndexFromVertexShader = true;
RHI_API EShaderModel RHI::MaxShaderModel                                 = EShaderModel::Unknown;
RHI_API bool         RHI::bSupportsBindless                              = false;

// -------------------------------------------------------------------------------------------
// View Instancing
// -------------------------------------------------------------------------------------------

RHI_API bool   RHI::bSupportsViewInstancing = false;
RHI_API uint32 RHI::MaxViewInstanceCount    = 1;

// -------------------------------------------------------------------------------------------
// Hardware Ray Tracing
// -------------------------------------------------------------------------------------------

RHI_API bool            RHI::bSupportsRayTracing         = false;
RHI_API ERayTracingTier RHI::RayTracingTier              = ERayTracingTier::NotSupported;
RHI_API uint32          RHI::RayTracingMaxRecursionDepth = 0;

RHI_API bool   RHI::bSupportsInlineRayTracing                                  = false;
RHI_API bool   RHI::bSupportsOpacityMicromap                                   = false;
RHI_API bool   RHI::bSupportsShaderExecutionReordering                         = false;
RHI_API bool   RHI::bShaderExecutionReorderingActuallyReorders                 = false;
RHI_API bool   RHI::bSupportsRayTracingPipelineAdditions                       = false;
RHI_API bool   RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure  = false;
RHI_API bool   RHI::bSupportsIndirectAccelerationStructureOperations           = false;
RHI_API bool   RHI::bSupportsDispatchRaysIndirect                              = false;
RHI_API uint32 RHI::RayTracingMaxTrianglesPerCluster                           = 0;
RHI_API uint32 RHI::RayTracingMaxVerticesPerCluster                            = 0;
RHI_API uint32 RHI::RayTracingMaxPartitionedInstanceCount                      = 0;
RHI_API bool   RHI::bSupportsShaderBindingTableDescriptors                     = false;
RHI_API bool   RHI::bSupportsToolsVisualization                                = false;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading
// -------------------------------------------------------------------------------------------

RHI_API bool             RHI::bSupportsVRS             = false;
RHI_API EShadingRateTier RHI::ShadingRateTier          = EShadingRateTier::NotSupported;
RHI_API uint32           RHI::ShadingRateImageTileSize = 0;

// -------------------------------------------------------------------------------------------
// Sampler Feedback
// -------------------------------------------------------------------------------------------

RHI_API bool                 RHI::bSupportsSamplerFeedback = false;
RHI_API ESamplerFeedbackTier RHI::SamplerFeedbackTier      = ESamplerFeedbackTier::NotSupported;

// -------------------------------------------------------------------------------------------
// Programmable Sample Positions
// -------------------------------------------------------------------------------------------

RHI_API bool                 RHI::bSupportsProgrammableSamplePositions = false;
RHI_API ESamplePositionsTier RHI::SamplePositionsTier                  = ESamplePositionsTier::NotSupported;
RHI_API uint32               RHI::MaxSamplePositionGridWidth           = 0;
RHI_API uint32               RHI::MaxSamplePositionGridHeight          = 0;
RHI_API uint32               RHI::SupportedSamplePositionSampleCounts  = 0;

// -------------------------------------------------------------------------------------------
// Draw Indirect
// -------------------------------------------------------------------------------------------

RHI_API bool   RHI::bSupportsDrawIndirect               = false;
RHI_API bool   RHI::bSupportsDrawIndirectCount          = false;
RHI_API bool   RHI::bSupportsDispatchIndirect           = false;
RHI_API bool   RHI::bSupportsDispatchMeshIndirect       = false;
RHI_API bool   RHI::bSupportsDispatchMeshIndirectCount  = false;
RHI_API uint32 RHI::MaxDrawIndirectCommandCount         = 1;
RHI_API uint32 RHI::MaxDispatchMeshIndirectCommandCount = 1;

// -------------------------------------------------------------------------------------------
// Texture / Image Limits
// -------------------------------------------------------------------------------------------

RHI_API uint32 RHI::MaxTexture1DSize        = 8192;
RHI_API uint32 RHI::MaxTexture1DArrayLayers = 256;

RHI_API uint32 RHI::MaxTexture2DSize        = 8192;
RHI_API uint32 RHI::MaxTexture2DArrayLayers = 256;

RHI_API uint32 RHI::MaxTexture3DWidth       = 2048;
RHI_API uint32 RHI::MaxTexture3DHeight      = 2048;
RHI_API uint32 RHI::MaxTexture3DDepth       = 2048;

RHI_API uint32 RHI::MaxCubeTextureSize      = 8192;
RHI_API uint32 RHI::MaxCubeArrayCount       = RHIArrayLayersToCubes(ETextureDimension::TextureCubeArray, 252); // 42

// -------------------------------------------------------------------------------------------
// Buffer / Memory Limits
// -------------------------------------------------------------------------------------------

RHI_API uint64 RHI::MaxBufferSize                        = uint64(~0);
RHI_API uint32 RHI::MaxConstantBufferSize                = 64 * 1024; // 64 KB
RHI_API uint64 RHI::MaxStorageBufferSize                 = uint64(~0);
RHI_API uint32 RHI::StructuredBufferMinStride            = 4;
RHI_API uint32 RHI::StructuredBufferMaxStride            = 2048;
RHI_API uint32 RHI::RawBufferRequiredAlignment           = 4;
RHI_API uint32 RHI::AccelerationStructureBufferAlignment = 256;

RHI_API bool RHI::bSupportsDynamicDepthBias = false;
RHI_API bool RHI::bSupportsDepthBoundsTest  = false;
RHI_API bool RHI::bSupportsStreamOutput     = false;

// -------------------------------------------------------------------------------------------
// Query Support
// -------------------------------------------------------------------------------------------

RHI_API bool RHI::bSupportsTimestampQueries           = false;
RHI_API bool RHI::bSupportsPipelineStatisticsQueries  = false;
RHI_API bool RHI::bSupportsGPUTimestampBubblesRemoval = false;

// -------------------------------------------------------------------------------------------
// Swap-Chain Defaults
// -------------------------------------------------------------------------------------------

RHI_API EFormat RHI::DefaultSwapChainFormat = EFormat::Unknown;

RHI_API FRHIHDRMetadata RHI::GetDefaultHDRMetadata()
{
    FRHIHDRMetadata Metadata           = FRHIHDRMetadata::CreateHDR10Default();
    Metadata.MaxMasteringLuminance     = CVarHDRMaxMasteringLuminance.GetValue();
    Metadata.MinMasteringLuminance     = CVarHDRMinMasteringLuminance.GetValue();
    Metadata.MaxContentLightLevel      = CVarHDRMaxContentLightLevel.GetValue();
    Metadata.MaxFrameAverageLightLevel = CVarHDRMaxFrameAverageLightLevel.GetValue();
    return Metadata;
}

RHI_API bool RHI::ShouldUseDisplayLuminance()
{
    return CVarHDRUseDisplayLuminance.GetValue();
}

// -------------------------------------------------------------------------------------------
// Ray-tracing capability reporting
// -------------------------------------------------------------------------------------------

RHI_API void RHI::DumpRayTracingCapabilities()
{
    const auto YesNo = [](bool bValue) -> const CHAR*
    {
        return bValue ? "Yes" : "No";
    };

    LOG_INFO("[RHI] -------------------------------- Ray Tracing Capabilities ---------------------------------");
    LOG_INFO("[RHI]   Hardware Ray Tracing                  : %s", YesNo(RHI::bSupportsRayTracing));
    LOG_INFO("[RHI]   Tier                                  : %s", ToString(RHI::RayTracingTier));
    LOG_INFO("[RHI]   Max Recursion Depth                   : %u", RHI::RayTracingMaxRecursionDepth);
    LOG_INFO("[RHI]   Inline Ray Tracing (RayQuery)         : %s", YesNo(RHI::bSupportsInlineRayTracing));
    LOG_INFO("[RHI]   Opacity Micromap                      : %s", YesNo(RHI::bSupportsOpacityMicromap));
    LOG_INFO("[RHI]   Shader Execution Reordering           : %s", YesNo(RHI::bSupportsShaderExecutionReordering));
    LOG_INFO("[RHI]   Shader Execution Reordering (Reorders): %s", YesNo(RHI::bShaderExecutionReorderingActuallyReorders));
    LOG_INFO("[RHI]   Pipeline Additions (AddToStateObject) : %s", YesNo(RHI::bSupportsRayTracingPipelineAdditions));
    LOG_INFO("[RHI]   Clusters + Partitioned Scene          : %s", YesNo(RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure));
    LOG_INFO("[RHI]   Indirect AS Operations                : %s", YesNo(RHI::bSupportsIndirectAccelerationStructureOperations));
    LOG_INFO("[RHI]   Indirect Ray Dispatch                 : %s", YesNo(RHI::bSupportsDispatchRaysIndirect));
    LOG_INFO("[RHI]   Max Triangles / Cluster               : %u", RHI::RayTracingMaxTrianglesPerCluster);
    LOG_INFO("[RHI]   Max Vertices / Cluster                : %u", RHI::RayTracingMaxVerticesPerCluster);
    LOG_INFO("[RHI]   Max Partitioned Instances             : %u", RHI::RayTracingMaxPartitionedInstanceCount);
    LOG_INFO("[RHI]   SBT Local Descriptors                 : %s", YesNo(RHI::bSupportsShaderBindingTableDescriptors));
    LOG_INFO("[RHI]   Tools Visualization                   : %s", YesNo(RHI::bSupportsToolsVisualization));
    LOG_INFO("[RHI] -------------------------------------------------------------------------------------------");
}

