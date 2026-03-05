#include "Core/Containers/Array.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanDeviceDebug.h"

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
VULKANRHI_API bool   GVulkanSupportsNullDescriptors             = false;
VULKANRHI_API bool   GVulkanSupportsConservativeRasterization   = false;
VULKANRHI_API bool   GVulkanSupportsPipelineCacheControl        = false;
VULKANRHI_API bool   GVulkanSupportsMultiviews                  = false;
VULKANRHI_API bool   GVulkanSupportsBindless                    = false;
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
// Ray Tracing (VK_KHR_ray_tracing_pipeline, VK_KHR_ray_query)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsRayTracingPipeline     = false;
VULKANRHI_API bool GVulkanSupportsRayQuery               = false;
VULKANRHI_API bool GVulkanSupportsAccelerationStructures = false;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading (VK_KHR_fragment_shading_rate)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanSupportsFragmentShadingRate = false;
VULKANRHI_API uint32 GVulkanShadingRateTileSize         = 0;

// -------------------------------------------------------------------------------------------
// Mesh Shaders (VK_EXT_mesh_shader)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanSupportsMeshShaders         = false;
VULKANRHI_API uint32 GVulkanMaxMeshOutputVertices       = 0;
VULKANRHI_API uint32 GVulkanMaxMeshWorkGroupInvocations = 0;
VULKANRHI_API uint32 GVulkanMaxTaskWorkGroupInvocations = 0;

// -------------------------------------------------------------------------------------------
// Dynamic Rendering (VK_KHR_dynamic_rendering / Vulkan 1.3)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanUseDynamicRendering = true;

// -------------------------------------------------------------------------------------------
// Descriptor Set Management
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanUseDescriptorCache = true;

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

VULKANRHI_API uint32 GVulkanMaxDescriptorSetSamplers       = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetSampledImages  = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageImages  = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetUniformBuffers = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageBuffers = 0;

// -------------------------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------------------------

static bool FilterExtensions(const VkExtensionProperties& ExtensionProperty)
{
    if (!GVulkanAllowNullDescriptors && FCString::Strcmp(ExtensionProperty.extensionName, VK_KHR_ROBUSTNESS_2_EXTENSION_NAME) == 0)
    {
        return false;
    }
#if VK_EXT_descriptor_buffer
    if (GVulkanGPUAssistedValidationEnabled && FCString::Strcmp(ExtensionProperty.extensionName, VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME) == 0)
    {
        return false;
    }
#endif
    return true;
}

template <typename FeatureStructType>
static bool CheckRequiredFeatures(const FeatureStructType& RequiredFeatures, const FeatureStructType& AvailableFeatures, const char* StructName, const char* DeviceName)
{
    TVulkanFeatureView<const FeatureStructType> RequiredFeaturesView(RequiredFeatures);
    TVulkanFeatureView<const FeatureStructType> AvailableFeaturesView(AvailableFeatures);

	for (SIZE_T i = 0; i < RequiredFeaturesView.Size(); ++i)
	{
		if (RequiredFeaturesView[i] == VK_TRUE && AvailableFeaturesView[i] != VK_TRUE)
		{
			VULKAN_WARNING("PhysicalDevice '%s' does not support all device-features. See %s[%llu]", DeviceName, StructName, static_cast<uint64>(i));
			return false;
		}
	}

	return true;
}

static bool CheckAvailability(VkPhysicalDevice PhysicalDevice, const FVulkanDeviceCreateInfo& DeviceCreateInfo)
{
	VkPhysicalDeviceProperties AdapterProperties = {};
	vkGetPhysicalDeviceProperties(PhysicalDevice, &AdapterProperties);

	VkPhysicalDeviceFeatures2 DeviceFeatures2 = {};
	DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	VkPhysicalDeviceVulkan11Features DeviceFeatures11 = {};
	DeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	VkPhysicalDeviceVulkan12Features DeviceFeatures12 = {};
	DeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	VkPhysicalDeviceVulkan13Features DeviceFeatures13 = {};
	DeviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

	FVulkanStructChain FeatureChain(DeviceFeatures2);
	FeatureChain.AddNext(DeviceFeatures11);
	FeatureChain.AddNext(DeviceFeatures12);
	FeatureChain.AddNext(DeviceFeatures13);

	vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);

	// Vulkan 1.0
	if (!CheckRequiredFeatures(DeviceCreateInfo.RequiredFeatures, DeviceFeatures2.features, "VkPhysicalDeviceFeatures", AdapterProperties.deviceName))
	{
		return false;
	}

	// Vulkan 1.1
	if (!CheckRequiredFeatures(DeviceCreateInfo.RequiredFeatures11, DeviceFeatures11, "VkPhysicalDeviceVulkan11Features", AdapterProperties.deviceName))
	{
		return false;
	}

	// Vulkan 1.2
	if (!CheckRequiredFeatures(DeviceCreateInfo.RequiredFeatures12, DeviceFeatures12, "VkPhysicalDeviceVulkan12Features", AdapterProperties.deviceName))
	{
		return false;
	}

	// Vulkan 1.3
	if (!CheckRequiredFeatures(DeviceCreateInfo.RequiredFeatures13, DeviceFeatures13, "VkPhysicalDeviceVulkan13Features", AdapterProperties.deviceName))
	{
		return false;
	}

	return true;
}

template <typename FeatureStructType>
static void EnableOptionalFeatures(FeatureStructType& EnableFeatures, const FeatureStructType& OptionalFeatures, const FeatureStructType& AvailableFeatures)
{
    TVulkanFeatureView<FeatureStructType> EnableFeaturesView(EnableFeatures);

    TVulkanFeatureView<const FeatureStructType> OptionalFeaturesView(OptionalFeatures);
	TVulkanFeatureView<const FeatureStructType> AvailableFeaturesView(AvailableFeatures);

	for (SIZE_T i = 0; i < AvailableFeaturesView.Size(); ++i)
	{
		if (OptionalFeaturesView[i] == VK_TRUE && AvailableFeaturesView[i] != VK_TRUE)
		{
            EnableFeaturesView[i] = VK_TRUE;
		}
	}
}

static FString GetQueuePropertiesAsString(const VkQueueFamilyProperties& Properties)
{
    FString PropertyString = "QueueCount=" + TTypeToString<int32>::ToString(Properties.queueCount) + ", QueueBits=(";
    if (Properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
        PropertyString += "GRAPHICS | ";
    }
    if (Properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        PropertyString += "COMPUTE | ";
    }
    if (Properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        PropertyString += "COPY | ";
    }

    PropertyString.Pop();
    PropertyString.Pop();
    PropertyString.Pop();
    PropertyString += ')';
    return PropertyString;
}

FVulkanPhysicalDevice::FVulkanPhysicalDevice(FVulkanInstance* InInstance)
    : Instance(InInstance)
    , PhysicalDevice(VK_NULL_HANDLE)
{
}

FVulkanPhysicalDevice::~FVulkanPhysicalDevice()
{
    Instance       = nullptr;
    PhysicalDevice = VK_NULL_HANDLE;
}

