#pragma once
#include "VulkanCore.h"

class FVulkanDevice;

class FVulkanDeviceChild
{
public:
    FVulkanDeviceChild(FVulkanDevice* InDevice)
        : Device(InDevice)
    {
        CHECK(Device != nullptr);
    }

    virtual ~FVulkanDeviceChild()
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