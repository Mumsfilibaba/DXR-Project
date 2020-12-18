#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"
#include "D3D12CommandAllocator.h"
#include "D3D12RootSignature.h"
#include "D3D12DescriptorHeap.h"

class D3D12ComputePipelineState;
class D3D12DescriptorTable;
class D3D12RootSignature;

/*
* D3D12CommandList
*/

class D3D12CommandList : public D3D12DeviceChild
{
public:
	inline D3D12CommandList::D3D12CommandList(D3D12Device* InDevice, ID3D12GraphicsCommandList* InCmdList)
		: D3D12DeviceChild(InDevice)
		, CmdList(InCmdList)
		, DXRCmdList(nullptr)
	{
	}
	
	~D3D12CommandList() = default;

	FORCEINLINE bool InitRayTracing()
	{
		if (FAILED(CmdList.As<ID3D12GraphicsCommandList4>(&DXRCmdList)))
		{
			LOG_ERROR("[D3D12CommandList]: FAILED to retrive DXR-CommandList");
			return false;
		}
		else
		{
			return true;
		}
	}

	FORCEINLINE bool Reset(D3D12CommandAllocator* Allocator)
	{
		IsReady = true;
		return SUCCEEDED(CmdList->Reset(Allocator->GetAllocator(), nullptr));
	}

	FORCEINLINE bool Close()
	{
		IsReady = false;
		return SUCCEEDED(CmdList->Close());
	}

	FORCEINLINE void ClearRenderTargetView(const D3D12RenderTargetView* View, const Float ClearColor[4])
	{
		CmdList->ClearRenderTargetView(View->GetOfflineHandle(), ClearColor, 0, nullptr);
	}

	FORCEINLINE void ClearDepthStencilView(
		const D3D12DepthStencilView* View, 
		D3D12_CLEAR_FLAGS Flags, 
		Float Depth, 
		const UInt8 Stencil)
	{
		CmdList->ClearDepthStencilView(View->GetOfflineHandle(), Flags, Depth, Stencil, 0, nullptr);
	}

	FORCEINLINE void ClearUnorderedAccessViewFloat(
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, 
		const D3D12UnorderedAccessView* View,
		 const Float ClearColor[4])
	{
		const D3D12Resource* Resource = View->GetResource();
		CmdList->ClearUnorderedAccessViewFloat(
			GPUHandle, 
			View->GetOfflineHandle(), 
			Resource->GetResource(), 
			ClearColor, 
			0, nullptr);
	}

	FORCEINLINE void CopyBuffer(
		D3D12Resource* Destination, 
		UInt64 DestinationOffset, 
		D3D12Resource* Source, 
		UInt64 SourceOffset, 
		UInt64 SizeInBytes)
	{
		CmdList->CopyBufferRegion(
			Destination->GetResource(), 
			DestinationOffset, 
			Source->GetResource(), 
			SourceOffset, 
			SizeInBytes);
	}

	FORCEINLINE void CopyTextureRegion(
		const D3D12_TEXTURE_COPY_LOCATION* Destination, 
		UInt32 x, 
		UInt32 y, 
		UInt32 z, 
		const D3D12_TEXTURE_COPY_LOCATION* Source, 
		const D3D12_BOX* SourceBox)
	{
		CmdList->CopyTextureRegion(Destination, x, y, z, Source, SourceBox);
	}

	FORCEINLINE void CopyResource(D3D12Resource* Destination, D3D12Resource* Source)
	{
		CmdList->CopyResource(Destination->GetResource(), Source->GetResource());
	}

	FORCEINLINE void ResolveSubresource(D3D12Resource* Destination, D3D12Resource* Source, DXGI_FORMAT Format)
	{
		CmdList->ResolveSubresource(Destination->GetResource(), 0, Source->GetResource(), 0, Format);
	}

