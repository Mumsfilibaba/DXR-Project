#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanExtensions.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/Platform/VulkanPlatform.h"

#include <vulkan/vulkan_beta.h>

// ---- Complex instance extension classes ----

#if VK_EXT_validation_features
class FVulkanEXTValidationFeaturesExtension : public FVulkanInstanceExtension
{
public:
    FVulkanEXTValidationFeaturesExtension()
        : FVulkanInstanceExtension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME, false, GVulkanGPUAssistedValidationEnabled)
    {
    }

    virtual void PrepareInstanceCreateInfo(VkInstanceCreateInfo& OutInstanceCreateInfo) override final
    {
        if (!GVulkanGPUAssistedValidationEnabled)
        {
            return;
        }

        ValidationFeatures.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        ValidationFeatures.enabledValidationFeatureCount = ARRAY_COUNT(GPUAVEnables);
        ValidationFeatures.pEnabledValidationFeatures    = GPUAVEnables;
        AddToStructChain(OutInstanceCreateInfo, ValidationFeatures);
    }

    VkValidationFeatureEnableEXT GPUAVEnables[2] =
    {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT
    };

    VkValidationFeaturesEXT ValidationFeatures = {};
};
#endif

// ---- Complex device extension classes ----

#if VK_KHR_acceleration_structure
class FVulkanKHRAccelerationStructureExtension : public FVulkanDeviceExtension
{
public:
    FVulkanKHRAccelerationStructureExtension()
        : FVulkanDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.accelerationStructure == VK_TRUE)
        {
            GVulkanSupportsAccelerationStructures = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsAccelerationStructures)
        {
            return;
        }

        EnableFeatures.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        EnableFeatures.accelerationStructure = AvailableFeatures.accelerationStructure;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR EnableFeatures    = {};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR AvailableFeatures = {};
};
#endif

#if VK_KHR_ray_tracing_pipeline
class FVulkanKHRRayTracingPipelineExtension : public FVulkanDeviceExtension
{
public:
    FVulkanKHRRayTracingPipelineExtension()
        : FVulkanDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.rayTracingPipeline)
        {
            GVulkanSupportsRayTracingPipeline = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsRayTracingPipeline)
        {
            return;
        }

        EnableFeatures.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        EnableFeatures.rayTracingPipeline = AvailableFeatures.rayTracingPipeline;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR EnableFeatures    = {};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR AvailableFeatures = {};
};
#endif

#if VK_KHR_ray_query
class FVulkanKHRRayQueryExtension : public FVulkanDeviceExtension
{
public:
    FVulkanKHRRayQueryExtension()
        : FVulkanDeviceExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.rayQuery == VK_TRUE)
        {
            GVulkanSupportsRayQuery = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsRayQuery)
        {
            return;
        }

        EnableFeatures.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        EnableFeatures.rayQuery = AvailableFeatures.rayQuery;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceRayQueryFeaturesKHR EnableFeatures    = {};
    VkPhysicalDeviceRayQueryFeaturesKHR AvailableFeatures = {};
};
#endif

#if VK_KHR_fragment_shading_rate
class FVulkanKHRFragmentShadingRateExtension : public FVulkanDeviceExtension
{
public:
    FVulkanKHRFragmentShadingRateExtension()
        : FVulkanDeviceExtension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void PrepareDeviceProperties(VkPhysicalDeviceProperties2& OutProperties) override final
    {
        AvailableProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
        AddToStructChain(OutProperties, AvailableProperties);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.pipelineFragmentShadingRate || AvailableFeatures.primitiveFragmentShadingRate ||
            AvailableFeatures.attachmentFragmentShadingRate)
        {
            GVulkanShadingRateTileSize         = Math::Max<uint32>(1u, AvailableProperties.minFragmentShadingRateAttachmentTexelSize.width);
            GVulkanSupportsFragmentShadingRate = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsFragmentShadingRate)
        {
            return;
        }

        EnableFeatures.sType                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        EnableFeatures.attachmentFragmentShadingRate = AvailableFeatures.attachmentFragmentShadingRate;
        EnableFeatures.pipelineFragmentShadingRate   = AvailableFeatures.pipelineFragmentShadingRate;
        EnableFeatures.primitiveFragmentShadingRate  = AvailableFeatures.primitiveFragmentShadingRate;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR   AvailableFeatures   = {};
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR   EnableFeatures      = {};
    VkPhysicalDeviceFragmentShadingRatePropertiesKHR AvailableProperties = {};
};
#endif

