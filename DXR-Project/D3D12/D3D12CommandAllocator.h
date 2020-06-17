#pragma once
#include "D3D12DeviceChild.h"

class D3D12CommandAllocator : public D3D12DeviceChild
{
public:
	D3D12CommandAllocator(D3D12Device* Device);
	~D3D12CommandAllocator();

	bool Initialize(D3D12_COMMAND_LIST_TYPE Type);

	bool Reset();

	ID3D12CommandAllocator* GetAllocator() const
	{
		return Allocator.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
};