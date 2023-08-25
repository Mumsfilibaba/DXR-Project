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
