#pragma once
#include "D3D12Helpers.h"

class D3D12Device;

class D3D12DeviceChild
{
public:
    D3D12DeviceChild(D3D12Device* InDevice)
        : Device(InDevice)
    {
         VALIDATE(Device != nullptr);
    }

    virtual ~D3D12DeviceChild()
    {
        Device = nullptr;
    }

    FORCEINLINE D3D12Device* GetDevice() const
    {
        return Device;
    }

protected:
    D3D12Device* Device = nullptr;
};