bool FVulkanPhysicalDevice::Initialize(const FVulkanDeviceCreateInfo& InDeviceCreateInfo)
{
    VkResult Result = VK_SUCCESS;

    uint32 AdapterCount = 0;
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve AdapterCount");
        return false;
    }

    TArray<VkPhysicalDevice> Adapters(AdapterCount);
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, Adapters.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve available Adapters");
        return false;
    }

    if (AdapterCount < 1)
    {
        VULKAN_ERROR_CRITICAL("No Adapters available");
        return false;
    }

    IConsoleVariable* CVarVerboseLogging = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging");

    const bool bVerboseLogging = CVarVerboseLogging && CVarVerboseLogging->GetBool();
    if (bVerboseLogging)
    {
        VULKAN_INFO("Available adapters:");

        for (VkPhysicalDevice CurrentAdapter : Adapters)
        {
            VkPhysicalDeviceProperties AdapterProperties;
            vkGetPhysicalDeviceProperties(CurrentAdapter, &AdapterProperties);
            LOG_INFO("    '%s' Supports Vulkan '%s'", AdapterProperties.deviceName, *GetVersionAsString(AdapterProperties.apiVersion));
        }
    }

    // GPU selection
    TArray<VkPhysicalDevice> AcceptedAdapers;
    AcceptedAdapers.Reserve(Adapters.Size());

    TArray<VkPhysicalDevice> DiscreteAdapers;
    DiscreteAdapers.Reserve(Adapters.Size());

    for (VkPhysicalDevice CurrentAdapter : Adapters)
    {
        VkPhysicalDeviceProperties AdapterProperties;
        vkGetPhysicalDeviceProperties(CurrentAdapter, &AdapterProperties);

        if (AdapterProperties.apiVersion < VK_API_VERSION_1_3)
        {
            VULKAN_INFO("Skipping device '%s' since it's api-version is below Vulkan 1.3 (apiVersion=%s)", AdapterProperties.deviceName, *GetVersionAsString(AdapterProperties.apiVersion));
            continue;
        }

        if (!CheckAvailability(CurrentAdapter, InDeviceCreateInfo))
        {
            continue;
        }

        // Find indices for queue-families
        TOptional<FVulkanQueueFamilyIndices> QueueIndices = GetQueueFamilyIndices(CurrentAdapter);
        if (!QueueIndices)
        {
            VULKAN_WARNING("PhysicalDevice '%s' does not support all required QueueFamilies", AdapterProperties.deviceName);
            continue;
        }

        // Check if required extension for device is supported
        uint32 DeviceExtensionCount = 0;
        Result = vkEnumerateDeviceExtensionProperties(CurrentAdapter, nullptr, &DeviceExtensionCount, nullptr);
        
        if (VULKAN_FAILED(Result))
        {
            continue;
        }

        TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
        Result = vkEnumerateDeviceExtensionProperties(CurrentAdapter, nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
        
        if (VULKAN_FAILED(Result))
        {
            continue;
        }

        if (bVerboseLogging)
        {
            VULKAN_INFO("The adapter '%s' has the following available extensions (NumExtensions=%d):", AdapterProperties.deviceName, AvailableDeviceExtensions.Size());
            for (const VkExtensionProperties& Extension : AvailableDeviceExtensions)
            {
                LOG_INFO("    '%s'", Extension.extensionName);
            }
        }
        
        // Verify the required extensions
        bool bIsAllExtensionsSupported = true;
        for (const CHAR* RequiredExtension : InDeviceCreateInfo.RequiredExtensionNames)
        {
            bool bIsSupported = false;
            for (const VkExtensionProperties& Extension : AvailableDeviceExtensions)
            {
                if (FCString::Strcmp(Extension.extensionName, RequiredExtension) == 0)
                {
                    bIsSupported = true;
                    break;
                }
            }

            if (!bIsSupported)
            {
                bIsAllExtensionsSupported = false;
                VULKAN_WARNING("Required Device Extension '%s' is not supported by '%s'", RequiredExtension, AdapterProperties.deviceName);
                break;
            }
        }

        // NOTE: At this point we now the device is acceptable, now check for the most optional
        if (bIsAllExtensionsSupported)
        {
            AcceptedAdapers.Add(CurrentAdapter);
            if (AdapterProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                DiscreteAdapers.Add(CurrentAdapter);
            }
        }
    }

    // Select the first discrete-adapter we found
    if (!DiscreteAdapers.IsEmpty())
    {
        PhysicalDevice = DiscreteAdapers[0];
    }
    
    // If no discrete adapter is found...
    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        if (AcceptedAdapers.IsEmpty())
        {
            // ... we failed to find a suitable adapter
            VULKAN_ERROR_CRITICAL("Failed to find a suitable PhysicalDevice");
            return false;
        }
        else
        {
            // ... select the first accepted one
            PhysicalDevice = AcceptedAdapers[0];
        }
    }
    
    // Retrieve and cache information about the physical-device
    vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);
    vkGetPhysicalDeviceFeatures(PhysicalDevice, &DeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &DeviceMemoryProperties);

    // Get Get Physical Device Properties
    FMemory::Memzero(&DeviceProperties2);
    DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    
#if VK_EXT_conservative_rasterization
    FMemory::Memzero(&ConservativeRasterizationProperties);
    ConservativeRasterizationProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
#endif

    // Helper for checking for extensions
    FVulkanStructChain DevicePropertiesChain(DeviceProperties2);
#if VK_EXT_conservative_rasterization
    DevicePropertiesChain.AddNext(ConservativeRasterizationProperties);
#endif

    vkGetPhysicalDeviceProperties2(PhysicalDevice, &DeviceProperties2);

    // Get Physical Device Feature (For Vulkan 1.1 and Vulkan 1.2 and extensions)
    FMemory::Memzero(&DeviceFeatures2);
    DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Vulkan 1.1 features
    FMemory::Memzero(&DeviceFeatures11);
    DeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    // Vulkan 1.2 features
    FMemory::Memzero(&DeviceFeatures12);
    DeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    
    // Helper for checking for extensions
    FVulkanStructChain DeviceFeaturesChain(DeviceFeatures2);
    DeviceFeaturesChain.AddNext(DeviceFeatures11);
    DeviceFeaturesChain.AddNext(DeviceFeatures12);

    // Get the physical device features
    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);

    // Get Physical Device Memory Properties
    FMemory::Memzero(&DeviceMemoryProperties2);
    DeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

    vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice, &DeviceMemoryProperties2);
    
    VULKAN_INFO("Using adapter '%s' Which supports Vulkan '%s'", DeviceProperties.deviceName, *GetVersionAsString(DeviceProperties.apiVersion));
    return true;
}

TOptional<FVulkanQueueFamilyIndices> FVulkanPhysicalDevice::GetQueueFamilyIndices(VkPhysicalDevice PhysicalDevice)
{
    const auto GetQueueFamilyIndex = [](VkQueueFlagBits QueueFlag, const TArray<VkQueueFamilyProperties>& QueueFamilies) -> uint32
    {
        if (QueueFlag == VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32 QueueIndex = 0; QueueIndex < uint32(QueueFamilies.Size()); ++QueueIndex)
            {
                const VkQueueFamilyProperties& Properties = QueueFamilies[QueueIndex];
                if (Properties.queueCount > 0)
                {
                    const uint32 CurrentQueueFlags = Properties.queueFlags;
                    if ((CurrentQueueFlags & QueueFlag) && ((CurrentQueueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                    {
                        return QueueIndex;
                    }
                }
            }
        }

        if (QueueFlag == VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32 QueueIndex = 0; QueueIndex < uint32(QueueFamilies.Size()); ++QueueIndex)
            {
                const VkQueueFamilyProperties& Properties = QueueFamilies[QueueIndex];
                if (Properties.queueCount > 0)
                {
                    const uint32 CurrentQueueFlags = Properties.queueFlags;
                    if ((CurrentQueueFlags & QueueFlag) && ((CurrentQueueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0))
                    {
                        return QueueIndex;
                    }
                }
            }
        }

        for (uint32 QueueIndex = 0; QueueIndex < uint32(QueueFamilies.Size()); ++QueueIndex)
        {
            const VkQueueFamilyProperties& Properties = QueueFamilies[QueueIndex];
            if (Properties.queueCount > 0)
            {
                if (Properties.queueFlags & QueueFlag)
                {
                    return QueueIndex;
                }
            }
        }

        return uint32(~0u);
    };

    // Retrieve the queue indices for the adapter
    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;

    uint32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);

    TArray<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.Data());
    
    IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging");
    if (VerboseVulkan && VerboseVulkan->GetBool())
    {
        uint32 Index = 0;
        for (const VkQueueFamilyProperties& Properties : QueueFamilies)
        {
            const FString PropertyString = GetQueuePropertiesAsString(Properties);
            VULKAN_INFO("Queue[%d]: %s", Index, *PropertyString);
            Index++;
        }
    }
    
    const uint32 GraphicsIndex = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, QueueFamilies);
    if (GraphicsIndex == (~0u))
    {
        return QueueIndicies;
    }

    QueueFamilies[GraphicsIndex].queueCount--;
    
    const uint32 CopyIndex = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, QueueFamilies);
    if (CopyIndex == (~0u))
    {
        return QueueIndicies;
    }
    
    QueueFamilies[CopyIndex].queueCount--;
    
    const uint32 ComputeIndex = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT , QueueFamilies);
    if (ComputeIndex == (~0u))
    {
        return QueueIndicies;
    }
    
    QueueFamilies[ComputeIndex].queueCount--;

    QueueIndicies.Emplace(GraphicsIndex, CopyIndex, ComputeIndex);
    return QueueIndicies;
}

