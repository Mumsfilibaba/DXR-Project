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

static TAutoConsoleVariable<int32> CVarMaxCommandsPerCommandList(
    "D3D12RHI.MaxCommandsPerCommandList",
    "Number of commands allowed before submitting the current CommandList to the GPU",
    10000);

static constexpr const bool GD3D12DebugResourceBarriers = false;

void FD3D12BarrierBatcher::AddTransitionBarrier(FD3D12Resource* InResource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex)
{
    CHECK(InResource != nullptr);

    if constexpr (GD3D12DebugResourceBarriers)
    {
        FString DebugName;
        InResource->GetDebugName(DebugName);
        D3D12_INFO("AddTransitionBarrier Resource=%s Subresource=%u Before=%s After=%s", *DebugName, SubresourceIndex, ToString(BeforeState), ToString(AfterState));
    }

    AddTransitionBarrier(InResource->GetD3D12Resource(), BeforeState, AfterState, SubresourceIndex);
}

void FD3D12BarrierBatcher::AddUnorderedAccessBarrier(FD3D12Resource* InResource)
{
    CHECK(InResource != nullptr);

    if constexpr (GD3D12DebugResourceBarriers)
    {
        FString DebugName;
        InResource->GetDebugName(DebugName);
        D3D12_INFO("AddUnorderedAccessBarrier Resource=%s", *DebugName);
    }

    AddUnorderedAccessBarrier(InResource->GetD3D12Resource());
}

void FD3D12BarrierBatcher::AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex)
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

void FD3D12BarrierBatcher::AddUnorderedAccessBarrier(ID3D12Resource* Resource)
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

void FD3D12BarrierBatcher::FlushBarriers(FD3D12CommandList& CommandList)
{
    if (!HasPendingBarriers())
    {
        return;
    }

    const uint32 NumBarriers = Barriers.Size();
    CommandList->ResourceBarrier(NumBarriers, Barriers.Data());

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
    , Commands(nullptr)
    , ContextState(InDevice, *this)
    , TimingQueryAllocator(InDevice, *this, EQueryType::Timestamp)
    , OcclusionQueryAllocator(InDevice, *this, EQueryType::Occlusion)
    , QueueType(InQueueType)
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

        ReopenEventStack();
    }

    if (!Commands)
    {
        Commands = new FD3D12Commands(GetDevice(), Queue);
    }
}

FD3D12ResourceState& FD3D12CommandContext::RetrievePendingResourceState(FD3D12Resource* Resource)
{
    CHECK(Resource != nullptr);

    FD3D12ResourceState& LocalState = PendingResourceStates.FindOrAdd(Resource);
    if (!LocalState.IsInitialized())
    {
        LocalState.Initialize(Resource->GetNumSubresources());
        LocalState.SetResourceState(D3D12_RESOURCE_STATE_TO_BE_DETERMINED);
    }

    return LocalState;
}

void FD3D12CommandContext::AddPendingBarrier(FD3D12Resource* Resource, D3D12_RESOURCE_STATES DesiredState, uint32 Subresource)
{
    FD3D12PendingBarrier PendingBarrier;
    PendingBarrier.Resource     = Resource;
    PendingBarrier.DesiredState = DesiredState;
    PendingBarrier.Subresource  = Subresource;
    PendingBarriers.Add(PendingBarrier);
}


void FD3D12CommandContext::FinishCommandList(bool bFlushAllocator)
{
    TRACE_FUNCTION_SCOPE();

    BarrierBatcher.FlushBarriers(GetCommandList());

    const uint32 RecordedCommands = CommandList->GetNumCommands();
    if (RecordedCommands > 0)
    {
        // NOTE: This is fine since using a query requires a command to be issues
        TimingQueryAllocator.PrepareForNewCommandList();
        OcclusionQueryAllocator.PrepareForNewCommandList();

        // Ensure that all QueryHeaps are resolved
        for (FD3D12QueryHeap* QueryHeap : Commands->QueryHeaps)
        {
            QueryHeap->ResolveQueries(GetCommandList());
        }

        CloseEventStack();

        if (!CommandList->Close())
        {
            D3D12_ERROR_CRITICAL("Failed to close CommandList");
            return;
        }

        CHECK(Commands != nullptr);

        Commands->PendingBarriers       = Move(PendingBarriers);
        Commands->PendingResourceStates = Move(PendingResourceStates);

        Commands->AddCommandList(CommandList);
        CommandList = nullptr;

        if (bFlushAllocator)
        {
            Commands->AddCommandAllocator(CommandAllocator);
            CommandAllocator = nullptr;
        }

        FD3D12RHI::Get()->SubmitCommands(Commands, true);
        Commands = nullptr;
    }
    else
    {
        PendingBarriers.Clear();
        PendingResourceStates.Clear();
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
        FD3D12Fence& Fence = Queue->GetSubmissionFence();
        Fence.WaitForValue(Fence.GetLastSignaledValue());
    }
    
    ObtainCommandList();
}

