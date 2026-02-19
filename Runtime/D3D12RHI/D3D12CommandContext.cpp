#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Vector2.h"
#include "Core/Misc/FrameProfiler.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12RayTracing.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12Loader.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include <pix.h>

static TAutoConsoleVariable<int32> CVarMaxDrawCallsPerCommandList(
    "D3D12RHI.MaxDrawCallsPerCommandList",
    "Number of draw-calls allowed before submitting the current CommandList to the GPU",
    10000);

static constexpr const bool GD3D12DebugResourceBarriers = false;

FResourceBarrierBatcher::FResourceBarrierBatcher(FD3D12CommandContext& InContext)
    : Context(InContext)
    , Barriers()
{
}

FResourceBarrierBatcher::~FResourceBarrierBatcher()
{
}

void FResourceBarrierBatcher::AddTransitionBarrier(FD3D12Resource* InResource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex)
{
    CHECK(InResource != nullptr);

    if constexpr (GD3D12DebugResourceBarriers)
    {
        const FString DebugName = InResource->GetDebugName();
        D3D12_INFO("AddTransitionBarrier Resource=%s Subresource=%u Before=%s After=%s", *DebugName, SubresourceIndex, ToString(BeforeState), ToString(AfterState));
    }

    AddTransitionBarrier(InResource->GetD3D12Resource(), BeforeState, AfterState, SubresourceIndex);
}

void FResourceBarrierBatcher::AddUnorderedAccessBarrier(FD3D12Resource* InResource)
{
    CHECK(InResource != nullptr);

    if constexpr (GD3D12DebugResourceBarriers)
    {
        const FString DebugName = InResource->GetDebugName();
        D3D12_INFO("AddUnorderedAccessBarrier Resource=%s", *DebugName);
    }

    AddUnorderedAccessBarrier(InResource->GetD3D12Resource());
}

