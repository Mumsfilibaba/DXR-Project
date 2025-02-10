#pragma once
#include "D3D12RHI/D3D12Core.h"

class FD3D12Device;

class FD3D12DeviceChild
{
public:
    FD3D12DeviceChild(FD3D12Device* InDevice)
        : Device(InDevice)
    {
        CHECK(Device != nullptr);
    }

    virtual ~FD3D12DeviceChild() = default;

    FORCEINLINE FD3D12Device* GetDevice() const
    {
        return Device;
    }

private:
    FD3D12Device* const Device;
};