void FD3D12CommandContext::SplitCommandListAndResetState(bool bFlushAllocator, bool bWaitForQueue)
{
    FinishCommandList(bFlushAllocator);

    if (bWaitForQueue)
    {
        FD3D12Queue* Queue = GetDevice()->GetQueue(QueueType);
        FD3D12Fence& Fence = Queue->GetSubmissionFence();
        Fence.WaitForValue(Fence.GetLastSignaledValue());
    }

    ContextState.ResetState();

    ObtainCommandList();
}

void FD3D12CommandContext::BeginFrame()
{
    FD3D12RHI::Get()->BeginFrame(this);
}

void FD3D12CommandContext::EndFrame()
{
    FD3D12RHI::Get()->EndFrame();
}

void FD3D12CommandContext::StartContext()
{
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Lock to the thread that started the context
    CommandContextCS.Lock();

    // Reset the state
    ContextState.ResetState();
    EventStack.Clear();

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

    BarrierBatcher.FlushBarriers(GetCommandList());

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
        FD3D12ResourceStorage ResourceStorage(GetDevice());
        if (GetDevice()->GetStagingBufferAllocator()->Allocate(BufferRegion.Size, 1, ResourceStorage) == nullptr || ResourceStorage.GetResource() == nullptr || ResourceStorage.GetMappedBaseAddress() == nullptr)
        {
            D3D12_ERROR_CRITICAL("Upload allocation failed");
            return;
        }

        FMemory::Memcpy(ResourceStorage.GetMappedBaseAddress(), SrcData, BufferRegion.Size);

        GetCommandList()->CopyBufferRegion(Resource->GetD3D12Resource(), BufferRegion.Offset, ResourceStorage.GetResource()->GetD3D12Resource(), ResourceStorage.GetResourceOffset(), BufferRegion.Size);
    }
}

void FD3D12CommandContext::BeginQuery(FRHIQuery* Query) 
{
    FD3D12Query* D3D12Query = static_cast<FD3D12Query*>(Query);
    CHECK(D3D12Query != nullptr);

    FD3D12QueryAllocation QueryAllocation = OcclusionQueryAllocator.Allocate(&D3D12Query->Result);
    if (!QueryAllocation.IsValid())
    {
        D3D12_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    CHECK(QueryAllocation.QueryHeap != nullptr);
    GetCommandList().UpdateResidency(QueryAllocation.QueryHeap->GetResidencyHandle());
    GetCommandList()->BeginQuery(QueryAllocation.QueryHeap->GetD3D12QueryHeap(), D3D12_QUERY_TYPE_OCCLUSION, QueryAllocation.IndexInQueryHeap);
    D3D12Query->QueryAllocation = QueryAllocation;
}

void FD3D12CommandContext::EndQuery(FRHIQuery* Query) 
{
    FD3D12Query* D3D12Query = static_cast<FD3D12Query*>(Query);
    CHECK(D3D12Query != nullptr);

    FD3D12QueryAllocation QueryAllocation = D3D12Query->QueryAllocation;
    if (!QueryAllocation.IsValid())
    {
        D3D12_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    CHECK(QueryAllocation.QueryHeap != nullptr);
    GetCommandList().UpdateResidency(QueryAllocation.QueryHeap->GetResidencyHandle());
    GetCommandList()->EndQuery(QueryAllocation.QueryHeap->GetD3D12QueryHeap(), D3D12_QUERY_TYPE_OCCLUSION, QueryAllocation.IndexInQueryHeap);
}

void FD3D12CommandContext::QueryTimestamp(FRHIQuery* Query)
{
    FD3D12Query* D3D12Query = static_cast<FD3D12Query*>(Query);
    CHECK(D3D12Query != nullptr);

    FD3D12QueryAllocation QueryAllocation = TimingQueryAllocator.Allocate(&D3D12Query->Result);
    if (!QueryAllocation.IsValid())
    {
        D3D12_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    CHECK(QueryAllocation.QueryHeap != nullptr);
    GetCommandList().UpdateResidency(QueryAllocation.QueryHeap->GetResidencyHandle());
    GetCommandList()->EndQuery(QueryAllocation.QueryHeap->GetD3D12QueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, QueryAllocation.IndexInQueryHeap);
    D3D12Query->QueryAllocation = QueryAllocation;
}

void FD3D12CommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(RenderTargetView.Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12RenderTargetView* D3D12RenderTargetView = D3D12Texture->GetOrCreateRenderTargetView(RenderTargetView);
    CHECK(D3D12RenderTargetView != nullptr);

    GetCommandList()->ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), ClearColor.XYZW, 0, nullptr);
}

void FD3D12CommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(DepthStencilView.Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12DepthStencilView* D3D12DepthStencilView = D3D12Texture->GetOrCreateDepthStencilView(DepthStencilView);
    CHECK(D3D12DepthStencilView != nullptr);

    GetCommandList()->ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Depth, Stencil, 0, nullptr);
}

