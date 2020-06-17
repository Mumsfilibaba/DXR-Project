#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

class D3D12Fence : public D3D12DeviceChild
{
public:
	D3D12Fence(D3D12Device* Device);
	~D3D12Fence();

	bool Initialize(Uint64 InitalValue);

	bool WaitForValue(Uint64 FenceValue);

	ID3D12Fence* GetFence() const
	{
		return Fence.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
	HANDLE Event = 0;
};