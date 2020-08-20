#pragma once
#include "Defines.h"
#include "Types.h"

#include <d3d12.h>

#include <wrl/client.h>

#include "Containers/String.h"

class D3D12Device;

class D3D12DeviceChild
{
public:
	D3D12DeviceChild(D3D12Device* InDevice)
		: Device(InDevice)
	{
	}

	~D3D12DeviceChild()
	{
		Device = nullptr;
	}

	virtual void SetDebugName(const std::string& InName) = 0;

	FORCEINLINE D3D12Device* GetDevice() const
	{
		return Device;
	}

protected:
	D3D12Device* Device = nullptr;
};