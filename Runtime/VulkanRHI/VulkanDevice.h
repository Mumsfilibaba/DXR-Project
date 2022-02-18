#pragma once
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"

class CVulkanDriverInstance;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDevice

class CVulkanDevice : public CRefCounted
{
public:

    static TSharedRef<CVulkanDevice> CreateDevice(CVulkanDriverInstance* InInstance, const TArray<const char*>& DeviceLayerNames, const TArray<const char*>& DeviceExtensionNames);

    FORCEINLINE CVulkanDriverInstance* GetInstance() const
    {
        return Instance;
    }

    FORCEINLINE VkDevice GetDevice() const
    {
        return Device;
    }

    FORCEINLINE VkPhysicalDevice GetPhysicalDevice() const
    {
        return PhysicalDevice;
    }

private:

    CVulkanDevice(CVulkanDriverInstance* InInstance);
    ~CVulkanDevice();

    bool Initialize(const TArray<const char*>& DeviceLayerNames, const TArray<const char*>& DeviceExtensionNames);

    CVulkanDriverInstance* Instance;

    VkDevice         Device;
    VkPhysicalDevice PhysicalDevice;
};