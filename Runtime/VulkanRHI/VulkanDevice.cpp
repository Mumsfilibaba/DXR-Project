#include "VulkanDevice.h"
#include "VulkanLoader.h"
#include "VulkanCommandContext.h"
#include "VulkanInstance.h"
#include "Core/Containers/Array.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"

////////////////////////////////////////////////////
// Global variables that describe different features

VULKANRHI_API bool GVulkanForceBinding = false;
VULKANRHI_API bool GVulkanForceDedicatedAllocations = false;
VULKANRHI_API bool GVulkanForceDedicatedImageAllocations = GVulkanForceDedicatedAllocations || true;
VULKANRHI_API bool GVulkanForceDedicatedBufferAllocations = GVulkanForceDedicatedAllocations || false;
VULKANRHI_API bool GVulkanAllowNullDescriptors = true;
VULKANRHI_API bool GVulkanAllowGeometryShaders = false;

VULKANRHI_API bool GVulkanSupportsDepthClip = false;
VULKANRHI_API bool GVulkanSupportsConservativeRasterization = false;
VULKANRHI_API bool GVulkanSupportsPipelineCacheControl = false;
VULKANRHI_API bool GVulkanSupportsAccelerationStructures = false;
VULKANRHI_API bool GVulkanSupportsMultiviews = false;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper

