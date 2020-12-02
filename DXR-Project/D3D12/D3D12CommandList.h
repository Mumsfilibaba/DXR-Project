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

	void UploadBufferData(class D3D12Buffer* Dest, const UInt32 DestOffset, const Void* Src, const UInt32 SizeInBytes);
	void UploadTextureData(D3D12Texture* Dest, const Void* Src, DXGI_FORMAT Format, const UInt32 Width, const UInt32 Height, const UInt32 Depth, const UInt32 Stride, const UInt32 RowPitch);

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

	FORCEINLINE void ClearRenderTargetView(const D3D12RenderTargetView* View, const Float ClearColor[4])
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearRenderTargetView(View->GetOfflineHandle(), ClearColor, 0, nullptr);
	}

	FORCEINLINE void ClearDepthStencilView(const D3D12DepthStencilView* View, D3D12_CLEAR_FLAGS Flags, Float Depth, const UInt8 Stencil)
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearDepthStencilView(View->GetOfflineHandle(), Flags, Depth, Stencil, 0, nullptr);
	}

	FORCEINLINE void ClearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, const D3D12UnorderedAccessView* View, const Float ClearColor[4])
	{
		FlushDeferredResourceBarriers();

		CommandList->ClearUnorderedAccessViewFloat(GPUHandle, View->GetOfflineHandle(), View->GetResource(), ClearColor, 0, nullptr);
	}

	FORCEINLINE void CopyBuffer(D3D12Resource* Destination, UInt64 DestinationOffset, D3D12Resource* Source, UInt64 SourceOffset, UInt64 SizeInBytes)
	{
		FlushDeferredResourceBarriers();

		CommandList->CopyBufferRegion(Destination->GetResource(), DestinationOffset, Source->GetResource(), SourceOffset, SizeInBytes);
	}

	FORCEINLINE void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* InDestination, UInt32 X, UInt32 Y, UInt32 Z, const D3D12_TEXTURE_COPY_LOCATION* InSource, const D3D12_BOX* InSourceBox)
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

	FORCEINLINE void Dispatch(UInt32 ThreadGroupCountX, UInt32 ThreadGroupCountY, UInt32 ThreadGroupCountZ)
	{
		FlushDeferredResourceBarriers();

		CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	FORCEINLINE void DrawInstanced(UInt32 VertexCountPerInstance, UInt32 InstanceCount, UInt32 StartVertexLocation, UInt32 StartInstanceLocation)
	{
		FlushDeferredResourceBarriers();

		CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);

		NumDrawCalls++;
	}

	FORCEINLINE void DrawIndexedInstanced(UInt32 IndexCountPerInstance, UInt32 InstanceCount, UInt32 StartIndexLocation, UInt32 BaseVertexLocation, UInt32 StartInstanceLocation)
	{
		FlushDeferredResourceBarriers();

		CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

		NumDrawCalls++;
	}

	FORCEINLINE void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, UInt32 DescriptorHeapCount)
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

	FORCEINLINE void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, UInt32 RootParameterIndex)
	{
		FlushDeferredResourceBarriers();

		CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, UInt32 RootParameterIndex)
	{
		FlushDeferredResourceBarriers();

		CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRoot32BitConstants(const Void* SourceData, UInt32 Num32BitValues, UInt32 DestOffsetIn32BitValues, UInt32 RootParameterIndex)
	{
		CommandList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void SetComputeRoot32BitConstants(const Void* SourceData, UInt32 Num32BitValues, UInt32 DestOffsetIn32BitValues, UInt32 RootParameterIndex)
	{
		CommandList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void IASetVertexBuffers(UInt32 StartSlot, const D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews, UInt32 VertexBufferViewCount)
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

	FORCEINLINE void RSSetViewports(const D3D12_VIEWPORT* Viewports, UInt32 ViewportCount)
	{
		CommandList->RSSetViewports(ViewportCount, Viewports);
	}

	FORCEINLINE void RSSetScissorRects(const D3D12_RECT* ScissorRects, UInt32 ScissorRectCount)
	{
		CommandList->RSSetScissorRects(ScissorRectCount, ScissorRects);
	}

	FORCEINLINE void OMSetBlendFactor(const Float BlendFactor[4])
	{
		CommandList->OMSetBlendFactor(BlendFactor);
	}

	FORCEINLINE void OMSetRenderTargets(const D3D12RenderTargetView* const * RenderTargetViews, UInt32 RenderTargetCount, const D3D12DepthStencilView* DepthStencilView)
	{
		for (UInt32 I = 0; I < RenderTargetCount; I++)
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
	
	FORCEINLINE UInt32 GetNumDrawCalls() const
	{
		return NumDrawCalls;
	}

public:
	// DeviceChild
	virtual void SetDebugName(const std::string& DebugName) override;

protected:
	bool CreateUploadBuffer(UInt32 SizeInBytes = 1024U);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCommandList;

	class D3D12Buffer* UploadBuffer = nullptr;
	Byte*	UploadPointer		= nullptr;
	UInt32	UploadBufferOffset	= 0;
	UInt32	NumDrawCalls		= 0;

	TArray<D3D12_RESOURCE_BARRIER> DeferredResourceBarriers;
	TArray<Microsoft::WRL::ComPtr<ID3D12Resource>> ResourcesPendingRelease;

	// There can maximum be 8 rendertargets at one time 
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetHandles[8];

	GenerateMipsHelper MipGenHelper;
};