#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12CommandAllocator.h"

#include "Types.h"

class D3D12CommandList : public D3D12DeviceChild
{
public:
	D3D12CommandList(D3D12Device* Device);
	~D3D12CommandList();

	bool Init(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline);

	bool Reset(D3D12CommandAllocator* Allocator)
	{
		return SUCCEEDED(CommandList->Reset(Allocator->GetAllocator(), nullptr));
	}

	bool Close()
	{
		FlushDeferredResourceBarriers();

		return SUCCEEDED(CommandList->Close());
	}

	void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE View, const Float32 ClearColor[4])
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearRenderTargetView(View, ClearColor, 0, nullptr);
	}

	void TransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
	{
		D3D12_RESOURCE_BARRIER Barrier = { };
		Barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
		Barrier.Transition.pResource	= Resource;
		Barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barrier.Transition.StateBefore	= BeforeState;
		Barrier.Transition.StateAfter	= AfterState;

		DeferredResourceBarriers.push_back(Barrier);
	}

	void UnorderedAccessBarrier(ID3D12Resource* Resource)
	{
		D3D12_RESOURCE_BARRIER Barrier = { };
		Barrier.Type			= D3D12_RESOURCE_BARRIER_TYPE_UAV;
		Barrier.UAV.pResource	= Resource;

		DeferredResourceBarriers.push_back(Barrier);
	}

	void CopyResource(ID3D12Resource* Destination, ID3D12Resource* Source)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyResource(Destination, Source);
	}

	void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* Desc)
	{
		FlushDeferredResourceBarriers();

		DXRCommandList->BuildRaytracingAccelerationStructure(Desc, 0, nullptr);
	}

	void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* Desc)
	{
		FlushDeferredResourceBarriers();

		DXRCommandList->DispatchRays(Desc);
	}

	void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, Uint32 DescriptorHeapCount)
	{
		CommandList->SetDescriptorHeaps(DescriptorHeapCount, DescriptorHeaps);
	}

	void SetStateObject(ID3D12StateObject* StateObject)
	{
		DXRCommandList->SetPipelineState1(StateObject);
	}

	void SetComputeRootSignature(ID3D12RootSignature* RootSignature)
	{
		CommandList->SetComputeRootSignature(RootSignature);
	}

	void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, Uint32 RootParameterIndex)
	{
		CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	void FlushDeferredResourceBarriers()
	{
		if (!DeferredResourceBarriers.empty())
		{
			CommandList->ResourceBarrier(static_cast<UINT>(DeferredResourceBarriers.size()), DeferredResourceBarriers.data());
			DeferredResourceBarriers.clear();
		}
	}

	ID3D12CommandList* GetCommandList() const
	{
		return CommandList.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCommandList;

	std::vector<D3D12_RESOURCE_BARRIER> DeferredResourceBarriers;
};