void FResourceBarrierBatcher::AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex)
{
    CHECK(Resource != nullptr);

    if (BeforeState == AfterState)
    {
        // No-op transition
        return;
    }

    // Try to coalesce with an existing transition for the same (sub)resource.
    for (TArray<D3D12_RESOURCE_BARRIER>::IteratorType It = Barriers.Iterator(); !It.IsEnd(); ++It)
    {
        if (It->Type != D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
        {
            continue;
        }

        const D3D12_RESOURCE_BARRIER& Existing = *It;
        if (Existing.Transition.pResource != Resource)
        {
            continue;
        }

        // We only coalesce when subresources match exactly (or both are ALL_SUBRESOURCES).
        const bool bSameSubresource     = (Existing.Transition.Subresource == SubresourceIndex);
        const bool bBothAllSubresources = (Existing.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) && (SubresourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

        if (!(bSameSubresource || bBothAllSubresources))
        {
            continue;
        }

        // Case 1: Redundant barrier (A->B then A->B again) => ignore new barrier
        D3D12_RESOURCE_TRANSITION_BARRIER& ExistingTransitionBarrier = It->Transition;
        if (ExistingTransitionBarrier.StateBefore == BeforeState && ExistingTransitionBarrier.StateAfter == AfterState)
        {
            if constexpr (GD3D12DebugResourceBarriers)
            {
                LOG_INFO("  Redundant barrier A->B kept (SubresourceIndex=%u, %s->%s)", SubresourceIndex, ToString(BeforeState), ToString(AfterState));
            }

            return;
        }

        // Case 2: Extend barrier (A->B then B->C) => A->C
        if (ExistingTransitionBarrier.StateAfter == BeforeState)
        {
            ExistingTransitionBarrier.StateAfter = AfterState;
            if constexpr (GD3D12DebugResourceBarriers)
            {
                LOG_INFO("  Extended barrier to %s->%s (SubresourceIndex=%u)", ToString(ExistingTransitionBarrier.StateBefore), ToString(ExistingTransitionBarrier.StateAfter), SubresourceIndex);
            }

            // If we changed state to the same before- and after-state, remove it
            if (ExistingTransitionBarrier.StateBefore == ExistingTransitionBarrier.StateAfter)
            {
                if constexpr (GD3D12DebugResourceBarriers)
                {
                    LOG_INFO("  Cancelled barrier (SubresourceIndex=%u, %s<->%s)", SubresourceIndex, ToString(ExistingTransitionBarrier.StateBefore), ToString(ExistingTransitionBarrier.StateAfter));
                }

                Barriers.RemoveAt(It.GetIndex());
            }

            return;
        }

        // Case 3: Cancel barrier (A->B then B->A) => remove
        if (ExistingTransitionBarrier.StateBefore == AfterState)
        {
            if constexpr (GD3D12DebugResourceBarriers)
            {
                LOG_INFO("  Cancelled barrier (SubresourceIndex=%u, %s<->%s)", SubresourceIndex, ToString(BeforeState), ToString(AfterState));
            }

            Barriers.RemoveAt(It.GetIndex());
            return;
        }
    }

    // Otherwise: Different, non-chainable states -> cannot coalesce. Add a new barrier.
    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition.pResource   = Resource;
    Barrier.Transition.StateBefore = BeforeState;
    Barrier.Transition.StateAfter  = AfterState;
    Barrier.Transition.Subresource = SubresourceIndex;
    Barriers.Emplace(Barrier);
}

void FResourceBarrierBatcher::AddUnorderedAccessBarrier(ID3D12Resource* Resource)
{
    CHECK(Resource != nullptr);

    for (TArray<D3D12_RESOURCE_BARRIER>::IteratorType It = Barriers.Iterator(); !It.IsEnd(); ++It)
    {
        if (It->Type == D3D12_RESOURCE_BARRIER_TYPE_UAV && It->UAV.pResource == Resource)
        {
            // Barrier is already present, nothing to add.
            return;
        }
    }

    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.Flags         = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.UAV.pResource = Resource;
    Barriers.Emplace(Barrier);
}

void FResourceBarrierBatcher::FlushBarriers()
{
    if (!HasPendingBarriers())
    {
        return;
    }

    const uint32 NumBarriers = Barriers.Size();
    Context.GetCommandList()->ResourceBarrier(NumBarriers, Barriers.Data());

    if constexpr (GD3D12DebugResourceBarriers)
    {
        D3D12_INFO("FlushBarriers NumBarriers=%u", NumBarriers);
    }

    Barriers.Clear();
}

FD3D12CommandContext::FD3D12CommandContext(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : IRHICommandContext()
    , FD3D12DeviceChild(InDevice)
    , CommandList(nullptr)
    , CommandAllocator(nullptr)
    , CommandSubmission(nullptr)
    , ContextState(InDevice, *this)
    , TimingQueryAllocator(InDevice, *this, EQueryType::Timestamp)
    , OcclusionQueryAllocator(InDevice, *this, EQueryType::Occlusion)
    , ResourceBarrierBatcher(*this)
    , QueueType(InQueueType)
    , NumDrawCalls(0)
    , bIsCapturing(false)
    , bIsRecording(false)
    , CommandContextCS()
{
}

FD3D12CommandContext::~FD3D12CommandContext()
{
}

bool FD3D12CommandContext::Initialize()
{
    if (!ContextState.Initialize())
    {
        D3D12_ERROR_CRITICAL("Failed to initialize ContextState");
        return false;
    }

    return true;
}

void FD3D12CommandContext::ObtainCommandList()
{
    TRACE_FUNCTION_SCOPE();

    FD3D12CommandAllocatorManager* CommandAllocatorManager = GetDevice()->GetCommandAllocatorManager(QueueType);
    CHECK(CommandAllocatorManager != nullptr);

    if (!CommandAllocator)
    {
        CommandAllocator = CommandAllocatorManager->ObtainAllocator();
        if (!CommandAllocator)
        {
            D3D12_ERROR_CRITICAL("Failed to Obtain CommandAllocator");
        }
    }

    FD3D12Queue* Queue = GetDevice()->GetQueue(QueueType);
    CHECK(Queue != nullptr);

    if (!CommandList)
    {
        CommandList = Queue->ObtainCommandList(CommandAllocator, nullptr);
        if (!CommandList)
        {
            D3D12_ERROR_CRITICAL("Failed to initialize CommandList");
        }
    }

    if (!CommandSubmission)
    {
        CommandSubmission = new FD3D12CommandSubmission(GetDevice(), Queue);
    }
}

void FD3D12CommandContext::FinishCommandList(bool bFlushAllocator)
{
    TRACE_FUNCTION_SCOPE();

    ResourceBarrierBatcher.FlushBarriers();

    // Flush Commands
    const uint32 NumCommands = CommandList->GetNumCommands();
    if (NumCommands > 0)
    {
        // NOTE: This is fine since using a query requires a command to be issues
        TimingQueryAllocator.PrepareForNewCommandList();
        OcclusionQueryAllocator.PrepareForNewCommandList();

        // Ensure that all QueryHeaps are resolved
        for (FD3D12QueryHeap* QueryHeap : CommandSubmission->QueryHeaps)
        {
            QueryHeap->ResolveQueries(GetCommandList());
        }

        if (!CommandList->Close())
        {
            D3D12_ERROR_CRITICAL("Failed to close CommandList");
            return;
        }

        CHECK(CommandSubmission != nullptr);
        CommandSubmission->AddCommandList(CommandList);
        CommandList = nullptr;

        if (bFlushAllocator)
        {
            CommandSubmission->AddCommandAllocator(CommandAllocator);
            CommandAllocator = nullptr;
        }

        FD3D12RHI::Get()->SubmitCommands(CommandSubmission, true);
        CommandSubmission = nullptr;

        // Reset the number of draw-calls for the current command-list
        NumDrawCalls = 0;
    }

    // Ensure that the state will rebind the necessary state when we obtain a new CommandList
    ContextState.ResetStateForNewCommandList();
}

void FD3D12CommandContext::SplitCommandList(bool bFlushAllocator, bool bWaitForQueue)
{
    FinishCommandList(bFlushAllocator);

    if (bWaitForQueue)
    {
        FD3D12Queue* Queue = GetDevice()->GetQueue(QueueType);
        Queue->GetFenceManager().WaitForFence();
    }
    
    ObtainCommandList();
}

void FD3D12CommandContext::SplitCommandListAndResetState(bool bFlushAllocator, bool bWaitForQueue)
{
    FinishCommandList(bFlushAllocator);

    if (bWaitForQueue)
    {
        FD3D12Queue* Queue = GetDevice()->GetQueue(QueueType);
        Queue->GetFenceManager().WaitForFence();
    }

    ContextState.ResetState();

    ObtainCommandList();
}

void FD3D12CommandContext::StartContext()
{
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Lock to the thread that started the context
    CommandContextCS.Lock();

    // Reset the state
    ContextState.ResetState();

    // Process submitted commands
    FD3D12RHI::Get()->ProcessPendingCommandSubmissions();

    // Retrieve a new CommandList
    ObtainCommandList();

    // Starting recording commands
    bIsRecording = true;
}

void FD3D12CommandContext::FinishContext()
{
    // Submit the CommandList
    FinishCommandList(true);

    // Stopped recording commands
    bIsRecording = false;

    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Unlock from the thread that started the context
    CommandContextCS.Unlock();
}

void FD3D12CommandContext::UpdateBuffer(FD3D12Resource* Resource, const FBufferRegion& BufferRegion, const void* SrcData)
{
    CHECK(Resource != nullptr);
    CHECK(SrcData != nullptr);

    if (!BufferRegion.Size)
    {
        D3D12_WARNING("Trying to update buffer with zero size");
        return;
    }

    ResourceBarrierBatcher.FlushBarriers();

    D3D12_HEAP_TYPE HeapType = Resource->GetHeapType();
    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        uint8* BufferData = reinterpret_cast<uint8*>(Resource->MapRange(0, nullptr));
        if (!BufferData)
        {
            D3D12_ERROR("Failed to map buffer data");
            return;
        }

        FMemory::Memcpy(BufferData + BufferRegion.Offset, SrcData, BufferRegion.Size);

        const D3D12_RANGE WrittenRange = { BufferRegion.Offset, BufferRegion.Offset + BufferRegion.Size };
        Resource->UnmapRange(0, &WrittenRange);
    }
    else
    {
        FD3D12UploadAllocation Allocation = GetDevice()->GetUploadAllocator().Allocate(BufferRegion.Size, 1);
        if (!Allocation.Resource || !Allocation.Memory)
        {
            D3D12_ERROR_CRITICAL("Upload allocation failed");
            return;
        }

        FMemory::Memcpy(Allocation.Memory, SrcData, BufferRegion.Size);

        GetCommandList()->CopyBufferRegion(Resource->GetD3D12Resource(), BufferRegion.Offset, Allocation.Resource.Get(), Allocation.ResourceOffset, BufferRegion.Size);

        FD3D12RHI::Get()->DeferDeletion(Allocation.Resource.Get());
    }
}

void FD3D12CommandContext::BeginQuery(FRHIQuery* Query) 
{
    FD3D12QueryRHI* D3D12Query = FD3D12RHI::ResourceCast(Query);
    CHECK(D3D12Query != nullptr);

    FD3D12QueryAllocation QueryAllocation = OcclusionQueryAllocator.Allocate(&D3D12Query->Result);
    if (!QueryAllocation.IsValid())
    {
        D3D12_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    CHECK(QueryAllocation.QueryHeap != nullptr);
    GetCommandList()->BeginQuery(QueryAllocation.QueryHeap->GetD3D12QueryHeap(), D3D12_QUERY_TYPE_OCCLUSION, QueryAllocation.IndexInQueryHeap);
    D3D12Query->QueryAllocation = QueryAllocation;
}

void FD3D12CommandContext::EndQuery(FRHIQuery* Query) 
{
    FD3D12QueryRHI* D3D12Query = FD3D12RHI::ResourceCast(Query);
    CHECK(D3D12Query != nullptr);

    FD3D12QueryAllocation QueryAllocation = D3D12Query->QueryAllocation;
    if (!QueryAllocation.IsValid())
    {
        D3D12_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    CHECK(QueryAllocation.QueryHeap != nullptr);
    GetCommandList()->EndQuery(QueryAllocation.QueryHeap->GetD3D12QueryHeap(), D3D12_QUERY_TYPE_OCCLUSION, QueryAllocation.IndexInQueryHeap);
}

void FD3D12CommandContext::QueryTimestamp(FRHIQuery* Query)
{
    FD3D12QueryRHI* D3D12Query = FD3D12RHI::ResourceCast(Query);
    CHECK(D3D12Query != nullptr);

    FD3D12QueryAllocation QueryAllocation = TimingQueryAllocator.Allocate(&D3D12Query->Result);
    if (!QueryAllocation.IsValid())
    {
        D3D12_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    CHECK(QueryAllocation.QueryHeap != nullptr);
    GetCommandList()->EndQuery(QueryAllocation.QueryHeap->GetD3D12QueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, QueryAllocation.IndexInQueryHeap);
    D3D12Query->QueryAllocation = QueryAllocation;
}

void FD3D12CommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    ResourceBarrierBatcher.FlushBarriers();

    FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(RenderTargetView.Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12RenderTargetView* D3D12RenderTargetView = D3D12Texture->GetOrCreateRenderTargetView(RenderTargetView);
    CHECK(D3D12RenderTargetView != nullptr);
    GetCommandList()->ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), ClearColor.XYZW, 0, nullptr);
}

void FD3D12CommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    ResourceBarrierBatcher.FlushBarriers();

    FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(DepthStencilView.Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12DepthStencilView* D3D12DepthStencilView = D3D12Texture->GetOrCreateDepthStencilView(DepthStencilView);
    CHECK(D3D12DepthStencilView != nullptr);
    GetCommandList()->ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Depth, Stencil, 0, nullptr);
}

void FD3D12CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12RHI::ResourceCast(UnorderedAccessView);
    CHECK(D3D12UnorderedAccessView != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

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

    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandleCPU = ResourceHeap.GetCPUHandle(HandeOffset);
    GetDevice()->GetD3D12Device()->CopyDescriptorsSimple(1, OnlineHandleCPU, D3D12UnorderedAccessView->GetOfflineHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandleGPU = ResourceHeap.GetGPUHandle(HandeOffset);
    GetCommandList()->ClearUnorderedAccessViewFloat(
        OnlineHandleGPU, 
        D3D12UnorderedAccessView->GetOfflineHandle(), 
        D3D12UnorderedAccessView->GetViewResource()->GetD3D12Resource(),
        ClearColor.XYZW,
        0,
        nullptr);
}

void FD3D12CommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    ResourceBarrierBatcher.FlushBarriers();

    FD3D12RenderTargetView* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    FD3D12DepthStencilView* DepthStencilView = nullptr;

    for (uint32 Index = 0; Index < BeginRenderPassDesc.NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& CurrentRTV = BeginRenderPassDesc.RenderTargets[Index];
        if (FD3D12TextureRHI* RenderTarget = FD3D12RHI::ResourceCast(CurrentRTV.Texture))
        {
            FD3D12RenderTargetView* CurrentRenderTargetView = RenderTarget->GetOrCreateRenderTargetView(CurrentRTV);
            CHECK(CurrentRenderTargetView != nullptr);

            // Clear the RenderTarget here, since we expect it to be cleared when the RenderPass begin, however
            // it is not certain that there will be a call to draw inside of the RenderPass
            if (CurrentRTV.LoadAction == EAttachmentLoadAction::Clear)
            {
                GetCommandList()->ClearRenderTargetView(CurrentRenderTargetView->GetOfflineHandle(), &CurrentRTV.ClearValue.R, 0, nullptr);
            }
            
            RenderTargetViews[Index] = CurrentRenderTargetView;
        }
        else
        {
            RenderTargetViews[Index] = nullptr;
        }
    }

    const FRHIDepthStencilView& CurrentDSV = BeginRenderPassDesc.DepthStencilView;
    if (FD3D12TextureRHI* DepthStencil = FD3D12RHI::ResourceCast(CurrentDSV.Texture))
    {
        FD3D12DepthStencilView* CurrentDepthStencilView = DepthStencil->GetOrCreateDepthStencilView(CurrentDSV);
        CHECK(CurrentDepthStencilView != nullptr);

        // Clear the DepthStencil here, since we expect it to be cleared when the RenderPass begin, however
        // it is not certain that there will be a call to draw inside of the RenderPass
        if (CurrentDSV.LoadAction == EAttachmentLoadAction::Clear)
        {
            const D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
            GetCommandList()->ClearDepthStencilView(CurrentDepthStencilView->GetOfflineHandle(), ClearFlags, CurrentDSV.ClearValue.Depth, static_cast<uint8>(CurrentDSV.ClearValue.Stencil), 0, nullptr);
        }

        DepthStencilView = CurrentDepthStencilView;
    }
    else
    {
        DepthStencilView = nullptr;
    }

    ContextState.SetRenderTargets(RenderTargetViews, BeginRenderPassDesc.NumRenderTargets, DepthStencilView);

    // ShadingRate
    FD3D12TextureRHI* ShadingRateImage = FD3D12RHI::ResourceCast(BeginRenderPassDesc.ShadingRateTexture);
    ContextState.SetShadingRateImage(ShadingRateImage);
    ContextState.SetShadingRate(BeginRenderPassDesc.StaticShadingRate);
}

void FD3D12CommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    D3D12_VIEWPORT Viewport = {};
    Viewport.Width    = ViewportRegion.Width;
    Viewport.Height   = ViewportRegion.Height;
    Viewport.TopLeftX = ViewportRegion.PositionX;
    Viewport.TopLeftY = ViewportRegion.PositionY;
    Viewport.MaxDepth = ViewportRegion.MaxDepth;
    Viewport.MinDepth = ViewportRegion.MinDepth;

    ContextState.SetViewports(&Viewport, 1);
}

void FD3D12CommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    D3D12_RECT ScissorRect = {};
    ScissorRect.left   = LONG(ScissorRegion.PositionX);
    ScissorRect.right  = LONG(ScissorRegion.PositionX) + LONG(ScissorRegion.Width);
    ScissorRect.top    = LONG(ScissorRegion.PositionY);
    ScissorRect.bottom = LONG(ScissorRegion.PositionY) + LONG(ScissorRegion.Height);

    ContextState.SetScissorRects(&ScissorRect, 1);
}

