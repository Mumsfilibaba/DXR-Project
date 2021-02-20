#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"
#include "D3D12CommandAllocator.h"
#include "D3D12RootSignature.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

class D3D12ComputePipelineState;

class D3D12CommandListHandle : public D3D12DeviceChild
{
public:
    D3D12CommandListHandle(D3D12Device* InDevice)
        : D3D12DeviceChild(InDevice)
        , CmdList(nullptr)
        , CmdList5(nullptr)
    {
    }

    FORCEINLINE Bool Init(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocatorHandle& Allocator,ID3D12PipelineState* InitalPipeline)
    {
        HRESULT Result = GetDevice()->GetDevice()->CreateCommandList(1, Type, Allocator.GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CmdList));
        if (SUCCEEDED(Result))
        {
            CmdList->Close();

            LOG_INFO("[D3D12Device]: Created CommandList");

            if (FAILED(CmdList.As<ID3D12GraphicsCommandList5>(&CmdList5)))
            {
                LOG_ERROR("[D3D12CommandList]: FAILED to retrive DXR-CommandList");
                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            LOG_ERROR("[D3D12CommandList]: FAILED to create CommandList");
            return false;
        }
    }

    FORCEINLINE Bool Reset(D3D12CommandAllocatorHandle& Allocator)
    {
        IsReady = true;
        return SUCCEEDED(CmdList->Reset(Allocator.GetAllocator(), nullptr));
    }

    FORCEINLINE Bool Close()
    {
        IsReady = false;
        return SUCCEEDED(CmdList->Close());
    }

