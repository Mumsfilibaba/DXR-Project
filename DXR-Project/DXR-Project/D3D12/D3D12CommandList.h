#pragma once
#include "D3D12DeviceChild.h"

#include "../Types.h"

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

	bool Reset(D3D12CommandAllocator* Allocator);
	bool Close();

	void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE View, const Float32 ClearColor[4]);

	void TransitionResourceState(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState);

	ID3D12CommandList* GetCommandList() const
	{
		return CommandList.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
};