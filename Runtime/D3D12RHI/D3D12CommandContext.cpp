#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12Core.h"
#include "D3D12Shader.h"
#include "D3D12RHI.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12PipelineState.h"
#include "D3D12RayTracing.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12TimestampQuery.h"
#include "D3D12CommandContext.h"
#include "DynamicD3D12.h"
#include "D3D12Viewport.h"
#include "Core/Math/Vector2.h"
#include "Core/Misc/FrameProfiler.h"

#include <pix.h>

void FD3D12ResourceBarrierBatcher::AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
    CHECK(Resource != nullptr);

    if (BeforeState != AfterState)
    {
        // Make sure we are not already have transition for this resource
        for (TArray<D3D12_RESOURCE_BARRIER>::IteratorType Iterator = Barriers.Iterator(); !Iterator.IsEnd(); Iterator++)
        {
            if (Iterator->Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
            {
                if (Iterator->Transition.pResource == Resource)
                {
                    if (Iterator->Transition.StateBefore == AfterState)
                    {
                        Barriers.RemoveAt(Iterator.GetIndex());
                    }
                    else
                    {
                        Iterator->Transition.StateAfter = AfterState;
                    }

                    return;
                }
            }
        }

        // Add new resource barrier
        D3D12_RESOURCE_BARRIER Barrier;
        FMemory::Memzero(&Barrier);

        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = Resource;
        Barrier.Transition.StateAfter  = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        Barriers.Emplace(Barrier);
    }
}

void FD3D12ResourceBarrierBatcher::AddUnorderedAccessBarrier(ID3D12Resource* Resource)
{
    CHECK(Resource != nullptr);

    // Make sure we are not already have UAV barrier for this resource
    for (TArray<D3D12_RESOURCE_BARRIER>::IteratorType Iterator = Barriers.Iterator(); !Iterator.IsEnd(); Iterator++)
    {
        if (Iterator->Type == D3D12_RESOURCE_BARRIER_TYPE_UAV)
        {
            if (Iterator->UAV.pResource == Resource)
            {
                Barriers.RemoveAt(Iterator.GetIndex());
                return;
            }
        }
    }

    D3D12_RESOURCE_BARRIER Barrier;
    FMemory::Memzero(&Barrier);

    Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.UAV.pResource = Resource;
    Barriers.Emplace(Barrier);
}


FD3D12CommandContext::FD3D12CommandContext(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : IRHICommandContext()
    , FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandList(nullptr)
    , CommandAllocator(nullptr)
    , CommandAllocatorManager(InDevice, InQueueType)
    , ContextState(InDevice, *this)
    , AssignedFenceValue(0)
    , CommandContextCS()
    , ResolveQueries()
    , BarrierBatcher()
{
}

FD3D12CommandContext::~FD3D12CommandContext()
{
    RHIFlush();
}

bool FD3D12CommandContext::Initialize()
{
    if (!ContextState.Initialize())
    {
        D3D12_ERROR("Failed to initialize ContextState");
        return false;
    }

    return true;
}

void FD3D12CommandContext::ObtainCommandList()
{
    TRACE_FUNCTION_SCOPE();

    if (!CommandAllocator)
    {
        CommandAllocator = CommandAllocatorManager.ObtainAllocator();
        if (!CommandAllocator)
        {
            D3D12_ERROR("Failed to Obtain CommandAllocator");
        }
    }

    if (!CommandList)
    {
        CommandList = GetDevice()->GetCommandListManager(QueueType)->ObtainCommandList(*CommandAllocator, nullptr);
        if (!CommandList)
        {
            D3D12_ERROR("Failed to initialize CommandList");
        }
    }
    else if (AssignedFenceValue == 0)
    {
        if (!CommandList->IsReady())
        {
            if (!CommandList->Reset(*CommandAllocator))
            {
                D3D12_ERROR("Failed to reset Commandlist");
            }
        }
    }

    if (FD3D12CommandListManager* CommandListManager = GetDevice()->GetCommandListManager(QueueType))
    {
        AssignedFenceValue = CommandListManager->GetFenceManager().GetCurrentValue() + 1;
        CHECK(AssignedFenceValue != 0);
    }
    else
    {
        DEBUG_BREAK();
    }
}

void FD3D12CommandContext::FinishCommandList()
{
    TRACE_FUNCTION_SCOPE();

    FlushResourceBarriers();

    for (int32 QueryIndex = 0; QueryIndex < ResolveQueries.Size(); ++QueryIndex)
    {
        ResolveQueries[QueryIndex]->ResolveQueries(*this);
    }

    ResolveQueries.Clear();

    // Only execute if we have executed any commands
    const uint32 NumCommands = CommandList->GetNumCommands();
    if (NumCommands > 0)
    {
        // Close CommandList
        if (!CommandList->Close())
        {
            D3D12_ERROR("Failed to close CommandList");
            return;
        }

        // Execute and update fence-value
        FD3D12FenceSyncPoint SyncPoint = GetDevice()->GetCommandListManager(QueueType)->ExecuteCommandList(CommandList, false);

        // Release Allocator
        CommandAllocatorManager.ReleaseAllocator(CommandAllocator);
        CommandAllocator = nullptr;
    }

    // Ensure that the state will rebind the necessary state when we obtain a new CommandList
    ContextState.ResetStateForNewCommandList();

    // TODO: With multiple contexts this will not be safe
    AssignedFenceValue = 0;
}

void FD3D12CommandContext::UpdateBuffer(FD3D12Resource* Resource, const FBufferRegion& BufferRegion, const void* SrcData)
{
    D3D12_ERROR_COND(Resource != nullptr, "Resource cannot be nullptr");
    D3D12_ERROR_COND(SrcData != nullptr, "SourceData cannot be nullptr");

    if (!BufferRegion.Size)
    {
        D3D12_WARNING("Trying to update buffer with zero size");
        return;
    }

    FlushResourceBarriers();

    D3D12_HEAP_TYPE HeapType = Resource->GetHeapType();
    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        const D3D12_RANGE BufferRange = 
        { 
            BufferRegion.Offset,
            BufferRegion.Offset + BufferRegion.Size
        };

        // Map buffer memory
        uint8* BufferData = reinterpret_cast<uint8*>(Resource->MapRange(0, &BufferRange));
        if (!BufferData)
        {
            D3D12_ERROR("Failed to map buffer data");
            return;
        }

        // Copy over relevant data
        FMemory::Memcpy(BufferData + BufferRegion.Offset, SrcData, BufferRegion.Size);

        // Unmap buffer memory
        Resource->UnmapRange(0, &BufferRange);
    }
    else
    {
        FD3D12UploadAllocation Allocation = GetDevice()->GetUploadAllocator().Allocate(BufferRegion.Size, 1);
        FMemory::Memcpy(Allocation.Memory, SrcData, BufferRegion.Size);

        // Copy on the GPU
        CommandList->CopyBufferRegion(Resource->GetD3D12Resource(), BufferRegion.Offset, Allocation.Resource.Get(), Allocation.ResourceOffset, BufferRegion.Size);

        // Defer deletion of the upload buffer
        GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Allocation.Resource.Get());
    }

    // Defer deletion of the destination resource
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Resource);
}

