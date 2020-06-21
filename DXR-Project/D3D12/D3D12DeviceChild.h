#pragma once
#include <d3d12.h>

#include <wrl/client.h>

#include <string>

class D3D12Device;

class D3D12DeviceChild
{
public:
	D3D12DeviceChild(D3D12Device* InDevice)
		: ParentDevice(InDevice)
	{
	}

	~D3D12DeviceChild()
	{
		ParentDevice = nullptr;
	}

	virtual void SetName(const std::string& InName) = 0;

	inline D3D12Device* GetDevice() const
	{
		return ParentDevice;
	}

protected:
	D3D12Device* ParentDevice = nullptr;
};