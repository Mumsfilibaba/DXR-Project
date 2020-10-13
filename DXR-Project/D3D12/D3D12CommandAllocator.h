#pragma once
#include "D3D12DeviceChild.h"

/*
* D3D12CommandAllocator
*/

class D3D12CommandAllocator : public D3D12DeviceChild
{
public:
	D3D12CommandAllocator(D3D12Device* InDevice);
	~D3D12CommandAllocator();

	bool Initialize(D3D12_COMMAND_LIST_TYPE Type);

	bool Reset();

	FORCEINLINE ID3D12CommandAllocator* GetAllocator() const
	{
		return Allocator.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
};