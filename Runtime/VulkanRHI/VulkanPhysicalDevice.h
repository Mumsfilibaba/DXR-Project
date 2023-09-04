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
        FMemory::Memzero(&RequiredFeatures11);
        FMemory::Memzero(&RequiredFeatures12);
    }

    TArray<const CHAR*> RequiredExtensionNames;
    TArray<const CHAR*> OptionalExtensionNames; // Used to select most optimal adapter

    VkPhysicalDeviceFeatures         RequiredFeatures;
    VkPhysicalDeviceVulkan11Features RequiredFeatures11;
    VkPhysicalDeviceVulkan12Features RequiredFeatures12;
    
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
    
    FVulkanInstance* GetInstance() const
    {
        return Instance;
    }

    VkPhysicalDevice GetVkPhysicalDevice() const
    {
        return PhysicalDevice;
    }

    const VkPhysicalDeviceProperties&       GetDeviceProperties()       const { return DeviceProperties; }
    const VkPhysicalDeviceFeatures&         GetDeviceFeatures()         const { return DeviceFeatures; }
    const VkPhysicalDeviceMemoryProperties& GetDeviceMemoryProperties() const { return DeviceMemoryProperties; }
    
    // Vulkan 1.1, Vulkan 1.2 features
    const VkPhysicalDeviceProperties2&       GetDeviceProperties2()       const { return DeviceProperties2; }
    const VkPhysicalDeviceFeatures2&         GetDeviceFeatures2()         const { return DeviceFeatures2; }
    const VkPhysicalDeviceMemoryProperties2& GetDeviceMemoryProperties2() const { return DeviceMemoryProperties2; }
    const VkPhysicalDeviceVulkan11Features&  GetDeviceFeaturesVulkan11()  const { return DeviceFeatures11; }
    const VkPhysicalDeviceVulkan12Features&  GetDeviceFeaturesVulkan12()  const { return DeviceFeatures12; }

    // Extension Information
#if VK_EXT_depth_clip_enable
    const VkPhysicalDeviceDepthClipEnableFeaturesEXT& GetDepthClipEnableFeatures() const { return DepthClipEnableFeatures; }
#endif
    
#if VK_EXT_conservative_rasterization
    const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& GetConservativeRasterizationProperties() const { return ConservativeRasterizationProperties; }
#endif

private:
    FVulkanInstance* Instance;
    VkPhysicalDevice PhysicalDevice;
    
    VkPhysicalDeviceProperties        DeviceProperties;
    VkPhysicalDeviceFeatures          DeviceFeatures;
    VkPhysicalDeviceMemoryProperties  DeviceMemoryProperties;

    // Vulkan 1.1, Vulkan 1.2 features
    VkPhysicalDeviceProperties2       DeviceProperties2;
    VkPhysicalDeviceFeatures2         DeviceFeatures2;
    VkPhysicalDeviceMemoryProperties2 DeviceMemoryProperties2;
    VkPhysicalDeviceVulkan11Features  DeviceFeatures11;
    VkPhysicalDeviceVulkan12Features  DeviceFeatures12;

    // Extension Information
#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT DepthClipEnableFeatures;
#endif
#if VK_EXT_conservative_rasterization
    VkPhysicalDeviceConservativeRasterizationPropertiesEXT ConservativeRasterizationProperties;
#endif
};
