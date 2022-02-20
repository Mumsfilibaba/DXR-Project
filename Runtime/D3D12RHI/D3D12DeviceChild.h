#pragma once
#include "D3D12Core.h"

class CD3D12Device;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12DeviceChild

class CD3D12DeviceObject
{
public:

    CD3D12DeviceObject(CD3D12Device* InDevice)
        : Device(InDevice)
    {
        D3D12_ERROR(Device != nullptr, "Device cannot be nullptr");
    }

    virtual ~CD3D12DeviceObject()
    {
        Device = nullptr;
    }

    FORCEINLINE CD3D12Device* GetDevice() const
    {
        return Device;
    }

private:
    CD3D12Device* Device;
};