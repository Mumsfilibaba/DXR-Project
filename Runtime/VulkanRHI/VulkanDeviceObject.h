#pragma once
#include "VulkanCore.h"

class CVulkanDevice;

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanInstance

class CVulkanDeviceObject
{
public:
    CVulkanDeviceObject(CVulkanDevice* InDevice)
        : Device(InDevice)
    {
    }

    FORCEINLINE CVulkanDevice* GetDevice() const noexcept
    {
        return Device;
    }

private:
    CVulkanDevice* Device;
};