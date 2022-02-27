#pragma once
#include "VulkanCore.h"

#include "Core/Templates/ClassUtilities.h"

class CVulkanDevice;

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDeviceObject

class CVulkanDeviceObject : public CNonCopyable
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