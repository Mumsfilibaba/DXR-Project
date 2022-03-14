#pragma once
#include "D3D12Device.h"
#include "D3D12Resource.h"
#include "D3D12RootSignature.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Views.h"

class CD3D12ComputePipelineState;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12CommandList

class CD3D12CommandList : public CD3D12DeviceObject
{
public:

    FORCEINLINE CD3D12CommandList(CD3D12Device* InDevice)
        : CD3D12DeviceObject(InDevice)
        , CommandList(nullptr)
        , CommandList5(nullptr)
    {
    }

    FORCEINLINE bool Initialize(D3D12_COMMAND_LIST_TYPE Type, CD3D12CommandAllocator& Allocator, ID3D12PipelineState* InitalPipeline)
    {
        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandList(1, Type, Allocator.GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CommandList));
        if (SUCCEEDED(Result))
        {
            CommandList->Close();

            D3D12_INFO("Created CommandList");

            if (FAILED(CommandList.GetAs<ID3D12GraphicsCommandList5>(&CommandList5)))
            {
                D3D12_ERROR_ALWAYS("FAILED to retrive DXR-CommandList");
                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            D3D12_ERROR_ALWAYS("[CD3D12CommandList]: FAILED to create CommandList");
            return false;
        }
    }

    FORCEINLINE bool Reset(CD3D12CommandAllocator& Allocator)
    {
        bIsRecording = true;

        HRESULT Result = CommandList->Reset(Allocator.GetAllocator(), nullptr);
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12RHIDeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE bool Close()
    {
        bIsRecording = false;

        HRESULT Result = CommandList->Close();
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12RHIDeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const float Color[4], uint32 NumRects, const D3D12_RECT* Rects)
    {
        CommandList->ClearRenderTargetView(RenderTargetView, Color, NumRects, Rects);
    }

    FORCEINLINE void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView, D3D12_CLEAR_FLAGS Flags, float Depth, const uint8 Stencil)
    {
        CommandList->ClearDepthStencilView(DepthStencilView, Flags, Depth, Stencil, 0, nullptr);
    }

