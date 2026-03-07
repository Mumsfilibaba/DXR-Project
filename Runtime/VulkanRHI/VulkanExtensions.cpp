#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanExtensions.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanLoader.h"

#include <vulkan/vulkan_beta.h>

static void RegisterVulkanExtensions(FVulkanExtensionRegistry* Registry);

FVulkanExtensionRegistry::FVulkanExtensionRegistry()
{
    RegisterVulkanExtensions(this);
}

FVulkanExtensionRegistry::~FVulkanExtensionRegistry()
{
    for (FVulkanInstanceExtension* Extension : InstanceExtensions)
    {
        delete Extension;
    }

    for (FVulkanDeviceExtension* Extension : DeviceExtensions)
    {
        delete Extension;
    }
}

FVulkanExtensionRegistry& FVulkanExtensionRegistry::AddExtension(FVulkanInstanceExtension* Extension)
{
    InstanceExtensions.Add(Extension);
    return *this;
}

FVulkanExtensionRegistry& FVulkanExtensionRegistry::AddExtension(FVulkanDeviceExtension* Extension)
{
    DeviceExtensions.Add(Extension);
    return *this;
}

bool FVulkanExtensionRegistry::ResolveInstanceExtensions(const TArray<VkExtensionProperties>& Available, TArray<const CHAR*>& OutEnabledNames)
{
    for (FVulkanInstanceExtension* Extension : InstanceExtensions)
    {
        Extension->SetEnabled(false);

        if (!Extension->ShouldEnable())
        {
            continue;
        }

        for (const VkExtensionProperties& Property : Available)
        {
            if (FCString::Strcmp(Extension->GetName(), Property.extensionName) == 0)
            {
                Extension->SetEnabled(true);
                OutEnabledNames.Add(Property.extensionName);
                EnabledInstanceExtensionNames.Emplace(Property.extensionName);
                break;
            }
        }

        if (!Extension->IsEnabled() && Extension->IsRequired())
        {
            VULKAN_ERROR_CRITICAL("Required instance extension '%s' is not available", Extension->GetName());
            return false;
        }
    }

    return true;
}

bool FVulkanExtensionRegistry::ResolveDeviceExtensions(const TArray<VkExtensionProperties>& Available, TArray<const CHAR*>& OutEnabledNames)
{
    for (FVulkanDeviceExtension* Extension : DeviceExtensions)
    {
        Extension->SetEnabled(false);

        if (!Extension->ShouldEnable())
        {
            continue;
        }

        for (const VkExtensionProperties& Property : Available)
        {
            if (FCString::Strcmp(Extension->GetName(), Property.extensionName) == 0)
            {
                Extension->SetEnabled(true);
                OutEnabledNames.Add(Property.extensionName);
                EnabledDeviceExtensionNames.Emplace(Property.extensionName);
                break;
            }
        }

        if (!Extension->IsEnabled() && Extension->IsRequired())
        {
            VULKAN_ERROR_CRITICAL("Required device extension '%s' is not available", Extension->GetName());
            return false;
        }
    }

    return true;
}

bool FVulkanExtensionRegistry::LoadInstanceFunctions(FVulkanInstance* Instance)
{
    for (FVulkanInstanceExtension* Extension : InstanceExtensions)
    {
        if (Extension->IsEnabled())
        {
            if (!Extension->LoadFunctions(Instance))
            {
                VULKAN_ERROR_CRITICAL("Failed to load functions for instance extension '%s'", Extension->GetName());
                return false;
            }
        }
    }

    return true;
}

bool FVulkanExtensionRegistry::LoadDeviceFunctions(FVulkanDevice* Device)
{
    for (FVulkanDeviceExtension* Extension : DeviceExtensions)
    {
        if (Extension->IsEnabled())
        {
            if (!Extension->LoadFunctions(Device))
            {
                VULKAN_ERROR_CRITICAL("Failed to load functions for device extension '%s'", Extension->GetName());
                return false;
            }
        }
    }
    
    return true;
}