uint32 FVulkanPhysicalDevice::FindMemoryTypeIndex(uint32 TypeFilter, VkMemoryPropertyFlags Properties)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

    for (uint32 MemoryIndex = 0; MemoryIndex < MemoryProperties.memoryTypeCount; ++MemoryIndex)
    {
        if ((TypeFilter & (1 << MemoryIndex)) && (MemoryProperties.memoryTypes[MemoryIndex].propertyFlags & Properties) == Properties)
        {
            return MemoryIndex;
        }
    }

    return TNumericLimits<int32>::Max();
}

VkFormatProperties FVulkanPhysicalDevice::GetFormatProperties(VkFormat Format) const
{
    VkFormatProperties FormatProperties = {};
    vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Format, &FormatProperties);
    return FormatProperties;
}

FVulkanDevice::FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , PhysicalDevice(InAdapter)
    , Device(VK_NULL_HANDLE)
    , RenderPassCache(nullptr)
    , MemoryManager(nullptr)
    , FenceManager(nullptr)
    , FrameFence(nullptr)
    , PipelineLayoutManager(nullptr)
    , PipelineStateManager(nullptr)
    , DescriptorSetCache(nullptr)
    , DescriptorPoolManager(nullptr)
    , TimingQueryPoolManager(nullptr)
    , OcclusionQueryPoolManager(nullptr)
{
    if (IConsoleVariable* UseDynamicRenderingVar = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.UseDynamicRendering"))
    {
        GVulkanUseDynamicRendering = UseDynamicRenderingVar->GetBool();
    }

    if (IConsoleVariable* UseDescriptorCacheVar = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.UseDescriptorCache"))
    {
        GVulkanUseDescriptorCache = UseDescriptorCacheVar->GetBool();
    }

    DescriptorSetCache        = new FVulkanDescriptorSetCache(this);
    DescriptorPoolManager     = GVulkanUseDescriptorCache ? nullptr : new FVulkanDescriptorPoolManager(this);
    TimingQueryPoolManager    = new FVulkanQueryPoolManager(this, EQueryType::Timestamp);
    OcclusionQueryPoolManager = new FVulkanQueryPoolManager(this, EQueryType::Occlusion);
    PipelineLayoutManager     = new FVulkanPipelineLayoutManager(this);
    FenceManager              = new FVulkanFenceManager(this);
    RenderPassCache           = new FVulkanRenderPassCache(this);
}

FVulkanDevice::~FVulkanDevice()
{
    // Release default resources
    DefaultResources.Release(*this);

    // Release all samplers
    {
        TScopedLock Lock(SamplerMapCS);

        for (const auto SamplerPair : SamplerMap)
        {
            if (VULKAN_CHECK_HANDLE(SamplerPair.Second))
            {
                vkDestroySampler(GetVkDevice(), SamplerPair.Second, nullptr);
            }
        }

        SamplerMap.Clear();
    }

    // Release the PipelineCache
    if (PipelineStateManager)
    {
        PipelineStateManager->SaveCacheData();
        delete PipelineStateManager;
    }
    
    SAFE_DELETE(DescriptorPoolManager);
    SAFE_DELETE(DescriptorSetCache);
    SAFE_DELETE(TimingQueryPoolManager);
    SAFE_DELETE(OcclusionQueryPoolManager);
    SAFE_DELETE(PipelineLayoutManager);
    SAFE_DELETE(RenderPassCache);
    SAFE_DELETE(FrameFence);
    SAFE_DELETE(FenceManager);
    SAFE_DELETE(MemoryManager);
    
    // Destroy the device here
    if (VULKAN_CHECK_HANDLE(Device))
    {
        vkDestroyDevice(Device, nullptr);
    }
}

bool FVulkanDevice::Initialize(const FVulkanDeviceCreateInfo& InDeviceCreateInfo)
{
    if (!PhysicalDevice)
    {
        VULKAN_ERROR_CRITICAL("PhysicalDevice is not initalized correctly");
        return false;
    }

    VkResult Result = VK_SUCCESS;

    // -------------------------------------------------------------------------------------------
    // Enumerate and select extensions
    // -------------------------------------------------------------------------------------------
    
    uint32 DeviceExtensionCount = 0;
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve the device extension count");
        return false;
    }

    TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve the device extensions");
        return false;
    }

    TArray<const CHAR*> EnabledExtensionNames;
    EnabledExtensionNames.Reserve(InDeviceCreateInfo.RequiredExtensionNames.Size() + InDeviceCreateInfo.OptionalExtensionNames.Size());

    for (const VkExtensionProperties& ExtensionProperties : AvailableDeviceExtensions)
    {
        if (!FilterExtensions(ExtensionProperties))
        {
            continue;
        }

        const auto MatchExtensionNames = [&](const CHAR* InExtensionName) { return FCString::Strcmp(ExtensionProperties.extensionName, InExtensionName) == 0; };
        if (InDeviceCreateInfo.RequiredExtensionNames.ContainsWithPredicate(MatchExtensionNames) || 
            InDeviceCreateInfo.OptionalExtensionNames.ContainsWithPredicate(MatchExtensionNames))
        {
            EnabledExtensionNames.Add(ExtensionProperties.extensionName);
            ExtensionNames.Emplace(ExtensionProperties.extensionName);
        }
    }

    for (const CHAR* RequiredExtensionName : InDeviceCreateInfo.RequiredExtensionNames)
    {
        const auto MatchExtensionNames = [&](const CHAR* InExtensionName) { return FCString::Strcmp(RequiredExtensionName, InExtensionName) == 0; };
        if (!EnabledExtensionNames.ContainsWithPredicate(MatchExtensionNames))
        {
            VULKAN_ERROR_CRITICAL("Device extension '%s' could not be enabled", RequiredExtensionName);
            return false;
        }
    }

    if (IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging"))
    {
        if (VerboseVulkan->GetBool() && !EnabledExtensionNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Device Extensions:");
            for (const CHAR* ExtensionName : EnabledExtensionNames)
            {
                LOG_INFO("    %s", ExtensionName);
            }
        }
    }

    // -------------------------------------------------------------------------------------------
    // Queues
    // -------------------------------------------------------------------------------------------

    QueueIndicies = FVulkanPhysicalDevice::GetQueueFamilyIndices(PhysicalDevice->GetVkPhysicalDevice());
    if (!QueueIndicies)
    {
        VULKAN_ERROR_CRITICAL("Failed to query queue indices");
        return false;
    }

    VULKAN_INFO("QueueIndicies: Graphics=%d, Compute=%d, Copy=%d", QueueIndicies->GraphicsQueueIndex, QueueIndicies->ComputeQueueIndex, QueueIndicies->CopyQueueIndex);

    const TSet<uint32> UniqueQueueIndices = 
    { 
        QueueIndicies->GraphicsQueueIndex, 
        QueueIndicies->CopyQueueIndex, 
        QueueIndicies->ComputeQueueIndex
    };

    const float DefaultQueuePriority = 0.0f;

    TArray<VkDeviceQueueCreateInfo> QueueCreateInfos;
    for (uint32 QueueFamilyIndex : UniqueQueueIndices)
    {
        VkDeviceQueueCreateInfo QueueCreateInfo = {};
        QueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
        QueueCreateInfo.queueCount       = 1;
        QueueCreateInfo.pQueuePriorities = &DefaultQueuePriority;
        QueueCreateInfos.Add(QueueCreateInfo);
    }

    // -------------------------------------------------------------------------------------------
    // Collect core features/properties + extension feature/property structs
    // -------------------------------------------------------------------------------------------

    // Core feature blocks
    VkPhysicalDeviceFeatures2 AvailableDeviceFeatures2 = {};
    AvailableDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceVulkan11Features AvailableDeviceFeatures11 = {};
    AvailableDeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    VkPhysicalDeviceVulkan12Features AvailableDeviceFeatures12 = {};
    AvailableDeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceVulkan13Features AvailableDeviceFeatures13 = {};
    AvailableDeviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    // Extension feature/property blocks
#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT AvailableDeviceDepthClipEnableFeatures = {};
    AvailableDeviceDepthClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
#endif

#if VK_KHR_robustness2
    VkPhysicalDeviceRobustness2FeaturesKHR AvailableDeviceRobustness2Features = {};
    AvailableDeviceRobustness2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
#endif

#if VK_EXT_device_fault
    VkPhysicalDeviceFaultFeaturesEXT AvailableDeviceFaultFeatures = {};
    AvailableDeviceFaultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
#endif

#if VK_KHR_fragment_shading_rate
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR AvailableDeviceFragmentShadingRateFeatures = {};
    AvailableDeviceFragmentShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

    VkPhysicalDeviceFragmentShadingRatePropertiesKHR AvailableDeviceFragmentShadingRateProperties = {};
    AvailableDeviceFragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
#endif

#if VK_KHR_acceleration_structure
    VkPhysicalDeviceAccelerationStructureFeaturesKHR AvailableDeviceAccelerationStructureFeatures = {};
    AvailableDeviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
#endif

#if VK_KHR_ray_tracing_pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR AvailableDeviceRayTracingPipelineFeatures = {};
    AvailableDeviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
#endif

#if VK_KHR_ray_query
    VkPhysicalDeviceRayQueryFeaturesKHR AvailableDeviceRayQueryFeatures = {};
    AvailableDeviceRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
#endif

#if VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderFeaturesEXT AvailableDeviceMeshShaderFeatures = {};
    AvailableDeviceMeshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

    VkPhysicalDeviceMeshShaderPropertiesEXT AvailableDeviceMeshShaderProperties = {};
    AvailableDeviceMeshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
#endif

#if VK_EXT_sample_locations
    VkPhysicalDeviceSampleLocationsPropertiesEXT AvailableDeviceSampleLocationsProperties = {};
    AvailableDeviceSampleLocationsProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT;
#endif

#if VK_EXT_fragment_shader_interlock
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT AvailableDeviceFragmentShaderInterlockFeatures = {};
    AvailableDeviceFragmentShaderInterlockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
#endif

    // Core property blocks
    VkPhysicalDeviceProperties2 AvailableDeviceProperties2 = {};
    AvailableDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

    VkPhysicalDeviceMultiviewProperties AvailableDeviceMultiviewProperties = {};
    AvailableDeviceMultiviewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;

    {
        FVulkanStructChain AvailableDeviceFeatureChain(AvailableDeviceFeatures2);
        AvailableDeviceFeatureChain.AddNext(AvailableDeviceFeatures11);
        AvailableDeviceFeatureChain.AddNext(AvailableDeviceFeatures12);
        AvailableDeviceFeatureChain.AddNext(AvailableDeviceFeatures13);

    #if VK_EXT_depth_clip_enable
        if (IsExtensionEnabled(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceDepthClipEnableFeatures);
        }
    #endif
    #if VK_KHR_robustness2
        if (IsExtensionEnabled(VK_KHR_ROBUSTNESS_2_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceRobustness2Features);
        }
    #endif
    #if VK_EXT_device_fault
        if (IsExtensionEnabled(VK_EXT_DEVICE_FAULT_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceFaultFeatures);
        }
    #endif
    #if VK_KHR_fragment_shading_rate
        if (IsExtensionEnabled(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceFragmentShadingRateFeatures);
        }
    #endif
    #if VK_KHR_acceleration_structure
        if (IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceAccelerationStructureFeatures);
        }
    #endif
    #if VK_KHR_ray_tracing_pipeline
        if (IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceRayTracingPipelineFeatures);
        }
    #endif
    #if VK_KHR_ray_query
        if (IsExtensionEnabled(VK_KHR_RAY_QUERY_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceRayQueryFeatures);
        }
    #endif
    #if VK_EXT_mesh_shader
        if (IsExtensionEnabled(VK_EXT_MESH_SHADER_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceMeshShaderFeatures);
        }
    #endif
    #if VK_EXT_fragment_shader_interlock
        if (IsExtensionEnabled(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME))
        {
            AvailableDeviceFeatureChain.AddNext(AvailableDeviceFragmentShaderInterlockFeatures);
        }
    #endif

        vkGetPhysicalDeviceFeatures2(PhysicalDevice->GetVkPhysicalDevice(), &AvailableDeviceFeatures2);
    }

    {
        FVulkanStructChain AvailableDevicePropertiesChain(AvailableDeviceProperties2);
        AvailableDevicePropertiesChain.AddNext(AvailableDeviceMultiviewProperties);

    #if VK_KHR_fragment_shading_rate
        if (IsExtensionEnabled(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
        {
            AvailableDevicePropertiesChain.AddNext(AvailableDeviceFragmentShadingRateProperties);
        }
    #endif
    #if VK_EXT_mesh_shader
        if (IsExtensionEnabled(VK_EXT_MESH_SHADER_EXTENSION_NAME))
        {
            AvailableDevicePropertiesChain.AddNext(AvailableDeviceMeshShaderProperties);
        }
    #endif
    #if VK_EXT_sample_locations
        if (IsExtensionEnabled(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME))
        {
            AvailableDevicePropertiesChain.AddNext(AvailableDeviceSampleLocationsProperties);
        }
    #endif

        vkGetPhysicalDeviceProperties2(PhysicalDevice->GetVkPhysicalDevice(), &AvailableDeviceProperties2);
    }

    // -------------------------------------------------------------------------------------------
    // Set all GVulkan* globals from collected caps
    // -------------------------------------------------------------------------------------------

    const VkPhysicalDeviceFeatures&   CoreDeviceFeatures10   = AvailableDeviceFeatures2.features;
    const VkPhysicalDeviceProperties& CoreDeviceProperties10 = PhysicalDevice->GetProperties();

    // Core/sparse/depth bounds
    GVulkanSupportsDepthBoundsTest        = (CoreDeviceFeatures10.depthBounds == VK_TRUE);
    GVulkanSupportsSparseBinding          = (CoreDeviceFeatures10.sparseBinding == VK_TRUE);
    GVulkanSupportsSparseResidency2D      = (CoreDeviceFeatures10.sparseResidencyImage2D == VK_TRUE);
    GVulkanSupportsSparseResidency3D      = (CoreDeviceFeatures10.sparseResidencyImage3D == VK_TRUE);
    GVulkanSupportsSparseResidencyAliased = (CoreDeviceFeatures10.sparseResidencyAliased == VK_TRUE);
    GVulkanSupportsGeometryShader         = (GVulkanAllowGeometryShaders && CoreDeviceFeatures10.geometryShader == VK_TRUE);
    GVulkanSupportsTessellation           = (CoreDeviceFeatures10.tessellationShader == VK_TRUE);

#if VK_KHR_robustness2
    if (IsExtensionEnabled(VK_KHR_ROBUSTNESS_2_EXTENSION_NAME) && AvailableDeviceRobustness2Features.nullDescriptor)
    {
        GVulkanSupportsNullDescriptors = true;
    }

    VULKAN_INFO("Robustness: robustBufferAccess=%s, nullDescriptor=%s, robustBufferAccess2=%s, robustImageAccess2=%s",
        GVulkanRobustBufferAccessEnabled ? "ON" : "OFF",
        GVulkanSupportsNullDescriptors   ? "ON" : "OFF",
        (AvailableDeviceRobustness2Features.robustBufferAccess2 && GVulkanRobustBufferAccessEnabled) ? "ON" : "OFF",
        AvailableDeviceRobustness2Features.robustImageAccess2 ? "ON" : "OFF");

    if (!GVulkanSupportsNullDescriptors)
    {
        VULKAN_WARNING("nullDescriptor not supported - unbound descriptors will use a %u-byte fallback buffer (OOB risk)", VULKAN_DEFAULT_BUFFER_NUM_BYTES);
    }
#endif

#if VK_EXT_depth_clip_enable
    if (IsExtensionEnabled(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) && AvailableDeviceDepthClipEnableFeatures.depthClipEnable && CoreDeviceFeatures10.depthClamp)
    {
        GVulkanSupportsDepthClip = true;
    }
#endif

#if VK_EXT_conservative_rasterization
    if (IsExtensionEnabled(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME))
    {
        GVulkanSupportsConservativeRasterization = true;
    }
#endif

    // Multiview (Core 1.1)
    if (AvailableDeviceFeatures11.multiview)
    {
        GVulkanSupportsMultiviews    = true;
        GVulkanMaxMultiviewViewCount = Math::Max<uint32>(1u, AvailableDeviceMultiviewProperties.maxMultiviewViewCount);
    }
    else
    {
        GVulkanSupportsMultiviews    = false;
        GVulkanMaxMultiviewViewCount = 1u;
    }

    // Pipeline cache control (Core 1.3)
    if (AvailableDeviceFeatures13.pipelineCreationCacheControl)
    {
        GVulkanSupportsPipelineCacheControl = true;
    }

    // Descriptor indexing (Bindless heuristic)
    if (AvailableDeviceFeatures12.descriptorIndexing)
    {
        GVulkanSupportsBindless = true;
    }

    // Ray Tracing
#if VK_KHR_acceleration_structure
    if (IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) && AvailableDeviceAccelerationStructureFeatures.accelerationStructure == VK_TRUE)
    {
        GVulkanSupportsAccelerationStructures = true;
    }
#endif
#if VK_KHR_ray_tracing_pipeline
    if (IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) && AvailableDeviceRayTracingPipelineFeatures.rayTracingPipeline)
    {
        GVulkanSupportsRayTracingPipeline = true;
    }
