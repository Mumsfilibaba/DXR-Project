#include "VulkanPhysicalDevice.h"
#include "VulkanLoader.h"
#include "VulkanInstance.h"
#include "Core/Containers/Array.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper

static bool CheckAvailability(VkPhysicalDevice PhysicalDevice, const FVulkanPhysicalDeviceDesc& DeviceDesc)
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

bool FVulkanPhysicalDevice::Initialize(const FVulkanPhysicalDeviceDesc& AdapterDesc)
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
