#pragma once
#include "D3D11RHI/D3D11Core.h"

class FD3D11Device;

class FD3D11DeviceChild
{
public:
    FD3D11DeviceChild(FD3D11Device* InDevice)
        : Device(InDevice)
    {
    }

    virtual ~FD3D11DeviceChild() = default;

    FORCEINLINE FD3D11Device* GetDevice() const
    {
        CHECK(Device != nullptr);
        return Device;
    }

private:
    FD3D11Device* Device;
};
