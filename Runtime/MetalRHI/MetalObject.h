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

    ~FMetalObject() = default;

    FORCEINLINE FMetalDeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
    FMetalDeviceContext* DeviceContext;
};