void FD3D12CommandContext::RHIStartContext()
{
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Lock to the thread that started the context
    CommandContextCS.Lock();

    // Reset the state
    ContextState.ResetState();

    // Retrieve a new CommandList
    ObtainCommandList();
}

void FD3D12CommandContext::RHIFinishContext()
{
    // Submit the CommandList
    FinishCommandList();

    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Unlock from the thread that started the context
    CommandContextCS.Unlock();
}

void FD3D12CommandContext::RHIBeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
    FD3D12TimestampQuery* D3D12TimestampQuery = static_cast<FD3D12TimestampQuery*>(TimestampQuery);
    D3D12_ERROR_COND(D3D12TimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* DxCmdList = CommandList->GetGraphicsCommandList();
    D3D12TimestampQuery->BeginQuery(DxCmdList, Index);

    ResolveQueries.AddUnique(MakeSharedRef<FD3D12TimestampQuery>(D3D12TimestampQuery));
}

void FD3D12CommandContext::RHIEndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
    FD3D12TimestampQuery* D3D12TimestampQuery = static_cast<FD3D12TimestampQuery*>(TimestampQuery);
    D3D12_ERROR_COND(D3D12TimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* D3D12CmdList = CommandList->GetGraphicsCommandList();
    D3D12TimestampQuery->EndQuery(D3D12CmdList, Index);
}

void FD3D12CommandContext::RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    FlushResourceBarriers();

    FD3D12Texture* D3D12Texture = GetD3D12Texture(RenderTargetView.Texture);
    D3D12_ERROR_COND(D3D12Texture != nullptr, "Texture cannot be nullptr when clearing the surface");

    FD3D12RenderTargetView* D3D12RenderTargetView = D3D12Texture->GetOrCreateRenderTargetView(RenderTargetView);
    CHECK(D3D12RenderTargetView != nullptr);

    CommandList->ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), ClearColor.Data(), 0, nullptr);
}

void FD3D12CommandContext::RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    FlushResourceBarriers();

    FD3D12Texture* D3D12Texture = GetD3D12Texture(DepthStencilView.Texture);
    D3D12_ERROR_COND(D3D12Texture != nullptr, "Texture cannot be nullptr when clearing the surface");

    FD3D12DepthStencilView* D3D12DepthStencilView = D3D12Texture->GetOrCreateDepthStencilView(DepthStencilView);
    CHECK(D3D12DepthStencilView != nullptr);

    CommandList->ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Depth, Stencil);
}

