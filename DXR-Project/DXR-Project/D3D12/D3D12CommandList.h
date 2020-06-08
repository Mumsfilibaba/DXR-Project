#pragma once
#include "D3D12DeviceChild.h"

class D3D12CommandAllocator;

class D3D12CommandList : public D3D12DeviceChild
{
	D3D12CommandList(D3D12CommandList&& Other)		= delete;
	D3D12CommandList(const D3D12CommandList& Other) = delete;

	D3D12CommandList& operator=(D3D12CommandList&& Other)		= delete;
	D3D12CommandList& operator=(const D3D12CommandList& Other)	= delete;

public:
	D3D12CommandList(D3D12Device* Device);
	~D3D12CommandList();

	bool Init(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline);

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
};