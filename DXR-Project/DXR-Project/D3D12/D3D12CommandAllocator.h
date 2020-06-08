#pragma once
#include "D3D12DeviceChild.h"

class D3D12CommandAllocator : public D3D12DeviceChild
{
	D3D12CommandAllocator(D3D12CommandAllocator&& Other)		= delete;
	D3D12CommandAllocator(const D3D12CommandAllocator& Other)	= delete;

	D3D12CommandAllocator& operator=(D3D12CommandAllocator&& Other)			= delete;
	D3D12CommandAllocator& operator=(const D3D12CommandAllocator& Other)	= delete;

public:
	D3D12CommandAllocator(D3D12Device* Device);
	~D3D12CommandAllocator();

	bool Init(D3D12_COMMAND_LIST_TYPE Type);

	bool Reset();

	ID3D12CommandAllocator* GetAllocator() const
	{
		return Allocator.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
};