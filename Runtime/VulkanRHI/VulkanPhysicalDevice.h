#pragma once 
#include "VulkanCore.h"
#include "VulkanLoader.h"
#include "VulkanRefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Optional.h"

class FVulkanInstance;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper

static bool CheckAvailability(VkPhysicalDevice PhysicalDevice, const VkPhysicalDeviceFeatures& Features)
{
    static constexpr uint32 NumFeatures = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

    VkPhysicalDeviceProperties AdapterProperties;
    vkGetPhysicalDeviceProperties(PhysicalDevice, &AdapterProperties);

    VkPhysicalDeviceFeatures AdapterFeatures;
    vkGetPhysicalDeviceFeatures(PhysicalDevice, &AdapterFeatures);

    const VkBool32* RequiredFeatures  = reinterpret_cast<const VkBool32*>(&Features);
    const VkBool32* AvailableFeatures = reinterpret_cast<const VkBool32*>(&AdapterFeatures);

    bool bHasAllFeatures = true;
    for (uint32 FeatureIndex = 0; FeatureIndex < NumFeatures; ++FeatureIndex)
    {
        const bool bRequiresFeature = RequiredFeatures[FeatureIndex]  == VK_TRUE;
        const bool bHasFeature      = AvailableFeatures[FeatureIndex] == VK_TRUE;
        if (bRequiresFeature && !bHasFeature)
        {
            VULKAN_WARNING("PhysicalDevice '%s' does not support all device-features. See PhysicalDeviceFeature[%d]", AdapterProperties.deviceName, FeatureIndex);
            bHasAllFeatures = false;
            break;
        }
    }

    return bHasAllFeatures;
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


struct FVulkanPhysicalDeviceDesc
{
    FVulkanPhysicalDeviceDesc()
        : RequiredExtensionNames()
        , OptionalExtensionNames()
    {
        FMemory::Memzero(&RequiredFeatures);
    }

    TArray<const CHAR*>      RequiredExtensionNames;
    TArray<const CHAR*>      OptionalExtensionNames; // Used to select most optimal adapter

    VkPhysicalDeviceFeatures RequiredFeatures; 
    // TODO: Optional features that can be used so select the best adapter
};

struct FVulkanQueueFamilyIndices
{
    FVulkanQueueFamilyIndices() = default;

    FVulkanQueueFamilyIndices(uint32 InGraphicsQueueIndex, uint32 InCopyQueueIndex, uint32 InComputeQueueIndex)
        : GraphicsQueueIndex(InGraphicsQueueIndex)
        , CopyQueueIndex(InCopyQueueIndex)
        , ComputeQueueIndex(InComputeQueueIndex)
    {
    }

    uint32 GraphicsQueueIndex = uint32(~0);
    uint32 CopyQueueIndex     = uint32(~0);
    uint32 ComputeQueueIndex  = uint32(~0);
};

class FVulkanPhysicalDevice : public FVulkanRefCounted
{
public:
    FVulkanPhysicalDevice(FVulkanInstance* InInstance);
    ~FVulkanPhysicalDevice();

    static TOptional<FVulkanQueueFamilyIndices> GetQueueFamilyIndices(VkPhysicalDevice physicalDevice);

    bool Initialize(const FVulkanPhysicalDeviceDesc& AdapterDesc);

    uint32 FindMemoryTypeIndex(uint32 TypeFilter, VkMemoryPropertyFlags Properties);

    FORCEINLINE FVulkanInstance* GetInstance() const
    {
        return Instance;
    }

    FORCEINLINE VkPhysicalDevice GetVkPhysicalDevice() const
    {
        return PhysicalDevice;
    }

    FORCEINLINE const VkPhysicalDeviceProperties& GetDeviceProperties() const
    {
        return DeviceProperties;
    }

    FORCEINLINE const VkPhysicalDeviceFeatures& GetDeviceFeatures() const
    {
        return DeviceFeatures;
    }

    FORCEINLINE const VkPhysicalDeviceMemoryProperties& GetDeviceMemoryProperties() const
    {
        return DeviceMemoryProperties;
    }
    
#if VK_KHR_get_physical_device_properties2
    FORCEINLINE const VkPhysicalDeviceProperties2& GetDeviceProperties2() const
    {
        return DeviceProperties2;
    }

    FORCEINLINE const VkPhysicalDeviceFeatures2& GetDeviceFeatures2() const
    {
        return DeviceFeatures2;
    }

    FORCEINLINE const VkPhysicalDeviceMemoryProperties2& GetDeviceMemoryProperties2() const
    {
        return DeviceMemoryProperties2;
    }
#endif

private:
    FVulkanInstance* Instance;
    VkPhysicalDevice PhysicalDevice;
    
    VkPhysicalDeviceProperties        DeviceProperties;
    VkPhysicalDeviceFeatures          DeviceFeatures;
    VkPhysicalDeviceMemoryProperties  DeviceMemoryProperties;
    
#if VK_KHR_get_physical_device_properties2
    VkPhysicalDeviceProperties2       DeviceProperties2;
    VkPhysicalDeviceFeatures2         DeviceFeatures2;
    VkPhysicalDeviceMemoryProperties2 DeviceMemoryProperties2;
#endif
};
