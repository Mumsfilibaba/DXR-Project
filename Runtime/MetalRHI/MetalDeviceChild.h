#pragma once
#include "MetalRHI/MetalCore.h"

class FMetalDevice;

class FMetalDeviceChild
{
public:
    FMetalDeviceChild(FMetalDevice* InDevice)
        : Device(InDevice)
    {
        CHECK(Device != nullptr);
    }

    virtual ~FMetalDeviceChild();

    FORCEINLINE FMetalDevice* GetDevice() const
    {
        return Device;
    }

private:
    FMetalDevice* Device;
};
