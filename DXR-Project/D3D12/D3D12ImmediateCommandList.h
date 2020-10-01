#pragma once
#include "D3D12CommandList.h"

/*
* D3D12ImmediateCommandList
*/

class D3D12ImmediateCommandList : public D3D12CommandList
{
public:
	D3D12ImmediateCommandList(D3D12Device* InDevice);
	~D3D12ImmediateCommandList();

	bool Initialize(D3D12_COMMAND_LIST_TYPE Type);
	
	void Flush();
	void WaitForCompletion();

public:
	// DeviceChild
	virtual void SetDebugName(const std::string& DebugName) override;

private:
	void WaitForValue(Uint64 FenceValue);

	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	Queue;
	Microsoft::WRL::ComPtr<ID3D12Fence>			Fence;
	
	TArray<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>	Allocators;

	HANDLE Event = 0;
	Uint64 CurrentFenceValue = 0;
	Uint32 CurrentAllocatorIndex = 0;

	TArray<Uint64> FenceValues;
};