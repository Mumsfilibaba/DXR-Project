#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanCapabilities.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanExtensions.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanSwapChain.h"

static FAutoConsoleCommand CCmdVulkanDumpRayTracingCapsCommand(
    "VulkanRHI.DumpRayTracingCaps",
    "Logs the backend-native Vulkan ray-tracing capability table (GVulkanSupports* RT globals)",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        DumpVulkanRayTracingCapabilities();
    }));

static FAutoConsoleCommand CCmdVulkanDumpCapsCommand(
    "VulkanRHI.DumpCaps",
    "Logs the backend-native Vulkan capability table (GVulkan* globals) followed by the ray-tracing table",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        DumpVulkanCapabilities();
    }));

// -------------------------------------------------------------------------------------------
// Vulkan Device Feature Support
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanForceBinding                        = false;
VULKANRHI_API bool   GVulkanAllowNullDescriptors                = true;
VULKANRHI_API bool   GVulkanAllowGeometryShaders                = true;
VULKANRHI_API bool   GVulkanAllowResetCommandBuffers            = false;
VULKANRHI_API bool   GVulkanRobustBufferAccessEnabled           = false;
VULKANRHI_API bool   GVulkanGPUAssistedValidationEnabled        = false;

VULKANRHI_API bool   GVulkanSupportsDepthClip                   = false;
VULKANRHI_API bool   GVulkanSupportsDepthClamp                  = false;
VULKANRHI_API bool   GVulkanSupportsNullDescriptors             = false;
VULKANRHI_API bool   GVulkanSupportsRobustness2                 = false;
VULKANRHI_API bool   GVulkanSupportsConservativeRasterization   = false;
VULKANRHI_API float  GVulkanMaxExtraPrimitiveOverestimationSize = 0.0f;
VULKANRHI_API bool   GVulkanSupportsPipelineCacheControl        = false;
VULKANRHI_API bool   GVulkanSupportsDynamicRendering            = false;
VULKANRHI_API bool   GVulkanSupportsSynchronization2            = false;
VULKANRHI_API bool   GVulkanSupportsMaintenance4                = false;
VULKANRHI_API bool   GVulkanSupportsMultiviews                  = false;
VULKANRHI_API bool   GVulkanSupportsBindless                    = false;
VULKANRHI_API bool   GVulkanSupportsMutableDescriptorType       = false;
VULKANRHI_API bool   GVulkanSupportsDepthBoundsTest             = false;
VULKANRHI_API bool   GVulkanSupportsSparseBinding               = false;
VULKANRHI_API bool   GVulkanSupportsSparseResidency2D           = false;
VULKANRHI_API bool   GVulkanSupportsSparseResidency3D           = false;
VULKANRHI_API bool   GVulkanSupportsSparseResidencyAliased      = false;
VULKANRHI_API bool   GVulkanSupportsGeometryShader              = false;
VULKANRHI_API bool   GVulkanSupportsTessellation                = false;

VULKANRHI_API uint32 GVulkanMaxMultiviewViewCount               = 1;
VULKANRHI_API uint32 GVulkanMaxDrawIndirectCount                = 1;

// -------------------------------------------------------------------------------------------
// Programmable sample positions (VK_EXT_sample_locations)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsSampleLocations = false;

// -------------------------------------------------------------------------------------------
// Fragment shader interlock (VK_EXT_fragment_shader_interlock)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsFragmentShaderInterlock = false;

// -------------------------------------------------------------------------------------------
// Shader Language Capabilities
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsWaveOperations            = false;
VULKANRHI_API bool GVulkanSupportsPartitionedWaveOperations = false;
VULKANRHI_API bool GVulkanSupportsNative16BitOps            = false;
VULKANRHI_API bool GVulkanSupportsInt64ShaderOps            = false;
VULKANRHI_API bool GVulkanSupportsInt64Atomics              = false;
VULKANRHI_API bool GVulkanSupportsMaximalReconvergence      = false;
VULKANRHI_API bool GVulkanSupportsQuadControl               = false;
VULKANRHI_API bool GVulkanSupportsIntegerDotProduct         = false;
VULKANRHI_API bool GVulkanSupportsFragmentBarycentric       = false;

VULKANRHI_API uint32 GVulkanSubgroupSupportedOperations = 0;
VULKANRHI_API uint32 GVulkanSubgroupSupportedStages     = 0;
VULKANRHI_API uint32 GVulkanSubgroupSize                = 0;

VULKANRHI_API EShaderModel GVulkanShaderModel = EShaderModel::Unknown;

// -------------------------------------------------------------------------------------------
// Ray Tracing (VK_KHR_ray_tracing_pipeline, VK_KHR_ray_query)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsRayTracingPipeline     = false;
VULKANRHI_API bool GVulkanSupportsRayQuery               = false;
VULKANRHI_API bool GVulkanSupportsAccelerationStructures = false;