void FD3D12CommandContext::RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    D3D12_ERROR_COND(UnorderedAccessView != nullptr, "UnorderedAccessView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(UnorderedAccessView);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, D3D12UnorderedAccessView);

    // Ensure there are enough allocators for a new descriptor
    FD3D12LocalDescriptorHeap& ResourceHeap = ContextState.GetDescriptorCache().GetResourceHeap();
    if (!ResourceHeap.HasSpace(1))
    {
        ResourceHeap.Realloc();
        CHECK(ResourceHeap.HasSpace(1));
    }

    ContextState.GetDescriptorCache().SetDescriptorHeaps();

    // Copy descriptor and clear the resource view
    const uint32 HandeOffset = ResourceHeap.AllocateHandles(1);

    const D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle   = D3D12UnorderedAccessView->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandleCPU = ResourceHeap.GetCPUHandle(HandeOffset);
    GetDevice()->GetD3D12Device()->CopyDescriptorsSimple(1, OnlineHandleCPU, OfflineHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandleGPU = ResourceHeap.GetGPUHandle(HandeOffset);
    CommandList->ClearUnorderedAccessViewFloat(OnlineHandleGPU, D3D12UnorderedAccessView, ClearColor.Data());
}

void FD3D12CommandContext::RHIBeginRenderPass(const FRHIRenderPassDesc& RenderPassInitializer)
{
    FlushResourceBarriers();

    FD3D12RenderTargetView* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    FD3D12DepthStencilView* DepthStencilView = nullptr;

    for (uint32 Index = 0; Index < RenderPassInitializer.NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& CurrentRTV = RenderPassInitializer.RenderTargets[Index];
        if (FD3D12Texture* RenderTarget = GetD3D12Texture(CurrentRTV.Texture))
        {
            FD3D12RenderTargetView* CurrentRenderTargetView = RenderTarget->GetOrCreateRenderTargetView(CurrentRTV);
            CHECK(CurrentRenderTargetView != nullptr);

            // Clear the RenderTarget here, since we expect it to be cleared when the RenderPass begin, however
            // it is not certain that there will be a call to draw inside of the RenderPass
            if (CurrentRTV.LoadAction == EAttachmentLoadAction::Clear)
            {
                CommandList->ClearRenderTargetView(CurrentRenderTargetView->GetOfflineHandle(), &CurrentRTV.ClearValue.r, 0, nullptr);
            }
            
            RenderTargetViews[Index] = CurrentRenderTargetView;
        }
        else
        {
            RenderTargetViews[Index] = nullptr;
        }
    }

    const FRHIDepthStencilView& CurrentDSV = RenderPassInitializer.DepthStencilView;
    if (FD3D12Texture* DepthStencil = GetD3D12Texture(CurrentDSV.Texture))
    {
        FD3D12DepthStencilView* CurrentDepthStencilView = DepthStencil->GetOrCreateDepthStencilView(CurrentDSV);
        CHECK(CurrentDepthStencilView != nullptr);

        // Clear the DepthStencil here, since we expect it to be cleared when the RenderPass begin, however
        // it is not certain that there will be a call to draw inside of the RenderPass
        if (CurrentDSV.LoadAction == EAttachmentLoadAction::Clear)
        {
            const D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
            CommandList->ClearDepthStencilView(CurrentDepthStencilView->GetOfflineHandle(), ClearFlags, CurrentDSV.ClearValue.Depth, CurrentDSV.ClearValue.Stencil);
        }

        DepthStencilView = CurrentDepthStencilView;
    }
    else
    {
        DepthStencilView = nullptr;
    }

    ContextState.SetRenderTargets(RenderTargetViews, RenderPassInitializer.NumRenderTargets, DepthStencilView);

    // ShadingRate
    FD3D12Texture* ShadingRateImage = GetD3D12Texture(RenderPassInitializer.ShadingRateTexture);
    ContextState.SetShadingRateImage(ShadingRateImage);
    ContextState.SetShadingRate(RenderPassInitializer.StaticShadingRate);
}

void FD3D12CommandContext::RHISetViewport(const FRHIViewportRegion& ViewportRegion)
{
    D3D12_VIEWPORT Viewport;
    Viewport.Width    = ViewportRegion.Width;
    Viewport.Height   = ViewportRegion.Height;
    Viewport.MaxDepth = ViewportRegion.MaxDepth;
    Viewport.MinDepth = ViewportRegion.MinDepth;
    Viewport.TopLeftX = ViewportRegion.PositionX;
    Viewport.TopLeftY = ViewportRegion.PositionY;

    ContextState.SetViewports(&Viewport, 1);
}

void FD3D12CommandContext::RHISetScissorRect(const FRHIScissorRegion& ScissorRegion)
{
    D3D12_RECT ScissorRect;
    ScissorRect.left   = LONG(ScissorRegion.PositionX);
    ScissorRect.right  = LONG(ScissorRegion.Width);
    ScissorRect.top    = LONG(ScissorRegion.PositionY);
    ScissorRect.bottom = LONG(ScissorRegion.Height);

    ContextState.SetScissorRects(&ScissorRect, 1);
}

void FD3D12CommandContext::RHISetBlendFactor(const FVector4& Color)
{
    ContextState.SetBlendFactor(Color.Data());
}

void FD3D12CommandContext::RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FD3D12Buffer* D3DVertexBuffer = static_cast<FD3D12Buffer*>(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(D3DVertexBuffer, BufferSlot + Index);
    }
}

void FD3D12CommandContext::RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FD3D12Buffer* D3DIndexBuffer = static_cast<FD3D12Buffer*>(IndexBuffer);
    ContextState.SetIndexBuffer(D3DIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FD3D12CommandContext::RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FD3D12GraphicsPipelineState* GraphicsPipelineState = static_cast<FD3D12GraphicsPipelineState*>(PipelineState);
    ContextState.SetGraphicsPipelineState(GraphicsPipelineState);
}

void FD3D12CommandContext::RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState)
{
    FD3D12ComputePipelineState* ComputePipelineState = static_cast<FD3D12ComputePipelineState*>(PipelineState);
    ContextState.SetComputePipelineState(ComputePipelineState);
}

void FD3D12CommandContext::RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    ContextState.SetShaderConstants(reinterpret_cast<const uint32*>(Shader32BitConstants), Num32BitConstants);
}