void FVulkanExtensionRegistry::BuildFeatureQueryChain(FVulkanStructChain& Chain)
{
    for (FVulkanDeviceExtension* Extension : DeviceExtensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->AddToFeatureQueryChain(Chain);
        }
    }
}

void FVulkanExtensionRegistry::BuildPropertyQueryChain(FVulkanStructChain& Chain)
{
    for (FVulkanDeviceExtension* Extension : DeviceExtensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->AddToPropertyQueryChain(Chain);
        }
    }
}

void FVulkanExtensionRegistry::ProcessQueriedFeatures()
{
    for (FVulkanDeviceExtension* Extension : DeviceExtensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->ProcessQueriedFeatures();
        }
    }
}

void FVulkanExtensionRegistry::BuildFeatureEnableChain(FVulkanStructChain& Chain)
{
    for (FVulkanDeviceExtension* Extension : DeviceExtensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->AddToFeatureEnableChain(Chain);
        }
    }
}

bool FVulkanExtensionRegistry::IsInstanceExtensionEnabled(const FString& Name) const
{
    return EnabledInstanceExtensionNames.Find(Name) != nullptr;
}

bool FVulkanExtensionRegistry::IsDeviceExtensionEnabled(const FString& Name) const
{
    return EnabledDeviceExtensionNames.Find(Name) != nullptr;
}

#if VK_KHR_surface
class FVulkanExtSurface : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_SURFACE_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanInstance* InInstance) override final
    {
        VkInstance Instance = InInstance->GetVkInstance();
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, DestroySurfaceKHR);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, GetPhysicalDeviceSurfaceFormatsKHR);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, GetPhysicalDeviceSurfacePresentModesKHR);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, GetPhysicalDeviceSurfaceSupportKHR);
        return true;
    }
};
#endif

#if VK_KHR_win32_surface
class FVulkanExtWin32Surface : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanInstance* InInstance) override final
    {
        VkInstance Instance = InInstance->GetVkInstance();
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CreateWin32SurfaceKHR);
        return true;
    }
};
#endif

#if VK_EXT_metal_surface
class FVulkanExtMetalSurface : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_METAL_SURFACE_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanInstance* InInstance) override final
    {
        VkInstance Instance = InInstance->GetVkInstance();
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CreateMetalSurfaceEXT);
        return true;
    }
};
#endif

#if VK_MVK_macos_surface
class FVulkanExtMacOSSurface : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanInstance* InInstance) override final
    {
        VkInstance Instance = InInstance->GetVkInstance();
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CreateMacOSSurfaceMVK);
        return true;
    }
};
#endif

#if VK_EXT_surface_maintenance1
class FVulkanExtSurfaceMaintenance1 : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME;
    }
};
#endif

#if VK_KHR_get_surface_capabilities2
class FVulkanExtGetSurfaceCapabilities2 : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
    }
};
#endif

#if VK_KHR_portability_enumeration
class FVulkanExtPortabilityEnumeration : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    }
};
#endif

// ---- VK_KHR_swapchain (Required) ----

#if VK_KHR_swapchain
class FVulkanExtSwapchain : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }
    virtual bool IsRequired() const override final { return true; }

    virtual bool LoadFunctions(FVulkanDevice* InDevice) override final
    {
        VkDevice Device = InDevice->GetVkDevice();
        VULKAN_LOAD_DEVICE_FUNCTION(Device, CreateSwapchainKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, DestroySwapchainKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, AcquireNextImageKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, QueuePresentKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, GetSwapchainImagesKHR);
        return true;
    }
};
#endif

// ---- VK_KHR_portability_subset ----

#if VK_KHR_portability_subset
class FVulkanExtPortabilitySubset : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
    }
};
#endif

// ---- VK_KHR_maintenance 5-9 ----

#if VK_KHR_maintenance5
class FVulkanExtMaintenance5 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_MAINTENANCE_5_EXTENSION_NAME;
    }
};
#endif

#if VK_KHR_maintenance6
class FVulkanExtMaintenance6 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_MAINTENANCE_6_EXTENSION_NAME;
    }
};
#endif

#if VK_KHR_maintenance7
class FVulkanExtMaintenance7 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_MAINTENANCE_7_EXTENSION_NAME;
    }
};
#endif