#endif
#if VK_KHR_ray_query
    if (IsExtensionEnabled(VK_KHR_RAY_QUERY_EXTENSION_NAME) && AvailableDeviceRayQueryFeatures.rayQuery == VK_TRUE)
    {
        GVulkanSupportsRayQuery = true;
    }
#endif

    // Fragment shading rate (VRS)
#if VK_KHR_fragment_shading_rate
    if (IsExtensionEnabled(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
    {
        if (AvailableDeviceFragmentShadingRateFeatures.pipelineFragmentShadingRate || AvailableDeviceFragmentShadingRateFeatures.primitiveFragmentShadingRate ||
            AvailableDeviceFragmentShadingRateFeatures.attachmentFragmentShadingRate)
	    {
		    GVulkanShadingRateTileSize = Math::Max<uint32>(1u, AvailableDeviceFragmentShadingRateProperties.minFragmentShadingRateAttachmentTexelSize.width);
		    GVulkanSupportsFragmentShadingRate = true;
	    }
    }
#endif

    // Mesh shaders
#if VK_EXT_mesh_shader
    if (IsExtensionEnabled(VK_EXT_MESH_SHADER_EXTENSION_NAME) && 
        (AvailableDeviceMeshShaderFeatures.meshShader || AvailableDeviceMeshShaderFeatures.taskShader))
    {
        GVulkanMaxMeshOutputVertices       = AvailableDeviceMeshShaderProperties.maxMeshOutputVertices;
        GVulkanMaxMeshWorkGroupInvocations = AvailableDeviceMeshShaderProperties.maxMeshWorkGroupInvocations;
        GVulkanMaxTaskWorkGroupInvocations = AvailableDeviceMeshShaderProperties.maxTaskWorkGroupInvocations;
        GVulkanSupportsMeshShaders         = true;
    }
#endif

    // Sample locations
#if VK_EXT_sample_locations
    if (IsExtensionEnabled(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME))
    {
        GVulkanSupportsSampleLocations = true;
    }
#endif

    // Fragment shader interlock
#if VK_EXT_fragment_shader_interlock
    if (IsExtensionEnabled(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME))
    {
        if (AvailableDeviceFragmentShaderInterlockFeatures.fragmentShaderSampleInterlock || AvailableDeviceFragmentShaderInterlockFeatures.fragmentShaderPixelInterlock ||
            AvailableDeviceFragmentShaderInterlockFeatures.fragmentShaderShadingRateInterlock)
        {
            GVulkanSupportsFragmentShaderInterlock = true;
        }
    }
#endif

    // Limits / counts
    GVulkanMaxDrawIndirectCount            = CoreDeviceProperties10.limits.maxDrawIndirectCount;
    GVulkanMaxDescriptorSetSamplers        = CoreDeviceProperties10.limits.maxDescriptorSetSamplers;
    GVulkanMaxDescriptorSetSampledImages   = CoreDeviceProperties10.limits.maxDescriptorSetSampledImages;
    GVulkanMaxDescriptorSetStorageImages   = CoreDeviceProperties10.limits.maxDescriptorSetStorageImages;
    GVulkanMaxDescriptorSetUniformBuffers  = CoreDeviceProperties10.limits.maxDescriptorSetUniformBuffers;
    GVulkanMaxDescriptorSetStorageBuffers  = CoreDeviceProperties10.limits.maxDescriptorSetStorageBuffers;

    // -------------------------------------------------------------------------------------------
    // Build the feature chain we actually want, then create the device
    // -------------------------------------------------------------------------------------------

    VkDeviceCreateInfo DeviceCreateInfo = {};
    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.enabledLayerCount       = 0;
    DeviceCreateInfo.ppEnabledLayerNames     = nullptr;
    DeviceCreateInfo.enabledExtensionCount   = EnabledExtensionNames.Size();
    DeviceCreateInfo.ppEnabledExtensionNames = EnabledExtensionNames.Data();
    DeviceCreateInfo.queueCreateInfoCount    = QueueCreateInfos.Size();
    DeviceCreateInfo.pQueueCreateInfos       = QueueCreateInfos.Data();

    // Core features to enable (start from Required + clamp by available)
    VkPhysicalDeviceFeatures2 EnableDeviceFeatures2 = {};
    EnableDeviceFeatures2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    EnableDeviceFeatures2.features = InDeviceCreateInfo.RequiredFeatures;
    EnableOptionalFeatures(EnableDeviceFeatures2.features, InDeviceCreateInfo.OptionalFeatures, InDeviceCreateInfo.RequiredFeatures);
    GVulkanRobustBufferAccessEnabled = (EnableDeviceFeatures2.features.robustBufferAccess == VK_TRUE);

    VkPhysicalDeviceVulkan11Features EnableDeviceFeatures11 = InDeviceCreateInfo.RequiredFeatures11;
    EnableDeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    EnableOptionalFeatures(EnableDeviceFeatures11, InDeviceCreateInfo.OptionalFeatures11, InDeviceCreateInfo.RequiredFeatures11);

    VkPhysicalDeviceVulkan12Features EnableDeviceFeatures12 = InDeviceCreateInfo.RequiredFeatures12;
    EnableDeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    EnableOptionalFeatures(EnableDeviceFeatures12, InDeviceCreateInfo.OptionalFeatures12, InDeviceCreateInfo.RequiredFeatures12);

    VkPhysicalDeviceVulkan13Features EnableDeviceFeatures13 = InDeviceCreateInfo.RequiredFeatures13;
	EnableDeviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    EnableOptionalFeatures(EnableDeviceFeatures13, InDeviceCreateInfo.OptionalFeatures13, InDeviceCreateInfo.RequiredFeatures13);

    // Extension features to enable
#if VK_KHR_robustness2
    VkPhysicalDeviceRobustness2FeaturesKHR EnableDeviceRobustness2Features = {};
    if (IsExtensionEnabled(VK_KHR_ROBUSTNESS_2_EXTENSION_NAME))
    {
        EnableDeviceRobustness2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;

        if (AvailableDeviceRobustness2Features.robustImageAccess2)
        {
            EnableDeviceRobustness2Features.robustImageAccess2 = VK_TRUE;
        }
		if (AvailableDeviceRobustness2Features.robustBufferAccess2 && GVulkanRobustBufferAccessEnabled)
		{
			EnableDeviceRobustness2Features.robustBufferAccess2 = VK_TRUE;
		}
		if (AvailableDeviceRobustness2Features.nullDescriptor)
		{
			EnableDeviceRobustness2Features.nullDescriptor = VK_TRUE;
		}
    }
#endif

#if VK_EXT_device_fault
    VkPhysicalDeviceFaultFeaturesEXT EnableDeviceFaultFeatures = {};
    if (IsExtensionEnabled(VK_EXT_DEVICE_FAULT_EXTENSION_NAME) && AvailableDeviceFaultFeatures.deviceFault)
    {
        EnableDeviceFaultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
        EnableDeviceFaultFeatures.deviceFault = VK_TRUE;
    }
#endif

#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT EnableDeviceDepthClipEnableFeatures = {};
    if (GVulkanSupportsDepthClip)
    {
        EnableDeviceDepthClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;

		if (AvailableDeviceDepthClipEnableFeatures.depthClipEnable)
		{
            EnableDeviceDepthClipEnableFeatures.depthClipEnable = VK_TRUE;
		}
		if (AvailableDeviceFeatures2.features.depthClamp)
		{
            EnableDeviceFeatures2.features.depthClamp = VK_TRUE;
		}
    }
#endif

#if VK_KHR_acceleration_structure
    VkPhysicalDeviceAccelerationStructureFeaturesKHR EnableDeviceAccelerationStructureFeatures = {};
    if (GVulkanSupportsAccelerationStructures)
    {
        EnableDeviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

		if (AvailableDeviceAccelerationStructureFeatures.accelerationStructure)
		{
            EnableDeviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		}
    }
#endif

#if VK_KHR_ray_tracing_pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR EnableDeviceRayTracingPipelineFeatures = {};
    if (GVulkanSupportsRayTracingPipeline)
    {
        EnableDeviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

		if (AvailableDeviceRayTracingPipelineFeatures.rayTracingPipeline)
		{
            EnableDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		}
    }
#endif

#if VK_KHR_ray_query
    VkPhysicalDeviceRayQueryFeaturesKHR EnableDeviceRayQueryFeatures = {};
    if (GVulkanSupportsRayQuery)
    {
        EnableDeviceRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;

        if (AvailableDeviceRayQueryFeatures.rayQuery)
        {
            EnableDeviceRayQueryFeatures.rayQuery = VK_TRUE;
        }
    }
#endif

#if VK_KHR_fragment_shading_rate
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR EnableDeviceFragmentShadingRateFeatures = {};
    if (GVulkanSupportsFragmentShadingRate)
    {
        EnableDeviceFragmentShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

		if (AvailableDeviceFragmentShadingRateFeatures.attachmentFragmentShadingRate)
		{
            EnableDeviceFragmentShadingRateFeatures.attachmentFragmentShadingRate = VK_TRUE;
		}
		if (AvailableDeviceFragmentShadingRateFeatures.pipelineFragmentShadingRate)
		{
            EnableDeviceFragmentShadingRateFeatures.pipelineFragmentShadingRate = VK_TRUE;
		}
		if (AvailableDeviceFragmentShadingRateFeatures.primitiveFragmentShadingRate)
		{
            EnableDeviceFragmentShadingRateFeatures.primitiveFragmentShadingRate = VK_TRUE;
		}
    }
#endif

#if VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderFeaturesEXT EnableDeviceMeshShaderFeatures = {};
    if (GVulkanSupportsMeshShaders)
    {
        EnableDeviceMeshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

        if (AvailableDeviceMeshShaderFeatures.meshShader)
        {
            EnableDeviceMeshShaderFeatures.meshShader = VK_TRUE;
        }
        if (AvailableDeviceMeshShaderFeatures.taskShader)
        {
            EnableDeviceMeshShaderFeatures.taskShader = VK_TRUE;
        }
    }
#endif

#if VK_EXT_fragment_shader_interlock
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT EnableDeviceFragmentShaderInterlockFeatures = {};
    if (GVulkanSupportsFragmentShaderInterlock)
    {
        EnableDeviceFragmentShaderInterlockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;

		if (AvailableDeviceFragmentShaderInterlockFeatures.fragmentShaderSampleInterlock)
		{
            EnableDeviceFragmentShaderInterlockFeatures.fragmentShaderSampleInterlock = VK_TRUE;
		}
		if (AvailableDeviceFragmentShaderInterlockFeatures.fragmentShaderPixelInterlock)
		{
            EnableDeviceFragmentShaderInterlockFeatures.fragmentShaderPixelInterlock = VK_TRUE;
		}
		if (AvailableDeviceFragmentShaderInterlockFeatures.fragmentShaderShadingRateInterlock)
		{
            EnableDeviceFragmentShaderInterlockFeatures.fragmentShaderShadingRateInterlock = VK_TRUE;
		}
    }
#endif

    // Build the "Enable" chain
    FVulkanStructChain EnableDeviceFeaturesChain(DeviceCreateInfo);
    EnableDeviceFeaturesChain.AddNext(EnableDeviceFeatures2);
    EnableDeviceFeaturesChain.AddNext(EnableDeviceFeatures11);
    EnableDeviceFeaturesChain.AddNext(EnableDeviceFeatures12);
    EnableDeviceFeaturesChain.AddNext(EnableDeviceFeatures13);

#if VK_KHR_robustness2
    if (IsExtensionEnabled(VK_KHR_ROBUSTNESS_2_EXTENSION_NAME))
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceRobustness2Features);
    }