void FD3D12CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(UnorderedAccessView);
    CHECK(D3D12UnorderedAccessView != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

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

void FD3D12CommandContext::BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo)
{
    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12RenderTargetView* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    FD3D12DepthStencilView* DepthStencilView = nullptr;

    for (uint32 Index = 0; Index < BeginRenderPassInfo.NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& CurrentRTV = BeginRenderPassInfo.RenderTargets[Index];
        if (FD3D12Texture* RenderTarget = FD3D12Texture::Cast(CurrentRTV.Texture))
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

    const FRHIDepthStencilView& CurrentDSV = BeginRenderPassInfo.DepthStencilView;
    if (FD3D12Texture* DepthStencil = FD3D12Texture::Cast(CurrentDSV.Texture))
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

    ContextState.SetRenderTargets(RenderTargetViews, BeginRenderPassInfo.NumRenderTargets, DepthStencilView);

    // ShadingRate
    FD3D12Texture* ShadingRateImage = FD3D12Texture::Cast(BeginRenderPassInfo.ShadingRateTexture);
    ContextState.SetShadingRateImage(ShadingRateImage);
    ContextState.SetShadingRate(BeginRenderPassInfo.StaticShadingRate);
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
        FD3D12Buffer* D3DVertexBuffer = static_cast<FD3D12Buffer*>(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(D3DVertexBuffer, BufferSlot + Index);
    }
}

void FD3D12CommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FD3D12Buffer* D3DIndexBuffer = static_cast<FD3D12Buffer*>(IndexBuffer);
    ContextState.SetIndexBuffer(D3DIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FD3D12CommandContext::SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FD3D12GraphicsPipelineState* GraphicsPipelineState = static_cast<FD3D12GraphicsPipelineState*>(PipelineState);
    ContextState.SetGraphicsPipelineState(GraphicsPipelineState);
}

void FD3D12CommandContext::SetComputePipelineState(class FRHIComputePipelineState* PipelineState)
{
    FD3D12ComputePipelineState* ComputePipelineState = static_cast<FD3D12ComputePipelineState*>(PipelineState);
    ContextState.SetComputePipelineState(ComputePipelineState);
}

void FD3D12CommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    ContextState.SetShaderConstants(reinterpret_cast<const uint32*>(ShaderConstants), NumShaderConstants);
}

void FD3D12CommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(ShaderResourceView);
    
    CHECK(RegisterIndex < D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
    ContextState.SetSRV(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), RegisterIndex);
}