void FD3D12CommandContext::RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex < D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(ShaderResourceView);
    ContextState.SetSRV(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex + InShaderResourceViews.Size() <= D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(InShaderResourceViews[Index]);
        ContextState.SetSRV(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex < D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(UnorderedAccessView);
    ContextState.SetUAV(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex + InUnorderedAccessViews.Size() <= D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex < D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);

    FD3D12ConstantBufferView* D3D12ConstantBufferView = nullptr;
    if (ConstantBuffer)
    {
        D3D12ConstantBufferView = static_cast<FD3D12Buffer*>(ConstantBuffer)->GetConstantBufferView();
        CHECK(D3D12ConstantBufferView != nullptr);
    }

    ContextState.SetCBV(D3D12ConstantBufferView, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex + InConstantBuffers.Size() <= D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);

    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FD3D12ConstantBufferView* D3D12ConstantBufferView = nullptr;
        if (InConstantBuffers[Index])
        {
            D3D12ConstantBufferView = static_cast<FD3D12Buffer*>(InConstantBuffers[Index])->GetConstantBufferView();
            CHECK(D3D12ConstantBufferView != nullptr);
        }

        ContextState.SetCBV(D3D12ConstantBufferView, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex < D3D12_DEFAULT_SAMPLER_STATE_COUNT);

    FD3D12SamplerState* D3D12SamplerState = static_cast<FD3D12SamplerState*>(SamplerState);
    ContextState.SetSampler(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);
    CHECK(ParameterIndex + InSamplerStates.Size() <= D3D12_DEFAULT_SAMPLER_STATE_COUNT);

    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FD3D12SamplerState* D3D12SamplerState = static_cast<FD3D12SamplerState*>(InSamplerStates[Index]);
        ContextState.SetSampler(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    D3D12_ERROR_COND(Dst != nullptr && Src != nullptr, "Dst or Src cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Texture* D3D12Destination = GetD3D12Texture(Dst);
    FD3D12Texture* D3D12Source      = GetD3D12Texture(Src);
    const DXGI_FORMAT DstFormat = D3D12Destination->GetDXGIFormat();
    const DXGI_FORMAT SrcFormat = D3D12Source->GetDXGIFormat();

    //TODO: For now texture must be the same format. I.e typeless does probably not work
    D3D12_ERROR_COND(DstFormat == SrcFormat, "Dst and Src texture must have the same format");
    CommandList->ResolveSubresource(D3D12Destination->GetD3D12Resource(), D3D12Source->GetD3D12Resource(), DstFormat);

    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Dst);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Src);
}

void FD3D12CommandContext::RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (BufferRegion.Size)
    {
        FD3D12Buffer* D3D12Destination = GetD3D12Buffer(Dst);
        UpdateBuffer(D3D12Destination->GetD3D12Resource(), BufferRegion, SrcData);
    }
}

void FD3D12CommandContext::RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    D3D12_ERROR_COND(SrcData != nullptr, "SrcData cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Texture* D3D12Destination = GetD3D12Texture(Dst);
    D3D12_ERROR_COND(D3D12Destination != nullptr, "Dst cannot be nullptr");

    FD3D12Resource* D3D12Resource = D3D12Destination->GetD3D12Resource();
    D3D12_ERROR_COND(D3D12Resource != nullptr, "Resource cannot be nullptr");

    D3D12_RESOURCE_DESC Desc = D3D12Resource->GetDesc();

    UINT64 RequiredSize = 0;
    UINT64 RowPitch     = 0;
    UINT32 NumRows      = 0;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubresourceFootprint;
    GetDevice()->GetD3D12Device()->GetCopyableFootprints(&Desc, MipLevel, 1, 0, &PlacedSubresourceFootprint, &NumRows, &RowPitch, &RequiredSize);

    const uint64 Alignment   = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    const uint64 AlignedSize = FMath::AlignUp<uint64>(RequiredSize, Alignment);

    FD3D12UploadAllocation Allocation = GetDevice()->GetUploadAllocator().Allocate(AlignedSize, Alignment);
    CHECK(Allocation.Memory   != nullptr);
    CHECK(Allocation.Resource != nullptr);

    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    for (uint64 y = 0; y < NumRows; y++)
    {
        FMemory::Memcpy(Allocation.Memory, Source, SrcRowPitch);
        Allocation.Memory += PlacedSubresourceFootprint.Footprint.RowPitch;
        Source            += SrcRowPitch;
    }

    // Copy to Dest
    D3D12_TEXTURE_COPY_LOCATION SourceLocation;
    FMemory::Memzero(&SourceLocation);

    SourceLocation.pResource                          = Allocation.Resource.Get();
    SourceLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SourceLocation.PlacedFootprint.Offset             = Allocation.ResourceOffset;
    SourceLocation.PlacedFootprint.Footprint.Format   = Desc.Format;
    SourceLocation.PlacedFootprint.Footprint.Width    = TextureRegion.Width;
    SourceLocation.PlacedFootprint.Footprint.Height   = TextureRegion.Height;
    SourceLocation.PlacedFootprint.Footprint.Depth    = 1;
    SourceLocation.PlacedFootprint.Footprint.RowPitch = PlacedSubresourceFootprint.Footprint.RowPitch;

    // TODO: MipLevel may not be the correct subresource
    // TODO: Add offset
    D3D12_TEXTURE_COPY_LOCATION DestLocation;
    FMemory::Memzero(&DestLocation);

    DestLocation.pResource        = D3D12Resource->GetD3D12Resource();
    DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestLocation.SubresourceIndex = MipLevel;

    CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Dst);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Allocation.Resource.Get());
}

void FD3D12CommandContext::RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyInfo)
{
    D3D12_ERROR_COND(Dst != nullptr && Src != nullptr, "Dst or Src cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Buffer* D3D12Destination = GetD3D12Buffer(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Buffer* D3D12Source = GetD3D12Buffer(Src);
    CHECK(D3D12Source != nullptr);

    CommandList->CopyBufferRegion(D3D12Destination->GetD3D12Resource(), CopyInfo.DstOffset, D3D12Source->GetD3D12Resource(), CopyInfo.SrcOffset, CopyInfo.Size);

    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Dst);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Src);
}

void FD3D12CommandContext::RHICopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    D3D12_ERROR_COND(Dst != nullptr && Src != nullptr, "Dst or Src cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Texture* D3D12Destination = GetD3D12Texture(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Texture* D3D12Source = GetD3D12Texture(Src);
    CHECK(D3D12Source != nullptr);
    
    CommandList->CopyResource(D3D12Destination->GetD3D12Resource(), D3D12Source->GetD3D12Resource());

    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Dst);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Src);
}

