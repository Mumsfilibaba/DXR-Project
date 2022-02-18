#pragma once 
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"

class CVulkanDriverInstance;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanPhysicalDevice

class CVulkanPhyscialDevice : public CRefCounted
{
public:

    static TSharedRef<CVulkanPhyscialDevice> CreatePhysicalDevice(CVulkanDriverInstance* InInstance, const TArray<const char*>& DeviceExtensionNames);

    FORCEINLINE CVulkanDriverInstance* GetInstance() const
    {
        return Instance;
    }

    FORCEINLINE VkPhysicalDevice GetPhysicalDevice() const
    {
        return PhysicalDevice;
    }

private:

    CVulkanPhyscialDevice(CVulkanDriverInstance* InInstance);
    ~CVulkanPhyscialDevice();

    bool Initialize(const TArray<const char*>& DeviceExtensionNames);

    CVulkanDriverInstance* Instance;
    VkPhysicalDevice       PhysicalDevice;
};