#if VK_KHR_maintenance8
class FVulkanExtMaintenance8 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_MAINTENANCE_8_EXTENSION_NAME;
    }
};
#endif

#if VK_KHR_maintenance9
class FVulkanExtMaintenance9 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_MAINTENANCE_9_EXTENSION_NAME;
    }
};
#endif

// ---- VK_KHR_deferred_host_operations ----

#if VK_KHR_deferred_host_operations
class FVulkanExtDeferredHostOperations : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME;
    }
};
#endif

// ---- VK_KHR_pipeline_library ----

#if VK_KHR_pipeline_library
class FVulkanExtPipelineLibrary : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME;
    }
};
#endif

// ---- VK_KHR_push_descriptor ----

#if VK_KHR_push_descriptor
class FVulkanExtPushDescriptor : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME;
    }
};
#endif

// ---- VK_KHR_acceleration_structure ----

#if VK_KHR_acceleration_structure
class FVulkanExtAccelerationStructure : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanDevice* InDevice) override final
    {
        VkDevice Device = InDevice->GetVkDevice();
        VULKAN_LOAD_DEVICE_FUNCTION(Device, CreateAccelerationStructureKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, DestroyAccelerationStructureKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, GetAccelerationStructureBuildSizesKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, GetAccelerationStructureDeviceAddressKHR);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, CmdBuildAccelerationStructuresKHR);
        return true;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.accelerationStructure == VK_TRUE)
        {
            GVulkanSupportsAccelerationStructures = true;
        }
    }

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!GVulkanSupportsAccelerationStructures)
        {
            return;
        }

        EnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        EnableFeatures.accelerationStructure = AvailableFeatures.accelerationStructure;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR AvailableFeatures = {};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR EnableFeatures    = {};
};
#endif

// ---- VK_KHR_ray_tracing_pipeline ----

#if VK_KHR_ray_tracing_pipeline
class FVulkanExtRayTracingPipeline : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.rayTracingPipeline)
        {
            GVulkanSupportsRayTracingPipeline = true;
        }
    }

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!GVulkanSupportsRayTracingPipeline)
        {
            return;
        }

        EnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        EnableFeatures.rayTracingPipeline = AvailableFeatures.rayTracingPipeline;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR AvailableFeatures = {};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR EnableFeatures    = {};
};
#endif

// ---- VK_KHR_ray_query ----

#if VK_KHR_ray_query
class FVulkanExtRayQuery : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_RAY_QUERY_EXTENSION_NAME;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.rayQuery == VK_TRUE)
        {
            GVulkanSupportsRayQuery = true;
        }
    }

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!GVulkanSupportsRayQuery)
        {
            return;
        }

        EnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        EnableFeatures.rayQuery = AvailableFeatures.rayQuery;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceRayQueryFeaturesKHR AvailableFeatures = {};
    VkPhysicalDeviceRayQueryFeaturesKHR EnableFeatures    = {};
};
#endif

// ---- VK_KHR_ray_tracing_maintenance1 ----

#if VK_KHR_ray_tracing_maintenance1
class FVulkanExtRayTracingMaintenance1 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME;
    }
};
#endif

// ---- VK_KHR_fragment_shading_rate ----

#if VK_KHR_fragment_shading_rate
class FVulkanExtFragmentShadingRate : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void AddToPropertyQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
        Chain.AddNext(AvailableProperties);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.pipelineFragmentShadingRate || AvailableFeatures.primitiveFragmentShadingRate ||
            AvailableFeatures.attachmentFragmentShadingRate)
        {
            GVulkanShadingRateTileSize = Math::Max<uint32>(1u, AvailableProperties.minFragmentShadingRateAttachmentTexelSize.width);
            GVulkanSupportsFragmentShadingRate = true;
        }
    }

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!GVulkanSupportsFragmentShadingRate)
        {
            return;
        }

        EnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        EnableFeatures.attachmentFragmentShadingRate = AvailableFeatures.attachmentFragmentShadingRate;
        EnableFeatures.pipelineFragmentShadingRate   = AvailableFeatures.pipelineFragmentShadingRate;
        EnableFeatures.primitiveFragmentShadingRate  = AvailableFeatures.primitiveFragmentShadingRate;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR   AvailableFeatures   = {};
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR   EnableFeatures      = {};
    VkPhysicalDeviceFragmentShadingRatePropertiesKHR AvailableProperties = {};
};
#endif

