#pragma once
#include "D3D12Core.h"

class CD3D12Device;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12DeviceChild

class CD3D12DeviceChild
{
public:

    CD3D12DeviceChild(CD3D12Device* InDevice)
        : Device(InDevice)
    {
        Check(Device != nullptr);
    }

    virtual ~CD3D12DeviceChild()
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