void FD3D12CommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(RegisterIndex + InShaderResourceViews.Size() <= D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(InShaderResourceViews[Index]);
        ContextState.SetSRV(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(UnorderedAccessView);
    
    CHECK(RegisterIndex < D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
    ContextState.SetUAV(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), RegisterIndex);
}

void FD3D12CommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(RegisterIndex + InUnorderedAccessViews.Size() <= D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    FD3D12Buffer* D3D12Buffer = ConstantBuffer ? FD3D12Buffer::Cast(ConstantBuffer) : nullptr;
    
    CHECK(RegisterIndex < D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);
    ContextState.SetCBV(D3D12Buffer, D3D12Shader->GetShaderVisibility(), RegisterIndex);
}

void FD3D12CommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(RegisterIndex + InConstantBuffers.Size() <= D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);
    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FD3D12Buffer* D3D12Buffer = InConstantBuffers[Index] ? FD3D12Buffer::Cast(InConstantBuffers[Index]) : nullptr;
        ContextState.SetCBV(D3D12Buffer, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    FD3D12SamplerState* D3D12SamplerState = static_cast<FD3D12SamplerState*>(SamplerState);

    CHECK(RegisterIndex < D3D12_DEFAULT_SAMPLER_STATE_COUNT);
    ContextState.SetSampler(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), RegisterIndex);
}

void FD3D12CommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    CHECK(RegisterIndex + InSamplerStates.Size() <= D3D12_DEFAULT_SAMPLER_STATE_COUNT);
    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FD3D12SamplerState* D3D12SamplerState = static_cast<FD3D12SamplerState*>(InSamplerStates[Index]);
        ContextState.SetSampler(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12Texture* D3D12Destination = FD3D12Texture::Cast(Dst);
    FD3D12Texture* D3D12Source      = FD3D12Texture::Cast(Src);

    const DXGI_FORMAT DstFormat = D3D12Destination->GetDXGIFormat();
    const DXGI_FORMAT SrcFormat = D3D12Source->GetDXGIFormat();

    //TODO: For now texture must be the same format. I.e typeless does probably not work
    if (DstFormat != SrcFormat)
    {
        D3D12_ERROR("Dst or Src must have the same formats");
        return;
    }

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    GetCommandList()->ResolveSubresource(D3D12Destination->GetResource()->GetD3D12Resource(), 0, D3D12Source->GetResource()->GetD3D12Resource(), 0, DstFormat);
}

void FD3D12CommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (!BufferRegion.Size)
    {
        return;
    }

    FD3D12Buffer* D3D12Destination = FD3D12Buffer::Cast(Dst);
    CHECK(D3D12Destination != nullptr);

    if (D3D12Destination->GetInfo().IsTransient())
    {
        FD3D12ResourceStorage& Storage = D3D12Destination->GetResourceStorage();

        void* MappedPtr = nullptr;
        if (D3D12Destination->GetInfo().IsConstantBuffer())
        {
            MappedPtr = GetDevice()->GetDynamicConstantsAllocator()->Allocate(BufferRegion.Size, Storage);
        }
        else
        {
            MappedPtr = GetDevice()->GetUploadHeapAllocator()->Allocate(BufferRegion.Size, 16, Storage);
        }

        if (!MappedPtr)
        {
            D3D12_ERROR_CRITICAL("Failed to allocate transient buffer memory");
            return;
        }

        FMemory::Memcpy(MappedPtr, SrcData, BufferRegion.Size);

        if (D3D12Destination->GetInfo().IsConstantBuffer())
        {
            D3D12Destination->GetOrCreateConstantBufferView();
        }
    }
    else
    {
        UpdateBuffer(D3D12Destination->GetResource(), BufferRegion, SrcData);
    }
}