void FD3D12CommandContext::RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& InCopyDesc)
{
    D3D12_ERROR_COND(Dst != nullptr && Src != nullptr, "Dst or Src cannot be nullptr");

    FD3D12Texture* D3D12Destination = GetD3D12Texture(Dst);
    CHECK(D3D12Destination != nullptr);
    
    FD3D12Texture* D3D12Source = GetD3D12Texture(Src);
    CHECK(D3D12Source != nullptr);

    FlushResourceBarriers();

    const ETextureDimension TextureDimension = Src->GetDimension();

    const uint32 NumArraySlices    = D3D12CalculateArraySlices(TextureDimension, InCopyDesc.NumArraySlices);
    const uint32 SrcArraySlice     = D3D12CalculateArraySlices(TextureDimension, InCopyDesc.SrcArraySlice);
    const uint32 DstArraySlice     = D3D12CalculateArraySlices(TextureDimension, InCopyDesc.DstArraySlice);
    const uint32 NumSrcArraySlices = D3D12CalculateArraySlices(TextureDimension, Src->GetNumArraySlices());
    const uint32 NumDstArraySlices = D3D12CalculateArraySlices(TextureDimension, Dst->GetNumArraySlices());

    for (int32 ArraySlice = 0; ArraySlice < NumArraySlices; ArraySlice++)
    {
        for (int32 MipLevel = 0; MipLevel < InCopyDesc.NumMipLevels; MipLevel++)
        {
            // Source
            D3D12_TEXTURE_COPY_LOCATION SourceLocation;
            FMemory::Memzero(&SourceLocation);

            SourceLocation.pResource        = D3D12Source->GetD3D12Resource()->GetD3D12Resource();
            SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            SourceLocation.SubresourceIndex = D3D12CalculateSubresource(InCopyDesc.SrcMipSlice + MipLevel, SrcArraySlice + ArraySlice, 0, Src->GetNumMipLevels(), NumSrcArraySlices);

            D3D12_BOX SourceBox;
            SourceBox.left   = InCopyDesc.SrcPosition.x >> MipLevel;
            SourceBox.right  = FMath::Max((InCopyDesc.SrcPosition.x + InCopyDesc.Size.x) >> MipLevel, 1);
            SourceBox.top    = InCopyDesc.SrcPosition.y >> MipLevel;
            SourceBox.bottom = FMath::Max((InCopyDesc.SrcPosition.y + InCopyDesc.Size.y) >> MipLevel, 1);
            SourceBox.front  = InCopyDesc.SrcPosition.z >> MipLevel;
            SourceBox.back   = FMath::Max((InCopyDesc.SrcPosition.z + InCopyDesc.Size.z) >> MipLevel, 1);

            // Destination
            D3D12_TEXTURE_COPY_LOCATION DestLocation;
            FMemory::Memzero(&DestLocation);

            DestLocation.pResource        = D3D12Destination->GetD3D12Resource()->GetD3D12Resource();
            DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            DestLocation.SubresourceIndex = D3D12CalculateSubresource(InCopyDesc.DstMipSlice + MipLevel, DstArraySlice + ArraySlice, 0, Dst->GetNumMipLevels(), NumDstArraySlices);

            const uint32 DestPositionX = InCopyDesc.DstPosition.x >> MipLevel;
            const uint32 DestPositionY = InCopyDesc.DstPosition.y >> MipLevel;
            const uint32 DestPositionZ = InCopyDesc.DstPosition.z >> MipLevel;

            CommandList->CopyTextureRegion(&DestLocation, DestPositionX, DestPositionY, DestPositionZ, &SourceLocation, &SourceBox);
        }
    }

    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Dst);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Src);
}

void FD3D12CommandContext::RHIDestroyResource(IRefCounted* Resource)
{
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, Resource);
}

void FD3D12CommandContext::RHIDiscardContents(FRHITexture* Texture)
{
    // TODO: Enable regions to be discarded

    if (FD3D12Resource* D3D12Resource = GetD3D12Resource(Texture))
    {
        CommandList->DiscardResource(D3D12Resource->GetD3D12Resource(), nullptr);
    }
}

void FD3D12CommandContext::RHIBuildRayTracingGeometry(
    FRHIRayTracingGeometry* RayTracingGeometry,
    FRHIBuffer*             VertexBuffer,
    uint32                  NumVertices,
    FRHIBuffer*             IndexBuffer,
    uint32                  NumIndices,
    EIndexFormat            IndexFormat,
    bool                    bUpdate)
{
    D3D12_ERROR_COND(RayTracingGeometry != nullptr, "RayTracingGeometry cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Buffer* D3D12VertexBuffer = static_cast<FD3D12Buffer*>(VertexBuffer);
    FD3D12Buffer* D3D12IndexBuffer  = static_cast<FD3D12Buffer*>(IndexBuffer);
    D3D12_ERROR_COND(D3D12VertexBuffer != nullptr, "VertexBuffer cannot be nullptr");

    FD3D12RayTracingGeometry* D3D12Geometry = static_cast<FD3D12RayTracingGeometry*>(RayTracingGeometry);
    D3D12Geometry->Build(*this, D3D12VertexBuffer, NumVertices, D3D12IndexBuffer, NumIndices, IndexFormat, bUpdate);

    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, VertexBuffer);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, IndexBuffer);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, RayTracingGeometry);
}

void FD3D12CommandContext::RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
    D3D12_ERROR_COND(RayTracingScene != nullptr, "RayTracingScene cannot be nullptr");

    FlushResourceBarriers();

    FD3D12RayTracingScene* D3D12Scene = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    D3D12Scene->Build(*this, Instances, bUpdate);
    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, RayTracingScene);
}

