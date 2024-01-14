#pragma once
#include "VulkanCore.h"

class FVulkanDevice;

class FVulkanDeviceObject
{
public:
    FVulkanDeviceObject(FVulkanDevice* InDevice)
        : Device(InDevice)
    {
        CHECK(Device != nullptr);
    }

    virtual ~FVulkanDeviceObject()
    {
        Device = nullptr;
    }

    FORCEINLINE FVulkanDevice* GetDevice() const noexcept
    {
        return Device;
    }

private:
    FVulkanDevice* Device;
};