void FD3D12CommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    CHECK(SrcData != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12Texture* D3D12Destination = FD3D12Texture::Cast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Resource* D3D12Resource = D3D12Destination->GetResource();
    CHECK(D3D12Resource != nullptr);

    D3D12_RESOURCE_DESC Desc = D3D12Resource->GetDesc();
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) != 0)
    {
        // Query APIs require tight-alignment resources to use Alignment=0 in the desc.
        Desc.Alignment = 0;
    }

    UINT64 RequiredSize = 0;
    UINT64 RowPitch     = 0;
    UINT32 NumRows      = 0;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubresourceFootprint;
    GetDevice()->GetD3D12Device()->GetCopyableFootprints(&Desc, MipLevel, 1, 0, &PlacedSubresourceFootprint, &NumRows, &RowPitch, &RequiredSize);

    const uint64 Alignment   = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    const uint64 AlignedSize = Math::AlignUp<uint64>(RequiredSize, Alignment);

    FD3D12ResourceStorage ResourceStorage(GetDevice());
    if (GetDevice()->GetStagingBufferAllocator()->Allocate(AlignedSize, Alignment, ResourceStorage) == nullptr || ResourceStorage.GetMappedBaseAddress() == nullptr || ResourceStorage.GetResource() == nullptr)
    {
        D3D12_ERROR_CRITICAL("Upload allocation failed");
        return;
    }

    uint8* WritePtr = reinterpret_cast<uint8*>(ResourceStorage.GetMappedBaseAddress());
    
    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    for (uint64 y = 0; y < NumRows; y++)
    {
        FMemory::Memcpy(WritePtr, Source, SrcRowPitch);
        
        WritePtr += PlacedSubresourceFootprint.Footprint.RowPitch;
        Source   += SrcRowPitch;
    }

    // Copy to Dest
    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
    SourceLocation.pResource                          = ResourceStorage.GetResource()->GetD3D12Resource();
    SourceLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SourceLocation.PlacedFootprint.Offset             = ResourceStorage.GetResourceOffset();
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
}

void FD3D12CommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyInfo)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12Buffer* D3D12Destination = FD3D12Buffer::Cast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Buffer* D3D12Source = FD3D12Buffer::Cast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    GetCommandList()->CopyBufferRegion(D3D12Destination->GetResource()->GetD3D12Resource(), CopyInfo.DstOffset, D3D12Source->GetResource()->GetD3D12Resource(), CopyInfo.SrcOffset, CopyInfo.Size);
}

void FD3D12CommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12Texture* D3D12Destination = FD3D12Texture::Cast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Texture* D3D12Source = FD3D12Texture::Cast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    GetCommandList()->CopyResource(D3D12Destination->GetResource()->GetD3D12Resource(), D3D12Source->GetResource()->GetD3D12Resource());
}

void FD3D12CommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& InCopyDesc)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D12Texture* D3D12Destination = FD3D12Texture::Cast(Dst);
    CHECK(D3D12Destination != nullptr);
    
    FD3D12Texture* D3D12Source = FD3D12Texture::Cast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    BarrierBatcher.FlushBarriers(GetCommandList());

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
 
    BarrierBatcher.FlushBarriers(GetCommandList()); 
 
    if ((DstOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) != 0) 
    { 
        D3D12_ERROR("CopyTextureRegionToBuffer requires DstOffset aligned to %u bytes. Offset=%llu", D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, DstOffset); 
        return; 
    } 
 
    FD3D12Buffer* D3D12Destination = FD3D12Buffer::Cast(Dst); 
    CHECK(D3D12Destination != nullptr); 
 
    FD3D12Texture* D3D12Source = FD3D12Texture::Cast(Src); 
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

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
    DestLocation.pResource                        = DstResource->GetD3D12Resource();
    DestLocation.Type                             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DestLocation.PlacedFootprint.Offset           = DstOffset;
    DestLocation.PlacedFootprint.Footprint.Format = ConvertFormat(Src->GetFormat());

    const uint32 SrcLeft      = SrcRegion.PositionX >> SrcMipLevel;
    const uint32 SrcTop       = SrcRegion.PositionY >> SrcMipLevel;
    const uint32 SrcRight     = Math::Max((SrcRegion.PositionX + SrcRegion.Width) >> SrcMipLevel, SrcLeft + 1);
    const uint32 SrcBottom    = Math::Max((SrcRegion.PositionY + SrcRegion.Height) >> SrcMipLevel, SrcTop + 1);
    const uint32 CopyWidth    = SrcRight - SrcLeft;
    const uint32 CopyHeight   = SrcBottom - SrcTop;
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

void FD3D12CommandContext::WriteFence(FRHIGpuFence* Fence)
{
    CHECK(Fence != nullptr);

    FD3D12GpuFence* D3D12Fence = static_cast<FD3D12GpuFence*>(Fence);

    SplitCommandList(true, false);
    D3D12Fence->Signal(GetDevice()->GetD3D12CommandQueue(QueueType));
}