static bool FilterExtensions(const VkExtensionProperties& ExtensionProperty)
{
    if (!GVulkanAllowNullDescriptors && FCString::Strcmp(ExtensionProperty.extensionName, VK_EXT_ROBUSTNESS_2_EXTENSION_NAME) == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static bool CheckAvailability(VkPhysicalDevice PhysicalDevice, const FVulkanPhysicalDeviceCreateInfo& DeviceDesc)
{
    // Get the physical device properties
    VkPhysicalDeviceProperties AdapterProperties;
    vkGetPhysicalDeviceProperties(PhysicalDevice, &AdapterProperties);

    // Get the physical device features
    VkPhysicalDeviceFeatures2 DeviceFeatures2;
    FMemory::Memzero(&DeviceFeatures2);
    DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    
    // Helper for checking for extensions
    FVulkanStructureHelper DeviceFeaturesHelper(DeviceFeatures2);

    // Vulkan 1.1 features
    VkPhysicalDeviceVulkan11Features DeviceFeatures11;
    FMemory::Memzero(&DeviceFeatures11);
    DeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    DeviceFeaturesHelper.AddNext(DeviceFeatures11);

    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features DeviceFeatures12;
    FMemory::Memzero(&DeviceFeatures12);
    DeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    DeviceFeaturesHelper.AddNext(DeviceFeatures12);

    // Get the physical device features
    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);

    // Check Vulkan 1.0 features
    const uint64 NumFeatures10 = ((OFFSETOF(VkPhysicalDeviceFeatures, inheritedQueries) - OFFSETOF(VkPhysicalDeviceFeatures, robustBufferAccess)) / sizeof(VkBool32)) + 1;

    const VkBool32* RequiredFeatures  = reinterpret_cast<const VkBool32*>(&DeviceDesc.RequiredFeatures);
    const VkBool32* AvailableFeatures = reinterpret_cast<const VkBool32*>(&DeviceFeatures2.features);

    bool bHasAllFeatures = true;
    for (uint32 FeatureIndex = 0; FeatureIndex < NumFeatures10; ++FeatureIndex)
    {
        if (RequiredFeatures[FeatureIndex]  == VK_TRUE && AvailableFeatures[FeatureIndex] != VK_TRUE)
        {
            VULKAN_WARNING("PhysicalDevice '%s' does not support all device-features. See VkPhysicalDeviceFeatures[%d]", AdapterProperties.deviceName, FeatureIndex);
            bHasAllFeatures = false;
            break;
        }
    }

    if (!bHasAllFeatures)
    {
        return false;
    }

    // Check Vulkan 1.1 features
    const uint64 NumFeatures11 = ((OFFSETOF(VkPhysicalDeviceVulkan11Features, shaderDrawParameters) - OFFSETOF(VkPhysicalDeviceVulkan11Features, storageBuffer16BitAccess)) / sizeof(VkBool32)) + 1;

    RequiredFeatures  = reinterpret_cast<const VkBool32*>(&DeviceDesc.RequiredFeatures11.storageBuffer16BitAccess);
    AvailableFeatures = reinterpret_cast<const VkBool32*>(&DeviceFeatures11.storageBuffer16BitAccess);

    bHasAllFeatures = true;
    for (uint32 FeatureIndex = 0; FeatureIndex < NumFeatures11; ++FeatureIndex)
    {
        if (RequiredFeatures[FeatureIndex]  == VK_TRUE && AvailableFeatures[FeatureIndex] != VK_TRUE)
        {
            VULKAN_WARNING("PhysicalDevice '%s' does not support all device-features. See VkPhysicalDeviceVulkan11Features[%d]", AdapterProperties.deviceName, FeatureIndex);
            bHasAllFeatures = false;
            break;
        }
    }

    if (!bHasAllFeatures)
    {
        return false;
    }

    // Check Vulkan 1.2 features
    const uint64 NumFeatures12 = ((OFFSETOF(VkPhysicalDeviceVulkan12Features, subgroupBroadcastDynamicId) - OFFSETOF(VkPhysicalDeviceVulkan12Features, samplerMirrorClampToEdge)) / sizeof(VkBool32)) + 1;

    RequiredFeatures  = reinterpret_cast<const VkBool32*>(&DeviceDesc.RequiredFeatures12.samplerMirrorClampToEdge);
    AvailableFeatures = reinterpret_cast<const VkBool32*>(&DeviceFeatures12.samplerMirrorClampToEdge);

    bHasAllFeatures = true;
    for (uint32 FeatureIndex = 0; FeatureIndex < NumFeatures12; ++FeatureIndex)
    {
        if (RequiredFeatures[FeatureIndex]  == VK_TRUE && AvailableFeatures[FeatureIndex] != VK_TRUE)
        {
            VULKAN_WARNING("PhysicalDevice '%s' does not support all device-features. See VkPhysicalDeviceVulkan12Features[%d]", AdapterProperties.deviceName, FeatureIndex);
            bHasAllFeatures = false;
            break;
        }
    }

    if (!bHasAllFeatures)
    {
        return false;
    }

    return true;
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

bool FVulkanPhysicalDevice::Initialize(const FVulkanPhysicalDeviceCreateInfo& AdapterDesc)
{
    VkResult Result = VK_SUCCESS;

    uint32 AdapterCount = 0;
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve AdapterCount");
        return false;
    }

    TArray<VkPhysicalDevice> Adapters(AdapterCount);
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, Adapters.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve available Adapters");
        return false;
    }

    if (AdapterCount < 1)
    {
        VULKAN_ERROR("No Adapters available");
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
            
            LOG_INFO("    '%s' Supports Vulkan '%s'", AdapterProperties.deviceName, GetVersionAsString(AdapterProperties.apiVersion).GetCString());
        }
    }

    // GPU selection
    TArray<VkPhysicalDevice> AcceptedAdapers;
    AcceptedAdapers.Reserve(Adapters.Size());
    
    for (VkPhysicalDevice CurrentAdapter : Adapters)
    {
        VkPhysicalDeviceProperties AdapterProperties;
        vkGetPhysicalDeviceProperties(CurrentAdapter, &AdapterProperties);

        if (!CheckAvailability(CurrentAdapter, AdapterDesc))
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
        for (const CHAR* RequiredExtension : AdapterDesc.RequiredExtensionNames)
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

        // TODO: At this point we now the device is acceptable, now check for the most optional

        if (bIsAllExtensionsSupported)
        {
            AcceptedAdapers.Add(CurrentAdapter);
            if (AdapterProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                PhysicalDevice = CurrentAdapter;
            }
        }
    }

    // If no discrete adapter is found, select the first accepted one
    if (!VULKAN_CHECK_HANDLE(PhysicalDevice) && !AcceptedAdapers.IsEmpty())
    {
        PhysicalDevice = AcceptedAdapers[0];
    }
    
    // If there still is no physical-device, then we failed
    if (!VULKAN_CHECK_HANDLE(PhysicalDevice))
    {
        VULKAN_ERROR("Failed to find a suitable PhysicalDevice");
        return false;
    }
    
    // Retrieve and cache information about the physical-device
    vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);
    vkGetPhysicalDeviceFeatures(PhysicalDevice, &DeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &DeviceMemoryProperties);

    // Get Get Physical Device Properties
    FMemory::Memzero(&DeviceProperties2);
    DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    
    // Helper for checking for extensions
    FVulkanStructureHelper DevicePropertiesHelper(DeviceProperties2);
    
