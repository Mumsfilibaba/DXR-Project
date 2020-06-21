#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12CommandAllocator.h"

#include "Types.h"

class D3D12CommandList : public D3D12DeviceChild
{
public:
	D3D12CommandList(D3D12Device* InDevice);
	~D3D12CommandList();

	bool Initialize(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* InAllocator, ID3D12PipelineState* InitalPipeline);

	bool Reset(D3D12CommandAllocator* InAllocator)
	{
		return SUCCEEDED(CommandList->Reset(InAllocator->GetAllocator(), nullptr));
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

	void CopyBuffer(ID3D12Resource* Destination, Uint64 DestinationOffset, ID3D12Resource* Source, Uint64 SourceOffset, Uint64 SizeInBytes)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyBufferRegion(Destination, DestinationOffset, Source, SourceOffset, SizeInBytes);
	}

	void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* InDestination, Uint32 X, Uint32 Y, Uint32 Z, const D3D12_TEXTURE_COPY_LOCATION* InSource, const D3D12_BOX* InSourceBox)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyTextureRegion(InDestination, X, Y, Z, InSource, InSourceBox);
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

	void DrawIndexedInstanced(Uint32 IndexCountPerInstance, Uint32 InstanceCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation, Uint32 StartInstanceLocation)
	{
		FlushDeferredResourceBarriers();

		CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, Uint32 DescriptorHeapCount)
	{
		CommandList->SetDescriptorHeaps(DescriptorHeapCount, DescriptorHeaps);
	}

	void SetStateObject(ID3D12StateObject* StateObject)
	{
		DXRCommandList->SetPipelineState1(StateObject);
	}

	void SetPipelineState(ID3D12PipelineState* PipelineState)
	{
		DXRCommandList->SetPipelineState(PipelineState);
	}

	void SetComputeRootSignature(ID3D12RootSignature* RootSignature)
	{
		CommandList->SetComputeRootSignature(RootSignature);
	}

	void SetGraphicsRootSignature(ID3D12RootSignature* RootSignature)
	{
		CommandList->SetGraphicsRootSignature(RootSignature);
	}

	void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, Uint32 RootParameterIndex)
	{
		CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	void SetGraphicsRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, Uint32 RootParameterIndex)
	{
		CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	void SetGraphicsRoot32BitConstants(const void* SourceData, Uint32 Num32BitValues, Uint32 DestOffsetIn32BitValues, Uint32 RootParameterIndex)
	{
		CommandList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	void IASetVertexBuffers(Uint32 StartSlot, const D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews, Uint32 VertexBufferViewCount)
	{
		CommandList->IASetVertexBuffers(StartSlot, VertexBufferViewCount, VertexBufferViews);
	}

	void IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* IndexBufferView)
	{
		CommandList->IASetIndexBuffer(IndexBufferView);
	}

	void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
	{
		CommandList->IASetPrimitiveTopology(PrimitiveTopology);
	}

	void RSSetViewports(const D3D12_VIEWPORT* Viewports, Uint32 ViewportCount)
	{
		CommandList->RSSetViewports(ViewportCount, Viewports);
	}

	void RSSetScissorRects(const D3D12_RECT* ScissorRects, Uint32 ScissorRectCount)
	{
		CommandList->RSSetScissorRects(ScissorRectCount, ScissorRects);
	}

	void OMSetBlendFactor(const Float32 BlendFactor[4])
	{
		CommandList->OMSetBlendFactor(BlendFactor);
	}

	void OMSetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* RenderTargetDescriptors, Uint32 RenderTargetCount, bool ContiguousDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* DepthStencilDescriptors)
	{
		CommandList->OMSetRenderTargets(RenderTargetCount, RenderTargetDescriptors, ContiguousDescriptorRange, DepthStencilDescriptors);
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

public:
	// DeviceChild
	virtual void SetName(const std::string& InName) override;

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCommandList;

	std::vector<D3D12_RESOURCE_BARRIER> DeferredResourceBarriers;
};