void FD3D12CommandContext::DiscardContents(FRHITexture* Texture)
{
    // TODO: Enable regions to be discarded
    if (FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(Texture))
    {
        GetCommandList()->DiscardResource(D3D12Texture->GetResource()->GetD3D12Resource(), nullptr);
    }
}

void FD3D12CommandContext::BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo)
{
    CHECK(RayTracingScene != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12RayTracingScene* D3D12RayTracingScene = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    D3D12RayTracingScene->Build(*this, BuildInfo);
}

void FD3D12CommandContext::BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo)
{
    CHECK(RayTracingGeometry != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12RayTracingGeometry* D3D12RayTracingGeometry = static_cast<FD3D12RayTracingGeometry*>(RayTracingGeometry);
    D3D12RayTracingGeometry->Build(*this, BuildInfo);
}

void FD3D12CommandContext::SetRayTracingBindings(FRHIRayTracingScene* /* RayTracingScene */, FRHIRayTracingPipelineState* /* PipelineState */, const FRayTracingShaderResources* /* GlobalResource */, const FRayTracingShaderResources* /* RayGenLocalResources */, const FRayTracingShaderResources* /* MissLocalResources */, const FRayTracingShaderResources* /* HitGroupResources */, uint32 /* NumHitGroupResources */)
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
        D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: FAILED to Build Shader Binding Table");
    }

    if (GlobalResource)
    {
        if (!GlobalResource->ConstantBuffers.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ConstantBuffers.Size(); i++)
            {
                FD3D12Buffer* D3D12Buffer = FD3D12Buffer::Cast(GlobalResource->ConstantBuffers[i]);
                ContextState.SetCBV(D3D12Buffer, ShaderVisibility_All, i);
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

void FD3D12CommandContext::TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(TextureTransition.BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(TextureTransition.AfterState);

    FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12Resource* Resource = D3D12Texture->GetResource();
    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);

    if (TextureTransition.MipLevel != RHI_ALL_MIP_LEVELS || TextureTransition.ArraySlice != RHI_ALL_ARRAY_SLICES)
    {
        const D3D12_RESOURCE_DESC& ResourceDesc = Resource->GetDesc();
        const uint32 NumArraySlices = ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? ResourceDesc.DepthOrArraySize : 1u;
        const uint32 NumMipLevels   = ResourceDesc.MipLevels;

        if (TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES)
        {
            CHECK(TextureTransition.MipLevel < NumMipLevels);
            for (uint32 ArraySlice = 0; ArraySlice < NumArraySlices; ArraySlice++)
            {
                const uint32 SubresourceIndex = D3D12CalculateSubresource(TextureTransition.MipLevel, ArraySlice, 0, NumMipLevels, NumArraySlices);
                CHECK(SubresourceIndex < Resource->GetNumSubresources());

                const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(SubresourceIndex);
                if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
                {
                    AddPendingBarrier(Resource, D3D12BeforeState, SubresourceIndex);
                }
                else
                {
                    CHECK(CurrentState == D3D12BeforeState);
                }

                BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
                LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
            }
        }
        else if (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS)
        {
            CHECK(TextureTransition.ArraySlice < NumArraySlices);
            for (uint32 MipLevel = 0; MipLevel < NumMipLevels; MipLevel++)
            {
                const uint32 SubresourceIndex = D3D12CalculateSubresource(MipLevel, TextureTransition.ArraySlice, 0, NumMipLevels, NumArraySlices);
                CHECK(SubresourceIndex < Resource->GetNumSubresources());

                const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(SubresourceIndex);
                if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
                {
                    AddPendingBarrier(Resource, D3D12BeforeState, SubresourceIndex);
                }
                else
                {
                    CHECK(CurrentState == D3D12BeforeState);
                }

                BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
                LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
            }
        }
        else
        {
            CHECK(TextureTransition.MipLevel < NumMipLevels);
            CHECK(TextureTransition.ArraySlice < NumArraySlices);

            const uint32 SubresourceIndex = D3D12CalculateSubresource(TextureTransition.MipLevel, TextureTransition.ArraySlice, 0, NumMipLevels, NumArraySlices);
            CHECK(SubresourceIndex < Resource->GetNumSubresources());

            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(SubresourceIndex);
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, D3D12BeforeState, SubresourceIndex);
            }
            else
            {
                CHECK(CurrentState == D3D12BeforeState);
            }

            BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
            LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
        }
    }
    else
    {
        const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();
        if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            AddPendingBarrier(Resource, D3D12BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        }
        else if (LocalState.AreAllSubresourcesSameState())
        {
            CHECK(CurrentState == D3D12BeforeState);
        }

        BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState);
        LocalState.SetResourceState(D3D12AfterState);
    }
}