#if VK_EXT_conservative_rasterization
    FMemory::Memzero(&ConservativeRasterizationProperties);
    ConservativeRasterizationProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
    DevicePropertiesHelper.AddNext(ConservativeRasterizationProperties);
#endif

    vkGetPhysicalDeviceProperties2(PhysicalDevice, &DeviceProperties2);

    // Get Physical Device Feature (For Vulkan 1.1 and Vulkan 1.2 and extensions)
    FMemory::Memzero(&DeviceFeatures2);
    DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Helper for checking for extensions
    FVulkanStructureHelper DeviceFeaturesHelper(DeviceFeatures2);
    
#if VK_EXT_depth_clip_enable
    FMemory::Memzero(&DepthClipEnableFeatures);
    DepthClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
    DeviceFeaturesHelper.AddNext(DepthClipEnableFeatures);
#endif
    
#if VK_EXT_robustness2
    FMemory::Memzero(&Robustness2Features);
    Robustness2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    DeviceFeaturesHelper.AddNext(Robustness2Features);
#endif
    
#if VK_EXT_pipeline_creation_cache_control
    FMemory::Memzero(&PipelineCreationCacheControlFeatures);
    PipelineCreationCacheControlFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT;
    DeviceFeaturesHelper.AddNext(PipelineCreationCacheControlFeatures);
#endif
    
#if VK_KHR_acceleration_structure
    FMemory::Memzero(&AccelerationStructureFeatures);
    AccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    DeviceFeaturesHelper.AddNext(AccelerationStructureFeatures);
#endif

#if VK_KHR_ray_tracing_pipeline
    FMemory::Memzero(&RayTracingPipelineFeatures);
    RayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    DeviceFeaturesHelper.AddNext(RayTracingPipelineFeatures);
#endif

#if VK_KHR_synchronization2
    FMemory::Memzero(&Synchronization2Features);
    Synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    DeviceFeaturesHelper.AddNext(Synchronization2Features);
#endif

    // Vulkan 1.1 features
    FMemory::Memzero(&DeviceFeatures11);
    DeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    DeviceFeaturesHelper.AddNext(DeviceFeatures11);

    // Vulkan 1.2 features
    FMemory::Memzero(&DeviceFeatures12);
    DeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    DeviceFeaturesHelper.AddNext(DeviceFeatures12);
    
    // Get the physical device features
    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);

    // Get Physical Device Memory Properties
    FMemory::Memzero(&DeviceMemoryProperties2);
    DeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice, &DeviceMemoryProperties2);
    
    VULKAN_INFO("Using adapter '%s' Which supports Vulkan '%s'", DeviceProperties.deviceName, GetVersionAsString(DeviceProperties.apiVersion).GetCString());
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
            VULKAN_INFO("Queue[%d]: %s", Index, PropertyString.GetCString());
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
    VkFormatProperties FormatProperties;
    FMemory::Memzero(&FormatProperties);
    
    vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Format, &FormatProperties);
    return FormatProperties;
}