void FD3D12CommandContext::RHISetRayTracingBindings(
    FRHIRayTracingScene*              RayTracingScene,
    FRHIRayTracingPipelineState*      PipelineState,
    const FRayTracingShaderResources* GlobalResource,
    const FRayTracingShaderResources* RayGenLocalResources,
    const FRayTracingShaderResources* MissLocalResources,
    const FRayTracingShaderResources* HitGroupResources,
    uint32                            NumHitGroupResources)
{
#if 0
    FD3D12RayTracingScene* D3D12Scene = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    D3D12_ERROR_COND(D3D12Scene != nullptr, "RayTracingScene cannot be nullptr");
    FD3D12RayTracingPipelineState* D3D12PipelineState = static_cast<FD3D12RayTracingPipelineState*>(PipelineState);
    D3D12_ERROR_COND(D3D12PipelineState != nullptr, "PipelineState cannot be nullptr");

    uint32 NumDescriptorsNeeded = 0;
    uint32 NumSamplersNeeded    = 0;
    if (GlobalResource)
    {
        NumDescriptorsNeeded += GlobalResource->NumResources();
        NumSamplersNeeded    += GlobalResource->NumSamplers();
    }
    if (RayGenLocalResources)
    {
        NumDescriptorsNeeded += RayGenLocalResources->NumResources();
        NumSamplersNeeded    += RayGenLocalResources->NumSamplers();
    }
    if (MissLocalResources)
    {
        NumDescriptorsNeeded += MissLocalResources->NumResources();
        NumSamplersNeeded    += MissLocalResources->NumSamplers();
    }

    for (uint32 i = 0; i < NumHitGroupResources; i++)
    {
        NumDescriptorsNeeded += HitGroupResources[i].NumResources();
        NumSamplersNeeded    += HitGroupResources[i].NumSamplers();
    }

    D3D12_ERROR_COND(NumDescriptorsNeeded < D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, "NumDescriptorsNeeded=%u, but the maximum is '%u'", NumDescriptorsNeeded, D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT);

    FD3D12OnlineDescriptorManager* ResourceHeap = CmdBatch->GetResourceDescriptorManager();
    if (!ResourceHeap->HasSpace(NumDescriptorsNeeded))
    {
        CHECK(false);
        // TODO: Fix this
        // ResourceHeap->AllocateFreshHeap();
    }

    D3D12_ERROR_COND(NumSamplersNeeded < D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, "NumDescriptorsNeeded=%u, but the maximum is '%u'", NumSamplersNeeded, D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT);

    FD3D12OnlineDescriptorManager* SamplerHeap = CmdBatch->GetSamplerDescriptorManager();
    if (!SamplerHeap->HasSpace(NumSamplersNeeded))
    {
        CHECK(false);
        // TODO: Fix this
        // SamplerHeap->AllocateFreshHeap();
    }

    // TODO: Fix this
    // if (!D3D12Scene->BuildBindingTable(*this, D3D12PipelineState, ResourceHeap, SamplerHeap, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources))
    {
        D3D12_ERROR("[FD3D12CommandContext]: FAILED to Build Shader Binding Table");
    }

    if (GlobalResource)
    {
        if (!GlobalResource->ConstantBuffers.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ConstantBuffers.Size(); i++)
            {
                FD3D12ConstantBufferView* D3D12ConstantBufferView = static_cast<FD3D12Buffer*>(GlobalResource->ConstantBuffers[i])->GetConstantBufferView();
                ContextState.DescriptorCache.SetConstantBufferView(ShaderVisibility_All, D3D12ConstantBufferView, i);
            }
        }
        if (!GlobalResource->ShaderResourceViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ShaderResourceViews.Size(); i++)
            {
                FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(GlobalResource->ShaderResourceViews[i]);
                ContextState.DescriptorCache.SetShaderResourceView(ShaderVisibility_All, D3D12ShaderResourceView, i);
            }
        }
        if (!GlobalResource->UnorderedAccessViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->UnorderedAccessViews.Size(); i++)
            {
                FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(GlobalResource->UnorderedAccessViews[i]);
                ContextState.DescriptorCache.SetUnorderedAccessView(ShaderVisibility_All, D3D12UnorderedAccessView, i);
            }
        }
        if (!GlobalResource->SamplerStates.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->SamplerStates.Size(); i++)
            {
                FD3D12SamplerState* DxSampler = static_cast<FD3D12SamplerState*>(GlobalResource->SamplerStates[i]);
                ContextState.DescriptorCache.SetSamplerState(ShaderVisibility_All, DxSampler, i);
            }
        }
    }

    ID3D12GraphicsCommandList4* DXRCommandList = CommandList->GetGraphicsCommandList4();

    FD3D12RootSignature* GlobalRootSignature = D3D12PipelineState->GetGlobalRootSignature();
    DXRCommandList->SetComputeRootSignature(GlobalRootSignature->GetD3D12RootSignature());

    ContextState.DescriptorCache.PrepareComputeDescriptors(CmdBatch, GlobalRootSignature);
#endif
}

