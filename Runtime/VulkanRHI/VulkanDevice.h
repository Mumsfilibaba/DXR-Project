#pragma once
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

class CVulkanDriverInstance;
class CVulkanPhysicalDevice;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanDeviceDesc

struct SVulkanDeviceDesc
{
    TArray<const char*> DeviceLayerNames;
    TArray<const char*> DeviceExtensionNames;
	
	bool bEnableValidationLayer = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDevice

class CVulkanDevice : public CRefCounted
{
public:

    static TSharedRef<CVulkanDevice> CreateDevice(CVulkanDriverInstance* InInstance, CVulkanPhysicalDevice* InAdapter, const SVulkanDeviceDesc& DeviceDesc);

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
};