// ---- VK_EXT_memory_budget ----

#if VK_EXT_memory_budget
class FVulkanExtMemoryBudget : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
    }
};
#endif

// ---- VK_EXT_mesh_shader ----

#if VK_EXT_mesh_shader
class FVulkanExtMeshShader : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_MESH_SHADER_EXTENSION_NAME;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void AddToPropertyQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
        Chain.AddNext(AvailableProperties);
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

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!GVulkanSupportsMeshShaders)
        {
            return;
        }

        EnableFeatures.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        EnableFeatures.meshShader = AvailableFeatures.meshShader;
        EnableFeatures.taskShader = AvailableFeatures.taskShader;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceMeshShaderFeaturesEXT   AvailableFeatures   = {};
    VkPhysicalDeviceMeshShaderFeaturesEXT   EnableFeatures      = {};
    VkPhysicalDeviceMeshShaderPropertiesEXT AvailableProperties = {};
};
#endif

// ---- VK_EXT_descriptor_buffer ----

#if VK_EXT_descriptor_buffer
class FVulkanExtDescriptorBuffer : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME;
    }
    virtual bool ShouldEnable() const override final { return !GVulkanGPUAssistedValidationEnabled; }
};
#endif

// ---- VK_EXT_depth_clip_enable ----

#if VK_EXT_depth_clip_enable
class FVulkanExtDepthClipEnable : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.depthClipEnable)
        {
            GVulkanSupportsDepthClip = true;
        }
    }

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!GVulkanSupportsDepthClip)
        {
            return;
        }

        EnableFeatures.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
        EnableFeatures.depthClipEnable = AvailableFeatures.depthClipEnable;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceDepthClipEnableFeaturesEXT AvailableFeatures = {};
    VkPhysicalDeviceDepthClipEnableFeaturesEXT EnableFeatures    = {};
};
#endif

// ---- VK_EXT_conservative_rasterization ----

#if VK_EXT_conservative_rasterization
class FVulkanExtConservativeRasterization : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME;
    }

    virtual void ProcessQueriedFeatures() override final
    {
        GVulkanSupportsConservativeRasterization = true;
    }
};
#endif

// ---- VK_KHR_robustness2 ----

#if VK_KHR_robustness2
class FVulkanExtRobustness2 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_KHR_ROBUSTNESS_2_EXTENSION_NAME;
    }
    virtual bool ShouldEnable() const override final { return GVulkanAllowNullDescriptors; }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
        Chain.AddNext(AvailableFeatures);
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

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
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
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceRobustness2FeaturesKHR AvailableFeatures = {};
    VkPhysicalDeviceRobustness2FeaturesKHR EnableFeatures    = {};
};
#endif

// ---- VK_EXT_swapchain_maintenance1 ----

#if VK_EXT_swapchain_maintenance1
class FVulkanExtSwapchainMaintenance1 : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME;
    }
};
#endif

// ---- VK_EXT_sample_locations ----

#if VK_EXT_sample_locations
class FVulkanExtSampleLocations : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME;
    }

    virtual void AddToPropertyQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT;
        Chain.AddNext(AvailableProperties);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        GVulkanSupportsSampleLocations = true;
    }

    VkPhysicalDeviceSampleLocationsPropertiesEXT AvailableProperties = {};
};
#endif

// ---- VK_EXT_fragment_shader_interlock ----

