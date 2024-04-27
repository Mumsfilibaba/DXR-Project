#pragma once
#include "MetalCore.h"

class FMetalDeviceContext;

class FMetalDeviceChild
{
public:
    FMetalDeviceChild(FMetalDeviceContext* InDeviceContext)
        : DeviceContext(InDeviceContext)
    {
        CHECK(DeviceContext != nullptr);
    }

    virtual ~FMetalDeviceChild()
    {
        DeviceContext = nullptr;
    }

    FORCEINLINE FMetalDeviceContext* GetDeviceContext() const
    {
        return DeviceContext;
    }

private:
    FMetalDeviceContext* DeviceContext;
};