#endif
#if VK_EXT_device_fault
    if (EnableDeviceFaultFeatures.deviceFault)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceFaultFeatures);
    }
#endif
#if VK_EXT_depth_clip_enable
    if (GVulkanSupportsDepthClip)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceDepthClipEnableFeatures);
    }
#endif
#if VK_KHR_acceleration_structure
    if (GVulkanSupportsAccelerationStructures)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceAccelerationStructureFeatures);
    }
#endif
#if VK_KHR_ray_tracing_pipeline
    if (GVulkanSupportsRayTracingPipeline)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceRayTracingPipelineFeatures);
    }
#endif
#if VK_KHR_ray_query
    if (GVulkanSupportsRayQuery)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceRayQueryFeatures);
    }
#endif
#if VK_KHR_fragment_shading_rate
    if (GVulkanSupportsFragmentShadingRate)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceFragmentShadingRateFeatures);
    }
#endif
#if VK_EXT_mesh_shader
    if (GVulkanSupportsMeshShaders)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceMeshShaderFeatures);
    }
#endif
#if VK_EXT_fragment_shader_interlock
    if (GVulkanSupportsFragmentShaderInterlock)
    {
        EnableDeviceFeaturesChain.AddNext(EnableDeviceFragmentShaderInterlockFeatures);
    }