void FD3D12CommandContext::RHIGenerateMips(FRHITexture* Texture)
{
    FD3D12Texture* D3D12Texture = GetD3D12Texture(Texture);
    if (!D3D12Texture)
    {
        D3D12_ERROR("Texture cannot be nullptr");
        return;
    }

    D3D12_RESOURCE_DESC Desc = D3D12Texture->GetD3D12Resource()->GetDesc();
    Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    if (Desc.MipLevels < 2)
    {
        D3D12_ERROR("MipLevels must be more than one in order to generate any Mips");
        return;
    }

    // TODO: Create this placed from a Heap? See what performance is
    // Also if the resource already have the unorderedaccess-flag there is no need to create a staging resource
    FD3D12ResourceRef StagingTexture = new FD3D12Resource(GetDevice(), Desc, D3D12Texture->GetD3D12Resource()->GetHeapType());
    if (!StagingTexture->Initialize(D3D12_RESOURCE_STATE_COMMON, nullptr))
    {
        D3D12_ERROR("[FD3D12CommandContext] Failed to create StagingTexture for GenerateMips");
        return;
    }
    else
    {
        StagingTexture->SetName("GenerateMips StagingTexture");
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
    FMemory::Memzero(&SrvDesc);

    SrvDesc.Format                  = Desc.Format;
    SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    const bool bIsTextureCube = Texture->GetDesc().IsTextureCube();
    if (bIsTextureCube)
    {
        SrvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        SrvDesc.TextureCube.MipLevels           = Desc.MipLevels;
        SrvDesc.TextureCube.MostDetailedMip     = 0;
        SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        SrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        SrvDesc.Texture2D.MipLevels       = Desc.MipLevels;
        SrvDesc.Texture2D.MostDetailedMip = 0;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
    FMemory::Memzero(&UavDesc);

    UavDesc.Format = Desc.Format;
    if (bIsTextureCube)
    {
        UavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UavDesc.Texture2DArray.ArraySize       = 6;
        UavDesc.Texture2DArray.FirstArraySlice = 0;
        UavDesc.Texture2DArray.PlaneSlice      = 0;
    }
    else
    {
        UavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        UavDesc.Texture2D.PlaneSlice = 0;
    }

    const uint32 MipLevelsPerDispatch = 4;
    const uint32 HandleCountUAV       = FMath::AlignUp<uint32>(Desc.MipLevels, MipLevelsPerDispatch);
    const uint32 NumDispatches        = HandleCountUAV / MipLevelsPerDispatch;

    // Ensure there are enough allocators for a new descriptor
    FD3D12LocalDescriptorHeap& ResourceHeap = ContextState.GetDescriptorCache().GetResourceHeap();
    if (!ResourceHeap.HasSpace(HandleCountUAV + 1))
    {
        ResourceHeap.Realloc();
        CHECK(ResourceHeap.HasSpace(HandleCountUAV + 1));
    }

    // Allocate an extra handle for SRV
    const uint32 HandleIndex = ResourceHeap.AllocateHandles(HandleCountUAV + 1);

    const D3D12_CPU_DESCRIPTOR_HANDLE SRVHandleCPU = ResourceHeap.GetCPUHandle(HandleIndex);
    GetDevice()->GetD3D12Device()->CreateShaderResourceView(D3D12Texture->GetD3D12Resource()->GetD3D12Resource(), &SrvDesc, SRVHandleCPU);

    const uint32 HandleIndexUAV = HandleIndex + 1;
    for (uint32 i = 0; i < Desc.MipLevels; i++)
    {
        if (bIsTextureCube)
        {
            UavDesc.Texture2DArray.MipSlice = i;
        }
        else
        {
            UavDesc.Texture2D.MipSlice = i;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE UAVHandleCPU = ResourceHeap.GetCPUHandle(HandleIndexUAV + i);
        GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(StagingTexture->GetD3D12Resource(), nullptr, &UavDesc, UAVHandleCPU);
    }

    for (uint32 i = Desc.MipLevels; i < HandleCountUAV; i++)
    {
        if (bIsTextureCube)
        {
            UavDesc.Texture2DArray.MipSlice = 0;
        }
        else
        {
            UavDesc.Texture2D.MipSlice = 0;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE UAVHandleCPU = ResourceHeap.GetCPUHandle(HandleIndexUAV + i);
        GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(nullptr, nullptr, &UavDesc, UAVHandleCPU);
    }

    // We assume the destination is in D3D12_RESOURCE_STATE_COPY_DEST
    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    
    FlushResourceBarriers();

    CommandList->CopyResource(StagingTexture.Get(), D3D12Texture->GetD3D12Resource());

    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    
    FlushResourceBarriers();

    FD3D12ComputePipelineStateRef PipelineState = bIsTextureCube ? 
        FD3D12RHI::GetRHI()->GetGenerateMipsPipelineTexureCube() : 
        FD3D12RHI::GetRHI()->GetGenerateMipsPipelineTexure2D();

    CommandList->SetPipelineState(PipelineState->GetD3D12PipelineState());
    CommandList->SetComputeRootSignature(PipelineState->GetRootSignature());

    const D3D12_GPU_DESCRIPTOR_HANDLE SRVHandleGPU = ResourceHeap.GetGPUHandle(HandleIndex);
    ContextState.GetDescriptorCache().SetDescriptorHeaps();

    struct FConstantBuffer
    {
        uint32   SrcMipLevel;
        uint32   NumMipLevels;
        FVector2 TexelSize;
    } ConstantData;

    uint32 DstWidth  = static_cast<uint32>(Desc.Width);
    uint32 DstHeight = Desc.Height;
    ConstantData.SrcMipLevel = 0;

    const uint32 ThreadsZ = bIsTextureCube ? 6 : 1;

    uint32 RemainingMiplevels = Desc.MipLevels;
    for (uint32 i = 0; i < NumDispatches; i++)
    {
        ConstantData.TexelSize    = FVector2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));
        ConstantData.NumMipLevels = FMath::Min<uint32>(4, RemainingMiplevels);

        CommandList->SetComputeRoot32BitConstants(&ConstantData, 4, 0, 0);
        CommandList->SetComputeRootDescriptorTable(SRVHandleGPU, 1);

        const uint32 GPUDescriptorHandleIndex = i * MipLevelsPerDispatch;

        const D3D12_GPU_DESCRIPTOR_HANDLE UAVHandleGPU = ResourceHeap.GetGPUHandle(HandleIndexUAV + GPUDescriptorHandleIndex);
        CommandList->SetComputeRootDescriptorTable(UAVHandleGPU, 2);

        constexpr uint32 ThreadCount = 8;

        const uint32 ThreadsX = FMath::DivideByMultiple(DstWidth, ThreadCount);
        const uint32 ThreadsY = FMath::DivideByMultiple(DstHeight, ThreadCount);
        CommandList->Dispatch(ThreadsX, ThreadsY, ThreadsZ);

        UnorderedAccessBarrier(StagingTexture.Get());

        TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        // TODO: Copy only MipLevels (Maybe faster?)
        CommandList->CopyResource(D3D12Texture->GetD3D12Resource(), StagingTexture.Get());

        TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        FlushResourceBarriers();

        DstWidth  = DstWidth / 16;
        DstHeight = DstHeight / 16;

        ConstantData.SrcMipLevel += 3;
        RemainingMiplevels       -= MipLevelsPerDispatch;
    }

    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

    FlushResourceBarriers();

    GetDevice()->GetDeferredDeletionQueue().DeferDeletion(AssignedFenceValue, StagingTexture.Get());
}

void FD3D12CommandContext::RHITransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    FD3D12Texture* D3D12Texture = GetD3D12Texture(Texture);
    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12BeforeState, D3D12AfterState);
}