#if VK_EXT_fragment_shader_interlock
class FVulkanExtFragmentShaderInterlock : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void ProcessQueriedFeatures() override final
    {
        if (AvailableFeatures.fragmentShaderSampleInterlock || AvailableFeatures.fragmentShaderPixelInterlock ||
            AvailableFeatures.fragmentShaderShadingRateInterlock)
        {
            GVulkanSupportsFragmentShaderInterlock = true;
        }
    }

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!GVulkanSupportsFragmentShaderInterlock)
        {
            return;
        }

        EnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
        EnableFeatures.fragmentShaderSampleInterlock      = AvailableFeatures.fragmentShaderSampleInterlock;
        EnableFeatures.fragmentShaderPixelInterlock       = AvailableFeatures.fragmentShaderPixelInterlock;
        EnableFeatures.fragmentShaderShadingRateInterlock = AvailableFeatures.fragmentShaderShadingRateInterlock;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT AvailableFeatures = {};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT EnableFeatures    = {};
};
#endif

// ---- VK_NV_ray_tracing_invocation_reorder ----

#if VK_NV_ray_tracing_invocation_reorder
class FVulkanExtRayTracingInvocationReorder : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME;
    }
};
#endif

// ---- VK_AMD_buffer_marker ----

#if VK_AMD_buffer_marker
class FVulkanExtAMDBufferMarker : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_AMD_BUFFER_MARKER_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanDevice* InDevice) override final
    {
        VkDevice Device = InDevice->GetVkDevice();
        VULKAN_LOAD_DEVICE_FUNCTION(Device, CmdWriteBufferMarkerAMD);
        return true;
    }
};
#endif

// ---- VK_NV_device_diagnostic_checkpoints ----

#if VK_NV_device_diagnostic_checkpoints
class FVulkanExtNVDiagnosticCheckpoints : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanDevice* InDevice) override final
    {
        VkDevice Device = InDevice->GetVkDevice();
        VULKAN_LOAD_DEVICE_FUNCTION(Device, CmdSetCheckpointNV);
        VULKAN_LOAD_DEVICE_FUNCTION(Device, GetQueueCheckpointDataNV);
        return true;
    }
};
#endif

// ---- VK_EXT_device_fault ----

#if VK_EXT_device_fault
class FVulkanExtDeviceFault : public FVulkanDeviceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_DEVICE_FAULT_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanDevice* InDevice) override final
    {
        VkDevice Device = InDevice->GetVkDevice();
        VULKAN_LOAD_DEVICE_FUNCTION(Device, GetDeviceFaultInfoEXT);
        return true;
    }

    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain) override final
    {
        AvailableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
        Chain.AddNext(AvailableFeatures);
    }

    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) override final
    {
        if (!AvailableFeatures.deviceFault)
        {
            return;
        }

        EnableFeatures.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
        EnableFeatures.deviceFault = VK_TRUE;
        Chain.AddNext(EnableFeatures);
    }

    VkPhysicalDeviceFaultFeaturesEXT AvailableFeatures = {};
    VkPhysicalDeviceFaultFeaturesEXT EnableFeatures    = {};
};
#endif

// ---- VK_EXT_debug_utils ----

#if VK_EXT_debug_utils
class FVulkanExtDebugUtils : public FVulkanInstanceExtension
{
public:
    virtual const CHAR* GetName() const override final
    {
        return VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    virtual bool LoadFunctions(FVulkanInstance* InInstance) override final
    {
        VkInstance Instance = InInstance->GetVkInstance();
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CreateDebugUtilsMessengerEXT);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, DestroyDebugUtilsMessengerEXT);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CmdInsertDebugUtilsLabelEXT);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CmdBeginDebugUtilsLabelEXT);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CmdEndDebugUtilsLabelEXT);
        VULKAN_LOAD_INSTANCE_FUNCTION(Instance, SetDebugUtilsObjectNameEXT);

        GVulkanSupportsDebugUtils = true;
        return true;
    }
};
#endif

