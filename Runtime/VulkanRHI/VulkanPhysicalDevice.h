#pragma once 
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Optional.h"

class CVulkanDriverInstance;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanPhysicalDeviceDesc

struct SVulkanPhysicalDeviceDesc
{
    SVulkanPhysicalDeviceDesc()
        : RequiredExtensionNames()
        , OptionalExtensionNames()
        , RequiredFeatures()
    {
        CMemory::Memzero(&RequiredFeatures);
    }

    TArray<const char*> RequiredExtensionNames;
    TArray<const char*> OptionalExtensionNames; // Used to select most optimal adapter

    VkPhysicalDeviceFeatures RequiredFeatures;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanQueueFamilyIndices

struct SVulkanQueueFamilyIndices
{
    SVulkanQueueFamilyIndices() = default;

    SVulkanQueueFamilyIndices(uint32 InGraphicsQueueIndex, uint32 InCopyQueueIndex, uint32 InComputeQueueIndex)
        : GraphicsQueueIndex(InGraphicsQueueIndex)
        , CopyQueueIndex(InCopyQueueIndex)
        , ComputeQueueIndex(InComputeQueueIndex)
    {
    }

    uint32 GraphicsQueueIndex = (~0);
    uint32 CopyQueueIndex     = (~0);
    uint32 ComputeQueueIndex  = (~0);
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanPhysicalDevice

class CVulkanPhysicalDevice : public CRefCounted
{
public:

    /* Creates a new VkPhyscialDevice */
    static TSharedRef<CVulkanPhysicalDevice> QueryAdapter(CVulkanDriverInstance* InInstance, const SVulkanPhysicalDeviceDesc& AdapterDesc);

    static TOptional<SVulkanQueueFamilyIndices> GetQueueFamilyIndices(VkPhysicalDevice physicalDevice);

    FORCEINLINE CVulkanDriverInstance* GetInstance() const
    {
        return Instance;
    }

    FORCEINLINE VkPhysicalDevice GetPhysicalDevice() const
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

	CVulkanPhysicalDevice(CVulkanDriverInstance* InInstance);
    ~CVulkanPhysicalDevice();

    bool Initialize(const SVulkanPhysicalDeviceDesc& AdapterDesc);

    CVulkanDriverInstance* Instance;
    VkPhysicalDevice       PhysicalDevice;
	
	VkPhysicalDeviceProperties        DeviceProperties;
	VkPhysicalDeviceFeatures          DeviceFeatures;
	VkPhysicalDeviceMemoryProperties  DeviceMemoryProperties;
	
#if VK_KHR_get_physical_device_properties2
	VkPhysicalDeviceProperties2       DeviceProperties2;
	VkPhysicalDeviceFeatures2         DeviceFeatures2;
	VkPhysicalDeviceMemoryProperties2 DeviceMemoryProperties2;
#endif
};
