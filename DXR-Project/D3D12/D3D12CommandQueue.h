#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

class D3D12CommandQueue : public D3D12DeviceChild
{
public:
	D3D12CommandQueue(D3D12Device* Device);
	~D3D12CommandQueue();

	bool Initialize();
	
	bool SignalFence(class D3D12Fence* Fence, Uint64 FenceValue);

	void ExecuteCommandList(class D3D12CommandList* CommandList);

	ID3D12CommandQueue* GetQueue() const
	{
		return Queue.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> Queue;
};

