#pragma once
#include "VulkanRHI/VulkanCore.h"

class FVulkanDevice;

class FVulkanDeviceChild
{
public:
    FVulkanDeviceChild(FVulkanDevice* InDevice)
        : Device(InDevice)
    {
    }

    virtual ~FVulkanDeviceChild()
    {
        Device = nullptr;
    }

    FORCEINLINE FVulkanDevice* GetDevice() const noexcept
    {
        CHECK(Device != nullptr);
        return Device;
    }

protected:
    FVulkanDevice* Device;
};
