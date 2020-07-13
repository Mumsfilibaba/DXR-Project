#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"
#include "D3D12CommandAllocator.h"

class D3D12CommandList : public D3D12DeviceChild
{
public:
	D3D12CommandList(D3D12Device* InDevice);
	~D3D12CommandList();

	bool Initialize(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline);

	FORCEINLINE bool Reset(D3D12CommandAllocator* Allocator)
	{
		return SUCCEEDED(CommandList->Reset(Allocator->GetAllocator(), nullptr));
	}

	FORCEINLINE bool Close()
	{
		FlushDeferredResourceBarriers();

		return SUCCEEDED(CommandList->Close());
	}

	FORCEINLINE void ClearRenderTargetView(const D3D12RenderTargetView* View, const Float32 ClearColor[4])
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearRenderTargetView(View->GetOfflineHandle(), ClearColor, 0, nullptr);
	}

	FORCEINLINE void ClearDepthStencilView(const D3D12DepthStencilView* View, D3D12_CLEAR_FLAGS Flags, Float32 Depth, const Uint8 Stencil)
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearDepthStencilView(View->GetOfflineHandle(), Flags, Depth, Stencil, 0, nullptr);
	}

	FORCEINLINE void TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
	{
		D3D12_RESOURCE_BARRIER Barrier = { };
		Barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
		Barrier.Transition.pResource	= Resource->GetResource();
		Barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barrier.Transition.StateBefore	= BeforeState;
		Barrier.Transition.StateAfter	= AfterState;

		DeferredResourceBarriers.push_back(Barrier);
	}

	FORCEINLINE void UnorderedAccessBarrier(D3D12Resource* Resource)
	{
		D3D12_RESOURCE_BARRIER Barrier = { };
		Barrier.Type			= D3D12_RESOURCE_BARRIER_TYPE_UAV;
		Barrier.UAV.pResource	= Resource->GetResource();

		DeferredResourceBarriers.push_back(Barrier);
	}

	FORCEINLINE void CopyBuffer(D3D12Resource* Destination, Uint64 DestinationOffset, D3D12Resource* Source, Uint64 SourceOffset, Uint64 SizeInBytes)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyBufferRegion(Destination->GetResource(), DestinationOffset, Source->GetResource(), SourceOffset, SizeInBytes);
	}

	FORCEINLINE void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* InDestination, Uint32 X, Uint32 Y, Uint32 Z, const D3D12_TEXTURE_COPY_LOCATION* InSource, const D3D12_BOX* InSourceBox)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyTextureRegion(InDestination, X, Y, Z, InSource, InSourceBox);
	}

	FORCEINLINE void CopyResource(D3D12Resource* Destination, D3D12Resource* Source)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyResource(Destination->GetResource(), Source->GetResource());
	}

	FORCEINLINE void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* Desc)
	{
		FlushDeferredResourceBarriers();

		DXRCommandList->BuildRaytracingAccelerationStructure(Desc, 0, nullptr);
	}

	FORCEINLINE void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* Desc)
	{
		FlushDeferredResourceBarriers();

		DXRCommandList->DispatchRays(Desc);
	}

	FORCEINLINE void Dispatch(Uint32 ThreadGroupCountX, Uint32 ThreadGroupCountY, Uint32 ThreadGroupCountZ)
	{
		FlushDeferredResourceBarriers();

		CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	FORCEINLINE void DrawInstanced(Uint32 VertexCountPerInstance, Uint32 InstanceCount, Uint32 StartVertexLocation, Uint32 StartInstanceLocation)
	{
		FlushDeferredResourceBarriers();

		CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	FORCEINLINE void DrawIndexedInstanced(Uint32 IndexCountPerInstance, Uint32 InstanceCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation, Uint32 StartInstanceLocation)
	{
		FlushDeferredResourceBarriers();

		CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	FORCEINLINE void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, Uint32 DescriptorHeapCount)
	{
		CommandList->SetDescriptorHeaps(DescriptorHeapCount, DescriptorHeaps);
	}

	FORCEINLINE void SetStateObject(ID3D12StateObject* StateObject)
	{
		DXRCommandList->SetPipelineState1(StateObject);
	}

	FORCEINLINE void SetPipelineState(ID3D12PipelineState* PipelineState)
	{
		DXRCommandList->SetPipelineState(PipelineState);
	}

	FORCEINLINE void SetComputeRootSignature(ID3D12RootSignature* RootSignature)
	{
		CommandList->SetComputeRootSignature(RootSignature);
	}

	FORCEINLINE void SetGraphicsRootSignature(ID3D12RootSignature* RootSignature)
	{
		CommandList->SetGraphicsRootSignature(RootSignature);
	}

	FORCEINLINE void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, Uint32 RootParameterIndex)
	{
		FlushDeferredResourceBarriers();

		CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, Uint32 RootParameterIndex)
	{
		FlushDeferredResourceBarriers();

		CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRoot32BitConstants(const void* SourceData, Uint32 Num32BitValues, Uint32 DestOffsetIn32BitValues, Uint32 RootParameterIndex)
	{
		CommandList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void SetComputeRoot32BitConstants(const void* SourceData, Uint32 Num32BitValues, Uint32 DestOffsetIn32BitValues, Uint32 RootParameterIndex)
	{
		CommandList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void IASetVertexBuffers(Uint32 StartSlot, const D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews, Uint32 VertexBufferViewCount)
	{
		CommandList->IASetVertexBuffers(StartSlot, VertexBufferViewCount, VertexBufferViews);
	}

	FORCEINLINE void IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* IndexBufferView)
	{
		CommandList->IASetIndexBuffer(IndexBufferView);
	}

	FORCEINLINE void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
	{
		CommandList->IASetPrimitiveTopology(PrimitiveTopology);
	}

	FORCEINLINE void RSSetViewports(const D3D12_VIEWPORT* Viewports, Uint32 ViewportCount)
	{
		CommandList->RSSetViewports(ViewportCount, Viewports);
	}

	FORCEINLINE void RSSetScissorRects(const D3D12_RECT* ScissorRects, Uint32 ScissorRectCount)
	{
		CommandList->RSSetScissorRects(ScissorRectCount, ScissorRects);
	}

	FORCEINLINE void OMSetBlendFactor(const Float32 BlendFactor[4])
	{
		CommandList->OMSetBlendFactor(BlendFactor);
	}

	FORCEINLINE void OMSetRenderTargets(const D3D12RenderTargetView* const * RenderTargetViews, Uint32 RenderTargetCount, const D3D12DepthStencilView* DepthStencilView)
	{
		for (Uint32 I = 0; I < RenderTargetCount; I++)
		{
			VALIDATE(RenderTargetViews[I] != nullptr);
			RenderTargetHandles[I] = RenderTargetViews[I]->GetOfflineHandle();
		}

		if (DepthStencilView)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle = DepthStencilView->GetOfflineHandle();
			CommandList->OMSetRenderTargets(RenderTargetCount, RenderTargetHandles, FALSE, &DepthStencilHandle);
		}
		else
		{
			CommandList->OMSetRenderTargets(RenderTargetCount, RenderTargetHandles, FALSE, nullptr);
		}
	}

	FORCEINLINE void FlushDeferredResourceBarriers()
	{
		if (!DeferredResourceBarriers.empty())
		{
			CommandList->ResourceBarrier(static_cast<UINT>(DeferredResourceBarriers.size()), DeferredResourceBarriers.data());
			DeferredResourceBarriers.clear();
		}
	}

	FORCEINLINE ID3D12CommandList* GetCommandList() const
	{
		return CommandList.Get();
	}

public:
	// DeviceChild
	virtual void SetName(const std::string& Name) override;

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCommandList;

	std::vector<D3D12_RESOURCE_BARRIER> DeferredResourceBarriers;
	
	// There can maximum be 8 rendertargets at one time 
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetHandles[8];
};