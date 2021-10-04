#pragma once
#include "D3D12Helpers.h"

class CD3D12Device;

class CD3D12DeviceChild
{
public:
    CD3D12DeviceChild( CD3D12Device* InDevice )
        : Device( InDevice )
    {
        Assert( Device != nullptr );
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