#endif

    // Finally create the device
    Result = vkCreateDevice(PhysicalDevice->GetVkPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Device");
        return false;
    }

    return true;
}

bool FVulkanDevice::PostLoaderInitalize()
{
    // Initialize PipelineStateManager
    PipelineStateManager = new FVulkanPipelineStateManager(this);
    if (!PipelineStateManager->Initialize())
    {
        return false;
    }

    // Initialize the device feature support
    if (!InitializeDeviceFeatureSupport())
    {
        return false;
    }

    MemoryManager = new FVulkanMemoryManager(this);
    if (!MemoryManager->Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanDevice: Failed to initialize MemoryManager");
        return false;
    }

    FrameFence = new FVulkanTimelineFence(this);
    if (!FrameFence->Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanDevice: Failed to initialize FrameFence");
        return false;
    }

    FrameFence->SetDebugName("FrameFence");

    return true;
}

bool FVulkanDevice::InitializeDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline defaults
    // -------------------------------------------------------------------------------------------

    RHIDeviceFeatureSupport::bSupportsGeometryShaders                       = false;
    RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = false;

    RHIDeviceFeatureSupport::bSupportsViewInstancing     = false;
    RHIDeviceFeatureSupport::MaxViewInstanceCount        = 1;

    RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
    RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
    RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;

    RHIDeviceFeatureSupport::bSupportsVRS                = false;
    RHIDeviceFeatureSupport::ShadingRateTier             = EShadingRateTier::NotSupported;
    RHIDeviceFeatureSupport::ShadingRateImageTileSize    = 0;

    RHIDeviceFeatureSupport::bSupportDrawIndirect        = true;
    RHIDeviceFeatureSupport::bSupportMultiDrawIndirect   = false;
    RHIDeviceFeatureSupport::MaxDrawIndirectCount        = 1;

    RHIDeviceFeatureSupport::MaxTexture1DSize            = 0;
    RHIDeviceFeatureSupport::MaxTexture1DArrayLayers     = 0;
    RHIDeviceFeatureSupport::MaxTexture2DSize            = 0;
    RHIDeviceFeatureSupport::MaxTexture2DArrayLayers     = 0;
    RHIDeviceFeatureSupport::MaxTexture3DWidth           = 0;
    RHIDeviceFeatureSupport::MaxTexture3DHeight          = 0;
    RHIDeviceFeatureSupport::MaxTexture3DDepth           = 0;
    RHIDeviceFeatureSupport::MaxCubeTextureSize          = 0;
    RHIDeviceFeatureSupport::MaxCubeArrayCount           = 0;

    RHIDeviceFeatureSupport::MaxBufferSize               = 0;
    RHIDeviceFeatureSupport::MaxConstantBufferSize       = 0;
    RHIDeviceFeatureSupport::MaxStorageBufferSize        = 0;
    RHIDeviceFeatureSupport::StructuredBufferMinStride   = 4;
    RHIDeviceFeatureSupport::StructuredBufferMaxStride   = 2048;
    RHIDeviceFeatureSupport::RawBufferRequiredAlignment  = 4;

    // -------------------------------------------------------------------------------------------
    // Core features/properties
    // -------------------------------------------------------------------------------------------

    VkPhysicalDevice PhysicalDeviceHandle = GetPhysicalDevice()->GetVkPhysicalDevice();

    // Core features
    const VkPhysicalDeviceFeatures& PhysicalDeviceFeatures = PhysicalDevice->GetFeatures();
    if (GVulkanAllowGeometryShaders && PhysicalDeviceFeatures.geometryShader)
    {
        RHIDeviceFeatureSupport::bSupportsGeometryShaders = true;
    }

    // Core properties
    const VkPhysicalDeviceProperties& PhysicalDeviceProperties = PhysicalDevice->GetProperties();

    {
        // DrawIndirect + MultiDrawIndirect
        if (PhysicalDeviceFeatures.multiDrawIndirect)
        {
            RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = true;
            RHIDeviceFeatureSupport::MaxDrawIndirectCount      = PhysicalDeviceProperties.limits.maxDrawIndirectCount;
        }
        else
        {
            RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = false;
            RHIDeviceFeatureSupport::MaxDrawIndirectCount      = 1;
        }

        // Texture / Image limits
        RHIDeviceFeatureSupport::MaxTexture1DSize        = PhysicalDeviceProperties.limits.maxImageDimension1D;
        RHIDeviceFeatureSupport::MaxTexture2DSize        = PhysicalDeviceProperties.limits.maxImageDimension2D;
        RHIDeviceFeatureSupport::MaxTexture3DWidth       = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHIDeviceFeatureSupport::MaxTexture3DHeight      = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHIDeviceFeatureSupport::MaxTexture3DDepth       = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHIDeviceFeatureSupport::MaxCubeTextureSize      = PhysicalDeviceProperties.limits.maxImageDimensionCube;
        RHIDeviceFeatureSupport::MaxTexture1DArrayLayers = PhysicalDeviceProperties.limits.maxImageArrayLayers;
        RHIDeviceFeatureSupport::MaxTexture2DArrayLayers = PhysicalDeviceProperties.limits.maxImageArrayLayers;
        RHIDeviceFeatureSupport::MaxCubeArrayCount       = PhysicalDeviceProperties.limits.maxImageArrayLayers / RHI_NUM_CUBE_FACES;

        // Buffer / Memory Limits 
        const uint32 MinBufferStride = sizeof(uint32); 
        RHIDeviceFeatureSupport::MaxConstantBufferSize      = PhysicalDeviceProperties.limits.maxUniformBufferRange; 
        RHIDeviceFeatureSupport::MaxStorageBufferSize       = PhysicalDeviceProperties.limits.maxStorageBufferRange; 
        RHIDeviceFeatureSupport::MaxBufferSize              = uint64(~0); 
        RHIDeviceFeatureSupport::StructuredBufferMinStride  = MinBufferStride; 
        RHIDeviceFeatureSupport::StructuredBufferMaxStride  = uint32(~0); 
        RHIDeviceFeatureSupport::RawBufferRequiredAlignment = MinBufferStride; 
    } 

    // -------------------------------------------------------------------------------------------
    // SV_RenderTargetArrayIndex from VS (shaderOutputLayer in Vulkan 1.2)
    // -------------------------------------------------------------------------------------------

    const VkPhysicalDeviceVulkan12Features& PhysicalDeviceFeatures12 = PhysicalDevice->GetFeaturesVulkan12();
    RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = PhysicalDeviceFeatures12.shaderOutputLayer ? true : false;

    // -------------------------------------------------------------------------------------------
    // View Instancing (multiview)
    // -------------------------------------------------------------------------------------------

    if (GVulkanSupportsMultiviews)
    {
        RHIDeviceFeatureSupport::MaxViewInstanceCount    = GVulkanMaxMultiviewViewCount;
        RHIDeviceFeatureSupport::bSupportsViewInstancing = RHIDeviceFeatureSupport::MaxViewInstanceCount > 1;
    }
    else
    {
        RHIDeviceFeatureSupport::MaxViewInstanceCount    = 1;
        RHIDeviceFeatureSupport::bSupportsViewInstancing = false;
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
        // Query pipeline RT properties for recursion depth
        VkPhysicalDeviceProperties2 DeviceProperties2 = {};
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR DeviceRayTracingPipelineProperties = {};
        DeviceRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        FVulkanStructChain DeviceProperties2Chain(DeviceProperties2);
        DeviceProperties2Chain.AddNext(DeviceRayTracingPipelineProperties);

        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        RHIDeviceFeatureSupport::bSupportsRayTracing         = true;
        RHIDeviceFeatureSupport::RayTracingTier              = bHasRayQuery ? ERayTracingTier::Tier1_1 : ERayTracingTier::Tier1;
        RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = DeviceRayTracingPipelineProperties.maxRayRecursionDepth;
    }
    else
    {
        RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
        RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
        RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;
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

        FVulkanStructChain DeviceFeaturesChain(DeviceFeatures2);
        DeviceFeaturesChain.AddNext(DeviceFragmentShadingRateFeatures);

        vkGetPhysicalDeviceFeatures2(PhysicalDeviceHandle, &DeviceFeatures2);

        // Query properties (tile size)
        VkPhysicalDeviceProperties2 DeviceProperties2 = {};
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceFragmentShadingRatePropertiesKHR DeviceFragmentShadingRateProperties = {};
        DeviceFragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

        FVulkanStructChain DevicePropertiesChain(DeviceProperties2);
        DevicePropertiesChain.AddNext(DeviceFragmentShadingRateProperties);

        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        if (DeviceFragmentShadingRateFeatures.attachmentFragmentShadingRate)
        {
            RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::Tier2; // image-based
        }
        else if (DeviceFragmentShadingRateFeatures.pipelineFragmentShadingRate || DeviceFragmentShadingRateFeatures.primitiveFragmentShadingRate)
        {
            RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::Tier1; // per-draw/per-primitive
        }
        else
        {
            RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::NotSupported;
        }

        RHIDeviceFeatureSupport::ShadingRateImageTileSize = Math::Max<uint32>(1u, DeviceFragmentShadingRateProperties.minFragmentShadingRateAttachmentTexelSize.width);
        RHIDeviceFeatureSupport::bSupportsVRS             = RHIDeviceFeatureSupport::ShadingRateTier != EShadingRateTier::NotSupported;
    }
    else
    {
        RHIDeviceFeatureSupport::bSupportsVRS             = false;
        RHIDeviceFeatureSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
        RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;
    }

    return true;
}

