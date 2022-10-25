#pragma once
#include "D3D12Core.h"

#include "Core/Templates/TypeTraits.h"

class FD3D12Device;

class FD3D12DeviceChild
{
public:
    FD3D12DeviceChild() = delete;

    FD3D12DeviceChild(FD3D12Device* InDevice)
        : Device(InDevice)
    {
        CHECK(Device != nullptr);
    }

    virtual ~FD3D12DeviceChild()
    {
        Device = nullptr;
    }

    FORCEINLINE FD3D12Device* GetDevice() const
    {
        return Device;
    }

private:
    FD3D12Device* Device;
};