#pragma once
#include "MetalCore.h"

class FMetalDeviceContext;

class FMetalObject
{
public:
    FMetalObject(FMetalDeviceContext* InDeviceContext)
        : DeviceContext(InDeviceContext)
    {
        CHECK(DeviceContext != nullptr);
    }

    virtual ~FMetalObject()
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