FVulkanDevice::FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , PhysicalDevice(InAdapter)
    , Device(VK_NULL_HANDLE)
    , RenderPassCache(nullptr)
    , UploadHeap(nullptr)
    , MemoryManager(nullptr)
    , FenceManager(nullptr)
    , PipelineLayoutManager(nullptr)
    , PipelineStateManager(nullptr)
    , DescriptorSetCache(nullptr)
    , TimingQueryPoolManager(nullptr)
    , OcclusionQueryPoolManager(nullptr)
{
    // Release DescriptorSetCache
    DescriptorSetCache = new FVulkanDescriptorSetCache(this);
    
    // Release all QueryPools
    TimingQueryPoolManager = new FVulkanQueryPoolManager(this, EQueryType::Timestamp);
    OcclusionQueryPoolManager = new FVulkanQueryPoolManager(this, EQueryType::Occlusion);

    // Release all PipelineLayoutManager
    PipelineLayoutManager = new FVulkanPipelineLayoutManager(this);

    // Create RenderPassCache
    RenderPassCache = new FVulkanRenderPassCache(this);

    // Ensure that the upload allocator is released before we destroy the device
    UploadHeap = new FVulkanUploadHeapAllocator(this);

    // Release all Fences
    FenceManager = new FVulkanFenceManager(this);
    
    // Release all heaps (Which will check for memory leaks)
    MemoryManager = new FVulkanMemoryManager(this);
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
    
    // Release DescriptorSetCache
    SAFE_DELETE(DescriptorSetCache);
    
    // Release all QueryPools
    SAFE_DELETE(TimingQueryPoolManager);
    SAFE_DELETE(OcclusionQueryPoolManager);

    // Release all PipelineLayoutManager
    SAFE_DELETE(PipelineLayoutManager);

    // Ensure that all RenderPasses and FrameBuffers are destroyed
    SAFE_DELETE(RenderPassCache);

    // Ensure that the upload allocator is released before we destroy the device
    SAFE_DELETE(UploadHeap);

    // Release all Fences
    SAFE_DELETE(FenceManager);
    
    // Release all heaps (Which will check for memory leaks)
    SAFE_DELETE(MemoryManager);
    
    // Destroy the device here
    if (VULKAN_CHECK_HANDLE(Device))
    {
        vkDestroyDevice(Device, nullptr);
    }
}