void FD3D12CommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    FD3D12Buffer* D3D12Buffer = FD3D12Buffer::Cast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    FD3D12Resource* Resource = D3D12Buffer->GetResource();
    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);

    const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();
    if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
    {
        AddPendingBarrier(Resource, D3D12BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    }
    else
    {
        CHECK(CurrentState == D3D12BeforeState);
    }

    BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState);
    LocalState.SetResourceState(D3D12AfterState);
}

void FD3D12CommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
    FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(Texture);
    CHECK(D3D12Texture != nullptr);

    FD3D12Resource* Resource = D3D12Texture->GetResource();
    CHECK(Resource != nullptr);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    
    const D3D12_RESOURCE_STATES DesiredState = ConvertResourceState(RequiredState.State);
    const D3D12_RESOURCE_DESC&  ResourceDesc = Resource->GetDesc();

    const uint32 NumArraySlices = ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? ResourceDesc.DepthOrArraySize : 1u;
    const uint32 NumMipLevels   = ResourceDesc.MipLevels;

    if (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS && RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES)
    {
        if (LocalState.AreAllSubresourcesSameState())
        {
            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, DesiredState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            }
            else if (CurrentState != DesiredState)
            {
                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState);
            }
        }
        else
        {
            for (uint32 i = 0; i < LocalState.GetNumSubresources(); i++)
            {
                const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(i);
                if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
                {
                    AddPendingBarrier(Resource, DesiredState, i);
                }
                else if (CurrentState != DesiredState)
                {
                    BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState, i);
                }
            }
        }

        LocalState.SetResourceState(DesiredState);
    }
    else if (RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES)
    {
        CHECK(RequiredState.MipLevel < NumMipLevels);

        for (uint32 ArraySlice = 0; ArraySlice < NumArraySlices; ArraySlice++)
        {
            const uint32 SubresourceIndex = D3D12CalculateSubresource(RequiredState.MipLevel, ArraySlice, 0, NumMipLevels, NumArraySlices);

            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(SubresourceIndex);
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, DesiredState, SubresourceIndex);
            }
            else if (CurrentState != DesiredState)
            {
                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState, SubresourceIndex);
            }

            LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
        }
    }
    else if (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS)
    {
        CHECK(RequiredState.ArraySlice < NumArraySlices);
        for (uint32 MipLevel = 0; MipLevel < NumMipLevels; MipLevel++)
        {
            const uint32 SubresourceIndex = D3D12CalculateSubresource(MipLevel, RequiredState.ArraySlice, 0, NumMipLevels, NumArraySlices);

            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(SubresourceIndex);
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, DesiredState, SubresourceIndex);
            }
            else if (CurrentState != DesiredState)
            {
                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState, SubresourceIndex);
            }
            
            LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
        }
    }
    else
    {
        CHECK(RequiredState.MipLevel < NumMipLevels);
        CHECK(RequiredState.ArraySlice < NumArraySlices);
        
        const uint32 SubresourceIndex = D3D12CalculateSubresource(RequiredState.MipLevel, RequiredState.ArraySlice, 0, NumMipLevels, NumArraySlices);
        
        const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(SubresourceIndex);
        if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            AddPendingBarrier(Resource, DesiredState, SubresourceIndex);
        }
        else if (CurrentState != DesiredState)
        {
            BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState, SubresourceIndex);
        }

        LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
    }
}

void FD3D12CommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
    FD3D12Buffer* D3D12Buffer = FD3D12Buffer::Cast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    FD3D12Resource* Resource = D3D12Buffer->GetResource();
    CHECK(Resource != nullptr);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);

    const D3D12_RESOURCE_STATES DesiredState = ConvertResourceState(RequiredState);
    const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();

    if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
    {
        AddPendingBarrier(Resource, DesiredState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        LocalState.SetResourceState(DesiredState);
    }
    else if (CurrentState != DesiredState)
    {
        BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState);
        LocalState.SetResourceState(DesiredState);
    }
}