void FD3D12CommandContext::SetBlendFactor(const FVector4& Color)
{
    ContextState.SetBlendFactor(Color.XYZW);
}

void FD3D12CommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FD3D12BufferRHI* D3DVertexBuffer = FD3D12RHI::ResourceCast(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(D3DVertexBuffer, BufferSlot + Index);
    }
}

void FD3D12CommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FD3D12BufferRHI* D3DIndexBuffer = FD3D12RHI::ResourceCast(IndexBuffer);
    ContextState.SetIndexBuffer(D3DIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FD3D12CommandContext::SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FD3D12GraphicsPipelineStateRHI* GraphicsPipelineState = FD3D12RHI::ResourceCast(PipelineState);
    ContextState.SetGraphicsPipelineState(GraphicsPipelineState);
}

void FD3D12CommandContext::SetComputePipelineState(class FRHIComputePipelineState* PipelineState)
{
    FD3D12ComputePipelineStateRHI* ComputePipelineState = FD3D12RHI::ResourceCast(PipelineState);
    ContextState.SetComputePipelineState(ComputePipelineState);
}

void FD3D12CommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    ContextState.SetShaderConstants(reinterpret_cast<const uint32*>(ShaderConstants), NumShaderConstants);
}

void FD3D12CommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex < D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
    FD3D12ShaderResourceViewRHI* D3D12ShaderResourceView = FD3D12RHI::ResourceCast(ShaderResourceView);
    ContextState.SetSRV(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex + InShaderResourceViews.Size() <= D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FD3D12ShaderResourceViewRHI* D3D12ShaderResourceView = FD3D12RHI::ResourceCast(InShaderResourceViews[Index]);
        ContextState.SetSRV(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex < D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
    FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12RHI::ResourceCast(UnorderedAccessView);
    ContextState.SetUAV(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex + InUnorderedAccessViews.Size() <= D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12RHI::ResourceCast(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex < D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);
    FD3D12ConstantBufferView* D3D12ConstantBufferView = nullptr;
    if (ConstantBuffer)
    {
        D3D12ConstantBufferView = FD3D12RHI::ResourceCast(ConstantBuffer)->GetConstantBufferView();
        CHECK(D3D12ConstantBufferView != nullptr);
    }

    ContextState.SetCBV(D3D12ConstantBufferView, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex + InConstantBuffers.Size() <= D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);
    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FD3D12ConstantBufferView* D3D12ConstantBufferView = nullptr;
        if (InConstantBuffers[Index])
        {
            D3D12ConstantBufferView = FD3D12RHI::ResourceCast(InConstantBuffers[Index])->GetConstantBufferView();
            CHECK(D3D12ConstantBufferView != nullptr);
        }

        ContextState.SetCBV(D3D12ConstantBufferView, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex < D3D12_DEFAULT_SAMPLER_STATE_COUNT);
    FD3D12SamplerStateRHI* D3D12SamplerState = FD3D12RHI::ResourceCast(SamplerState);
    ContextState.SetSampler(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), ParameterIndex);
}

void FD3D12CommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(ParameterIndex + InSamplerStates.Size() <= D3D12_DEFAULT_SAMPLER_STATE_COUNT);
    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FD3D12SamplerStateRHI* D3D12SamplerState = FD3D12RHI::ResourceCast(InSamplerStates[Index]);
        ContextState.SetSampler(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FD3D12CommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    FD3D12TextureRHI* D3D12Destination = FD3D12RHI::ResourceCast(Dst);
    FD3D12TextureRHI* D3D12Source      = FD3D12RHI::ResourceCast(Src);
    const DXGI_FORMAT DstFormat = D3D12Destination->GetDXGIFormat();
    const DXGI_FORMAT SrcFormat = D3D12Source->GetDXGIFormat();

    //TODO: For now texture must be the same format. I.e typeless does probably not work
    if (DstFormat != SrcFormat)
    {
        D3D12_ERROR("Dst or Src must have the same formats");
        return;
    }

    GetCommandList()->ResolveSubresource(D3D12Destination->GetResource()->GetD3D12Resource(), 0, D3D12Source->GetResource()->GetD3D12Resource(), 0, DstFormat);
}

void FD3D12CommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (BufferRegion.Size)
    {
        FD3D12BufferRHI* D3D12Destination = FD3D12RHI::ResourceCast(Dst);
        CHECK(D3D12Destination != nullptr);

        UpdateBuffer(D3D12Destination->GetResource(), BufferRegion, SrcData);
    }
}