static void RegisterVulkanExtensions(FVulkanExtensionRegistry* Registry)
{
    // Instance extensions
#if VK_EXT_debug_utils
    Registry->AddExtension(new FVulkanExtDebugUtils());
#endif
#if VK_KHR_surface
    Registry->AddExtension(new FVulkanExtSurface());
#endif
#if VK_KHR_win32_surface
    Registry->AddExtension(new FVulkanExtWin32Surface());
#endif
#if VK_EXT_metal_surface
    Registry->AddExtension(new FVulkanExtMetalSurface());
#endif
#if VK_MVK_macos_surface
    Registry->AddExtension(new FVulkanExtMacOSSurface());
#endif
#if VK_EXT_surface_maintenance1
    Registry->AddExtension(new FVulkanExtSurfaceMaintenance1());
#endif
#if VK_KHR_get_surface_capabilities2
    Registry->AddExtension(new FVulkanExtGetSurfaceCapabilities2());
#endif
#if VK_KHR_portability_enumeration
    Registry->AddExtension(new FVulkanExtPortabilityEnumeration());
#endif

    // Device extensions
#if VK_KHR_swapchain
    Registry->AddExtension(new FVulkanExtSwapchain());
#endif
#if VK_KHR_portability_subset
    Registry->AddExtension(new FVulkanExtPortabilitySubset());
#endif
#if VK_KHR_maintenance5
    Registry->AddExtension(new FVulkanExtMaintenance5());
#endif
#if VK_KHR_maintenance6
    Registry->AddExtension(new FVulkanExtMaintenance6());
#endif
#if VK_KHR_maintenance7
    Registry->AddExtension(new FVulkanExtMaintenance7());
#endif
#if VK_KHR_maintenance8
    Registry->AddExtension(new FVulkanExtMaintenance8());
#endif
#if VK_KHR_maintenance9
    Registry->AddExtension(new FVulkanExtMaintenance9());
#endif
#if VK_KHR_deferred_host_operations
    Registry->AddExtension(new FVulkanExtDeferredHostOperations());
#endif
#if VK_KHR_pipeline_library
    Registry->AddExtension(new FVulkanExtPipelineLibrary());
#endif
#if VK_KHR_push_descriptor
    Registry->AddExtension(new FVulkanExtPushDescriptor());
#endif
#if VK_KHR_acceleration_structure
    Registry->AddExtension(new FVulkanExtAccelerationStructure());
#endif
#if VK_KHR_ray_tracing_pipeline
    Registry->AddExtension(new FVulkanExtRayTracingPipeline());
#endif
#if VK_KHR_ray_query
    Registry->AddExtension(new FVulkanExtRayQuery());
#endif
#if VK_KHR_ray_tracing_maintenance1
    Registry->AddExtension(new FVulkanExtRayTracingMaintenance1());
#endif
#if VK_KHR_fragment_shading_rate
    Registry->AddExtension(new FVulkanExtFragmentShadingRate());
#endif
#if VK_EXT_memory_budget
    Registry->AddExtension(new FVulkanExtMemoryBudget());
#endif
#if VK_EXT_mesh_shader
    Registry->AddExtension(new FVulkanExtMeshShader());
#endif
#if VK_EXT_descriptor_buffer
    Registry->AddExtension(new FVulkanExtDescriptorBuffer());
#endif
#if VK_EXT_depth_clip_enable
    Registry->AddExtension(new FVulkanExtDepthClipEnable());
#endif
#if VK_EXT_conservative_rasterization
    Registry->AddExtension(new FVulkanExtConservativeRasterization());
#endif
#if VK_KHR_robustness2
    Registry->AddExtension(new FVulkanExtRobustness2());
#endif
#if VK_EXT_swapchain_maintenance1
    Registry->AddExtension(new FVulkanExtSwapchainMaintenance1());
#endif
#if VK_EXT_sample_locations
    Registry->AddExtension(new FVulkanExtSampleLocations());
#endif
#if VK_EXT_fragment_shader_interlock
    Registry->AddExtension(new FVulkanExtFragmentShaderInterlock());
#endif
#if VK_NV_ray_tracing_invocation_reorder
    Registry->AddExtension(new FVulkanExtRayTracingInvocationReorder());
#endif
#if VK_AMD_buffer_marker
    Registry->AddExtension(new FVulkanExtAMDBufferMarker());
#endif
#if VK_NV_device_diagnostic_checkpoints
    Registry->AddExtension(new FVulkanExtNVDiagnosticCheckpoints());
#endif
#if VK_EXT_device_fault
    Registry->AddExtension(new FVulkanExtDeviceFault());
#endif
}