void FD3D12CommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(Texture);
    CHECK(D3D12Texture != nullptr);

    BarrierBatcher.AddUnorderedAccessBarrier(D3D12Texture->GetResource());
}

void FD3D12CommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    FD3D12Buffer* D3D12Buffer = FD3D12Buffer::Cast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    BarrierBatcher.AddUnorderedAccessBarrier(D3D12Buffer->GetResource());
}

void FD3D12CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsStates();
    GetCommandList()->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void FD3D12CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsStates();
    GetCommandList()->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FD3D12CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsStates();
    GetCommandList()->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsStates();
    GetCommandList()->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::ConditionalSplitCommandList()
{
    const uint32 MaxCommands = static_cast<uint32>(CVarMaxCommandsPerCommandList.GetValue());
    if (CommandList->GetNumCommands() >= MaxCommands)
    {
        SplitCommandList(true, false);
    }
}

void FD3D12CommandContext::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    if (ThreadGroupCountX == 0 || ThreadGroupCountY == 0 || ThreadGroupCountZ == 0)
    {
        return;
    }

    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindComputeState();
    GetCommandList()->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FD3D12CommandContext::DispatchRays(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    FD3D12RayTracingScene* D3D12Scene = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    CHECK(D3D12Scene != nullptr);

    FD3D12RayTracingPipelineState* D3D12PipelineState = static_cast<FD3D12RayTracingPipelineState*>(PipelineState);
    CHECK(D3D12PipelineState != nullptr);

    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());

    GetCommandList().UpdateResidency(D3D12Scene->GetResource()->GetResidencyHandle());
    if (D3D12Scene->GetBindingTable())
    {
        GetCommandList().UpdateResidency(D3D12Scene->GetBindingTable()->GetResidencyHandle());
    }
    if (D3D12Scene->GetInstanceBuffer())
    {
        GetCommandList().UpdateResidency(D3D12Scene->GetInstanceBuffer()->GetResidencyHandle());
    }

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

    FD3D12SwapChain* D3D12SwapChain = static_cast<FD3D12SwapChain*>(SwapChain);
    D3D12SwapChain->Present(bVerticalSync);

    // Start recording again
    ObtainCommandList();
}

void FD3D12CommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height)
{
    FD3D12SwapChain* D3D12SwapChain = static_cast<FD3D12SwapChain*>(SwapChain);
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

    FD3D12Fence& Fence = Queue->GetSubmissionFence();
    Fence.Signal(Queue->GetD3D12CommandQueue());
    Fence.WaitForValue(Fence.GetLastSignaledValue());

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

    FD3D12Fence& Fence = Queue->GetSubmissionFence();
    Fence.Signal(Queue->GetD3D12CommandQueue());
    Fence.WaitForValue(Fence.GetLastSignaledValue());
}

void FD3D12CommandContext::PushEvent(const FStringView& Name)
{
    EventStack.Emplace(Name.Data());

    if (D3D12Functions::PIXBeginEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());
        D3D12Functions::PIXBeginEventOnCommandList(GraphicsCommandList, PIX_COLOR(255, 255, 255), *Name);
    }
}

void FD3D12CommandContext::PopEvent()
{
    if (!EventStack.IsEmpty())
    {
        EventStack.Pop();
    }

    if (D3D12Functions::PIXEndEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());
        D3D12Functions::PIXEndEventOnCommandList(GraphicsCommandList);
    }
}

void FD3D12CommandContext::CloseEventStack()
{
    if (D3D12Functions::PIXEndEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());
        for (int32 i = EventStack.Size() - 1; i >= 0; --i)
        {
            D3D12Functions::PIXEndEventOnCommandList(GraphicsCommandList);
        }
    }
}

void FD3D12CommandContext::ReopenEventStack()
{
    if (D3D12Functions::PIXBeginEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());
        for (int32 i = 0; i < EventStack.Size(); ++i)
        {
            D3D12Functions::PIXBeginEventOnCommandList(GraphicsCommandList, PIX_COLOR(255, 255, 255), *EventStack[i]);
        }
    }
}