void FD3D12CommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    CHECK(SrcData != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    FD3D12TextureRHI* D3D12Destination = FD3D12RHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Resource* D3D12Resource = D3D12Destination->GetResource();
    CHECK(D3D12Resource != nullptr);

    D3D12_RESOURCE_DESC Desc = D3D12Resource->GetDesc();

    UINT64 RequiredSize = 0;
    UINT64 RowPitch     = 0;
    UINT32 NumRows      = 0;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubresourceFootprint;
    GetDevice()->GetD3D12Device()->GetCopyableFootprints(&Desc, MipLevel, 1, 0, &PlacedSubresourceFootprint, &NumRows, &RowPitch, &RequiredSize);

    const uint64 Alignment   = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    const uint64 AlignedSize = Math::AlignUp<uint64>(RequiredSize, Alignment);

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
    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
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

    GetCommandList()->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

    FD3D12RHI::Get()->DeferDeletion(Allocation.Resource.Get());
}

void FD3D12CommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    FD3D12BufferRHI* D3D12Destination = FD3D12RHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12BufferRHI* D3D12Source = FD3D12RHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList()->CopyBufferRegion(D3D12Destination->GetResource()->GetD3D12Resource(), CopyDesc.DstOffset, D3D12Source->GetResource()->GetD3D12Resource(), CopyDesc.SrcOffset, CopyDesc.Size);
}

