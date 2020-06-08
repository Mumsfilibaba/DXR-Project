#pragma once
#include "D3D12DeviceChild.h"

#include "../Types.h"

class D3D12Fence : public D3D12DeviceChild
{
	D3D12Fence(D3D12Fence&& Other)		= delete;
	D3D12Fence(const D3D12Fence& Other) = delete;

	D3D12Fence& operator=(D3D12Fence&& Other)		= delete;
	D3D12Fence& operator=(const D3D12Fence& Other)	= delete;

public:
	D3D12Fence(D3D12Device* Device);
	~D3D12Fence();

	bool Init(Uint64 InitalValue);

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
	Uint64 FenceValue	= 0;
	HANDLE Event		= 0;
};