// -------------------------------------------------------------------------------------------
// Initialize default resources that are just for null bindings
// -------------------------------------------------------------------------------------------

bool FVulkanDevice::InitializeDefaultResources(FVulkanCommandContext& CommandContext)
{
    // Create the resources
    if (!DefaultResources.Initialize(*this))
    {
        VULKAN_ERROR_CRITICAL("Failed to create DefaultResources");
        return false;
    }

    // If null-descriptors are supported then we can return here, since we have no DefaultResources to upload
    if (GVulkanSupportsNullDescriptors)
    {
        return true;
    }

    CommandContext.ObtainCommandBuffer();

    VkBuffer DefaultBuffer = DefaultResources.NullBuffer;
    CommandContext.GetCommandBuffer()->FillBuffer(DefaultBuffer, 0, VULKAN_DEFAULT_BUFFER_NUM_BYTES, 0);

    VkImage DefaultImage = DefaultResources.NullImage;

    VkImageMemoryBarrier2 ImageBarrier = {};
    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.image                           = DefaultImage;
    ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
    ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VK_FORMAT_R8G8B8A8_UNORM);
    ImageBarrier.subresourceRange.baseArrayLayer = 0;
    ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageBarrier.subresourceRange.baseMipLevel   = 0;
    ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    CommandContext.GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);

    VkBufferImageCopy BufferImageCopy = {};
    BufferImageCopy.bufferOffset                    = 0;
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = ImageBarrier.subresourceRange.aspectMask;
    BufferImageCopy.imageSubresource.mipLevel       = 0;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { 0, 0, 0 };
    BufferImageCopy.imageExtent                     = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    CommandContext.GetBarrierBatcher().FlushBarriers(CommandContext.GetCommandBuffer());
    CommandContext.GetCommandBuffer()->CopyBufferToImage(DefaultBuffer, DefaultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

    ImageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImageBarrier.newLayout     = VK_IMAGE_LAYOUT_GENERAL;
    ImageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    ImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    CommandContext.GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
    CommandContext.FinishCommandBuffer(true);
    return true;
}

