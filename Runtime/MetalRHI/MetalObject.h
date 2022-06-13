#pragma once
#include "MetalCore.h"

class CMetalDeviceContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalObject

class CMetalObject
{
public:

    CMetalObject(CMetalDeviceContext* InDeviceContext)
        : DeviceContext(InDeviceContext)
    {
        Check(DeviceContext != nullptr);
    }

    ~CMetalObject() = default;

    FORCEINLINE CMetalDeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
    CMetalDeviceContext* DeviceContext;
};
