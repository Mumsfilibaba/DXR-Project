#pragma once
#include "D3D12DeviceChild.h"

#include "Types.h"

class D3D12CommandQueue : public D3D12DeviceChild
{
public:
	D3D12CommandQueue(D3D12Device* InDevice);
	~D3D12CommandQueue();

	bool Initialize(D3D12_COMMAND_LIST_TYPE Type);
	
	bool SignalFence(class D3D12Fence* Fence, Uint64 FenceValue);

	void ExecuteCommandList(class D3D12CommandList* CommandList);

	FORCEINLINE ID3D12CommandQueue* GetQueue() const
	{
		return Queue.Get();
	}

public:
	// DeviceChild
	virtual void SetName(const std::string& Name) override;

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> Queue;
};

