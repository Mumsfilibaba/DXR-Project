#include "VulkanPhysicalDevice.h"
#include "VulkanLoader.h"
#include "VulkanInstance.h"
#include "Core/Containers/Array.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"

FVulkanPhysicalDevice::FVulkanPhysicalDevice(FVulkanInstance* InInstance)
    : Instance(InInstance)
    , PhysicalDevice(VK_NULL_HANDLE)
{
}

FVulkanPhysicalDevice::~FVulkanPhysicalDevice()
{
}

bool FVulkanPhysicalDevice::Initialize(const FVulkanPhysicalDeviceDesc& AdapterDesc)
{
    VkResult Result = VK_SUCCESS;

    uint32 AdapterCount = 0;
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve AdapterCount");

    TArray<VkPhysicalDevice> Adapters(AdapterCount);
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, Adapters.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve available Adapters");

    if (AdapterCount < 1)
    {
        VULKAN_ERROR("No Adapters available");
        return false;
    }

    IConsoleVariable* CVarVerboseLogging = FConsoleManager::Get().FindConsoleVariable("Vulkan.VerboseLogging");

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

        if (!CheckAvailability(CurrentAdapter, AdapterDesc.RequiredFeatures))
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
    
#if VK_KHR_get_physical_device_properties2
    if (Instance->IsExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        FMemory::Memzero(&DeviceProperties2);
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        DeviceProperties2.pNext = nullptr;
        
        vkGetPhysicalDeviceProperties2(PhysicalDevice, &DeviceProperties2);
        
        FMemory::Memzero(&DeviceFeatures2);
        DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        DeviceFeatures2.pNext = nullptr;
        
        vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);
        
        FMemory::Memzero(&DeviceMemoryProperties2);
        DeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        DeviceMemoryProperties2.pNext = nullptr;
        
        vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice, &DeviceMemoryProperties2);
    }
#endif
    
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
    
    IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("Vulkan.VerboseLogging");
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

    return UINT32_MAX;
}
