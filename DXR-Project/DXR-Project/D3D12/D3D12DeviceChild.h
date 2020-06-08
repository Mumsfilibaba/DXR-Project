#pragma once
#include <d3d12.h>

#include <wrl/client.h>

class D3D12Device;

class D3D12DeviceChild
{
public:
	D3D12DeviceChild(D3D12Device* Device)
		: Device(Device)
	{
	}

	~D3D12DeviceChild() = default;

protected:
	D3D12Device* Device = nullptr;
};