void FD3D12CommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    FD3D12TextureRHI* D3D12Destination = FD3D12RHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12TextureRHI* D3D12Source = FD3D12RHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);
    
    GetCommandList()->CopyResource(D3D12Destination->GetResource()->GetD3D12Resource(), D3D12Source->GetResource()->GetD3D12Resource());
}

void FD3D12CommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& InCopyDesc)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D12TextureRHI* D3D12Destination = FD3D12RHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);
    
    FD3D12TextureRHI* D3D12Source = FD3D12RHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    const ETextureDimension TextureDimension = Src->GetDimension();

    const uint32 NumArraySlices    = D3D12CalculateArraySlices(TextureDimension, InCopyDesc.NumArraySlices);
    const uint32 SrcArraySlice     = D3D12CalculateArraySlices(TextureDimension, InCopyDesc.SrcArraySlice);
    const uint32 DstArraySlice     = D3D12CalculateArraySlices(TextureDimension, InCopyDesc.DstArraySlice);
    const uint32 NumSrcArraySlices = D3D12CalculateArraySlices(TextureDimension, Src->GetNumArraySlices());
    const uint32 NumDstArraySlices = D3D12CalculateArraySlices(TextureDimension, Dst->GetNumArraySlices());

    for (uint32 ArraySlice = 0; ArraySlice < NumArraySlices; ArraySlice++)
    {
        for (uint32 MipLevel = 0; MipLevel < InCopyDesc.NumMipLevels; MipLevel++)
        {
            // Source
            D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
            SourceLocation.pResource        = D3D12Source->GetResource()->GetD3D12Resource();
            SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            SourceLocation.SubresourceIndex = D3D12CalculateSubresource(InCopyDesc.SrcMipSlice + MipLevel, SrcArraySlice + ArraySlice, 0, Src->GetNumMipLevels(), NumSrcArraySlices);

            D3D12_BOX SourceBox;
            SourceBox.left   = InCopyDesc.SrcPosition.X >> MipLevel;
            SourceBox.right  = Math::Max((InCopyDesc.SrcPosition.X + InCopyDesc.Size.X) >> MipLevel, 1);
            SourceBox.top    = InCopyDesc.SrcPosition.Y >> MipLevel;
            SourceBox.bottom = Math::Max((InCopyDesc.SrcPosition.Y + InCopyDesc.Size.Y) >> MipLevel, 1);
            SourceBox.front  = InCopyDesc.SrcPosition.Z >> MipLevel;
            SourceBox.back   = Math::Max((InCopyDesc.SrcPosition.Z + InCopyDesc.Size.Z) >> MipLevel, 1);

            // Destination
            D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
            DestLocation.pResource        = D3D12Destination->GetResource()->GetD3D12Resource();
            DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            DestLocation.SubresourceIndex = D3D12CalculateSubresource(InCopyDesc.DstMipSlice + MipLevel, DstArraySlice + ArraySlice, 0, Dst->GetNumMipLevels(), NumDstArraySlices);

            const uint32 DestPositionX = InCopyDesc.DstPosition.X >> MipLevel;
            const uint32 DestPositionY = InCopyDesc.DstPosition.Y >> MipLevel;
            const uint32 DestPositionZ = InCopyDesc.DstPosition.Z >> MipLevel;

            GetCommandList()->CopyTextureRegion(&DestLocation, DestPositionX, DestPositionY, DestPositionZ, &SourceLocation, &SourceBox);
        }
    }
}

