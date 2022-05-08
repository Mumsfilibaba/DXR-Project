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
    { }

    ~CMetalObject() = default;

    FORCEINLINE CMetalDeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
    CMetalDeviceContext* DeviceContext;
};