VULKANRHI_API bool GVulkanSupportsOpacityMicromap                         = false;
VULKANRHI_API bool GVulkanSupportsShaderExecutionReordering               = false;
VULKANRHI_API bool GVulkanShaderExecutionReorderingActuallyReorders       = false;
VULKANRHI_API bool GVulkanSupportsClustersAndPTLAS                        = false;
VULKANRHI_API bool GVulkanSupportsIndirectAccelerationStructureOperations = false;
VULKANRHI_API bool GVulkanSupportsIndirectRayDispatch                     = false;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading (VK_KHR_fragment_shading_rate)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanSupportsFragmentShadingRate = false;
VULKANRHI_API uint32 GVulkanShadingRateTileSize         = 0;

// -------------------------------------------------------------------------------------------
// Mesh Shaders (VK_EXT_mesh_shader)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanSupportsMeshShaders                            = false;
VULKANRHI_API bool   GVulkanSupportsTaskShaders                            = false;
VULKANRHI_API bool   GVulkanSupportsMeshShaderMultiview                    = false;
VULKANRHI_API bool   GVulkanSupportsMeshShaderQueries                      = false;
VULKANRHI_API bool   GVulkanSupportsMeshShaderPrimitiveFragmentShadingRate = false;
VULKANRHI_API uint32 GVulkanMaxMeshOutputVertices                          = 0;
VULKANRHI_API uint32 GVulkanMaxMeshWorkGroupInvocations                    = 0;
VULKANRHI_API uint32 GVulkanMaxTaskWorkGroupInvocations                    = 0;

// -------------------------------------------------------------------------------------------
// Transform Feedback / Stream Output (VK_EXT_transform_feedback)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsTransformFeedback = false;

// -------------------------------------------------------------------------------------------
// Dynamic Rendering (VK_KHR_dynamic_rendering / Vulkan 1.3)
// -------------------------------------------------------------------------------------------

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
VULKANRHI_API bool GVulkanUseDynamicRendering = true;
#endif

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

VULKANRHI_API uint32 GVulkanMaxDescriptorSetSamplers       = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetSampledImages  = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageImages  = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetUniformBuffers = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageBuffers = 0;

static EShaderModel DeriveEquivalentShaderModel(const FVulkanCoreFeatures& Features)
{
    // SM 6.8 requires SV_StartVertexLocation and SV_StartInstanceLocation (Extended Command Information).
    const bool bDrawParameters = (Features.Features11.shaderDrawParameters == VK_TRUE);

    // SM 6.9 promotes native 16-bit ops, wave ops and Int64 ops from optional to required.
    const bool bNative16BitOps = GVulkanSupportsNative16BitOps;
    const bool bWaveOps        = GVulkanSupportsWaveOperations;
    const bool bInt64Ops       = GVulkanSupportsInt64ShaderOps;

    struct FShaderModelRung
    {
        EShaderModel ShaderModel;
        bool         bSatisfied;
    };

    const FShaderModelRung Rungs[] =
    {
        { EShaderModel::SM_6_0, true }, // Wave intrinsics, 64-bit int arithmetic  -> Subgroup ops, shaderInt64
        { EShaderModel::SM_6_1, true }, // SV_ViewID, SV_Barycentrics              -> Multiview, VK_KHR_fragment_shader_barycentric
        { EShaderModel::SM_6_2, true }, // Native 16-bit types, denorm mode        -> ShaderFloat16/shaderInt16, 16-bit storage
        { EShaderModel::SM_6_3, true }, // DXR, DXIL libraries and linking         -> VK_KHR_ray_tracing_pipeline
        { EShaderModel::SM_6_4, true }, // VRS, packed dot products, subobjects    -> VK_KHR_fragment_shading_rate, shaderIntegerDotProduct
        { EShaderModel::SM_6_5, true }, // DXR 1.1, mesh/amp, feedback, WaveMatch  -> VK_KHR_ray_query, VK_EXT_mesh_shader, VK_NV_shader_subgroup_partitioned
        { EShaderModel::SM_6_6, true }, // Dynamic resources, 64-bit atomics       -> Descriptor indexing (GVulkanSupportsBindless), Int64 atomics
        { EShaderModel::SM_6_7, true }, // Advanced texture ops, QuadAny/QuadAll   -> VK_KHR_shader_quad_control

        // SM 6.8 requires the extended command information system values.
        { EShaderModel::SM_6_8, bDrawParameters },

        // SM 6.9's three promoted features.
        { EShaderModel::SM_6_9, bNative16BitOps && bWaveOps && bInt64Ops },
    };

    EShaderModel Result = EShaderModel::Unknown;
    for (const FShaderModelRung& Rung : Rungs)
    {
        if (!Rung.bSatisfied)
        {
            break;
        }

        Result = Rung.ShaderModel;
    }

    return Result;
}