	FORCEINLINE void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* Desc)
	{
		DXRCmdList->BuildRaytracingAccelerationStructure(Desc, 0, nullptr);
	}

	FORCEINLINE void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* Desc)
	{
		DXRCmdList->DispatchRays(Desc);
	}

	FORCEINLINE void Dispatch(
		UInt32 ThreadGroupCountX, 
		UInt32 ThreadGroupCountY, 
		UInt32 ThreadGroupCountZ)
	{
		CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	FORCEINLINE void DrawInstanced(
		UInt32 VertexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartVertexLocation, 
		UInt32 StartInstanceLocation)
	{
		CmdList->DrawInstanced(
			VertexCountPerInstance, 
			InstanceCount, 
			StartVertexLocation, 
			StartInstanceLocation);
	}

	FORCEINLINE void DrawIndexedInstanced(
		UInt32 IndexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartIndexLocation, 
		UInt32 BaseVertexLocation, 
		UInt32 StartInstanceLocation)
	{
		CmdList->DrawIndexedInstanced(
			IndexCountPerInstance, 
			InstanceCount, 
			StartIndexLocation, 
			BaseVertexLocation, 
			StartInstanceLocation);
	}

	FORCEINLINE void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, UInt32 DescriptorHeapCount)
	{
		CmdList->SetDescriptorHeaps(DescriptorHeapCount, DescriptorHeaps);
	}

	FORCEINLINE void SetStateObject(ID3D12StateObject* StateObject)
	{
		DXRCmdList->SetPipelineState1(StateObject);
	}

	FORCEINLINE void SetPipelineState(ID3D12PipelineState* PipelineState)
	{
		CmdList->SetPipelineState(PipelineState);
	}

	FORCEINLINE void SetComputeRootSignature(D3D12RootSignature* RootSignature)
	{
		CmdList->SetComputeRootSignature(RootSignature->GetRootSignature());
	}

	FORCEINLINE void SetGraphicsRootSignature(D3D12RootSignature* RootSignature)
	{
		CmdList->SetGraphicsRootSignature(RootSignature->GetRootSignature());
	}

	FORCEINLINE void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, UInt32 RootParameterIndex)
	{
		CmdList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, UInt32 RootParameterIndex)
	{
		CmdList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
	}

	FORCEINLINE void SetGraphicsRoot32BitConstants(
		const Void* SourceData, 
		UInt32 Num32BitValues, 
		UInt32 DestOffsetIn32BitValues, 
		UInt32 RootParameterIndex)
	{
		CmdList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void SetComputeRoot32BitConstants(
		const Void* SourceData, 
		UInt32 Num32BitValues, 
		UInt32 DestOffsetIn32BitValues, 
		UInt32 RootParameterIndex)
	{
		CmdList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
	}

	FORCEINLINE void IASetVertexBuffers(
		UInt32 StartSlot, 
		const D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews, 
		UInt32 VertexBufferViewCount)
	{
		CmdList->IASetVertexBuffers(StartSlot, VertexBufferViewCount, VertexBufferViews);
	}

	FORCEINLINE void IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* IndexBufferView)
	{
		CmdList->IASetIndexBuffer(IndexBufferView);
	}

	FORCEINLINE void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
	{
		CmdList->IASetPrimitiveTopology(PrimitiveTopology);
	}

	FORCEINLINE void RSSetViewports(const D3D12_VIEWPORT* Viewports, UInt32 ViewportCount)
	{
		CmdList->RSSetViewports(ViewportCount, Viewports);
	}

	FORCEINLINE void RSSetScissorRects(const D3D12_RECT* ScissorRects, UInt32 ScissorRectCount)
	{
		CmdList->RSSetScissorRects(ScissorRectCount, ScissorRects);
	}

	FORCEINLINE void OMSetBlendFactor(const Float BlendFactor[4])
	{
		CmdList->OMSetBlendFactor(BlendFactor);
	}

	FORCEINLINE void OMSetRenderTargets(
		const D3D12_CPU_DESCRIPTOR_HANDLE* RenderTargetDescriptors,
		UInt32 NumRenderTargetDescriptors,
		BOOL RTsSingleHandleToDescriptorRange,
		const D3D12_CPU_DESCRIPTOR_HANDLE* DepthStencilDescriptor)
	{
		CmdList->OMSetRenderTargets(
			NumRenderTargetDescriptors, 
			RenderTargetDescriptors, 
			RTsSingleHandleToDescriptorRange, 
			DepthStencilDescriptor);
	}

	FORCEINLINE void ResourceBarrier(const D3D12_RESOURCE_BARRIER* Barriers, UInt32 NumBarriers)
	{
		CmdList->ResourceBarrier(NumBarriers, Barriers);
	}

	FORCEINLINE bool IsRecordning() const
	{
		return IsReady;
	}

	FORCEINLINE void SetName(const std::string& Name)
	{
		std::wstring WideName = ConvertToWide(Name);
		CmdList->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12CommandList* GetCommandList() const
	{
		return CmdList.Get();
	}

	FORCEINLINE ID3D12GraphicsCommandList* GetGraphicsCommandList() const
	{
		return CmdList.Get();
	}

	FORCEINLINE ID3D12GraphicsCommandList4* GetDXRCommandList() const
	{
		return DXRCmdList.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CmdList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCmdList;
	bool IsReady = false;
};