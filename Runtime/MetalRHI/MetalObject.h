#pragma once
#include "MetalCore.h"

class FMetalDeviceContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalObject

class FMetalObject
{
public:

    FMetalObject(FMetalDeviceContext* InDeviceContext)
        : DeviceContext(InDeviceContext)
    {
        Check(DeviceContext != nullptr);
    }

    ~FMetalObject() = default;

    FORCEINLINE FMetalDeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
    FMetalDeviceContext* DeviceContext;
};