VULKANRHI_API void DumpVulkanRayTracingCapabilities()
{
    const auto YesNo = [](bool bValue) -> const CHAR*
    {
        return bValue ? "Yes" : "No";
    };

    LOG_INFO("[VulkanRHI] ------------------------ Vulkan Ray Tracing Capabilities (native) ------------------------");
    LOG_INFO("[VulkanRHI]   Ray Tracing Pipeline                  : %s", YesNo(GVulkanSupportsRayTracingPipeline));
    LOG_INFO("[VulkanRHI]   Ray Query (Inline)                    : %s", YesNo(GVulkanSupportsRayQuery));
    LOG_INFO("[VulkanRHI]   Acceleration Structures               : %s", YesNo(GVulkanSupportsAccelerationStructures));
    LOG_INFO("[VulkanRHI]   Opacity Micromap                      : %s", YesNo(GVulkanSupportsOpacityMicromap));
    LOG_INFO("[VulkanRHI]   Shader Execution Reordering           : %s", YesNo(GVulkanSupportsShaderExecutionReordering));
    LOG_INFO("[VulkanRHI]   Shader Execution Reordering (Reorders): %s", YesNo(GVulkanShaderExecutionReorderingActuallyReorders));
    LOG_INFO("[VulkanRHI]   Clusters + Partitioned Scene (PTLAS)  : %s", YesNo(GVulkanSupportsClustersAndPTLAS));
    LOG_INFO("[VulkanRHI]   Indirect AS Operations                : %s", YesNo(GVulkanSupportsIndirectAccelerationStructureOperations));
    LOG_INFO("[VulkanRHI]   Indirect Ray Dispatch                 : %s", YesNo(GVulkanSupportsIndirectRayDispatch));
    LOG_INFO("[VulkanRHI] -----------------------------------------------------------------------------------------");
}

VULKANRHI_API void DumpVulkanCapabilities()
{
    const auto YesNo = [](bool bValue) -> const CHAR*
    {
        return bValue ? "Yes" : "No";
    };

    LOG_INFO("[VulkanRHI] --------------------------- Vulkan Capabilities (native) ---------------------------");
    LOG_INFO("[VulkanRHI]   Force Binding                         : %s", YesNo(GVulkanForceBinding));
    LOG_INFO("[VulkanRHI]   Bindless                              : %s", YesNo(GVulkanSupportsBindless));
    LOG_INFO("[VulkanRHI]   Mutable Descriptor Type               : %s", YesNo(GVulkanSupportsMutableDescriptorType));
    LOG_INFO("[VulkanRHI]   Null Descriptors                      : %s", YesNo(GVulkanSupportsNullDescriptors));
    LOG_INFO("[VulkanRHI]   Robustness2                           : %s", YesNo(GVulkanSupportsRobustness2));
    LOG_INFO("[VulkanRHI]   Dynamic Rendering                     : %s", YesNo(GVulkanUseDynamicRendering));
    LOG_INFO("[VulkanRHI]   Geometry Shader                       : %s", YesNo(GVulkanSupportsGeometryShader));
    LOG_INFO("[VulkanRHI]   Tessellation                          : %s", YesNo(GVulkanSupportsTessellation));
    LOG_INFO("[VulkanRHI]   Mesh Shaders                          : %s", YesNo(GVulkanSupportsMeshShaders));
    LOG_INFO("[VulkanRHI]   Task Shaders                          : %s", YesNo(GVulkanSupportsTaskShaders));
    LOG_INFO("[VulkanRHI]   Fragment Shading Rate                 : %s", YesNo(GVulkanSupportsFragmentShadingRate));
    LOG_INFO("[VulkanRHI]   Conservative Rasterization            : %s", YesNo(GVulkanSupportsConservativeRasterization));
    LOG_INFO("[VulkanRHI]   Multiview                             : %s", YesNo(GVulkanSupportsMultiviews));
    LOG_INFO("[VulkanRHI]   Depth Bounds Test                     : %s", YesNo(GVulkanSupportsDepthBoundsTest));
    LOG_INFO("[VulkanRHI]   Depth Clip                            : %s", YesNo(GVulkanSupportsDepthClip));
    LOG_INFO("[VulkanRHI]   Depth Clamp                           : %s", YesNo(GVulkanSupportsDepthClamp));
    LOG_INFO("[VulkanRHI]   Sample Locations                      : %s", YesNo(GVulkanSupportsSampleLocations));
    LOG_INFO("[VulkanRHI]   Fragment Shader Interlock             : %s", YesNo(GVulkanSupportsFragmentShaderInterlock));
    LOG_INFO("[VulkanRHI]   Transform Feedback                    : %s", YesNo(GVulkanSupportsTransformFeedback));
    LOG_INFO("[VulkanRHI]   Sparse Binding                        : %s", YesNo(GVulkanSupportsSparseBinding));
    LOG_INFO("[VulkanRHI]   Pipeline Cache Control                : %s", YesNo(GVulkanSupportsPipelineCacheControl));
    LOG_INFO("[VulkanRHI]   Dynamic Rendering                     : %s", YesNo(GVulkanSupportsDynamicRendering));
    LOG_INFO("[VulkanRHI]   Synchronization2                      : %s", YesNo(GVulkanSupportsSynchronization2));
    LOG_INFO("[VulkanRHI]   Maintenance4                          : %s", YesNo(GVulkanSupportsMaintenance4));
    LOG_INFO("[VulkanRHI]   Shading Rate Tile Size                : %u", GVulkanShadingRateTileSize);
    LOG_INFO("[VulkanRHI]   Max Draw Indirect Count               : %u", GVulkanMaxDrawIndirectCount);
    LOG_INFO("[VulkanRHI]   Equivalent Shader Model               : %s", ToString(GVulkanShaderModel));
    LOG_INFO("[VulkanRHI]   Wave Operations                       : %s", YesNo(GVulkanSupportsWaveOperations));
    LOG_INFO("[VulkanRHI]   Wave Operations (Partitioned)         : %s", YesNo(GVulkanSupportsPartitionedWaveOperations));
    LOG_INFO("[VulkanRHI]   Subgroup Size                         : %u", GVulkanSubgroupSize);
    LOG_INFO("[VulkanRHI]   Subgroup Supported Operations         : 0x%X", GVulkanSubgroupSupportedOperations);
    LOG_INFO("[VulkanRHI]   Subgroup Supported Stages             : 0x%X", GVulkanSubgroupSupportedStages);
    LOG_INFO("[VulkanRHI]   Native 16-bit Shader Ops              : %s", YesNo(GVulkanSupportsNative16BitOps));
    LOG_INFO("[VulkanRHI]   Int64 Shader Ops                      : %s", YesNo(GVulkanSupportsInt64ShaderOps));
    LOG_INFO("[VulkanRHI]   Int64 Atomics                         : %s", YesNo(GVulkanSupportsInt64Atomics));
    LOG_INFO("[VulkanRHI]   Integer Dot Product                   : %s", YesNo(GVulkanSupportsIntegerDotProduct));
    LOG_INFO("[VulkanRHI]   Shader Quad Control                   : %s", YesNo(GVulkanSupportsQuadControl));
    LOG_INFO("[VulkanRHI]   Fragment Shader Barycentric           : %s", YesNo(GVulkanSupportsFragmentBarycentric));
    LOG_INFO("[VulkanRHI] ----------------------------------------------------------------------------------");

    DumpVulkanRayTracingCapabilities();
}