#if VK_EXT_mesh_shader
class FVulkanEXTMeshShaderExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTMeshShaderExtension()
        : FVulkanDeviceExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void PrepareDeviceProperties(VkPhysicalDeviceProperties2& OutProperties) override final
    {
        AvailableProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
        AddToStructChain(OutProperties, AvailableProperties);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.meshShader || AvailableFeatures.taskShader)
        {
            GVulkanMaxMeshOutputVertices       = AvailableProperties.maxMeshOutputVertices;
            GVulkanMaxMeshWorkGroupInvocations = AvailableProperties.maxMeshWorkGroupInvocations;
            GVulkanMaxTaskWorkGroupInvocations = AvailableProperties.maxTaskWorkGroupInvocations;
            GVulkanSupportsMeshShaders         = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsMeshShaders)
        {
            return;
        }

        EnableFeatures.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        EnableFeatures.meshShader = AvailableFeatures.meshShader;
        EnableFeatures.taskShader = AvailableFeatures.taskShader;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceMeshShaderFeaturesEXT   AvailableFeatures   = {};
    VkPhysicalDeviceMeshShaderFeaturesEXT   EnableFeatures      = {};
    VkPhysicalDeviceMeshShaderPropertiesEXT AvailableProperties = {};
};
#endif

#if VK_EXT_depth_clip_enable
class FVulkanEXTDepthClipEnableExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTDepthClipEnableExtension()
        : FVulkanDeviceExtension(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.depthClipEnable)
        {
            GVulkanSupportsDepthClip = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsDepthClip)
        {
            return;
        }

        EnableFeatures.sType           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
        EnableFeatures.depthClipEnable = AvailableFeatures.depthClipEnable;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceDepthClipEnableFeaturesEXT EnableFeatures    = {};
    VkPhysicalDeviceDepthClipEnableFeaturesEXT AvailableFeatures = {};
};
#endif

#if VK_EXT_conservative_rasterization
class FVulkanEXTConservativeRasterizationExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTConservativeRasterizationExtension()
        : FVulkanDeviceExtension(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceProperties(VkPhysicalDeviceProperties2& OutProperties) override final
    {
        AvailableProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
        AddToStructChain(OutProperties, AvailableProperties);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        GVulkanSupportsConservativeRasterization  = true;
        GVulkanMaxExtraPrimitiveOverestimationSize = AvailableProperties.maxExtraPrimitiveOverestimationSize;
    }

    VkPhysicalDeviceConservativeRasterizationPropertiesEXT AvailableProperties = {};
};
#endif

#if VK_KHR_robustness2
class FVulkanKHRRobustness2Extension : public FVulkanDeviceExtension
{
public:
    FVulkanKHRRobustness2Extension()
        : FVulkanDeviceExtension(VK_KHR_ROBUSTNESS_2_EXTENSION_NAME, false, GVulkanAllowNullDescriptors)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.nullDescriptor)
        {
            GVulkanSupportsNullDescriptors = true;
        }

        VULKAN_INFO("Robustness: robustBufferAccess=%s, nullDescriptor=%s, robustBufferAccess2=%s, robustImageAccess2=%s",
            GVulkanRobustBufferAccessEnabled ? "ON" : "OFF",
            GVulkanSupportsNullDescriptors   ? "ON" : "OFF",
            (AvailableFeatures.robustBufferAccess2 && GVulkanRobustBufferAccessEnabled) ? "ON" : "OFF",
            AvailableFeatures.robustImageAccess2 ? "ON" : "OFF");

        if (!GVulkanSupportsNullDescriptors)
        {
            VULKAN_WARNING("nullDescriptor not supported - unbound descriptors will use a %u-byte fallback buffer (OOB risk)", VULKAN_DEFAULT_BUFFER_NUM_BYTES);
        }

        GVulkanSupportsRobustness2 = true;
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        EnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;

        if (AvailableFeatures.robustImageAccess2)
        {
            EnableFeatures.robustImageAccess2 = VK_TRUE;
        }
        
        if (AvailableFeatures.robustBufferAccess2 && GVulkanRobustBufferAccessEnabled)
        {
            EnableFeatures.robustBufferAccess2 = VK_TRUE;
        }
        
        if (AvailableFeatures.nullDescriptor)
        {
            EnableFeatures.nullDescriptor = VK_TRUE;
        }

        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceRobustness2FeaturesKHR AvailableFeatures = {};
    VkPhysicalDeviceRobustness2FeaturesKHR EnableFeatures    = {};
};
#endif