void FD3D12CommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) 
{ 
    CHECK(Dst != nullptr); 
    CHECK(Src != nullptr); 
 
    ResourceBarrierBatcher.FlushBarriers(); 
 
    if ((DstOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) != 0) 
    { 
        D3D12_ERROR("CopyTextureRegionToBuffer requires DstOffset aligned to %u bytes. Offset=%llu", D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, DstOffset); 
        return; 
    } 
 
    FD3D12BufferRHI* D3D12Destination = FD3D12RHI::ResourceCast(Dst); 
    CHECK(D3D12Destination != nullptr); 
 
    FD3D12TextureRHI* D3D12Source = FD3D12RHI::ResourceCast(Src); 
    CHECK(D3D12Source != nullptr);

    const uint32 BytesPerPixel = GetByteStrideFromFormat(Src->GetFormat());
    if (BytesPerPixel == 0 || IsBlockCompressed(Src->GetFormat()))
    {
        D3D12_ERROR("CopyTextureRegionToBuffer requires a non-block-compressed, supported format. SrcFormat=%s", ToString(Src->GetFormat()));
        return;
    }

    FD3D12Resource* DstResource = D3D12Destination->GetResource();
    CHECK(DstResource != nullptr);

    const ETextureDimension TextureDimension = Src->GetDimension();
    const uint32 NumArraySlices = D3D12CalculateArraySlices(TextureDimension, Src->GetNumArraySlices());
    const uint32 SrcSubresource = D3D12CalculateSubresource(SrcMipLevel, 0, 0, Src->GetNumMipLevels(), NumArraySlices);

    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
    SourceLocation.pResource        = D3D12Source->GetResource()->GetD3D12Resource();
    SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    SourceLocation.SubresourceIndex = SrcSubresource;

    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.pResource                      = DstResource->GetD3D12Resource();
    DestLocation.Type                           = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DestLocation.PlacedFootprint.Offset         = DstOffset;
    DestLocation.PlacedFootprint.Footprint.Format   = ConvertFormat(Src->GetFormat());
    const uint32 SrcLeft   = SrcRegion.PositionX >> SrcMipLevel;
    const uint32 SrcTop    = SrcRegion.PositionY >> SrcMipLevel;
    const uint32 SrcRight  = Math::Max((SrcRegion.PositionX + SrcRegion.Width) >> SrcMipLevel, SrcLeft + 1);
    const uint32 SrcBottom = Math::Max((SrcRegion.PositionY + SrcRegion.Height) >> SrcMipLevel, SrcTop + 1);

    const uint32 CopyWidth  = SrcRight - SrcLeft;
    const uint32 CopyHeight = SrcBottom - SrcTop;

    const uint32 RowPitch     = Math::AlignUp<uint32>(BytesPerPixel * CopyWidth, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    const uint64 RequiredSize = uint64(RowPitch) * uint64(CopyHeight);
    CHECK(DstOffset + RequiredSize <= DstResource->GetSize());

    DestLocation.PlacedFootprint.Footprint.Width    = CopyWidth;
    DestLocation.PlacedFootprint.Footprint.Height   = CopyHeight;
    DestLocation.PlacedFootprint.Footprint.Depth    = 1;
    DestLocation.PlacedFootprint.Footprint.RowPitch = RowPitch;

    D3D12_BOX SourceBox = {};
    SourceBox.left   = SrcLeft;
    SourceBox.right  = SrcRight;
    SourceBox.top    = SrcTop;
    SourceBox.bottom = SrcBottom;
    SourceBox.front  = 0;
    SourceBox.back   = 1;

    GetCommandList()->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, &SourceBox);
}

void FD3D12CommandContext::WriteFence(FRHIFence* Fence)
{
    CHECK(Fence != nullptr);

    FD3D12FenceRHI* D3D12Fence = FD3D12RHI::ResourceCast(Fence);

    // Submit all work recorded so far, then signal the fence on the queue.
    SplitCommandList(true, false);
    D3D12Fence->Signal(QueueType);
}

void FD3D12CommandContext::DiscardContents(FRHITexture* Texture)
{
    // TODO: Enable regions to be discarded
    if (FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(Texture))
    {
        GetCommandList()->DiscardResource(D3D12Texture->GetResource()->GetD3D12Resource(), nullptr);
    }
}

void FD3D12CommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    CHECK(RayTracingScene != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    FD3D12SceneAccelerationStructureRHI* D3D12RayTracingScene = FD3D12RHI::ResourceCast(RayTracingScene);
    D3D12RayTracingScene->Build(*this, BuildDesc);
}

void FD3D12CommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    CHECK(RayTracingGeometry != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    FD3D12GeometryAccelerationStructureRHI* D3D12RayTracingGeometry = FD3D12RHI::ResourceCast(RayTracingGeometry);
    D3D12RayTracingGeometry->Build(*this, BuildDesc);
}