void FVulkanDevice::DeriveCoreCapabilities(
    FVulkanDeviceCreateInfo&                   InDeviceCreateInfo,
    const FVulkanCoreFeatures&                 AvailableFeatures,
    const VkPhysicalDeviceProperties&          CoreDeviceProperties10,
    const VkPhysicalDeviceMultiviewProperties& MultiviewProperties,
    const VkPhysicalDeviceSubgroupProperties&  SubgroupProperties)
{
    const VkPhysicalDeviceFeatures& CoreDeviceFeatures10 = AvailableFeatures.Features10;

    GVulkanSupportsDepthBoundsTest        = (CoreDeviceFeatures10.depthBounds == VK_TRUE);
    GVulkanSupportsSparseBinding          = (CoreDeviceFeatures10.sparseBinding == VK_TRUE);
    GVulkanSupportsSparseResidency2D      = (CoreDeviceFeatures10.sparseResidencyImage2D == VK_TRUE);
    GVulkanSupportsSparseResidency3D      = (CoreDeviceFeatures10.sparseResidencyImage3D == VK_TRUE);
    GVulkanSupportsSparseResidencyAliased = (CoreDeviceFeatures10.sparseResidencyAliased == VK_TRUE);
    GVulkanSupportsGeometryShader         = (GVulkanAllowGeometryShaders && CoreDeviceFeatures10.geometryShader == VK_TRUE);
    GVulkanSupportsTessellation           = (CoreDeviceFeatures10.tessellationShader == VK_TRUE);

    if (AvailableFeatures.Features11.multiview)
    {
        GVulkanSupportsMultiviews    = true;
        GVulkanMaxMultiviewViewCount = Math::Max<uint32>(1u, MultiviewProperties.maxMultiviewViewCount);
    }
    else
    {
        GVulkanSupportsMultiviews    = false;
        GVulkanMaxMultiviewViewCount = 1u;
    }

    GVulkanSupportsBindless = (AvailableFeatures.Features12.descriptorIndexing         == VK_TRUE)
        && (AvailableFeatures.Features12.runtimeDescriptorArray                        == VK_TRUE)
        && (AvailableFeatures.Features12.descriptorBindingPartiallyBound               == VK_TRUE)
        && (AvailableFeatures.Features12.descriptorBindingSampledImageUpdateAfterBind  == VK_TRUE)
        && (AvailableFeatures.Features12.descriptorBindingStorageImageUpdateAfterBind  == VK_TRUE)
        && (AvailableFeatures.Features12.descriptorBindingUniformBufferUpdateAfterBind == VK_TRUE)
        && (AvailableFeatures.Features12.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE)
        && (AvailableFeatures.Features12.shaderSampledImageArrayNonUniformIndexing     == VK_TRUE)
        && (AvailableFeatures.Features12.shaderStorageImageArrayNonUniformIndexing     == VK_TRUE)
        && (AvailableFeatures.Features12.shaderStorageBufferArrayNonUniformIndexing    == VK_TRUE)
        && (AvailableFeatures.Features12.shaderUniformBufferArrayNonUniformIndexing    == VK_TRUE);

    GVulkanSubgroupSupportedOperations = SubgroupProperties.supportedOperations;
    GVulkanSubgroupSupportedStages     = SubgroupProperties.supportedStages;
    GVulkanSubgroupSize                = SubgroupProperties.subgroupSize;

    {
        constexpr VkSubgroupFeatureFlags RequiredOperations =
            VK_SUBGROUP_FEATURE_BASIC_BIT      |
            VK_SUBGROUP_FEATURE_VOTE_BIT       |
            VK_SUBGROUP_FEATURE_BALLOT_BIT     |
            VK_SUBGROUP_FEATURE_ARITHMETIC_BIT |
            VK_SUBGROUP_FEATURE_SHUFFLE_BIT    |
            VK_SUBGROUP_FEATURE_QUAD_BIT;

        // Shader Model 6.5 makes wave intrinsics available in every stage except the ray-tracing ones.
        constexpr VkShaderStageFlags RequiredStages =
            VK_SHADER_STAGE_COMPUTE_BIT  |
            VK_SHADER_STAGE_FRAGMENT_BIT |
            VK_SHADER_STAGE_VERTEX_BIT;

        GVulkanSupportsWaveOperations =
            ((GVulkanSubgroupSupportedOperations & RequiredOperations) == RequiredOperations) &&
            ((GVulkanSubgroupSupportedStages & RequiredStages) == RequiredStages);

        GVulkanSupportsPartitionedWaveOperations =
            (GVulkanSubgroupSupportedOperations & VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV) != 0;
    }

#if VULKAN_ENABLE_CRASH_MARKERS
    #if VK_AMD_buffer_marker
        bSupportsAMDBufferMarker = IsExtensionEnabled(VK_AMD_BUFFER_MARKER_EXTENSION_NAME);
    #endif
    #if VK_NV_device_diagnostic_checkpoints
        bSupportsNVDiagnosticCheckpoints = IsExtensionEnabled(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
    #endif
#endif

    GVulkanMaxDrawIndirectCount           = CoreDeviceProperties10.limits.maxDrawIndirectCount;
    GVulkanMaxDescriptorSetSamplers       = CoreDeviceProperties10.limits.maxDescriptorSetSamplers;
    GVulkanMaxDescriptorSetSampledImages  = CoreDeviceProperties10.limits.maxDescriptorSetSampledImages;
    GVulkanMaxDescriptorSetStorageImages  = CoreDeviceProperties10.limits.maxDescriptorSetStorageImages;
    GVulkanMaxDescriptorSetUniformBuffers = CoreDeviceProperties10.limits.maxDescriptorSetUniformBuffers;
    GVulkanMaxDescriptorSetStorageBuffers = CoreDeviceProperties10.limits.maxDescriptorSetStorageBuffers;

    for (const TUniquePtr<FVulkanDeviceExtension>& Extension : InDeviceCreateInfo.Extensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->ProcessQueriedFeatures();
        }
    }

    if (GVulkanSupportsBindless && !GVulkanSupportsMutableDescriptorType)
    {
        VULKAN_INFO("Bindless disabled: VK_EXT_mutable_descriptor_type not supported by this device");
        GVulkanSupportsBindless = false;
    }

    GVulkanSupportsDepthClamp = (CoreDeviceFeatures10.depthClamp == VK_TRUE);

    if (GVulkanSupportsDepthClip && !CoreDeviceFeatures10.depthClamp)
    {
        GVulkanSupportsDepthClip = false;
    }
}