#if VK_EXT_sample_locations
class FVulkanEXTSampleLocationsExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTSampleLocationsExtension()
        : FVulkanDeviceExtension(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceProperties(VkPhysicalDeviceProperties2& OutProperties) override final
    {
        AvailableProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT;
        AddToStructChain(OutProperties, AvailableProperties);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        GVulkanSupportsSampleLocations = true;
    }

    VkPhysicalDeviceSampleLocationsPropertiesEXT AvailableProperties = {};
};
#endif

#if VK_EXT_fragment_shader_interlock
class FVulkanEXTFragmentShaderInterlockExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTFragmentShaderInterlockExtension()
        : FVulkanDeviceExtension(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.fragmentShaderSampleInterlock || AvailableFeatures.fragmentShaderPixelInterlock ||
            AvailableFeatures.fragmentShaderShadingRateInterlock)
        {
            GVulkanSupportsFragmentShaderInterlock = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsFragmentShaderInterlock)
        {
            return;
        }

        EnableFeatures.sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
        EnableFeatures.fragmentShaderSampleInterlock      = AvailableFeatures.fragmentShaderSampleInterlock;
        EnableFeatures.fragmentShaderPixelInterlock       = AvailableFeatures.fragmentShaderPixelInterlock;
        EnableFeatures.fragmentShaderShadingRateInterlock = AvailableFeatures.fragmentShaderShadingRateInterlock;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT EnableFeatures    = {};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT AvailableFeatures = {};
};
#endif

#if VK_EXT_transform_feedback
class FVulkanEXTTransformFeedbackExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTTransformFeedbackExtension()
        : FVulkanDeviceExtension(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.transformFeedback)
        {
            GVulkanSupportsTransformFeedback = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsTransformFeedback)
        {
            return;
        }

        EnableFeatures.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
        EnableFeatures.transformFeedback = VK_TRUE;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceTransformFeedbackFeaturesEXT EnableFeatures    = {};
    VkPhysicalDeviceTransformFeedbackFeaturesEXT AvailableFeatures = {};
};
#endif

#if VK_EXT_mutable_descriptor_type
class FVulkanEXTMutableDescriptorTypeExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTMutableDescriptorTypeExtension()
        : FVulkanDeviceExtension(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.mutableDescriptorType == VK_TRUE)
        {
            GVulkanSupportsMutableDescriptorType = true;
        }
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!GVulkanSupportsMutableDescriptorType)
        {
            return;
        }

        EnableFeatures.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
        EnableFeatures.mutableDescriptorType = VK_TRUE;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT EnableFeatures    = {};
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT AvailableFeatures = {};
};
#endif

#if VK_EXT_device_fault
class FVulkanEXTDeviceFaultExtension : public FVulkanDeviceExtension
{
public:
    FVulkanEXTDeviceFaultExtension()
        : FVulkanDeviceExtension(VK_EXT_DEVICE_FAULT_EXTENSION_NAME, false, true)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2& OutFeatures) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
        AddToStructChain(OutFeatures, AvailableFeatures);
    }

    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo& OutDeviceCreateInfo) override final
    {
        if (!AvailableFeatures.deviceFault)
        {
            return;
        }

        EnableFeatures.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
        EnableFeatures.deviceFault = VK_TRUE;
        AddToStructChain(OutDeviceCreateInfo, EnableFeatures);
    }

    VkPhysicalDeviceFaultFeaturesEXT EnableFeatures    = {};
    VkPhysicalDeviceFaultFeaturesEXT AvailableFeatures = {};
};
#endif

// ---- Extension registration ----