bool FVulkanDevice::Initialize(const FVulkanDeviceCreateInfo& DeviceDesc)
{
    if (!PhysicalDevice)
    {
        VULKAN_ERROR("PhysicalDevice is not initalized correctly");
        return false;
    }

    VkResult Result = VK_SUCCESS;

    // TODO: Device layers

    // Verify Extensions
    uint32 DeviceExtensionCount = 0;
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve the device extension count");
        return false;
    }

    TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve the device extensions");
        return false;
    }

    TArray<const CHAR*> EnabledExtensionNames;
    for (const VkExtensionProperties& ExtensionProperty : AvailableDeviceExtensions)
    {
        const auto CompareExtension = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(ExtensionProperty.extensionName, Other) == 0;
        };

        // Filter out some extensions based on CVars etc. (If an extension is available but we want to disable it for debugging or similar)
        if (!FilterExtensions(ExtensionProperty))
        {
            continue;
        }

        if (DeviceDesc.RequiredExtensionNames.ContainsWithPredicate(CompareExtension) || DeviceDesc.OptionalExtensionNames.ContainsWithPredicate(CompareExtension))
        {
            EnabledExtensionNames.Add(ExtensionProperty.extensionName);
            ExtensionNames.Emplace(ExtensionProperty.extensionName);
        }
    }

    for (const CHAR* ExtensionName : DeviceDesc.RequiredExtensionNames)
    {
        const auto CompareExtension = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(ExtensionName, Other) == 0;
        };

        if (!EnabledExtensionNames.ContainsWithPredicate(CompareExtension))
        {
            VULKAN_ERROR("Instance layer '%s' could not be enabled", ExtensionName);
            return false;
        }
    }

    // Log enabled extensions and layers
    IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging");
    if (VerboseVulkan && VerboseVulkan->GetBool())
    {
        if (!EnabledExtensionNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Device Extensions:");
            for (const CHAR* ExtensionName : EnabledExtensionNames)
            {
                LOG_INFO("    %s", ExtensionName);
            }
        }
    }

    QueueIndicies = FVulkanPhysicalDevice::GetQueueFamilyIndices(PhysicalDevice->GetVkPhysicalDevice());
    if (!QueueIndicies)
    {
        VULKAN_ERROR("Failed to query queue indices");
        return false;
    }

    VULKAN_INFO("QueueIndicies: Graphics=%d, Compute=%d, Copy=%d", QueueIndicies->GraphicsQueueIndex, QueueIndicies->ComputeQueueIndex, QueueIndicies->CopyQueueIndex);

    TArray<VkDeviceQueueCreateInfo> QueueCreateInfos;

    const TSet<uint32> UniqueQueueFamilies =
    {
        QueueIndicies->GraphicsQueueIndex,
        QueueIndicies->CopyQueueIndex,
        QueueIndicies->ComputeQueueIndex
    };

    const float DefaultQueuePriority = 0.0f;
    for (uint32 QueueFamiliy : UniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo QueueCreateInfo;
        FMemory::Memzero(&QueueCreateInfo);

        QueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.pQueuePriorities = &DefaultQueuePriority;
        QueueCreateInfo.queueFamilyIndex = QueueFamiliy;
        QueueCreateInfo.queueCount       = 1;

        QueueCreateInfos.Add(QueueCreateInfo);
    }

    VkDeviceCreateInfo DeviceCreateInfo;
    FMemory::Memzero(&DeviceCreateInfo);

    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.enabledLayerCount       = 0;
    DeviceCreateInfo.ppEnabledLayerNames     = nullptr;
    DeviceCreateInfo.enabledExtensionCount   = EnabledExtensionNames.Size();
    DeviceCreateInfo.ppEnabledExtensionNames = EnabledExtensionNames.Data();
    DeviceCreateInfo.queueCreateInfoCount    = QueueCreateInfos.Size();
    DeviceCreateInfo.pQueueCreateInfos       = QueueCreateInfos.Data();

    VkPhysicalDeviceFeatures2 DeviceFeatures2;
    FMemory::Memzero(&DeviceFeatures2);
    DeviceFeatures2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    DeviceFeatures2.features = DeviceDesc.RequiredFeatures;

    const VkPhysicalDeviceFeatures& AvailableDeviceFeatures = GetPhysicalDevice()->GetFeatures();
    if (AvailableDeviceFeatures.robustBufferAccess)
    {
        DeviceFeatures2.features.robustBufferAccess = VK_TRUE;
    }

    // Vulkan 1.1 Features
    VkPhysicalDeviceVulkan11Features DeviceFeaturesVulkan11 = DeviceDesc.RequiredFeatures11;
    DeviceFeaturesVulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    const VkPhysicalDeviceVulkan11Features& AvailableDeviceFeaturesVulkan11 = GetPhysicalDevice()->GetFeaturesVulkan11();
    if (AvailableDeviceFeaturesVulkan11.multiview)
    {
        DeviceFeaturesVulkan11.multiview = VK_TRUE;
        GVulkanSupportsMultiviews = true;
    }

    // Vulkan 1.2 Features
    VkPhysicalDeviceVulkan12Features DeviceFeaturesVulkan12 = DeviceDesc.RequiredFeatures12;
    DeviceFeaturesVulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    // Enable 'bufferDeviceAddress' if available
    const VkPhysicalDeviceVulkan12Features& AvailableDeviceFeaturesVulkan12 = GetPhysicalDevice()->GetFeaturesVulkan12();
    if (AvailableDeviceFeaturesVulkan12.bufferDeviceAddress)
    {
        DeviceFeaturesVulkan12.bufferDeviceAddress = VK_TRUE;
    }
    // Enable 'timelineSemaphore' if available
    if (AvailableDeviceFeaturesVulkan12.timelineSemaphore)
    {
        DeviceFeaturesVulkan12.timelineSemaphore = VK_TRUE;
    }
    // Enable 'descriptorIndexing' if available
    if (AvailableDeviceFeaturesVulkan12.descriptorIndexing)
    {
        DeviceFeaturesVulkan12.descriptorIndexing = VK_TRUE;
    }

    // Construct the pNext chain
    FVulkanStructureHelper DeviceCreateHelper(DeviceCreateInfo);
    DeviceCreateHelper.AddNext(DeviceFeatures2);
    DeviceCreateHelper.AddNext(DeviceFeaturesVulkan11);
    DeviceCreateHelper.AddNext(DeviceFeaturesVulkan12);

    // Check and enable extension features