void FD3D12CommandContext::SetRayTracingBindings(FRHISceneAccelerationStructure* /* RayTracingScene */, FRHIRayTracingPipelineState* /* PipelineState */, const FRayTracingShaderResources* /* GlobalResource */, const FRayTracingShaderResources* /* RayGenLocalResources */, const FRayTracingShaderResources* /* MissLocalResources */, const FRayTracingShaderResources* /* HitGroupResources */, uint32 /* NumHitGroupResources */)
{
#if 0
    FD3D12SceneAccelerationStructureRHI* D3D12Scene = FD3D12RHI::ResourceCast(RayTracingScene);
    D3D12_ERROR_COND(D3D12Scene != nullptr, "RayTracingScene cannot be nullptr");
    FD3D12RayTracingPipelineStateRHI* D3D12PipelineState = FD3D12RHI::ResourceCast(PipelineState);
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
        D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: FAILED to Build Shader Binding Table");
    }

    if (GlobalResource)
    {
        if (!GlobalResource->ConstantBuffers.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ConstantBuffers.Size(); i++)
            {
                FD3D12ConstantBufferView* D3D12ConstantBufferView = FD3D12RHI::ResourceCast(GlobalResource->ConstantBuffers[i])->GetConstantBufferView();
                ContextState.DescriptorCache.SetConstantBufferView(ShaderVisibility_All, D3D12ConstantBufferView, i);
            }
        }
        if (!GlobalResource->ShaderResourceViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ShaderResourceViews.Size(); i++)
            {
                FD3D12ShaderResourceViewRHI* D3D12ShaderResourceView = FD3D12RHI::ResourceCast(GlobalResource->ShaderResourceViews[i]);
                ContextState.DescriptorCache.SetShaderResourceView(ShaderVisibility_All, D3D12ShaderResourceView, i);
            }
        }
        if (!GlobalResource->UnorderedAccessViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->UnorderedAccessViews.Size(); i++)
            {
                FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12RHI::ResourceCast(GlobalResource->UnorderedAccessViews[i]);
                ContextState.DescriptorCache.SetUnorderedAccessView(ShaderVisibility_All, D3D12UnorderedAccessView, i);
            }
        }
        if (!GlobalResource->SamplerStates.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->SamplerStates.Size(); i++)
            {
                FD3D12SamplerStateRHI* DxSampler = FD3D12RHI::ResourceCast(GlobalResource->SamplerStates[i]);
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

void FD3D12CommandContext::TransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(TextureTransition.BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(TextureTransition.AfterState);

    FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12Resource*      D3D12Resource = D3D12Texture->GetResource();
    FD3D12ResourceState* ResourceState = D3D12Texture->GetResourceState();

    const bool bTrackState = ResourceState != nullptr;
    if (TextureTransition.MipLevel != RHI_ALL_MIP_LEVELS || TextureTransition.ArraySlice != RHI_ALL_ARRAY_SLICES)
    {
        const D3D12_RESOURCE_DESC& ResourceDesc = D3D12Resource->GetDesc();

        // Texture3D has a single array-slice
        const uint32 NumArraySlices = ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? ResourceDesc.DepthOrArraySize : 1u;
        const uint32 NumMipLevels   = ResourceDesc.MipLevels;

        // Handle subresources based on if we want to transition all miplevels or arrayslices
        if (TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES)
        {
            CHECK(TextureTransition.MipLevel < NumMipLevels);

            // Make one transition for each ArraySlice
            for (uint32 ArraySlice = 0; ArraySlice < NumArraySlices; ArraySlice++)
            {
                const uint32 SubresourceIndex = D3D12CalculateSubresource(TextureTransition.MipLevel, ArraySlice, 0, NumMipLevels, NumArraySlices);
                CHECK(SubresourceIndex < D3D12Resource->GetNumSubresources());
                ResourceBarrierBatcher.AddTransitionBarrier(D3D12Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
                
                if (bTrackState && ResourceState->IsSubresourceTrackingEnabled())
                {
                    ResourceState->UpdateSubresourceState(D3D12AfterState, TextureTransition.MipLevel, ArraySlice);
                }
            }
        }
        else if (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS)
        {
            CHECK(TextureTransition.ArraySlice < NumArraySlices);

            // Make one transition for each MipLevel
            for (uint32 MipLevel = 0; MipLevel < NumMipLevels; MipLevel++)
            {
                const uint32 SubresourceIndex = D3D12CalculateSubresource(MipLevel, TextureTransition.ArraySlice, 0, NumMipLevels, NumArraySlices);
                CHECK(SubresourceIndex < D3D12Resource->GetNumSubresources());
                ResourceBarrierBatcher.AddTransitionBarrier(D3D12Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
                
                if (bTrackState && ResourceState->IsSubresourceTrackingEnabled())
                {
                    ResourceState->UpdateSubresourceState(D3D12AfterState, MipLevel, TextureTransition.ArraySlice);
                }
            }
        }
        else
        {
            CHECK(TextureTransition.MipLevel < NumMipLevels);
            CHECK(TextureTransition.ArraySlice < NumArraySlices);

            const uint32 SubresourceIndex = D3D12CalculateSubresource(TextureTransition.MipLevel, TextureTransition.ArraySlice, 0, NumMipLevels, NumArraySlices);
            CHECK(SubresourceIndex < D3D12Resource->GetNumSubresources());
            ResourceBarrierBatcher.AddTransitionBarrier(D3D12Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
            
            if (bTrackState && ResourceState->IsSubresourceTrackingEnabled())
            {
                ResourceState->UpdateSubresourceState(D3D12AfterState, TextureTransition.MipLevel, TextureTransition.ArraySlice);
            }
        }
    }
    else
    {
        ResourceBarrierBatcher.AddTransitionBarrier(D3D12Resource, D3D12BeforeState, D3D12AfterState);
        if (bTrackState)
        {
            ResourceState->SetState(D3D12AfterState);
        }
    }
}

void FD3D12CommandContext::TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    FD3D12BufferRHI* D3D12Buffer = FD3D12RHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    ResourceBarrierBatcher.AddTransitionBarrier(D3D12Buffer->GetResource(), D3D12BeforeState, D3D12AfterState);
    if (FD3D12ResourceState* ResourceState = D3D12Buffer->GetResourceState())
    {
        ResourceState->SetState(D3D12AfterState);
    }
}

void FD3D12CommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
    FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12ResourceState* ResourceState = D3D12Texture->GetResourceState();
    CHECK(ResourceState != nullptr);

    const D3D12_RESOURCE_STATES RequiredD3D12State = ConvertResourceState(RequiredState.RequiredState);

    FD3D12Resource* D3D12Resource = D3D12Texture->GetResource();
    if (!ResourceState->IsSubresourceTrackingEnabled())
    {
        const D3D12_RESOURCE_STATES BeforeState = ResourceState->GetState();
        if (BeforeState != RequiredD3D12State)
        {
            ResourceBarrierBatcher.AddTransitionBarrier(D3D12Resource, BeforeState, RequiredD3D12State);
            ResourceState->SetState(RequiredD3D12State);
        }

        return;
    }

    const uint32 MipCount   = ResourceState->GetSubresourceMipCount();
    const uint32 ArrayCount = ResourceState->GetSubresourceArrayCount();

    auto TransitionSubresource = [&](uint32 MipLevel, uint32 ArraySlice)
    {
        CHECK(MipLevel < MipCount);
        CHECK(ArraySlice < ArrayCount);

        const D3D12_RESOURCE_STATES BeforeState = ResourceState->GetSubresourceState(MipLevel, ArraySlice);
        if (BeforeState == RequiredD3D12State)
        {
            return;
        }

        const uint32 SubresourceIndex = D3D12CalculateSubresource(MipLevel, ArraySlice, 0, MipCount, ArrayCount);
        CHECK(SubresourceIndex < D3D12Resource->GetNumSubresources());
        ResourceBarrierBatcher.AddTransitionBarrier(D3D12Resource, BeforeState, RequiredD3D12State, SubresourceIndex);
        ResourceState->UpdateSubresourceState(RequiredD3D12State, MipLevel, ArraySlice);
    };

    if (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS && RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES)
    {
        for (uint32 ArraySlice = 0; ArraySlice < ArrayCount; ++ArraySlice)
        {
            for (uint32 MipLevel = 0; MipLevel < MipCount; ++MipLevel)
            {
                TransitionSubresource(MipLevel, ArraySlice);
            }
        }

        ResourceState->SetState(RequiredD3D12State);
        return;
    }

    if (RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES)
    {
        CHECK(RequiredState.MipLevel < MipCount);

        for (uint32 ArraySlice = 0; ArraySlice < ArrayCount; ++ArraySlice)
        {
            TransitionSubresource(RequiredState.MipLevel, ArraySlice);
        }

        return;
    }

    if (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS)
    {
        CHECK(RequiredState.ArraySlice < ArrayCount);

        for (uint32 MipLevel = 0; MipLevel < MipCount; ++MipLevel)
        {
            TransitionSubresource(MipLevel, RequiredState.ArraySlice);
        }

        return;
    }

    TransitionSubresource(RequiredState.MipLevel, RequiredState.ArraySlice);
}

void FD3D12CommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess InRequiredState)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12RHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    FD3D12ResourceState* ResourceState = D3D12Buffer->GetResourceState();
    CHECK(ResourceState);

    const D3D12_RESOURCE_STATES RequiredState = ConvertResourceState(InRequiredState);
    const D3D12_RESOURCE_STATES BeforeState   = ResourceState->GetState();

    if (BeforeState != RequiredState)
    {
        ResourceBarrierBatcher.AddTransitionBarrier(D3D12Buffer->GetResource(), BeforeState, RequiredState);
        ResourceState->SetState(RequiredState);
    }
}