    FORCEINLINE void ClearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, const CD3D12UnorderedAccessView* View, const float ClearColor[4])
    {
        const CD3D12Resource* Resource = View->GetResource();
        CommandList->ClearUnorderedAccessViewFloat(GPUHandle, View->GetOfflineHandle(), Resource->GetD3D12Resource(), ClearColor, 0, nullptr);
    }

    FORCEINLINE void CopyBufferRegion(CD3D12Resource* Destination, uint64 DestinationOffset, CD3D12Resource* Source, uint64 SourceOffset, uint64 SizeInBytes)
    {
        CopyBufferRegion(Destination->GetD3D12Resource(), DestinationOffset, Source->GetD3D12Resource(), SourceOffset, SizeInBytes);
    }

    FORCEINLINE void CopyBufferRegion(ID3D12Resource* Destination, uint64 DestinationOffset, ID3D12Resource* Source, uint64 SourceOffset, uint64 SizeInBytes)
    {
        CommandList->CopyBufferRegion(Destination, DestinationOffset, Source, SourceOffset, SizeInBytes);
    }

    FORCEINLINE void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* Destination, uint32 x, uint32 y, uint32 z, const D3D12_TEXTURE_COPY_LOCATION* Source, const D3D12_BOX* SourceBox)
    {
        CommandList->CopyTextureRegion(Destination, x, y, z, Source, SourceBox);
    }

    FORCEINLINE void CopyResource(CD3D12Resource* Destination, CD3D12Resource* Source)
    {
        CopyResource(Destination->GetD3D12Resource(), Source->GetD3D12Resource());
    }

    FORCEINLINE void CopyResource(ID3D12Resource* Destination, ID3D12Resource* Source)
    {
        CommandList->CopyResource(Destination, Source);
    }

    FORCEINLINE void ResolveSubresource(CD3D12Resource* Destination, CD3D12Resource* Source, DXGI_FORMAT Format)
    {
        CommandList->ResolveSubresource(Destination->GetD3D12Resource(), 0, Source->GetD3D12Resource(), 0, Format);
    }

    FORCEINLINE void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* Desc)
    {
        D3D12_ERROR(CommandList5 != nullptr, "Ray Tracing is not supported");
        CommandList5->BuildRaytracingAccelerationStructure(Desc, 0, nullptr);
    }

    FORCEINLINE void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* Desc)
    {
        D3D12_ERROR(CommandList5 != nullptr, "Ray Tracing is not supported");
        CommandList5->DispatchRays(Desc);
    }

    FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
    {
        CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    FORCEINLINE void SetDescriptorHeaps(ID3D12DescriptorHeap* const* DescriptorHeaps, uint32 DescriptorHeapCount)
    {
        CommandList->SetDescriptorHeaps(DescriptorHeapCount, DescriptorHeaps);
    }

    FORCEINLINE void SetStateObject(ID3D12StateObject* StateObject)
    {
        D3D12_ERROR(CommandList5 != nullptr, "StateObjects are not supported");
        CommandList5->SetPipelineState1(StateObject);
    }

    FORCEINLINE void SetPipelineState(ID3D12PipelineState* PipelineState)
    {
        CommandList->SetPipelineState(PipelineState);
    }

    FORCEINLINE void SetComputeRootSignature(CD3D12RootSignature* RootSignature)
    {
        CommandList->SetComputeRootSignature(RootSignature->GetRootSignature());
    }

    FORCEINLINE void SetGraphicsRootSignature(CD3D12RootSignature* RootSignature)
    {
        CommandList->SetGraphicsRootSignature(RootSignature->GetRootSignature());
    }

    FORCEINLINE void SetComputeRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, uint32 RootParameterIndex)
    {
        CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
    }

    FORCEINLINE void SetGraphicsRootDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor, uint32 RootParameterIndex)
    {
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

    FORCEINLINE void RSSetShadingRate(D3D12_SHADING_RATE BaseShadingRate, const D3D12_SHADING_RATE_COMBINER* Combiners)
    {
        D3D12_ERROR(CommandList5 != nullptr, "Shading-Rate is not supported");
        CommandList5->RSSetShadingRate(BaseShadingRate, Combiners);
    }

    FORCEINLINE void RSSetShadingRateImage(ID3D12Resource* ShadingRateImage)
    {
        D3D12_ERROR(CommandList5 != nullptr, "Shading-Rate is not supported");
        CommandList5->RSSetShadingRateImage(ShadingRateImage);
    }

    FORCEINLINE void OMSetBlendFactor(const float BlendFactor[4])
    {
        CommandList->OMSetBlendFactor(BlendFactor);
    }

    FORCEINLINE void OMSetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* RenderTargetDescriptors, uint32 NumRenderTargetDescriptors, bool bRTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* DepthStencilDescriptor)
    {
        CommandList->OMSetRenderTargets(NumRenderTargetDescriptors, RenderTargetDescriptors, bRTsSingleHandleToDescriptorRange, DepthStencilDescriptor);
    }

    FORCEINLINE void ResourceBarrier(const D3D12_RESOURCE_BARRIER* Barriers, uint32 NumBarriers)
    {
        CommandList->ResourceBarrier(NumBarriers, Barriers);
    }

    FORCEINLINE void DiscardResource(ID3D12Resource* Resource, const D3D12_DISCARD_REGION* Region)
    {
        CommandList->DiscardResource(Resource, Region);
    }

    FORCEINLINE void TransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, UINT Subresource)
    {
        D3D12_RESOURCE_BARRIER Barrier;
        CMemory::Memzero(&Barrier);

        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = Resource;
        Barrier.Transition.StateAfter  = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = Subresource;

        CommandList->ResourceBarrier(1, &Barrier);
    }

    FORCEINLINE void UnorderedAccessBarrier(ID3D12Resource* Resource)
    {
        D3D12_RESOURCE_BARRIER Barrier;
        CMemory::Memzero(&Barrier);

        Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barrier.UAV.pResource = Resource;

        CommandList->ResourceBarrier(1, &Barrier);
    }

    FORCEINLINE bool IsRecording() const
    {
        return bIsRecording;
    }

    FORCEINLINE void SetName(const String& Name)
    {
        CommandList->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), Name.CStr());
    }

    inline ID3D12CommandList*          GetCommandList() const         { return CommandList.Get(); }
    inline ID3D12GraphicsCommandList*  GetGraphicsCommandList() const { return CommandList.Get(); }
    inline ID3D12GraphicsCommandList5* GetDXRCommandList() const      { return CommandList5.Get(); }

private:
    TComPtr<ID3D12GraphicsCommandList>  CommandList;
    TComPtr<ID3D12GraphicsCommandList5> CommandList5;

    bool bIsRecording = false;
};