void FVulkanInstanceExtension::RegisterExtensions(TArray<TUniquePtr<FVulkanInstanceExtension>>& OutExtensions)
{
#if VK_EXT_validation_features && VULKAN_ENABLE_GPU_VALIDATION
    {
        bool bEnableDebugLayer = false;
        if (IConsoleVariable* Var = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
        {
            bEnableDebugLayer = Var->GetBool();
        }

        if (bEnableDebugLayer)
        {
            if (IConsoleVariable* Var = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.EnableGPUAssistedValidation"))
            {
                GVulkanGPUAssistedValidationEnabled = Var->GetBool();
            }
        }
    }

    if (GVulkanGPUAssistedValidationEnabled)
    {
        VULKAN_INFO("GPU-Assisted Validation enabled - VK_EXT_descriptor_buffer will be disabled");
    }
#endif

#if VK_EXT_debug_utils
    OutExtensions.Add(MakeUniquePtr<FVulkanInstanceExtension>(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false, true));
#endif
#if VK_EXT_validation_features
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTValidationFeaturesExtension>());
#endif
#if VK_EXT_surface_maintenance1
    OutExtensions.Add(MakeUniquePtr<FVulkanInstanceExtension>(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_get_surface_capabilities2
    OutExtensions.Add(MakeUniquePtr<FVulkanInstanceExtension>(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, false, true));
#endif
}

void FVulkanDeviceExtension::RegisterExtensions(TArray<TUniquePtr<FVulkanDeviceExtension>>& OutExtensions)
{
#if !VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH && VK_KHR_dynamic_rendering
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, /*bRequired=*/true, /*bShouldEnable=*/true));
#endif
#if VK_KHR_portability_subset
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_maintenance5
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_MAINTENANCE_5_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_maintenance6
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_MAINTENANCE_6_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_maintenance7
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_MAINTENANCE_7_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_maintenance8
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_MAINTENANCE_8_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_maintenance9
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_MAINTENANCE_9_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_deferred_host_operations
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_pipeline_library
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_push_descriptor
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, false, true));
#endif
#if VK_KHR_ray_tracing_maintenance1
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME, false, true));
#endif
#if VK_EXT_memory_budget
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, false, true));
#endif
#if VK_EXT_descriptor_buffer
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME, false, !GVulkanGPUAssistedValidationEnabled));
#endif
#if VK_EXT_swapchain_maintenance1
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME, false, true));
#endif
#if VK_NV_ray_tracing_invocation_reorder
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME, false, true));
#endif
#if VK_AMD_buffer_marker
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_AMD_BUFFER_MARKER_EXTENSION_NAME, false, true));
#endif
#if VK_NV_device_diagnostic_checkpoints
    OutExtensions.Add(MakeUniquePtr<FVulkanDeviceExtension>(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME, false, true));
#endif

#if VK_KHR_acceleration_structure
    OutExtensions.Add(MakeUniquePtr<FVulkanKHRAccelerationStructureExtension>());
#endif
#if VK_KHR_ray_tracing_pipeline
    OutExtensions.Add(MakeUniquePtr<FVulkanKHRRayTracingPipelineExtension>());
#endif
#if VK_KHR_ray_query
    OutExtensions.Add(MakeUniquePtr<FVulkanKHRRayQueryExtension>());
#endif
#if VK_KHR_fragment_shading_rate
    OutExtensions.Add(MakeUniquePtr<FVulkanKHRFragmentShadingRateExtension>());
#endif
#if VK_EXT_mesh_shader
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTMeshShaderExtension>());
#endif
#if VK_EXT_depth_clip_enable
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTDepthClipEnableExtension>());
#endif
#if VK_EXT_conservative_rasterization
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTConservativeRasterizationExtension>());
#endif
#if VK_KHR_robustness2
    OutExtensions.Add(MakeUniquePtr<FVulkanKHRRobustness2Extension>());
#endif
#if VK_EXT_sample_locations
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTSampleLocationsExtension>());
#endif
#if VK_EXT_fragment_shader_interlock
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTFragmentShaderInterlockExtension>());
#endif
#if VK_EXT_transform_feedback
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTTransformFeedbackExtension>());
#endif
#if VK_EXT_mutable_descriptor_type
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTMutableDescriptorTypeExtension>());
#endif
#if VK_EXT_device_fault
    OutExtensions.Add(MakeUniquePtr<FVulkanEXTDeviceFaultExtension>());
#endif
}