void FVulkanDevice::DeriveEnabledFeatureCapabilities(const FVulkanCoreFeatures& EnabledFeatures)
{
    GVulkanRobustBufferAccessEnabled = (EnabledFeatures.Features10.robustBufferAccess == VK_TRUE);

    GVulkanSupportsNative16BitOps = (EnabledFeatures.Features12.shaderFloat16 == VK_TRUE)
        && (EnabledFeatures.Features10.shaderInt16                            == VK_TRUE)
        && (EnabledFeatures.Features11.storageBuffer16BitAccess               == VK_TRUE)
        && (EnabledFeatures.Features11.uniformAndStorageBuffer16BitAccess     == VK_TRUE);

    GVulkanSupportsInt64ShaderOps = (EnabledFeatures.Features10.shaderInt64 == VK_TRUE);

    GVulkanSupportsInt64Atomics = (EnabledFeatures.Features12.shaderBufferInt64Atomics == VK_TRUE)
        && (EnabledFeatures.Features12.shaderSharedInt64Atomics                        == VK_TRUE);

    GVulkanShaderModel  = DeriveEquivalentShaderModel(EnabledFeatures);
    RHI::MaxShaderModel = GVulkanShaderModel;
}

bool FVulkanDevice::InitializeDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline defaults
    // -------------------------------------------------------------------------------------------

    RHI::DefaultSwapChainFormat = GetVulkanDefaultBackBufferFormat();

    RHI::bSupportsGeometryShaders                       = false;
    RHI::bSupportRenderTargetArrayIndexFromVertexShader = false;

    RHI::bSupportsViewInstancing     = false;
    RHI::MaxViewInstanceCount        = 1;

    RHI::bSupportsRayTracing         = false;
    RHI::RayTracingTier              = ERayTracingTier::NotSupported;
    RHI::RayTracingMaxRecursionDepth = 0;

    RHI::bSupportsVRS                = false;
    RHI::ShadingRateTier             = EShadingRateTier::NotSupported;
    RHI::ShadingRateImageTileSize    = 0;

    RHI::bSupportsDrawIndirect               = true;
    RHI::bSupportsDrawIndirectCount          = false;
    RHI::bSupportsDispatchIndirect           = true;
    RHI::bSupportsDispatchMeshIndirect       = false;
    RHI::bSupportsDispatchMeshIndirectCount  = false;
    RHI::bSupportsDispatchRaysIndirect       = false;
    RHI::MaxDrawIndirectCommandCount         = 1;
    RHI::MaxDispatchMeshIndirectCommandCount = 1;

    RHI::MaxTexture1DSize            = 0;
    RHI::MaxTexture1DArrayLayers     = 0;
    RHI::MaxTexture2DSize            = 0;
    RHI::MaxTexture2DArrayLayers     = 0;
    RHI::MaxTexture3DWidth           = 0;
    RHI::MaxTexture3DHeight          = 0;
    RHI::MaxTexture3DDepth           = 0;
    RHI::MaxCubeTextureSize          = 0;
    RHI::MaxCubeArrayCount           = 0;

    RHI::MaxBufferSize               = 0;
    RHI::MaxConstantBufferSize       = 0;
    RHI::MaxStorageBufferSize        = 0;
    RHI::StructuredBufferMinStride   = 4;
    RHI::StructuredBufferMaxStride   = 2048;
    RHI::RawBufferRequiredAlignment  = 4;

    // -------------------------------------------------------------------------------------------
    // Core features/properties
    // -------------------------------------------------------------------------------------------

    VkPhysicalDevice PhysicalDeviceHandle = GetPhysicalDevice()->GetVkPhysicalDevice();

    // Core features
    const VkPhysicalDeviceFeatures& PhysicalDeviceFeatures = PhysicalDevice->GetFeatures();
    if (GVulkanAllowGeometryShaders && PhysicalDeviceFeatures.geometryShader)
    {
        RHI::bSupportsGeometryShaders = true;
    }

    // Core properties
    const VkPhysicalDeviceProperties& PhysicalDeviceProperties = PhysicalDevice->GetProperties();

    {
        const VkPhysicalDeviceVulkan12Features& PhysicalDeviceFeatures12 = PhysicalDevice->GetFeaturesVulkan12();
        RHI::bSupportsDrawIndirectCount  = PhysicalDeviceFeatures12.drawIndirectCount == VK_TRUE && vkCmdDrawIndirectCount && vkCmdDrawIndexedIndirectCount;
        RHI::MaxDrawIndirectCommandCount = PhysicalDeviceFeatures.multiDrawIndirect ? PhysicalDeviceProperties.limits.maxDrawIndirectCount : 1;
    #if VK_EXT_mesh_shader
        RHI::bSupportsDispatchMeshIndirect       = GVulkanSupportsMeshShaders && vkCmdDrawMeshTasksIndirectEXT;
        RHI::bSupportsDispatchMeshIndirectCount  = RHI::bSupportsDispatchMeshIndirect && RHI::bSupportsDrawIndirectCount && vkCmdDrawMeshTasksIndirectCountEXT;
        RHI::MaxDispatchMeshIndirectCommandCount = RHI::bSupportsDispatchMeshIndirect ? RHI::MaxDrawIndirectCommandCount : 1;
    #endif

        // Texture / Image limits
        RHI::MaxTexture1DSize        = PhysicalDeviceProperties.limits.maxImageDimension1D;
        RHI::MaxTexture2DSize        = PhysicalDeviceProperties.limits.maxImageDimension2D;
        RHI::MaxTexture3DWidth       = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHI::MaxTexture3DHeight      = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHI::MaxTexture3DDepth       = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHI::MaxCubeTextureSize      = PhysicalDeviceProperties.limits.maxImageDimensionCube;
        RHI::MaxTexture1DArrayLayers = PhysicalDeviceProperties.limits.maxImageArrayLayers;
        RHI::MaxTexture2DArrayLayers = PhysicalDeviceProperties.limits.maxImageArrayLayers;
        RHI::MaxCubeArrayCount       = PhysicalDeviceProperties.limits.maxImageArrayLayers / RHI_NUM_CUBE_FACES;

        // Buffer / Memory Limits 
        const uint32 MinBufferStride = sizeof(uint32); 
        RHI::MaxConstantBufferSize      = PhysicalDeviceProperties.limits.maxUniformBufferRange; 
        RHI::MaxStorageBufferSize       = PhysicalDeviceProperties.limits.maxStorageBufferRange; 
        RHI::MaxBufferSize              = uint64(~0); 
        RHI::StructuredBufferMinStride  = MinBufferStride; 
        RHI::StructuredBufferMaxStride  = uint32(~0); 
        RHI::RawBufferRequiredAlignment = MinBufferStride; 
    } 

    // -------------------------------------------------------------------------------------------
    // SV_RenderTargetArrayIndex from VS (shaderOutputLayer in Vulkan 1.2)
    // -------------------------------------------------------------------------------------------

    const VkPhysicalDeviceVulkan12Features& PhysicalDeviceFeatures12 = PhysicalDevice->GetFeaturesVulkan12();
    RHI::bSupportRenderTargetArrayIndexFromVertexShader = PhysicalDeviceFeatures12.shaderOutputLayer ? true : false;
    RHI::bSupportsDynamicDepthBias = true;
    RHI::bSupportsStreamOutput     = GVulkanSupportsTransformFeedback;

    // -------------------------------------------------------------------------------------------
    // Query Support
    // -------------------------------------------------------------------------------------------

    RHI::bSupportsTimestampQueries           = PhysicalDeviceProperties.limits.timestampComputeAndGraphics ? true : false;
    RHI::bSupportsPipelineStatisticsQueries  = PhysicalDeviceFeatures.pipelineStatisticsQuery ? true : false;
    RHI::bSupportsGPUTimestampBubblesRemoval = true;

    // -------------------------------------------------------------------------------------------
    // View Instancing (multiview)
    // -------------------------------------------------------------------------------------------

    if (GVulkanSupportsMultiviews)
    {
        RHI::MaxViewInstanceCount    = GVulkanMaxMultiviewViewCount;
        RHI::bSupportsViewInstancing = RHI::MaxViewInstanceCount > 1;
    }
    else
    {
        RHI::MaxViewInstanceCount    = 1;
        RHI::bSupportsViewInstancing = false;
    }

    // -------------------------------------------------------------------------------------------
    // Ray Tracing
    // Tier1_1: Only if VK_KHR_ray_query is available
    // Tier1: Pipeline ray tracing without ray query
    // Supports ray tracing if acceleration structures + (pipeline OR ray query)
    // -------------------------------------------------------------------------------------------

    const bool bHasRayTracingPipeline     = IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    const bool bHasRayQuery               = IsExtensionEnabled(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    const bool bHasAccelerationStructures = IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

    if (bHasAccelerationStructures && bHasRayTracingPipeline)
    {
    #if VK_EXT_ray_tracing_invocation_reorder || VK_NV_ray_tracing_invocation_reorder
        bool bHasInvocationReorder    = false;
        bool bReorderActuallyReorders = false;
    #endif
    #if VK_EXT_ray_tracing_invocation_reorder
        if (IsExtensionEnabled(VK_EXT_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME))
        {
            VkPhysicalDeviceRayTracingInvocationReorderPropertiesEXT ReorderProperties = {};
            ReorderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_PROPERTIES_EXT;

            VkPhysicalDeviceProperties2 DeviceProperties2 = {};
            DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

            AddToStructChain(DeviceProperties2, ReorderProperties);
            vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

            bHasInvocationReorder    = true;
            bReorderActuallyReorders = (ReorderProperties.rayTracingInvocationReorderReorderingHint != VK_RAY_TRACING_INVOCATION_REORDER_MODE_NONE_EXT);
        }
    #endif
    #if VK_NV_ray_tracing_invocation_reorder
        if (!bHasInvocationReorder && IsExtensionEnabled(VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME))
        {
            VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV ReorderProperties = {};
            ReorderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_PROPERTIES_NV;

            VkPhysicalDeviceProperties2 DeviceProperties2 = {};
            DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

            AddToStructChain(DeviceProperties2, ReorderProperties);
            vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

            bHasInvocationReorder    = true;
            bReorderActuallyReorders = (ReorderProperties.rayTracingInvocationReorderReorderingHint != VK_RAY_TRACING_INVOCATION_REORDER_MODE_NONE_NV);
        }
    #endif

        GVulkanSupportsOpacityMicromap                         = false;
        GVulkanSupportsShaderExecutionReordering               = false;
        GVulkanShaderExecutionReorderingActuallyReorders       = false;
        GVulkanSupportsClustersAndPTLAS                        = false;
        GVulkanSupportsIndirectAccelerationStructureOperations = false;

        RHI::bSupportsRayTracing         = true;
        RHI::RayTracingTier              = bHasRayQuery ? ERayTracingTier::Tier1_1 : ERayTracingTier::Tier1;
    #if VK_KHR_ray_tracing_pipeline
        RHI::RayTracingMaxRecursionDepth = GetPhysicalDevice()->GetRayTracingPipelineProperties().maxRayRecursionDepth;
    #else
        RHI::RayTracingMaxRecursionDepth = 1;
    #endif

        RHI::bSupportsInlineRayTracing = bHasRayQuery;

        // Vulkan has no in-place equivalent to D3D12 AddToStateObject
        RHI::bSupportsRayTracingPipelineAdditions = false;

        // Vulkan SBT records can only carry raw data / buffer device addresses, not image descriptors.
        RHI::bSupportsShaderBindingTableDescriptors = false;

    #if VK_KHR_ray_tracing_maintenance1 && VK_KHR_ray_tracing_pipeline
        GVulkanSupportsIndirectRayDispatch = GVulkanSupportsIndirectRayDispatch && IsExtensionEnabled(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME) && vkCmdTraceRaysIndirect2KHR;
    #else
        GVulkanSupportsIndirectRayDispatch = false;
    #endif

    #if VK_EXT_opacity_micromap
        GVulkanSupportsOpacityMicromap = IsExtensionEnabled(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME);
    #endif
    #if VK_EXT_ray_tracing_invocation_reorder || VK_NV_ray_tracing_invocation_reorder
        GVulkanSupportsShaderExecutionReordering         = bHasInvocationReorder;
        GVulkanShaderExecutionReorderingActuallyReorders = bReorderActuallyReorders;
    #endif
    #if VK_NV_cluster_acceleration_structure && VK_NV_partitioned_acceleration_structure
        GVulkanSupportsClustersAndPTLAS =
            IsExtensionEnabled(VK_NV_CLUSTER_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
            IsExtensionEnabled(VK_NV_PARTITIONED_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        GVulkanSupportsIndirectAccelerationStructureOperations = GVulkanSupportsClustersAndPTLAS;

        if (GVulkanSupportsClustersAndPTLAS)
        {
            VkPhysicalDeviceClusterAccelerationStructurePropertiesNV ClusterProperties = {};
            ClusterProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_ACCELERATION_STRUCTURE_PROPERTIES_NV;

            VkPhysicalDevicePartitionedAccelerationStructurePropertiesNV PartitionedProperties = {};
            PartitionedProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PARTITIONED_ACCELERATION_STRUCTURE_PROPERTIES_NV;

            VkPhysicalDeviceProperties2 ClusterDeviceProperties2 = {};
            ClusterDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

            AddToStructChain(ClusterDeviceProperties2, ClusterProperties);
            AddToStructChain(ClusterDeviceProperties2, PartitionedProperties);

            vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &ClusterDeviceProperties2);

            RHI::RayTracingMaxTrianglesPerCluster      = ClusterProperties.maxTrianglesPerCluster;
            RHI::RayTracingMaxVerticesPerCluster       = ClusterProperties.maxVerticesPerCluster;
            RHI::RayTracingMaxPartitionedInstanceCount = PartitionedProperties.maxPartitionCount;
        }
    #endif

        RHI::bSupportsOpacityMicromap                                  = GVulkanSupportsOpacityMicromap;
        RHI::bSupportsShaderExecutionReordering                        = GVulkanSupportsShaderExecutionReordering;
        RHI::bShaderExecutionReorderingActuallyReorders                = GVulkanShaderExecutionReorderingActuallyReorders;
        RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure = GVulkanSupportsClustersAndPTLAS;
        RHI::bSupportsIndirectAccelerationStructureOperations          = GVulkanSupportsIndirectAccelerationStructureOperations;
        RHI::bSupportsDispatchRaysIndirect                             = GVulkanSupportsIndirectRayDispatch;

        DumpVulkanCapabilities();
    }
    else
    {
        GVulkanSupportsIndirectRayDispatch = false;
        RHI::bSupportsRayTracing           = false;
        RHI::RayTracingTier                = ERayTracingTier::NotSupported;
        RHI::RayTracingMaxRecursionDepth   = 0;
        RHI::bSupportsInlineRayTracing     = false;
        RHI::bSupportsDispatchRaysIndirect = false;
    }

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (fragment shading rate)
    // Tier2: If attachmentFragmentShadingRate (image-based) is supported
    // Tier1: If pipeline/primitive shading rate is supported
    // -------------------------------------------------------------------------------------------
    
    if (IsExtensionEnabled(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
    {
        // Query features
        VkPhysicalDeviceFeatures2 DeviceFeatures2 = {};
        DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceFragmentShadingRateFeaturesKHR DeviceFragmentShadingRateFeatures = {};
        DeviceFragmentShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

        AddToStructChain(DeviceFeatures2, DeviceFragmentShadingRateFeatures);

        vkGetPhysicalDeviceFeatures2(PhysicalDeviceHandle, &DeviceFeatures2);

        // Query properties (tile size)
        VkPhysicalDeviceProperties2 DeviceProperties2 = {};
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceFragmentShadingRatePropertiesKHR DeviceFragmentShadingRateProperties = {};
        DeviceFragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

        AddToStructChain(DeviceProperties2, DeviceFragmentShadingRateProperties);

        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        if (DeviceFragmentShadingRateFeatures.attachmentFragmentShadingRate)
        {
            RHI::ShadingRateTier = EShadingRateTier::Tier2; // image-based
        }
        else if (DeviceFragmentShadingRateFeatures.pipelineFragmentShadingRate || DeviceFragmentShadingRateFeatures.primitiveFragmentShadingRate)
        {
            RHI::ShadingRateTier = EShadingRateTier::Tier1; // per-draw/per-primitive
        }
        else
        {
            RHI::ShadingRateTier = EShadingRateTier::NotSupported;
        }

        RHI::ShadingRateImageTileSize = Math::Max<uint32>(1u, DeviceFragmentShadingRateProperties.minFragmentShadingRateAttachmentTexelSize.width);
        RHI::bSupportsVRS             = RHI::ShadingRateTier != EShadingRateTier::NotSupported;
    }
    else
    {
        RHI::bSupportsVRS             = false;
        RHI::ShadingRateTier          = EShadingRateTier::NotSupported;
        RHI::ShadingRateImageTileSize = 0;
    }

    return true;
}
