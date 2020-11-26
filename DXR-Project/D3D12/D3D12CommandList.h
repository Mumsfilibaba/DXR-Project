#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"

class D3D12Texture;
class D3D12ComputePipelineState;
class D3D12RootSignature;

/*
* D3D12CommandList
*/

class D3D12CommandList : public D3D12DeviceChild
{
	struct GenerateMipsHelper
	{
		TUniquePtr<D3D12ComputePipelineState>		GenerateMipsTex2D_PSO;
		TUniquePtr<D3D12ComputePipelineState>		GenerateMipsTexCube_PSO;
		TUniquePtr<D3D12RootSignature>				GenerateMipsRootSignature;
		TUniquePtr<D3D12UnorderedAccessView>		NULLView;
		TUniquePtr<D3D12DescriptorTable>			SRVDescriptorTable;
		TArray<TUniquePtr<D3D12DescriptorTable>>	UAVDescriptorTables;
	};

public:
	D3D12CommandList(D3D12Device* InDevice);
	~D3D12CommandList();

	bool Initialize(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline);

	void GenerateMips(D3D12Texture* Dest);

	void FlushDeferredResourceBarriers();

	void BindGlobalOnlineDescriptorHeaps();

	void UploadBufferData(class D3D12Buffer* Dest, const uint32 DestOffset, const void* Src, const uint32 SizeInBytes);
	void UploadTextureData(D3D12Texture* Dest, const void* Src, DXGI_FORMAT Format, const uint32 Width, const uint32 Height, const uint32 Depth, const uint32 Stride, const uint32 RowPitch);

	void DeferDestruction(D3D12Resource* Resource);

	void TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState);
	void UnorderedAccessBarrier(D3D12Resource* Resource);

	FORCEINLINE void ReleaseDeferredResources()
	{
		UploadBufferOffset = 0;
		ResourcesPendingRelease.Clear();
	}

	FORCEINLINE bool Reset(D3D12CommandAllocator* Allocator)
	{
		// Reset NumDrawcalls
		NumDrawCalls = 0;

		ReleaseDeferredResources(); // TODO: Make sure that we can do this here
		return SUCCEEDED(CommandList->Reset(Allocator->GetAllocator(), nullptr));
	}

	FORCEINLINE bool Close()
	{
		FlushDeferredResourceBarriers();

		return SUCCEEDED(CommandList->Close());
	}

	FORCEINLINE void ClearRenderTargetView(const D3D12RenderTargetView* View, const float ClearColor[4])
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearRenderTargetView(View->GetOfflineHandle(), ClearColor, 0, nullptr);
	}

	FORCEINLINE void ClearDepthStencilView(const D3D12DepthStencilView* View, D3D12_CLEAR_FLAGS Flags, float Depth, const uint8 Stencil)
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearDepthStencilView(View->GetOfflineHandle(), Flags, Depth, Stencil, 0, nullptr);
	}

	FORCEINLINE void CopyBuffer(D3D12Resource* Destination, uint64 DestinationOffset, D3D12Resource* Source, uint64 SourceOffset, uint64 SizeInBytes)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyBufferRegion(Destination->GetResource(), DestinationOffset, Source->GetResource(), SourceOffset, SizeInBytes);
	}

	FORCEINLINE void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* InDestination, uint32 X, uint32 Y, uint32 Z, const D3D12_TEXTURE_COPY_LOCATION* InSource, const D3D12_BOX* InSourceBox)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyTextureRegion(InDestination, X, Y, Z, InSource, InSourceBox);
	}

	FORCEINLINE void CopyResource(D3D12Resource* Destination, D3D12Resource* Source)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyResource(Destination->GetResource(), Source->GetResource());
	}

	FORCEINLINE void ResolveSubresource(D3D12Resource* Destination, D3D12Resource* Source, DXGI_FORMAT Format)
	{
		FlushDeferredResourceBarriers();

		CommandList->ResolveSubresource(Destination->GetResource(), 0, Source->GetResource(), 0, Format);
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

	FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
		FlushDeferredResourceBarriers();

		CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
	{
		FlushDeferredResourceBarriers();

		CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);

		NumDrawCalls++;
	}

	FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
	{
		FlushDeferredResourceBarriers();

		CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

		NumDrawCalls++;
	}

	FORCEINLINE void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, uint32 DescriptorHeapCount)
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

	FORCEINLINE void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, uint32 RootParameterIndex)
	{
		FlushDeferredResourceBarriers();

		CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, uint32 RootParameterIndex)
	{
		FlushDeferredResourceBarriers();

		CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRoot32BitConstants(const void* SourceData, uint32 Num32BitValues, uint32 DestOffsetIn32BitValues, uint32 RootParameterIndex)
	{
		CommandList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void SetComputeRoot32BitConstants(const void* SourceData, uint32 Num32BitValues, uint32 DestOffsetIn32BitValues, uint32 RootParameterIndex)
	{
		CommandList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void IASetVertexBuffers(uint32 StartSlot, const D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews, uint32 VertexBufferViewCount)
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

	FORCEINLINE void RSSetViewports(const D3D12_VIEWPORT* Viewports, uint32 ViewportCount)
	{
		CommandList->RSSetViewports(ViewportCount, Viewports);
	}

	FORCEINLINE void RSSetScissorRects(const D3D12_RECT* ScissorRects, uint32 ScissorRectCount)
	{
		CommandList->RSSetScissorRects(ScissorRectCount, ScissorRects);
	}

	FORCEINLINE void OMSetBlendFactor(const float BlendFactor[4])
	{
		CommandList->OMSetBlendFactor(BlendFactor);
	}

	FORCEINLINE void OMSetRenderTargets(const D3D12RenderTargetView* const * RenderTargetViews, uint32 RenderTargetCount, const D3D12DepthStencilView* DepthStencilView)
	{
		for (uint32 I = 0; I < RenderTargetCount; I++)
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

	FORCEINLINE ID3D12CommandList* GetCommandList() const
	{
		return CommandList.Get();
	}
	
	FORCEINLINE uint32 GetNumDrawCalls() const
	{
		return NumDrawCalls;
	}

public:
	// DeviceChild
	virtual void SetDebugName(const std::string& DebugName) override;

protected:
	bool CreateUploadBuffer(uint32 SizeInBytes = 1024U);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCommandList;

	class D3D12Buffer* UploadBuffer = nullptr;
	byte*	UploadPointer		= nullptr;
	uint32	UploadBufferOffset	= 0;
	uint32	NumDrawCalls		= 0;

	TArray<D3D12_RESOURCE_BARRIER> DeferredResourceBarriers;
	TArray<Microsoft::WRL::ComPtr<ID3D12Resource>> ResourcesPendingRelease;

	// There can maximum be 8 rendertargets at one time 
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetHandles[8];

	GenerateMipsHelper MipGenHelper;
};