void FD3D12CommandContext::EnableResourceStateTracking(FRHITexture* Texture, EResourceAccess InitialState)
{
    FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);
    D3D12Texture->EnableStateTracking(InitialState);
}

void FD3D12CommandContext::DisableResourceStateTracking(FRHITexture* Texture, EResourceAccess TargetState)
{
    FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);
    (void)TargetState;

    FD3D12ResourceState* ResourceState = D3D12Texture->GetResourceState();
    if (!ResourceState)
    {
        return;
    }

    D3D12Texture->DisableStateTracking(this);
}

void FD3D12CommandContext::EnableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess InitialState)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12RHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);
    D3D12Buffer->EnableStateTracking(InitialState);
}

void FD3D12CommandContext::DisableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess TargetState)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12RHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);
    (void)TargetState;

    FD3D12ResourceState* ResourceState = D3D12Buffer->GetResourceState();
    if (!ResourceState)
    {
        return;
    }

    D3D12Buffer->DisableStateTracking(this);
}

void FD3D12CommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    FD3D12TextureRHI* D3D12Texture = FD3D12RHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);

    ResourceBarrierBatcher.AddUnorderedAccessBarrier(D3D12Texture->GetResource());
}

void FD3D12CommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12RHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    ResourceBarrierBatcher.AddUnorderedAccessBarrier(D3D12Buffer->GetResource());
}

void FD3D12CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ConditionalSubmitCommandListOnDrawCall();
    GetCommandList()->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void FD3D12CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ConditionalSubmitCommandListOnDrawCall();
    GetCommandList()->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FD3D12CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSubmitCommandListOnDrawCall();
    GetCommandList()->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSubmitCommandListOnDrawCall();
    GetCommandList()->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::ConditionalSubmitCommandListOnDrawCall()
{
    // Split the current command-list if we have reached the maximum amount of draw-calls
    const uint32 MaxDrawCalls = static_cast<uint32>(CVarMaxDrawCallsPerCommandList.GetValue());
    if (NumDrawCalls >= MaxDrawCalls)
    {
        SplitCommandList(true, false);
    }
    else
    {
        ResourceBarrierBatcher.FlushBarriers();
    }

    ContextState.BindGraphicsStates();
    NumDrawCalls++;
}

void FD3D12CommandContext::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    ResourceBarrierBatcher.FlushBarriers();

    ContextState.BindComputeState();
    GetCommandList()->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FD3D12CommandContext::DispatchRays(FRHISceneAccelerationStructure* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    FD3D12SceneAccelerationStructureRHI* D3D12Scene = FD3D12RHI::ResourceCast(RayTracingScene);
    CHECK(D3D12Scene != nullptr);
    
    FD3D12RayTracingPipelineStateRHI* D3D12PipelineState = FD3D12RHI::ResourceCast(PipelineState);
    CHECK(D3D12PipelineState != nullptr);

    ResourceBarrierBatcher.FlushBarriers();

    D3D12_DISPATCH_RAYS_DESC RayDispatchDesc = {};
    RayDispatchDesc.RayGenerationShaderRecord = D3D12Scene->GetRayGenShaderRecord();
    RayDispatchDesc.MissShaderTable           = D3D12Scene->GetMissShaderTable();
    RayDispatchDesc.HitGroupTable             = D3D12Scene->GetHitGroupTable();

    RayDispatchDesc.Width  = Width;
    RayDispatchDesc.Height = Height;
    RayDispatchDesc.Depth  = Depth;

    CommandList->GetGraphicsCommandList4()->SetPipelineState1(D3D12PipelineState->GetD3D12StateObject());
    CommandList->GetGraphicsCommandList4()->DispatchRays(&RayDispatchDesc);
}

void FD3D12CommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    // Ensure that commands are submitted
    FinishCommandList(true);

    FD3D12SwapChainRHI* D3D12SwapChain = FD3D12RHI::ResourceCast(SwapChain);
    D3D12SwapChain->Present(bVerticalSync);

    // Start recording again
    ObtainCommandList();
}

void FD3D12CommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height)
{
    FD3D12SwapChainRHI* D3D12SwapChain = FD3D12RHI::ResourceCast(SwapChain);
    D3D12SwapChain->Resize(this, Width, Height);
}

void FD3D12CommandContext::ClearState()
{
    SCOPED_LOCK(CommandContextCS);
 
    if (CommandList)
    {
        FinishCommandList(true);
        ObtainCommandList();
    }

    FD3D12Queue* Queue = GetDevice()->GetQueue(QueueType);
    CHECK(Queue != nullptr);

    Queue->GetFenceManager().SignalGPU(QueueType);
    Queue->GetFenceManager().WaitForFence();

    ContextState.ResetState();
}

void FD3D12CommandContext::Flush()
{
    SCOPED_LOCK(CommandContextCS);

    if (CommandList)
    {
        FinishCommandList(true);
        ObtainCommandList();
    }

    FD3D12Queue* Queue = GetDevice()->GetQueue(QueueType);
    CHECK(Queue != nullptr);

    Queue->GetFenceManager().SignalGPU(QueueType);
    Queue->GetFenceManager().WaitForFence();
}

void FD3D12CommandContext::InsertMarker(const FStringView& Message)
{
    if (D3D12Functions::SetMarkerOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());
        D3D12Functions::SetMarkerOnCommandList(GraphicsCommandList, PIX_COLOR(255, 255, 255), *Message);
    }
}

void FD3D12CommandContext::BeginExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetAdapter()->GetGraphicsAnalysis();
    if (GraphicsAnalysis && !bIsCapturing)
    {
        GraphicsAnalysis->BeginCapture();
        bIsCapturing = true;
    }
}

void FD3D12CommandContext::EndExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetAdapter()->GetGraphicsAnalysis();
    if (GraphicsAnalysis && bIsCapturing)
    {
        GraphicsAnalysis->EndCapture();
        bIsCapturing = false;
    }
}