bool FVulkanDevice::FindOrCreateSampler(const VkSamplerCreateInfo& SamplerCreateInfo, VkSampler& OutSampler)
{
    CHECK(SamplerCreateInfo.pNext == nullptr);

    TScopedLock Lock(SamplerMapCS);

    FVulkanHashableSamplerCreateInfo HashableCreateInfo;
    HashableCreateInfo.Flags                   = SamplerCreateInfo.flags;
    HashableCreateInfo.MagFilter               = SamplerCreateInfo.magFilter;
    HashableCreateInfo.MinFilter               = SamplerCreateInfo.minFilter;
    HashableCreateInfo.MipmapMode              = SamplerCreateInfo.mipmapMode;
    HashableCreateInfo.AddressModeU            = SamplerCreateInfo.addressModeU;
    HashableCreateInfo.AddressModeV            = SamplerCreateInfo.addressModeV;
    HashableCreateInfo.AddressModeW            = SamplerCreateInfo.addressModeW;
    HashableCreateInfo.MipLodBias              = SamplerCreateInfo.mipLodBias;
    HashableCreateInfo.AnisotropyEnable        = SamplerCreateInfo.anisotropyEnable;
    HashableCreateInfo.MaxAnisotropy           = SamplerCreateInfo.maxAnisotropy;
    HashableCreateInfo.CompareEnable           = SamplerCreateInfo.compareEnable;
    HashableCreateInfo.CompareOp               = SamplerCreateInfo.compareOp;
    HashableCreateInfo.MinLod                  = SamplerCreateInfo.minLod;
    HashableCreateInfo.MaxLod                  = SamplerCreateInfo.maxLod;
    HashableCreateInfo.BorderColor             = SamplerCreateInfo.borderColor;
    HashableCreateInfo.UnnormalizedCoordinates = SamplerCreateInfo.unnormalizedCoordinates;

    if (VkSampler* Sampler = SamplerMap.Find(HashableCreateInfo))
    {
        OutSampler = *Sampler;
        return true;
    }

    VkResult Result = vkCreateSampler(GetVkDevice(), &SamplerCreateInfo, nullptr, &OutSampler);
    if (VULKAN_FAILED(Result))
    {
        OutSampler = VK_NULL_HANDLE;
        VULKAN_ERROR_CRITICAL("Failed to create sampler");
        return false;
    }
    else
    {
        const FString DebugName = FString::CreateFormatted("Sampler %d", SamplerMap.Size());
        VulkanDebugUtilsEXT::SetObjectName(GetVkDevice(), DebugName.Data(), OutSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    SamplerMap.Add(HashableCreateInfo, OutSampler);
    return true;
}

FVulkanQueryPoolManager* FVulkanDevice::GetQueryPoolManager(EQueryType QueryType)
{
    if (QueryType == EQueryType::Timestamp)
    {
        CHECK(TimingQueryPoolManager != nullptr);
        return TimingQueryPoolManager;
    }
    else if (QueryType == EQueryType::Occlusion)
    {
        CHECK(OcclusionQueryPoolManager != nullptr);
        return OcclusionQueryPoolManager;
    }
    else
    {
        DEBUG_BREAK();
        return nullptr;
    }
}

uint32 FVulkanDevice::GetQueueIndexFromType(EVulkanCommandQueueType Type) const
{
    if (Type == EVulkanCommandQueueType::Graphics)
    {
        return QueueIndicies->GraphicsQueueIndex;
    }
    else if (Type == EVulkanCommandQueueType::Compute)
    {
        return QueueIndicies->ComputeQueueIndex;
    }
    else if (Type == EVulkanCommandQueueType::Copy)
    {
        return QueueIndicies->CopyQueueIndex;
    }
    else
    {
        VULKAN_ERROR_CRITICAL("Invalid CommandQueueType");
        return (~0U);
    }
}

bool FVulkanDefaultResources::Initialize(FVulkanDevice& Device)
{
    // We only need to actually create these resources if we don't support null-descriptors
    if (!GVulkanSupportsNullDescriptors)
    {
        if (!InitializeNullBufferAndImage(Device))
        {
            return false;
        }
    }

    // Create a NullSampler
    VkSamplerCreateInfo SamplerCreateInfo = {};
    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.mipLodBias              = 0.0f;
    SamplerCreateInfo.anisotropyEnable        = VK_FALSE;
    SamplerCreateInfo.maxAnisotropy           = 1.0f;
    SamplerCreateInfo.compareEnable           = VK_FALSE;
    SamplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
    SamplerCreateInfo.minLod                  = 0.0f;
    SamplerCreateInfo.maxLod                  = VK_LOD_CLAMP_NONE;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;

    if (!Device.FindOrCreateSampler(SamplerCreateInfo, NullSampler))
    {
        VULKAN_ERROR_CRITICAL("vkCreateSampler failed");
        return false;
    }
    else
    {
        VulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullSampler", NullSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    return true;
}

bool FVulkanDefaultResources::InitializeNullBufferAndImage(FVulkanDevice& Device)
{
    // Create NullBuffer
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = VULKAN_DEFAULT_BUFFER_NUM_BYTES;
	BufferCreateInfo.usage =
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VkResult Result = vkCreateBuffer(Device.GetVkDevice(), &BufferCreateInfo, nullptr, &NullBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Buffer");
        return false;
    }
    else
    {
        VulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullBuffer", NullBuffer, VK_OBJECT_TYPE_BUFFER);
    }

    {
        FVulkanMemoryStorage TempStorage(&Device);
        NullBufferStorage.Swap(TempStorage);
    }

    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, BufferCreateInfo.usage, 0, BufferCreateInfo.size, 256, NullBufferStorage))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate buffer memory");
        return false;
    }

    VkResult BindResult = vkBindBufferMemory(Device.GetVkDevice(), NullBuffer, NullBufferStorage.GetMemory(), NullBufferStorage.GetMemoryOffset());
    if (VULKAN_FAILED(BindResult))
    {
        VULKAN_ERROR_CRITICAL("Failed to bind NullBuffer memory");
        return false;
    }

    // Create a NullImage
    constexpr VkExtent3D NullExtent = 
    { 
        VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 
        VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT,
        1 
    };

    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType             = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ImageCreateInfo.format                = VK_FORMAT_R8G8B8A8_UNORM;
    ImageCreateInfo.extent                = NullExtent;
    ImageCreateInfo.mipLevels             = 1;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.arrayLayers           = 1;

    Result = vkCreateImage(Device.GetVkDevice(), &ImageCreateInfo, nullptr, &NullImage);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create image");
        return false;
    }
    else
    {
        VulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImage", NullImage, VK_OBJECT_TYPE_IMAGE);
    }

    {
        FVulkanMemoryStorage TempStorage(&Device);
        NullImageStorage.Swap(TempStorage);
    }

    if (!MemoryManager.AllocateImageMemory(NullImage, ImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, NullImageStorage))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate ImageMemory");
        return false;
    }

    VkResult BindImageResult = vkBindImageMemory(Device.GetVkDevice(), NullImage, NullImageStorage.GetMemory(), NullImageStorage.GetMemoryOffset());
    if (VULKAN_FAILED(BindImageResult))
    {
        VULKAN_ERROR_CRITICAL("Failed to bind NullImage memory");
        return false;
    }

    // Create NullImageView
    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags                           = 0;
    ImageViewCreateInfo.format                          = ImageCreateInfo.format;
    ImageViewCreateInfo.image                           = NullImage;
    ImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    ImageViewCreateInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    Result = vkCreateImageView(Device.GetVkDevice(), &ImageViewCreateInfo, nullptr, &NullImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkCreateImageView failed");
        return false;
    }
    else
    {
        VulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImageView", NullImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    return true;
}

void FVulkanDefaultResources::Release(FVulkanDevice& Device)
{
    VkDevice VulkanDevice = Device.GetVkDevice();
    if (VULKAN_CHECK_HANDLE(NullBuffer))
    {
        vkDestroyBuffer(VulkanDevice, NullBuffer, nullptr);
        NullBuffer = VK_NULL_HANDLE;
        NullBufferStorage.ReleaseMemory();
    }

    if (VULKAN_CHECK_HANDLE(NullImageView))
    {
        vkDestroyImageView(VulkanDevice, NullImageView, nullptr);
        NullImageView = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(NullImage))
    {
        vkDestroyImage(VulkanDevice, NullImage, nullptr);
        NullImage = VK_NULL_HANDLE;
        NullImageStorage.ReleaseMemory();
    }

    NullSampler = VK_NULL_HANDLE;
}
