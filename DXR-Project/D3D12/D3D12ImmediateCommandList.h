#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Fence.h"

class D3D12ImmediateCommandContext : public D3D12DeviceChild
{
public:
	D3D12ImmediateCommandContext(D3D12Device* InDevice);
	~D3D12ImmediateCommandContext();

	bool Initialize();
	void Flush();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandList>	CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	Queue;
	Microsoft::WRL::ComPtr<ID3D12Fence>			Fence;
	
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>	Allocators;
	
	HANDLE	Event		= 0;
	Uint64	FenceValue	= 0;
};