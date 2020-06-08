#pragma once
#include "D3D12DeviceChild.h"

class D3D12CommandQueue : public D3D12DeviceChild
{
	D3D12CommandQueue(D3D12CommandQueue&& Other)		= delete;
	D3D12CommandQueue(const D3D12CommandQueue& Other)	= delete;

	D3D12CommandQueue& operator=(D3D12CommandQueue&& Other)			= delete;
	D3D12CommandQueue& operator=(const D3D12CommandQueue& Other)	= delete;

public:
	D3D12CommandQueue(D3D12Device* Device);
	~D3D12CommandQueue();

	bool Init();

	ID3D12CommandQueue* GetQueue() const
	{
		return Queue.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> Queue;
};