#if VK_EXT_robustness2
    VkPhysicalDeviceRobustness2FeaturesEXT Robustness2Features;
    if (IsExtensionEnabled(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
    {
        const VkPhysicalDeviceRobustness2FeaturesEXT& AvailableRobustness2Features = GetPhysicalDevice()->GetRobustness2Features();

        FMemory::Memzero(&Robustness2Features);
        Robustness2Features.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
        Robustness2Features.robustImageAccess2  = AvailableRobustness2Features.robustImageAccess2;
        Robustness2Features.robustBufferAccess2 = AvailableRobustness2Features.robustBufferAccess2;
        Robustness2Features.nullDescriptor      = AvailableRobustness2Features.nullDescriptor;
        DeviceCreateHelper.AddNext(Robustness2Features);
    }
#endif

#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT DepthClipEnableFeatures;
    if (IsExtensionEnabled(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME))
    {
        const VkPhysicalDeviceDepthClipEnableFeaturesEXT& AvailableDepthClipEnableFeatures = GetPhysicalDevice()->GetDepthClipEnableFeatures();
        if (AvailableDepthClipEnableFeatures.depthClipEnable && AvailableDeviceFeatures.depthClamp)
        {
            FMemory::Memzero(&DepthClipEnableFeatures);
            DepthClipEnableFeatures.sType           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
            DepthClipEnableFeatures.depthClipEnable = VK_TRUE;
            DeviceFeatures2.features.depthClamp     = VK_TRUE;
            DeviceCreateHelper.AddNext(DepthClipEnableFeatures);
            GVulkanSupportsDepthClip = true;
        }
    }
#endif

#if VK_EXT_conservative_rasterization
    if (IsExtensionEnabled(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME))
    {
        GVulkanSupportsConservativeRasterization = true;
    }
#endif

#if VK_EXT_pipeline_creation_cache_control
    VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT PipelineCreationCacheControlFeatures;
    if (IsExtensionEnabled(VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME))
    {
        const VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT& AvailablePipelineCreationCacheControlFeatures = GetPhysicalDevice()->GetPipelineCreationCacheControlFeatures();
        if (AvailablePipelineCreationCacheControlFeatures.pipelineCreationCacheControl)
        {
            FMemory::Memzero(&PipelineCreationCacheControlFeatures);
            PipelineCreationCacheControlFeatures.sType                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT;
            PipelineCreationCacheControlFeatures.pipelineCreationCacheControl = VK_TRUE;
            DeviceCreateHelper.AddNext(PipelineCreationCacheControlFeatures);
            GVulkanSupportsPipelineCacheControl = true;
        }
    }
#endif

#if VK_KHR_acceleration_structure
    VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures;
    if (IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
    {
        const VkPhysicalDeviceAccelerationStructureFeaturesKHR& AvailableAccelerationStructureFeatures = GetPhysicalDevice()->GetAccelerationStructureFeatures();
        if (AvailableAccelerationStructureFeatures.accelerationStructure)
        {
            FMemory::Memzero(&AccelerationStructureFeatures);
            AccelerationStructureFeatures.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            AccelerationStructureFeatures.accelerationStructure = VK_TRUE;
            DeviceCreateHelper.AddNext(AccelerationStructureFeatures);
            GVulkanSupportsAccelerationStructures = true;
        }
    }
#endif

#if VK_KHR_ray_tracing_pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures;
    if (IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
    {
        const VkPhysicalDeviceRayTracingPipelineFeaturesKHR& AvailableRayTracingPipelineFeatures = GetPhysicalDevice()->GetRayTracingPipelineFeatures();
        if (AvailableRayTracingPipelineFeatures.rayTracingPipeline)
        {
            FMemory::Memzero(&RayTracingPipelineFeatures);
            RayTracingPipelineFeatures.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            RayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
            DeviceCreateHelper.AddNext(RayTracingPipelineFeatures);
        }
    }
#endif

#if VK_KHR_synchronization2
    VkPhysicalDeviceSynchronization2Features Synchronization2Features;
    if (IsExtensionEnabled(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME))
    {
        const VkPhysicalDeviceSynchronization2Features& AvailableSynchronization2Features = GetPhysicalDevice()->GetSynchronization2Features();
        if (AvailableSynchronization2Features.synchronization2)
        {
            FMemory::Memzero(&Synchronization2Features);
            Synchronization2Features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
            Synchronization2Features.synchronization2 = VK_TRUE;
            DeviceCreateHelper.AddNext(Synchronization2Features);
        }
        else
        {
            VULKAN_ERROR("VK_KHR_synchronization2 is not available");
            return false;
        }
    }
    else
    {
        VULKAN_ERROR("VK_KHR_synchronization2 is not available");
        return false;
    }
#endif

    Result = vkCreateDevice(PhysicalDevice->GetVkPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Device");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanDevice::PostLoaderInitalize()
{
    // Initialize PipelineStateManager
    PipelineStateManager = new FVulkanPipelineStateManager(this);
    if (!PipelineStateManager->Initialize())
    {
        return false;
    }

    // Ray Tracing Support
    VkPhysicalDevice PhysicalDeviceHandle = GetPhysicalDevice()->GetVkPhysicalDevice();
    if (IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) && IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
    {
        VkPhysicalDeviceProperties2 DeviceProperties2;
        FMemory::Memzero(&DeviceProperties2);
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingPipelineProperties;
        FMemory::Memzero(&RayTracingPipelineProperties);
        RayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        FVulkanStructureHelper DevicePropertiesHelper(DeviceProperties2);
        DevicePropertiesHelper.AddNext(RayTracingPipelineProperties);
        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        // Check if RayQueries are supported, then the Tier is kind of like Tier 1.1 (Inline RayTracing in DXR)
        if (IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        {
            GRHIRayTracingTier = ERayTracingTier::Tier1_1;
        }
        else
        {
            GRHIRayTracingTier = ERayTracingTier::Tier1;
        }

        GRHIRayTracingMaxRecursionDepth = RayTracingPipelineProperties.maxRayRecursionDepth;
    }
    else
    {
        GRHIRayTracingTier = ERayTracingTier::NotSupported;
    }

    GRHISupportsRayTracing = GRHIRayTracingTier != ERayTracingTier::NotSupported;

    // Variable Rate Shading Support
    if (IsExtensionEnabled(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
    {
        VkPhysicalDeviceProperties2 DeviceProperties2;
        FMemory::Memzero(&DeviceProperties2);
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceFragmentShadingRatePropertiesKHR FragmentShadingRateProperties;
        FMemory::Memzero(&FragmentShadingRateProperties);
        FragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

        FVulkanStructureHelper DevicePropertiesHelper(DeviceProperties2);
        DevicePropertiesHelper.AddNext(FragmentShadingRateProperties);
        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        // TODO: Finish this part
        GRHIShadingRateImageTileSize = 0;
        GRHIShadingRateTier = EShadingRateTier::NotSupported;
    }
    else
    {
        GRHIShadingRateTier = EShadingRateTier::NotSupported;
    }

    GRHISupportsVRS = GRHIShadingRateTier != EShadingRateTier::NotSupported;

    // GeometryShader Support 
    const VkPhysicalDeviceFeatures& DeviceFeatures = PhysicalDevice->GetFeatures();
    if (GVulkanAllowGeometryShaders && DeviceFeatures.geometryShader)
    {
        GRHISupportsGeometryShaders = true;
    }

    // View Instancing Support
    if (IsExtensionEnabled(VK_KHR_MULTIVIEW_EXTENSION_NAME))
    {
        VkPhysicalDeviceProperties2 DeviceProperties2;
        FMemory::Memzero(&DeviceProperties2);
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceMultiviewPropertiesKHR MultiviewProperties;
        FMemory::Memzero(&MultiviewProperties);
        MultiviewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;

        FVulkanStructureHelper DevicePropertiesHelper(DeviceProperties2);
        DevicePropertiesHelper.AddNext(MultiviewProperties);
        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        GRHIMaxViewInstanceCount = MultiviewProperties.maxMultiviewViewCount;
        GRHISupportsViewInstancing = true;
    }
    else
    {
        GRHIMaxViewInstanceCount = 0;
        GRHISupportsViewInstancing = false;
    }

    return true;
}

// Initialize default resources that are just for null bindings
bool FVulkanDevice::InitializeDefaultResources(FVulkanCommandContext& CommandContext)
{
    // Create the resources
    if (!DefaultResources.Initialize(*this))
    {
        VULKAN_ERROR("Failed to create DefaultResources");
        return false;
    }

    // If null-descriptors are supported then we can return here, since we have no DefaultResources to upload
    if (FVulkanRobustness2EXT::SupportsNullDescriptors())
    {
        return true;
    }

    CommandContext.ObtainCommandBuffer();

    VkBuffer DefaultBuffer = DefaultResources.NullBuffer;
    CommandContext.GetCommandBuffer()->FillBuffer(DefaultBuffer, 0, VULKAN_DEFAULT_BUFFER_NUM_BYTES, 0);

    VkImage DefaultImage = DefaultResources.NullImage;

    VkImageMemoryBarrier2 ImageBarrier;
    FMemory::Memzero(&ImageBarrier);

    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
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

    VkBufferImageCopy BufferImageCopy;
    BufferImageCopy.bufferOffset                    = 0;
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = ImageBarrier.subresourceRange.aspectMask;
    BufferImageCopy.imageSubresource.mipLevel       = 0;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { 0, 0, 0 };
    BufferImageCopy.imageExtent                     = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    CommandContext.GetBarrierBatcher().FlushBarriers();
    CommandContext.GetCommandBuffer()->CopyBufferToImage(DefaultBuffer, DefaultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

    ImageBarrier.newLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
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
        VULKAN_ERROR("Failed to create sampler");
        return false;
    }
    else
    {
        const FString DebugName = FString::CreateFormatted("Sampler %d", SamplerMap.Size());
        FVulkanDebugUtilsEXT::SetObjectName(GetVkDevice(), DebugName.Data(), OutSampler, VK_OBJECT_TYPE_SAMPLER);
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
        VULKAN_ERROR("Invalid CommandQueueType");
        return (~0U);
    }
}

bool FVulkanDefaultResources::Initialize(FVulkanDevice& Device)
{
    // We only need to actually create these resources if we don't support null-descriptors
    if (!FVulkanRobustness2EXT::SupportsNullDescriptors())
    {
        if (!InitializeBuffersAndImages(Device))
        {
            return false;
        }
    }

    // Create a NullSampler
    VkSamplerCreateInfo SamplerCreateInfo;
    FMemory::Memzero(&SamplerCreateInfo);

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
        VULKAN_ERROR("vkCreateSampler failed");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullSampler", NullSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    return true;
}

bool FVulkanDefaultResources::InitializeBuffersAndImages(FVulkanDevice& Device)
{
    // Create NullBuffer
    VkBufferCreateInfo BufferCreateInfo;
    FMemory::Memzero(&BufferCreateInfo);

    BufferCreateInfo.usage = 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = VULKAN_DEFAULT_BUFFER_NUM_BYTES;

    VkResult Result = vkCreateBuffer(Device.GetVkDevice(), &BufferCreateInfo, nullptr, &NullBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Buffer");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullBuffer", NullBuffer, VK_OBJECT_TYPE_BUFFER);
    }

    // Allocate memory based on the buffer
    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(NullBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, false, NullBufferMemory))
    {
        VULKAN_ERROR("Failed to allocate buffer memory");
        return false;
    }

    // Create a NullImage
    constexpr VkExtent3D NullExtent = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    VkImageCreateInfo ImageCreateInfo;
    FMemory::Memzero(&ImageCreateInfo);

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
        VULKAN_ERROR("Failed to create image");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImage", NullImage, VK_OBJECT_TYPE_IMAGE);
    }

    if (!MemoryManager.AllocateImageMemory(NullImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, false, NullImageMemory))
    {
        VULKAN_ERROR("Failed to allocate ImageMemory");
        return false;
    }

    // Create NullImageView
    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

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
        VULKAN_ERROR("vkCreateImageView failed");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImageView", NullImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
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

        FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
        MemoryManager.Free(NullBufferMemory);
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

        FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
        MemoryManager.Free(NullImageMemory);
    }

    // All samplers are cached and deleted by the device
    NullSampler = VK_NULL_HANDLE;
}