    FORCEINLINE void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const Float Color[4], UInt32 NumRects, const D3D12_RECT* Rects)
    {
        CmdList->ClearRenderTargetView(RenderTargetView, Color, NumRects, Rects);
    }

    FORCEINLINE void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView, D3D12_CLEAR_FLAGS Flags, Float Depth, const UInt8 Stencil)
    {
        CmdList->ClearDepthStencilView(DepthStencilView, Flags, Depth, Stencil, 0, nullptr);
    }

    FORCEINLINE void ClearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, const D3D12UnorderedAccessView* View, const Float ClearColor[4])
    {
        const D3D12Resource* Resource = View->GetResource();
        CmdList->ClearUnorderedAccessViewFloat(GPUHandle, View->GetOfflineHandle(), Resource->GetResource(), ClearColor, 0, nullptr);
    }

    FORCEINLINE void CopyBufferRegion(D3D12Resource* Destination, UInt64 DestinationOffset, D3D12Resource* Source, UInt64 SourceOffset, UInt64 SizeInBytes)
    {
        CopyBufferRegion(Destination->GetResource(), DestinationOffset, Source->GetResource(), SourceOffset, SizeInBytes);
    }

    FORCEINLINE void CopyBufferRegion(ID3D12Resource* Destination, UInt64 DestinationOffset, ID3D12Resource* Source, UInt64 SourceOffset, UInt64 SizeInBytes)
    {
        CmdList->CopyBufferRegion(Destination, DestinationOffset, Source, SourceOffset, SizeInBytes);
    }

    FORCEINLINE void CopyTextureRegion(
        const D3D12_TEXTURE_COPY_LOCATION* Destination, 
        UInt32 x, UInt32 y, UInt32 z, 
        const D3D12_TEXTURE_COPY_LOCATION* Source, 
        const D3D12_BOX* SourceBox)
    {
        CmdList->CopyTextureRegion(Destination, x, y, z, Source, SourceBox);
    }

    FORCEINLINE void CopyResource(D3D12Resource* Destination, D3D12Resource* Source)
    {
        CopyResource(Destination->GetResource(), Source->GetResource());
    }

    FORCEINLINE void CopyResource(ID3D12Resource* Destination, ID3D12Resource* Source)
    {
        CmdList->CopyResource(Destination, Source);
    }

    FORCEINLINE void ResolveSubresource(D3D12Resource* Destination, D3D12Resource* Source, DXGI_FORMAT Format)
    {
        CmdList->ResolveSubresource(Destination->GetResource(), 0, Source->GetResource(), 0, Format);
    }

    FORCEINLINE void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* Desc)
    {
        CmdList5->BuildRaytracingAccelerationStructure(Desc, 0, nullptr);
    }

    FORCEINLINE void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* Desc)
    {
        CmdList5->DispatchRays(Desc);
    }

    FORCEINLINE void Dispatch(UInt32 ThreadGroupCountX, UInt32 ThreadGroupCountY, UInt32 ThreadGroupCountZ)
    {
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    FORCEINLINE void DrawInstanced(UInt32 VertexCountPerInstance, UInt32 InstanceCount, UInt32 StartVertexLocation, UInt32 StartInstanceLocation)
    {
        CmdList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    FORCEINLINE void DrawIndexedInstanced(
        UInt32 IndexCountPerInstance, 
        UInt32 InstanceCount, 
        UInt32 StartIndexLocation, 
        UInt32 BaseVertexLocation, 
        UInt32 StartInstanceLocation)
    {
        CmdList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    FORCEINLINE void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, UInt32 DescriptorHeapCount)
    {
        CmdList->SetDescriptorHeaps(DescriptorHeapCount, DescriptorHeaps);
    }

    FORCEINLINE void SetStateObject(ID3D12StateObject* StateObject)
    {
        CmdList5->SetPipelineState1(StateObject);
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

    FORCEINLINE void SetGraphicsRoot32BitConstants(const Void* SourceData, UInt32 Num32BitValues, UInt32 DestOffsetIn32BitValues, UInt32 RootParameterIndex)
    {
        CmdList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
    }

    FORCEINLINE void SetComputeRoot32BitConstants(const Void* SourceData, UInt32 Num32BitValues, UInt32 DestOffsetIn32BitValues, UInt32 RootParameterIndex)
    {
        CmdList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
    }

    FORCEINLINE void IASetVertexBuffers(UInt32 StartSlot, const D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews, UInt32 VertexBufferViewCount)
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

    FORCEINLINE void RSSetShadingRate(D3D12_SHADING_RATE BaseShadingRate, const D3D12_SHADING_RATE_COMBINER* Combiners)
    {
        CmdList5->RSSetShadingRate(BaseShadingRate, Combiners);
    }

    FORCEINLINE void RSSetShadingRateImage(ID3D12Resource* ShadingRateImage)
    {
        CmdList5->RSSetShadingRateImage(ShadingRateImage);
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
        CmdList->OMSetRenderTargets(NumRenderTargetDescriptors, RenderTargetDescriptors, RTsSingleHandleToDescriptorRange, DepthStencilDescriptor);
    }

    FORCEINLINE void ResourceBarrier(const D3D12_RESOURCE_BARRIER* Barriers, UInt32 NumBarriers)
    {
        CmdList->ResourceBarrier(NumBarriers, Barriers);
    }

    FORCEINLINE void UnorderedAccessBarrier(ID3D12Resource* Resource)
    {
        D3D12_RESOURCE_BARRIER Barrier;
        Memory::Memzero(&Barrier);

        Barrier.Type          =  D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barrier.UAV.pResource = Resource;

        CmdList->ResourceBarrier(1, &Barrier);
    }

    FORCEINLINE Bool IsRecordning() const
    {
        return IsReady;
    }

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        CmdList->SetName(WideName.c_str());
    }

    FORCEINLINE ID3D12CommandList*          GetCommandList()         const { return CmdList.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList*  GetGraphicsCommandList() const { return CmdList.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList4* GetDXRCommandList()      const { return CmdList5.Get(); }

private:
    TComPtr<ID3D12GraphicsCommandList>  CmdList;
    TComPtr<ID3D12GraphicsCommandList5> CmdList5;
    Bool IsReady = false;
};