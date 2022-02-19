#pragma once
#include "VulkanCore.h"
#include "VulkanFunctions.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"

class CVulkanDriverInstance;
class CVulkanPhysicalDevice;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanDeviceDesc

struct SVulkanDeviceDesc
{
	TArray<const char*> RequiredExtensionNames;
	TArray<const char*> OptionalExtensionNames;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDevice

class CVulkanDevice : public CRefCounted
{
public:

    /* Creates a new wrapper for VkDevice */
    static TSharedRef<CVulkanDevice> CreateDevice(CVulkanDriverInstance* InInstance, CVulkanPhysicalDevice* InAdapter, const SVulkanDeviceDesc& DeviceDesc);

    FORCEINLINE bool IsLayerEnabled(const String& LayerName)
    {
        return LayerNames.find(LayerName) != LayerNames.end();
    }

    FORCEINLINE bool IsExtensionEnabled(const String& ExtensionName)
    {
        return ExtensionNames.find(ExtensionName) != ExtensionNames.end();
    }

    FORCEINLINE VulkanVoidFunction LoadFunction(const char* Name) const
	{
		VULKAN_ERROR(vkGetDeviceProcAddr != nullptr, "Vulkan Driver Instance is not initialized properly");
        VULKAN_ERROR(Device != VK_NULL_HANDLE      , "Vulkan Device is not initialized properly");
		return reinterpret_cast<VulkanVoidFunction>(vkGetDeviceProcAddr(Device, Name));
	}

    template<typename FunctionType>
    FORCEINLINE FunctionType LoadFunction(const char* Name) const
	{
		return reinterpret_cast<FunctionType>(LoadFunction(Name));
	}

    FORCEINLINE CVulkanDriverInstance* GetInstance() const
    {
        return Instance;
    }
	
	FORCEINLINE CVulkanPhysicalDevice* GetPhysicalDevice() const
	{
		return Adapter;
	}

    FORCEINLINE VkDevice GetDevice() const
    {
        return Device;
    }

private:

    CVulkanDevice(CVulkanDriverInstance* InInstance, CVulkanPhysicalDevice* InAdapter);
    ~CVulkanDevice();

    bool Initialize(const SVulkanDeviceDesc& DeviceDesc);

    CVulkanDriverInstance* Instance;
	CVulkanPhysicalDevice* Adapter;
    VkDevice               Device;

    TSet<String> ExtensionNames;
    TSet<String> LayerNames;
};