void FD3D12CommandContext::RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Buffer);
    CHECK(D3D12Buffer != nullptr);

    TransitionResource(D3D12Buffer->GetD3D12Resource(), D3D12BeforeState, D3D12AfterState);
}

void FD3D12CommandContext::RHIUnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    FD3D12Texture* D3D12Texture = GetD3D12Texture(Texture);
    CHECK(D3D12Texture != nullptr);
    UnorderedAccessBarrier(D3D12Texture->GetD3D12Resource());
}

void FD3D12CommandContext::RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Buffer);
    CHECK(D3D12Buffer != nullptr);
    UnorderedAccessBarrier(D3D12Buffer->GetD3D12Resource());
}

void FD3D12CommandContext::RHIDraw(uint32 VertexCount, uint32 StartVertexLocation)
{
    FlushResourceBarriers();

    ContextState.BindGraphicsStates();
    CommandList->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void FD3D12CommandContext::RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    FlushResourceBarriers();

    ContextState.BindGraphicsStates();
    CommandList->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FD3D12CommandContext::RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    ContextState.BindGraphicsStates();
    CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    ContextState.BindGraphicsStates();
    CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::RHIDispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    FlushResourceBarriers();

    ContextState.BindComputeState();
    CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FD3D12CommandContext::RHIDispatchRays(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    FD3D12RayTracingScene* D3D12Scene = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    D3D12_ERROR_COND(D3D12Scene != nullptr, "RayTracingScene cannot be nullptr");

    FD3D12RayTracingPipelineState* D3D12PipelineState = static_cast<FD3D12RayTracingPipelineState*>(PipelineState);
    D3D12_ERROR_COND(D3D12PipelineState != nullptr, "PipelineState cannot be nullptr");

    ID3D12GraphicsCommandList4* DXRCommandList = CommandList->GetGraphicsCommandList4();
    D3D12_ERROR_COND(DXRCommandList != nullptr, "DXRCommandList is nullptr, DXR is not supported");

    FlushResourceBarriers();

    D3D12_DISPATCH_RAYS_DESC RayDispatchDesc;
    FMemory::Memzero(&RayDispatchDesc);

    RayDispatchDesc.RayGenerationShaderRecord = D3D12Scene->GetRayGenShaderRecord();
    RayDispatchDesc.MissShaderTable           = D3D12Scene->GetMissShaderTable();
    RayDispatchDesc.HitGroupTable             = D3D12Scene->GetHitGroupTable();

    RayDispatchDesc.Width  = Width;
    RayDispatchDesc.Height = Height;
    RayDispatchDesc.Depth  = Depth;

    DXRCommandList->SetPipelineState1(D3D12PipelineState->GetD3D12StateObject());
    DXRCommandList->DispatchRays(&RayDispatchDesc);
}

void FD3D12CommandContext::RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    // Ensure that commands are submitted
    FinishCommandList();

    FD3D12Viewport* D3D12Viewport = static_cast<FD3D12Viewport*>(Viewport);
    D3D12Viewport->Present(bVerticalSync);

    // Start recording again
    ObtainCommandList();
}

void FD3D12CommandContext::RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height)
{
    FD3D12Viewport* D3D12Viewport = static_cast<FD3D12Viewport*>(Viewport);
    D3D12Viewport->Resize(Width, Height);
}

void FD3D12CommandContext::RHIClearState()
{
    RHIFlush();
    ContextState.ResetState();
}

void FD3D12CommandContext::RHIFlush()
{
    SCOPED_LOCK(CommandContextCS);

    if (CommandList)
    {
        FinishCommandList();
    }

    FD3D12CommandListManager* CommandListManager = GetDevice()->GetCommandListManager(QueueType);
    CHECK(CommandListManager != nullptr);

    CommandListManager->GetFenceManager().SignalGPU(QueueType);
    CommandListManager->GetFenceManager().WaitForFence();
}

void FD3D12CommandContext::RHIInsertMarker(const FStringView& Message)
{
    if (FDynamicD3D12::SetMarkerOnCommandList)
    {
        FDynamicD3D12::SetMarkerOnCommandList(CommandList->GetGraphicsCommandList(), PIX_COLOR(255, 255, 255), Message.GetCString());
    }
}

void FD3D12CommandContext::RHIBeginExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetAdapter()->GetGraphicsAnalysis();
    if (GraphicsAnalysis && !bIsCapturing)
    {
        GraphicsAnalysis->BeginCapture();
        bIsCapturing = true;
    }
}

void FD3D12CommandContext::RHIEndExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetAdapter()->GetGraphicsAnalysis();
    if (GraphicsAnalysis && bIsCapturing)
    {
        GraphicsAnalysis->EndCapture();
        bIsCapturing = false;
    }
}
