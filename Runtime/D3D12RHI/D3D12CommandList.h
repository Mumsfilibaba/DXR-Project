#pragma once
#include "D3D12Resource.h"
#include "D3D12RootSignature.h"
#include "D3D12Descriptors.h"
#include "D3D12CommandAllocator.h"
#include "D3D12ResourceViews.h"
#include "D3D12RefCounted.h"

class FD3D12ComputePipelineState;

typedef TSharedRef<class FD3D12CommandList> FD3D12CommandListRef;

class FD3D12CommandList : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12CommandList(FD3D12Device* InDevice);

    bool Initialize(D3D12_COMMAND_LIST_TYPE Type, FD3D12CommandAllocator& Allocator, ID3D12PipelineState* InitalPipeline);

    bool Reset(FD3D12CommandAllocator& Allocator);
    
    bool Close();

    FORCEINLINE void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const float Color[4], uint32 NumRects, const D3D12_RECT* Rects)
    {
        CmdList->ClearRenderTargetView(RenderTargetView, Color, NumRects, Rects);
        NumCommands++;
    }

    FORCEINLINE void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView, D3D12_CLEAR_FLAGS Flags, float Depth, const uint8 Stencil)
    {
        CmdList->ClearDepthStencilView(DepthStencilView, Flags, Depth, Stencil, 0, nullptr);
        NumCommands++;
    }

    FORCEINLINE void ClearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, const FD3D12UnorderedAccessView* View, const float ClearColor[4])
    {
        const FD3D12Resource* Resource = View->GetD3D12Resource();
        CmdList->ClearUnorderedAccessViewFloat(GPUHandle, View->GetOfflineHandle(), Resource->GetD3D12Resource(), ClearColor, 0, nullptr);
        NumCommands++;
    }

    FORCEINLINE void CopyBufferRegion(FD3D12Resource* Destination, uint64 DestinationOffset, FD3D12Resource* Source, uint64 SourceOffset, uint64 SizeInBytes)
    {
        CopyBufferRegion(Destination->GetD3D12Resource(), DestinationOffset, Source->GetD3D12Resource(), SourceOffset, SizeInBytes);
        NumCommands++;
    }

    FORCEINLINE void CopyBufferRegion(ID3D12Resource* Destination, uint64 DestinationOffset, ID3D12Resource* Source, uint64 SourceOffset, uint64 SizeInBytes)
    {
        CmdList->CopyBufferRegion(Destination, DestinationOffset, Source, SourceOffset, SizeInBytes);
        NumCommands++;
    }

    FORCEINLINE void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* Destination, uint32 x, uint32 y, uint32 z, const D3D12_TEXTURE_COPY_LOCATION* Source, const D3D12_BOX* SourceBox)
    {
        CmdList->CopyTextureRegion(Destination, x, y, z, Source, SourceBox);
        NumCommands++;
    }

    FORCEINLINE void CopyResource(FD3D12Resource* Destination, FD3D12Resource* Source)
    {
        CopyResource(Destination->GetD3D12Resource(), Source->GetD3D12Resource());
    }

    FORCEINLINE void CopyResource(ID3D12Resource* Destination, ID3D12Resource* Source)
    {
        CmdList->CopyResource(Destination, Source);
        NumCommands++;
    }

    FORCEINLINE void ResolveSubresource(FD3D12Resource* Destination, FD3D12Resource* Source, DXGI_FORMAT Format)
    {
        CmdList->ResolveSubresource(Destination->GetD3D12Resource(), 0, Source->GetD3D12Resource(), 0, Format);
        NumCommands++;
    }

    FORCEINLINE void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* Desc)
    {
        D3D12_ERROR_COND(CmdList5 != nullptr, "Ray Tracing is not supported");
        CmdList5->BuildRaytracingAccelerationStructure(Desc, 0, nullptr);
        NumCommands++;
    }

    FORCEINLINE void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* Desc)
    {
        D3D12_ERROR_COND(CmdList5 != nullptr, "Ray Tracing is not supported");
        CmdList5->DispatchRays(Desc);
        NumCommands++;
    }

    FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumCommands++;
    }

    FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        CmdList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumCommands++;
    }

    FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
    {
        CmdList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        NumCommands++;
    }

    FORCEINLINE void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, uint32 DescriptorHeapCount)
    {
        CmdList->SetDescriptorHeaps(DescriptorHeapCount, DescriptorHeaps);
        NumCommands++;
    }

    FORCEINLINE void SetStateObject(ID3D12StateObject* StateObject)
    {
        D3D12_ERROR_COND(CmdList5 != nullptr, "StateObjects are not supported");
        CmdList5->SetPipelineState1(StateObject);
        NumCommands++;
    }

    FORCEINLINE void SetPipelineState(ID3D12PipelineState* PipelineState)
    {
        CmdList->SetPipelineState(PipelineState);
        NumCommands++;
    }

    FORCEINLINE void SetComputeRootSignature(FD3D12RootSignature* RootSignature)
    {
        CmdList->SetComputeRootSignature(RootSignature->GetD3D12RootSignature());
        NumCommands++;
    }

    FORCEINLINE void SetGraphicsRootSignature(FD3D12RootSignature* RootSignature)
    {
        CmdList->SetGraphicsRootSignature(RootSignature->GetD3D12RootSignature());
        NumCommands++;
    }

    FORCEINLINE void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, uint32 RootParameterIndex)
    {
        CmdList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
        NumCommands++;
    }

    FORCEINLINE void SetGraphicsRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, uint32 RootParameterIndex)
    {
        CmdList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
        NumCommands++;
    }

    FORCEINLINE void SetGraphicsRoot32BitConstants(const void* SourceData, uint32 Num32BitValues, uint32 DestOffsetIn32BitValues, uint32 RootParameterIndex)
    {
        CmdList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
        NumCommands++;
    }

    FORCEINLINE void SetComputeRoot32BitConstants(const void* SourceData, uint32 Num32BitValues, uint32 DestOffsetIn32BitValues, uint32 RootParameterIndex)
    {
        CmdList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValues, SourceData, DestOffsetIn32BitValues);
        NumCommands++;
    }

    FORCEINLINE void IASetVertexBuffers(uint32 StartSlot, const D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews, uint32 NumVertexBufferViews)
    {
        CmdList->IASetVertexBuffers(StartSlot, NumVertexBufferViews, VertexBufferViews);
        NumCommands++;
    }

    FORCEINLINE void IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* IndexBufferView)
    {
        CmdList->IASetIndexBuffer(IndexBufferView);
        NumCommands++;
    }

    FORCEINLINE void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
    {
        CmdList->IASetPrimitiveTopology(PrimitiveTopology);
        NumCommands++;
    }

    FORCEINLINE void RSSetViewports(const D3D12_VIEWPORT* Viewports, uint32 ViewportCount)
    {
        CmdList->RSSetViewports(ViewportCount, Viewports);
        NumCommands++;
    }

    FORCEINLINE void RSSetScissorRects(const D3D12_RECT* ScissorRects, uint32 ScissorRectCount)
    {
        CmdList->RSSetScissorRects(ScissorRectCount, ScissorRects);
        NumCommands++;
    }

    FORCEINLINE void RSSetShadingRate(D3D12_SHADING_RATE BaseShadingRate, const D3D12_SHADING_RATE_COMBINER* Combiners)
    {
        D3D12_ERROR_COND(CmdList5 != nullptr, "Shading-Rate is not supported");
        CmdList5->RSSetShadingRate(BaseShadingRate, Combiners);
        NumCommands++;
    }

    FORCEINLINE void RSSetShadingRateImage(ID3D12Resource* ShadingRateImage)
    {
        D3D12_ERROR_COND(CmdList5 != nullptr, "Shading-Rate is not supported");
        CmdList5->RSSetShadingRateImage(ShadingRateImage);
        NumCommands++;
    }

    FORCEINLINE void OMSetBlendFactor(const float BlendFactor[4])
    {
        CmdList->OMSetBlendFactor(BlendFactor);
        NumCommands++;
    }

    FORCEINLINE void OMSetRenderTargets(
        const D3D12_CPU_DESCRIPTOR_HANDLE* RenderTargetDescriptors,
        uint32                             NumRenderTargetDescriptors,
        bool                               bRTsSingleHandleToDescriptorRange,
        const D3D12_CPU_DESCRIPTOR_HANDLE* DepthStencilDescriptor)
    {
        CmdList->OMSetRenderTargets(NumRenderTargetDescriptors, RenderTargetDescriptors, bRTsSingleHandleToDescriptorRange, DepthStencilDescriptor);
        NumCommands++;
    }

    FORCEINLINE void ResourceBarrier(const D3D12_RESOURCE_BARRIER* Barriers, uint32 NumBarriers)
    {
        CmdList->ResourceBarrier(NumBarriers, Barriers);
        NumCommands++;
    }

    FORCEINLINE void DiscardResource(ID3D12Resource* Resource, const D3D12_DISCARD_REGION* Region)
    {
        CmdList->DiscardResource(Resource, Region);
        NumCommands++;
    }

    FORCEINLINE void TransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, UINT Subresource)
    {
        D3D12_RESOURCE_BARRIER Barrier;
        FMemory::Memzero(&Barrier);

        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = Resource;
        Barrier.Transition.StateAfter  = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = Subresource;

        CmdList->ResourceBarrier(1, &Barrier);
        NumCommands++;
    }

    FORCEINLINE void UnorderedAccessBarrier(ID3D12Resource* Resource)
    {
        D3D12_RESOURCE_BARRIER Barrier;
        FMemory::Memzero(&Barrier);

        Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barrier.UAV.pResource = Resource;

        CmdList->ResourceBarrier(1, &Barrier);
        NumCommands++;
    }

    FORCEINLINE bool IsReady() const
    {
        return bIsReady;
    }

    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

    FORCEINLINE void SetDebugName(const FString& Name)
    {
        FStringWide WideName = CharToWide(Name);
        CmdList->SetName(WideName.GetCString());
    }

    FORCEINLINE ID3D12GraphicsCommandList*  GetGraphicsCommandList()  const { return CmdList.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList1* GetGraphicsCommandList1() const { return CmdList1.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList2* GetGraphicsCommandList2() const { return CmdList2.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList3* GetGraphicsCommandList3() const { return CmdList3.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const { return CmdList4.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList5* GetGraphicsCommandList5() const { return CmdList5.Get(); }
    FORCEINLINE ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const { return CmdList6.Get(); }
    FORCEINLINE ID3D12CommandList*          GetCommandList()          const { return CmdList.Get(); }

private:
    TComPtr<ID3D12GraphicsCommandList>  CmdList;
    TComPtr<ID3D12GraphicsCommandList1> CmdList1;
    TComPtr<ID3D12GraphicsCommandList2> CmdList2;
    TComPtr<ID3D12GraphicsCommandList3> CmdList3;
    TComPtr<ID3D12GraphicsCommandList4> CmdList4;
    TComPtr<ID3D12GraphicsCommandList5> CmdList5;
    TComPtr<ID3D12GraphicsCommandList6> CmdList6;

    uint32 NumCommands = 0;
    bool   bIsReady    = false;
};