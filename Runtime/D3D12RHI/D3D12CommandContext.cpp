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
#include "D3D12RHI/D3D12Descriptors.h"
#include <pix.h>

static TAutoConsoleVariable<int32> CVarMaxCommandsPerCommandList(
    "D3D12RHI.MaxCommandsPerCommandList",
    "Number of commands allowed before submitting the current CommandList to the GPU",
    10000);

void FD3D12BarrierBatcher::AddTransitionBarrier(FD3D12Resource* InResource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex)
{
    CHECK(InResource != nullptr);

#if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    {
        FString DebugName;
        InResource->GetDebugName(DebugName);
        D3D12_INFO("AddTransitionBarrier Resource=%s Subresource=%u Before=%s After=%s", *DebugName, SubresourceIndex, ToString(BeforeState), ToString(AfterState));
    }
#endif

    AddTransitionBarrier(InResource->GetD3D12Resource(), BeforeState, AfterState, SubresourceIndex);
}

void FD3D12BarrierBatcher::AddUnorderedAccessBarrier(FD3D12Resource* InResource)
{
    CHECK(InResource != nullptr);

#if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    {
        FString DebugName;
        InResource->GetDebugName(DebugName);
        D3D12_INFO("AddUnorderedAccessBarrier Resource=%s", *DebugName);
    }
#endif

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
        const bool bBothAllSubresources = (Existing.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) &&
            (SubresourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        
        if (!(bSameSubresource || bBothAllSubresources))
        {
            continue;
        }

        // Case 1: Redundant barrier (FirstRange->SecondRange then FirstRange->SecondRange again) => ignore new barrier
        D3D12_RESOURCE_TRANSITION_BARRIER& ExistingTransitionBarrier = It->Transition;
        if (ExistingTransitionBarrier.StateBefore == BeforeState && ExistingTransitionBarrier.StateAfter == AfterState)
        {
        #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
            LOG_INFO("  Redundant barrier FirstRange->SecondRange kept (SubresourceIndex=%u, %s->%s)",SubresourceIndex, ToString(BeforeState), ToString(AfterState));
        #endif
            return;
        }

        // Case 2: Extend barrier (FirstRange->SecondRange then SecondRange->C) => FirstRange->C
        if (ExistingTransitionBarrier.StateAfter == BeforeState)
        {
            ExistingTransitionBarrier.StateAfter = AfterState;
        #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
            LOG_INFO("  Extended barrier to %s->%s (SubresourceIndex=%u)", 
                ToString(ExistingTransitionBarrier.StateBefore), ToString(ExistingTransitionBarrier.StateAfter), SubresourceIndex);
        #endif

            // If we changed state to the same before- and after-state, remove it
            if (ExistingTransitionBarrier.StateBefore == ExistingTransitionBarrier.StateAfter)
            {
            #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
                LOG_INFO("  Cancelled barrier (SubresourceIndex=%u, %s<->%s)", 
                    SubresourceIndex, ToString(ExistingTransitionBarrier.StateBefore), ToString(ExistingTransitionBarrier.StateAfter));
            #endif
                Barriers.RemoveAt(It.GetIndex());
            }

            return;
        }

        // Case 3: Cancel barrier (FirstRange->SecondRange then SecondRange->FirstRange) => remove
        if (ExistingTransitionBarrier.StateBefore == AfterState)
        {
        #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
            LOG_INFO("  Cancelled barrier (SubresourceIndex=%u, %s<->%s)", SubresourceIndex, ToString(BeforeState), ToString(AfterState));
        #endif
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

#if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    D3D12_INFO("FlushBarriers NumBarriers=%u", NumBarriers);
#endif

    Barriers.Clear();
}

FD3D12CommandContext::FD3D12CommandContext(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : IRHICommandContext()
    , FD3D12DeviceChild(InDevice)
    , CommandList(nullptr)
    , CommandAllocator(nullptr)
    , Commands(nullptr)
    , ContextState(InDevice, *this)
    , TimingQueryAllocator(InDevice, D3D12_QUERY_HEAP_TYPE_TIMESTAMP)
    , OcclusionQueryAllocator(InDevice, D3D12_QUERY_HEAP_TYPE_OCCLUSION)
    , PipelineStatsQueryAllocator(InDevice, GetPipelineStatsHeapType())
    , QueueType(InQueueType)
    , ActiveQueryCount(0)
    , bIsRecording(false)
    , CommandContextCS()
{
}

FD3D12CommandContext::~FD3D12CommandContext() = default;

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

        CommandList->InsertBeginTimestamp(TimingQueryAllocator);
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

void FD3D12CommandContext::FinishCommandList(bool bFlushAllocator, bool bResolveQueries)
{
    TRACE_FUNCTION_SCOPE();

    // -------------------------------------------------------------------------------------------
    // Drain bindless descriptor writes accumulated during recording. The bindless heap aliases
    // the shader-visible global descriptor heap. Doing this before Close()/SubmitCommands() makes
    // sure every descriptor referenced via a bindless index in this command list is up-to-date
    // before the GPU starts executing it. Safe no-op when no writes were enqueued.
    // -------------------------------------------------------------------------------------------

    if (FD3D12BindlessDescriptorHeap* ResourceBindlessHeap = GetDevice()->GetResourceBindlessHeap())
    {
        ResourceBindlessHeap->Flush();
    }

    if (FD3D12BindlessDescriptorHeap* SamplerBindlessHeap = GetDevice()->GetSamplerBindlessHeap())
    {
        SamplerBindlessHeap->Flush();
    }

    BarrierBatcher.FlushBarriers(GetCommandList());
    CommandList->InsertEndTimestamp(TimingQueryAllocator);

    const uint32 RecordedCommands = CommandList->GetNumCommands();
    if (RecordedCommands > 0)
    {
        if (bResolveQueries)
        {
            TimingQueryAllocator.Reset(Commands->QueryRanges);
            OcclusionQueryAllocator.Reset(Commands->QueryRanges);
            PipelineStatsQueryAllocator.Reset(Commands->QueryRanges);

            Commands->Flags |= ED3D12CommandsFlags::ResolveQueries;

            TArray<FD3D12QueryRange>& AllRanges = Commands->QueryRanges;

            AllRanges.SortWithPredicate([](const FD3D12QueryRange& FirstRange, const FD3D12QueryRange& SecondRange)
            {
                if (FirstRange.Heap != SecondRange.Heap)
                {
                    return reinterpret_cast<uintptr_t>(FirstRange.Heap) < reinterpret_cast<uintptr_t>(SecondRange.Heap);
                }
                
                return FirstRange.StartIndex < SecondRange.StartIndex;
            });

            for (int32 i = 0; i < AllRanges.Size(); )
            {
                const FD3D12QueryRange& Range = AllRanges[i];
                if (Range.Count <= 0 || !Range.Heap)
                {
                    i++;
                    continue;
                }

                FD3D12QueryHeap* Heap = Range.Heap;

                int32 MergedStart = Range.StartIndex;
                int32 MergedEnd   = MergedStart + Range.Count;

                int32 j = i + 1;
                while (j < AllRanges.Size() && AllRanges[j].Heap == Heap && AllRanges[j].StartIndex <= MergedEnd)
                {
                    int32 RangeEnd = AllRanges[j].StartIndex + AllRanges[j].Count;
                    if (RangeEnd > MergedEnd)
                    {
                        MergedEnd = RangeEnd;
                    }

                    j++;
                }

                GetCommandList().UpdateResidency(Heap->GetResidencyHandle());
                GetCommandList().UpdateResidency(Heap->GetReadbackResource()->GetResidencyHandle());

                GetCommandList()->ResolveQueryData(
                    Heap->GetD3D12QueryHeap(),
                    GetResolveQueryType(Heap->QueryHeapType),
                    MergedStart,
                    MergedEnd - MergedStart,
                    Heap->GetReadbackResource()->GetD3D12Resource(),
                    MergedStart * Heap->GetQuerySize()
                );

                i = j;
            }
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

        Commands->PendingQueries = Move(PendingQueries);

        FD3D12DeviceRHI::Get()->FlushDeletionQueue(Commands);

        Commands->Queue->SubmitCommands(Commands);
        Commands = nullptr;
    }
    else
    {
        PendingBarriers.Clear();
        PendingResourceStates.Clear();
    }

    ContextState.ResetStateForNewCommandList();
}

void FD3D12CommandContext::SplitCommandList(bool bFlushAllocator, bool bWaitForQueue)
{
    FinishCommandList(bFlushAllocator, false);

    if (bWaitForQueue)
    {
        FD3D12Fence& Fence = GetDevice()->GetQueue(QueueType)->GetSubmissionFence();
        Fence.WaitForValue(Fence.GetLastSignaledValue());
    }

    ObtainCommandList();
}

void FD3D12CommandContext::SplitCommandListAndResetState(bool bFlushAllocator, bool bWaitForQueue)
{
    FinishCommandList(bFlushAllocator, false);

    if (bWaitForQueue)
    {
        FD3D12Fence& Fence = GetDevice()->GetQueue(QueueType)->GetSubmissionFence();
        Fence.WaitForValue(Fence.GetLastSignaledValue());
    }

    ContextState.ResetState();
    ObtainCommandList();
}

void FD3D12CommandContext::SplitCommandListForDescriptorHeapRollover()
{
    SplitCommandList(true, true);
    FD3D12DeviceRHI::Get()->FlushCompletedSubmissions();
}

void FD3D12CommandContext::BeginFrame()
{
    FD3D12DeviceRHI::Get()->BeginFrame(this);
}

void FD3D12CommandContext::EndFrame()
{
    FD3D12DeviceRHI::Get()->EndFrame();
}

void FD3D12CommandContext::StartContext()
{
    // -------------------------------------------------------------------------------------------
    // NOTE: This context is intended to be used from a single thread. The lock only enforces 
    // that the same thread which starts the context is the one that later finishes it. Once 
    // the codebase guarantees single-threaded use per context, this lock can be removed.
    // -------------------------------------------------------------------------------------------

    CommandContextCS.Lock();

    // -------------------------------------------------------------------------------------------
    // Phase Transition: Finished -> Recording
    // -------------------------------------------------------------------------------------------

    bIsRecording = true;

    // -------------------------------------------------------------------------------------------
    // Clear cached bindings, barriers, and any transient state accumulated in the previous 
    // frame/phase.
    // -------------------------------------------------------------------------------------------

    ContextState.ResetState();
    EventStack.Clear();

    // -------------------------------------------------------------------------------------------
    // Acquire/allocate a fresh command list so the caller can immediately begin recording 
    // GPU work in this context.
    // -------------------------------------------------------------------------------------------

    ObtainCommandList();
}

void FD3D12CommandContext::FinishContext()
{
    // -------------------------------------------------------------------------------------------
    // Phase Validation
    // -------------------------------------------------------------------------------------------

    CHECK(bIsRecording);

    // -------------------------------------------------------------------------------------------
    // Close and submit the active command-list. Unlike Vulkan, D3D12 does not maintain a 
    // persistent command-pool that accumulates allocations across frames: every 
    // FinishCommandList(true) hands the paired ID3D12CommandAllocator to the submitted 
    // FD3D12Commands, which recycles (and Reset()s) it after GPU completion.
    // -------------------------------------------------------------------------------------------

    FinishCommandList(true);

    // -------------------------------------------------------------------------------------------
    // Phase Transition: Recording -> Finished
    // -------------------------------------------------------------------------------------------

    bIsRecording = false;

    // -------------------------------------------------------------------------------------------
    // See note in StartContext(): once guaranteed single-threaded use is enforced by design, 
    // this lock can be removed.
    // -------------------------------------------------------------------------------------------

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

        Memory::Memcpy(BufferData + BufferRegion.Offset, SrcData, BufferRegion.Size);

        const D3D12_RANGE WrittenRange =
        {
            BufferRegion.Offset,
            BufferRegion.Offset + BufferRegion.Size
        };

        Resource->UnmapRange(0, &WrittenRange);
    }
    else
    {
        FD3D12ResourceStorage ResourceStorage(GetDevice());

        void* AllocatedBytes = GetDevice()->GetStagingBufferAllocator()->Allocate(BufferRegion.Size, 1, ResourceStorage);
        if (AllocatedBytes == nullptr || ResourceStorage.GetResource() == nullptr || ResourceStorage.GetMappedBaseAddress() == nullptr)
        {
            D3D12_ERROR_CRITICAL("Upload allocation failed");
            return;
        }

        Memory::Memcpy(ResourceStorage.GetMappedBaseAddress(), SrcData, BufferRegion.Size);

        GetCommandList()->CopyBufferRegion(
            Resource->GetD3D12Resource(), 
            BufferRegion.Offset, 
            ResourceStorage.GetResource()->GetD3D12Resource(), 
            ResourceStorage.GetResourceOffset(), 
            BufferRegion.Size);
    }
}

void FD3D12CommandContext::BeginQuery(FRHIQuery* Query)
{
    FD3D12QueryRHI* D3D12Query = FD3D12DeviceRHI::ResourceCast(Query);
    CHECK(D3D12Query != nullptr);

    const EQueryType Type = D3D12Query->GetType();
    if (Type == EQueryType::Occlusion)
    {
        OcclusionQueryAllocator.Allocate(
            D3D12Query->CurrentQuery, 
            D3D12Query->QueryResult, 
            ED3D12QueryType::Occlusion);
    }
    else if (Type == EQueryType::PipelineStatistics)
    {
        PipelineStatsQueryAllocator.Allocate(
            D3D12Query->CurrentQuery, 
            D3D12Query->QueryResult, 
            ED3D12QueryType::PipelineStatistics);
    }
    else
    {
        D3D12_ERROR_CRITICAL("BeginQuery is not supported for this query type");
        return;
    }

    if (!D3D12Query->CurrentQuery.IsValid())
    {
        D3D12_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    GetCommandList().BeginQuery(D3D12Query->CurrentQuery);
    PendingQueries.Add(D3D12Query);
    
    ActiveQueryCount++;
}

void FD3D12CommandContext::EndQuery(FRHIQuery* Query)
{
    FD3D12QueryRHI* D3D12Query = FD3D12DeviceRHI::ResourceCast(Query);
    CHECK(D3D12Query != nullptr);

    if (!D3D12Query->CurrentQuery.IsValid())
    {
        D3D12_ERROR_CRITICAL("EndQuery called on query with no allocation");
        return;
    }

    ActiveQueryCount--;
    CHECK(ActiveQueryCount >= 0);

    GetCommandList().EndQuery(D3D12Query->CurrentQuery);
}

void FD3D12CommandContext::QueryTimestamp(FRHIQuery* Query)
{
    FD3D12QueryRHI* D3D12Query = FD3D12DeviceRHI::ResourceCast(Query);
    CHECK(D3D12Query != nullptr);

    if (!TimingQueryAllocator.Allocate(D3D12Query->CurrentQuery, D3D12Query->QueryResult, ED3D12QueryType::Timestamp))
    {
        D3D12_ERROR_CRITICAL("Failed to allocate timestamp query");
        return;
    }

    GetCommandList().EndQuery(D3D12Query->CurrentQuery);
    PendingQueries.Add(D3D12Query);
}

void FD3D12CommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const FVector4& ClearColor)
{
    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12RenderTargetViewRHI* D3D12RenderTargetView = FD3D12DeviceRHI::ResourceCast(RenderTargetView);
    CHECK(D3D12RenderTargetView != nullptr);

    GetCommandList()->ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), ClearColor.XYZW, 0, nullptr);
}

void FD3D12CommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12DepthStencilViewRHI* D3D12DepthStencilView = FD3D12DeviceRHI::ResourceCast(DepthStencilView);
    CHECK(D3D12DepthStencilView != nullptr);

    D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
    if (IsStencilFormat(D3D12DepthStencilView->GetD3D12Desc().Format))
    {
        ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
    }

    GetCommandList()->ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), ClearFlags, Depth, Stencil, 0, nullptr);
}

void FD3D12CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12DeviceRHI::ResourceCast(UnorderedAccessView);
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
    GetDevice()->GetD3D12Device()->CopyDescriptorsSimple(
        1, 
        OnlineHandleCPU, 
        D3D12UnorderedAccessView->GetOfflineHandle(), 
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandleGPU = ResourceHeap.GetGPUHandle(HandeOffset);
    GetCommandList().UpdateResidency(D3D12UnorderedAccessView->GetResourceResidencyHandle());
    GetCommandList()->ClearUnorderedAccessViewFloat(
        OnlineHandleGPU,
        D3D12UnorderedAccessView->GetOfflineHandle(),
        D3D12UnorderedAccessView->GetViewResource()->GetD3D12Resource(),
        ClearColor.XYZW,
        0,
        nullptr);
}

void FD3D12CommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
    FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12DeviceRHI::ResourceCast(UnorderedAccessView);
    CHECK(D3D12UnorderedAccessView != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12LocalDescriptorHeap& ResourceHeap = ContextState.GetDescriptorCache().GetResourceHeap();
    if (!ResourceHeap.HasSpace(1))
    {
        ResourceHeap.Realloc();
        CHECK(ResourceHeap.HasSpace(1));
    }

    ContextState.GetDescriptorCache().SetDescriptorHeaps();

    const uint32 HandeOffset = ResourceHeap.AllocateHandles(1);

    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandleCPU = ResourceHeap.GetCPUHandle(HandeOffset);
    GetDevice()->GetD3D12Device()->CopyDescriptorsSimple(
        1, 
        OnlineHandleCPU, 
        D3D12UnorderedAccessView->GetOfflineHandle(), 
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandleGPU = ResourceHeap.GetGPUHandle(HandeOffset);
    GetCommandList().UpdateResidency(D3D12UnorderedAccessView->GetResourceResidencyHandle());
    GetCommandList()->ClearUnorderedAccessViewUint(
        OnlineHandleGPU,
        D3D12UnorderedAccessView->GetOfflineHandle(),
        D3D12UnorderedAccessView->GetViewResource()->GetD3D12Resource(),
        Values,
        0,
        nullptr);
}

void FD3D12CommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12RenderTargetViewRHI* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    FD3D12DepthStencilViewRHI* DepthStencilView = nullptr;

    for (uint32 Index = 0; Index < BeginRenderPassDesc.NumRenderTargets; ++Index)
    {
        const FRHIRenderPassAttachment& CurrentAttachment = BeginRenderPassDesc.RenderTargets[Index];
        if (FD3D12RenderTargetViewRHI* CurrentRenderTargetView = FD3D12DeviceRHI::ResourceCast(CurrentAttachment.View))
        {
            // Clear the RenderTarget here, since we expect it to be cleared when the RenderPass begin, however
            // it is not certain that there will be a call to draw inside of the RenderPass

            if (CurrentAttachment.LoadAction == EAttachmentLoadAction::Clear)
            {
                GetCommandList()->ClearRenderTargetView(
                    CurrentRenderTargetView->GetOfflineHandle(), 
                    CurrentAttachment.ClearValue.RGBA, 
                    0, 
                    nullptr);
            }
            
            RenderTargetViews[Index] = CurrentRenderTargetView;
        }
        else
        {
            RenderTargetViews[Index] = nullptr;
        }
    }

    const FRHIDepthStencilAttachment& CurrentDSAttachment = BeginRenderPassDesc.DepthStencilAttachment;
    if (FD3D12DepthStencilViewRHI* CurrentDepthStencilView = FD3D12DeviceRHI::ResourceCast(CurrentDSAttachment.View))
    {
        // Clear the DepthStencil here, since we expect it to be cleared when the RenderPass begin, however
        // it is not certain that there will be a call to draw inside of the RenderPass

        if (CurrentDSAttachment.LoadAction == EAttachmentLoadAction::Clear)
        {
            D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
            if (IsStencilFormat(CurrentDepthStencilView->GetD3D12Desc().Format))
            {
                ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
            }

            GetCommandList()->ClearDepthStencilView(
                CurrentDepthStencilView->GetOfflineHandle(),
                ClearFlags,
                CurrentDSAttachment.ClearValue.Depth,
                static_cast<uint8>(CurrentDSAttachment.ClearValue.Stencil),
                0,
                nullptr);
        }

        DepthStencilView = CurrentDepthStencilView;
    }

    ContextState.SetRenderTargets(RenderTargetViews, BeginRenderPassDesc.NumRenderTargets, DepthStencilView);

    // ShadingRate
    FD3D12TextureRHI* ShadingRateImage = FD3D12DeviceRHI::ResourceCast(BeginRenderPassDesc.ShadingRateTexture);
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

void FD3D12CommandContext::SetStencilRef(uint32 StencilRef)
{
    ContextState.SetStencilRef(StencilRef);
}

void FD3D12CommandContext::SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias)
{
    ContextState.SetDepthBias(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
}

void FD3D12CommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    ContextState.SetStreamOutputTargets(Buffers, Offsets);
}

void FD3D12CommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FD3D12BufferRHI* D3DVertexBuffer = FD3D12DeviceRHI::ResourceCast(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(D3DVertexBuffer, BufferSlot + Index);
    }
}

void FD3D12CommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FD3D12BufferRHI* D3DIndexBuffer = FD3D12DeviceRHI::ResourceCast(IndexBuffer);
    ContextState.SetIndexBuffer(D3DIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FD3D12CommandContext::SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FD3D12GraphicsPipelineStateRHI* GraphicsPipelineState = FD3D12DeviceRHI::ResourceCast(PipelineState);
    ContextState.SetGraphicsPipelineState(GraphicsPipelineState);
}

void FD3D12CommandContext::SetComputePipelineState(class FRHIComputePipelineState* PipelineState)
{
    FD3D12ComputePipelineStateRHI* ComputePipelineState = FD3D12DeviceRHI::ResourceCast(PipelineState);
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

    FD3D12ShaderResourceViewRHI* D3D12ShaderResourceView = FD3D12DeviceRHI::ResourceCast(ShaderResourceView);
    
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
        FD3D12ShaderResourceViewRHI* D3D12ShaderResourceView = FD3D12DeviceRHI::ResourceCast(InShaderResourceViews[Index]);
        ContextState.SetSRV(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12DeviceRHI::ResourceCast(UnorderedAccessView);
    
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
        FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12DeviceRHI::ResourceCast(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    FD3D12BufferRHI* D3D12Buffer = ConstantBuffer ? FD3D12DeviceRHI::ResourceCast(ConstantBuffer) : nullptr;
    
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
        FD3D12BufferRHI* D3D12Buffer = InConstantBuffers[Index] ? FD3D12DeviceRHI::ResourceCast(InConstantBuffers[Index]) : nullptr;
        ContextState.SetCBV(D3D12Buffer, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    CHECK(D3D12Shader != nullptr);

    FD3D12SamplerStateRHI* D3D12SamplerState = FD3D12DeviceRHI::ResourceCast(SamplerState);

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
        FD3D12SamplerStateRHI* D3D12SamplerState = FD3D12DeviceRHI::ResourceCast(InSamplerStates[Index]);
        ContextState.SetSampler(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D12CommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12TextureRHI* D3D12Source      = FD3D12DeviceRHI::ResourceCast(Src);
    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);

    const DXGI_FORMAT DstFormat = D3D12CastShaderResourceFormat(D3D12Destination->GetDXGIFormat());
    const DXGI_FORMAT SrcFormat = D3D12CastShaderResourceFormat(D3D12Source->GetDXGIFormat());

    if (DstFormat != SrcFormat)
    {
        D3D12_ERROR("Dst and Src must have compatible formats for resolve");
        return;
    }

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    GetCommandList()->ResolveSubresource(
        D3D12Destination->GetResource()->GetD3D12Resource(),
        0,
        D3D12Source->GetResource()->GetD3D12Resource(),
        0,
        DstFormat);
}

void FD3D12CommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (!BufferRegion.Size)
    {
        return;
    }

    FD3D12BufferRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    if (D3D12Destination->GetDesc().IsTransient())
    {
        FD3D12ResourceStorage& Storage = D3D12Destination->GetResourceStorage();

        void* MappedPtr = nullptr;
        if (D3D12Destination->GetDesc().IsConstantBuffer())
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

        Memory::Memcpy(MappedPtr, SrcData, BufferRegion.Size);

        D3D12Destination->ResourceRelocated(&Storage);
    }
    else
    {
        const FBufferRegion AdjustedRegion(BufferRegion.Offset + D3D12Destination->GetResourceStorage().GetResourceOffset(), BufferRegion.Size);
        UpdateBuffer(D3D12Destination->GetResource(), AdjustedRegion, SrcData);
    }
}

void FD3D12CommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    CHECK(SrcData != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
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

    void* AllocatedBytes = GetDevice()->GetStagingBufferAllocator()->Allocate(AlignedSize, Alignment, ResourceStorage);
    if (AllocatedBytes == nullptr || ResourceStorage.GetMappedBaseAddress() == nullptr || ResourceStorage.GetResource() == nullptr)
    {
        D3D12_ERROR_CRITICAL("Upload allocation failed");
        return;
    }

    uint8*       WritePtr = reinterpret_cast<uint8*>(ResourceStorage.GetMappedBaseAddress());
    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    
    for (uint64 y = 0; y < NumRows; y++)
    {
        Memory::Memcpy(WritePtr, Source, SrcRowPitch);
        
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

    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.pResource        = D3D12Resource->GetD3D12Resource();
    DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestLocation.SubresourceIndex = MipLevel;

    GetCommandList()->CopyTextureRegion(&DestLocation, TextureRegion.PositionX, TextureRegion.PositionY, 0, &SourceLocation, nullptr);
}

void FD3D12CommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    CHECK(SrcData != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Resource* D3D12Resource = D3D12Destination->GetResource();
    CHECK(D3D12Resource != nullptr);

    D3D12_RESOURCE_DESC Desc = D3D12Resource->GetDesc();
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) != 0)
    {
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

    void* AllocatedBytes = GetDevice()->GetStagingBufferAllocator()->Allocate(AlignedSize, Alignment, ResourceStorage);
    if (AllocatedBytes == nullptr || ResourceStorage.GetMappedBaseAddress() == nullptr || ResourceStorage.GetResource() == nullptr)
    {
        D3D12_ERROR_CRITICAL("Upload allocation failed");
        return;
    }

    uint8* WritePtr = reinterpret_cast<uint8*>(ResourceStorage.GetMappedBaseAddress());
    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);

    const uint32 DstSlicePitch = PlacedSubresourceFootprint.Footprint.RowPitch * NumRows;
    for (uint32 z = 0; z < TextureRegion.Depth; z++)
    {
        const uint8* SliceSource = Source + z * SrcDepthPitch;
        uint8* SliceDest = WritePtr + z * DstSlicePitch;

        for (uint32 y = 0; y < NumRows; y++)
        {
            Memory::Memcpy(SliceDest, SliceSource, SrcRowPitch);

            SliceDest   += PlacedSubresourceFootprint.Footprint.RowPitch;
            SliceSource += SrcRowPitch;
        }
    }

    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
    SourceLocation.pResource                          = ResourceStorage.GetResource()->GetD3D12Resource();
    SourceLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SourceLocation.PlacedFootprint.Offset             = ResourceStorage.GetResourceOffset();
    SourceLocation.PlacedFootprint.Footprint.Format   = Desc.Format;
    SourceLocation.PlacedFootprint.Footprint.Width    = TextureRegion.Width;
    SourceLocation.PlacedFootprint.Footprint.Height   = TextureRegion.Height;
    SourceLocation.PlacedFootprint.Footprint.Depth    = TextureRegion.Depth;
    SourceLocation.PlacedFootprint.Footprint.RowPitch = PlacedSubresourceFootprint.Footprint.RowPitch;

    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.pResource        = D3D12Resource->GetD3D12Resource();
    DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestLocation.SubresourceIndex = MipLevel;

    GetCommandList()->CopyTextureRegion(&DestLocation, TextureRegion.PositionX, TextureRegion.PositionY, TextureRegion.PositionZ, &SourceLocation, nullptr);
}

void FD3D12CommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12BufferRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12BufferRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    const uint64 DstOffset = CopyDesc.DstOffset + D3D12Destination->GetResourceStorage().GetResourceOffset();
    const uint64 SrcOffset = CopyDesc.SrcOffset + D3D12Source->GetResourceStorage().GetResourceOffset();
    
    GetCommandList()->CopyBufferRegion(
        D3D12Destination->GetResource()->GetD3D12Resource(),
        DstOffset,
        D3D12Source->GetResource()->GetD3D12Resource(),
        SrcOffset,
        CopyDesc.Size);
}

void FD3D12CommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    GetCommandList()->CopyResource(D3D12Destination->GetResource()->GetD3D12Resource(), D3D12Source->GetResource()->GetD3D12Resource());
}

void FD3D12CommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& InCopyDesc)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    {
        const ETextureDimension SrcDimension = Src->GetDesc().Dimension;
        const ETextureDimension DstDimension = Dst->GetDesc().Dimension;
        
        const uint32 SrcNumArrayLayers = RHIDimensionArrayLayers(SrcDimension, Src->GetDesc().NumArraySlices);
        const uint32 DstNumArrayLayers = RHIDimensionArrayLayers(DstDimension, Dst->GetDesc().NumArraySlices);
        CHECK(InCopyDesc.SrcArraySlice + InCopyDesc.NumArraySlices <= SrcNumArrayLayers);
        CHECK(InCopyDesc.DstArraySlice + InCopyDesc.NumArraySlices <= DstNumArrayLayers);
    }

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);
    
    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    BarrierBatcher.FlushBarriers(GetCommandList());

    const ETextureDimension TextureDimension = Src->GetDesc().Dimension;

    const uint32 NumArraySlices    = InCopyDesc.NumArraySlices;
    const uint32 SrcArraySlice     = InCopyDesc.SrcArraySlice;
    const uint32 DstArraySlice     = InCopyDesc.DstArraySlice;
    const uint32 NumSrcArraySlices = RHIDimensionArrayLayers(TextureDimension, Src->GetDesc().NumArraySlices);
    const uint32 NumDstArraySlices = RHIDimensionArrayLayers(Dst->GetDesc().Dimension, Dst->GetDesc().NumArraySlices);

    for (uint32 ArraySlice = 0; ArraySlice < NumArraySlices; ArraySlice++)
    {
        for (uint32 MipLevel = 0; MipLevel < InCopyDesc.NumMipLevels; MipLevel++)
        {
            const uint32 SrcSubresourceIndex = D3D12CalculateSubresource(
                InCopyDesc.SrcMipSlice + MipLevel,
                SrcArraySlice + ArraySlice,
                0,
                Src->GetDesc().NumMipLevels,
                NumSrcArraySlices);

            const uint32 DstSubresourceIndex = D3D12CalculateSubresource(
                InCopyDesc.DstMipSlice + MipLevel,
                DstArraySlice + ArraySlice,
                0,
                Dst->GetDesc().NumMipLevels,
                NumDstArraySlices);

            // Source
            D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
            SourceLocation.pResource        = D3D12Source->GetResource()->GetD3D12Resource();
            SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            SourceLocation.SubresourceIndex = SrcSubresourceIndex;

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
            DestLocation.SubresourceIndex = DstSubresourceIndex;

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
 
    FD3D12BufferRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst); 
    CHECK(D3D12Destination != nullptr); 
 
    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src); 
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());

    const uint32 BytesPerPixel = GetByteStrideFromFormat(Src->GetDesc().Format);
    if (BytesPerPixel == 0 || IsBlockCompressed(Src->GetDesc().Format))
    {
        D3D12_ERROR("CopyTextureRegionToBuffer requires a non-block-compressed, supported format. SrcFormat=%s", ToString(Src->GetDesc().Format));
        return;
    }

    FD3D12Resource* DstResource = D3D12Destination->GetResource();
    CHECK(DstResource != nullptr);

    const ETextureDimension TextureDimension = Src->GetDesc().Dimension;

    const uint32 NumArraySlices = RHIDimensionArrayLayers(TextureDimension, Src->GetDesc().NumArraySlices);
    const uint32 SrcSubresource = D3D12CalculateSubresource(SrcMipLevel, 0, 0, Src->GetDesc().NumMipLevels, NumArraySlices);

    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
    SourceLocation.pResource        = D3D12Source->GetResource()->GetD3D12Resource();
    SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    SourceLocation.SubresourceIndex = SrcSubresource;

    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.pResource                        = DstResource->GetD3D12Resource();
    DestLocation.Type                             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DestLocation.PlacedFootprint.Offset           = DstOffset;
    DestLocation.PlacedFootprint.Footprint.Format = ConvertFormat(Src->GetDesc().Format);

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

void FD3D12CommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    {
        const ETextureDimension SrcDimension = Src->GetDesc().Dimension;
        const uint32 SrcNumArrayLayers = RHIDimensionArrayLayers(SrcDimension, Src->GetDesc().NumArraySlices);
        CHECK(SrcArraySlice < SrcNumArrayLayers);
    }

    BarrierBatcher.FlushBarriers(GetCommandList());

    if ((DstOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) != 0)
    {
        D3D12_ERROR("CopyTextureSubresourceToBuffer requires DstOffset aligned to %u bytes. Offset=%llu", D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, DstOffset);
        return;
    }

    FD3D12BufferRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    const uint32 BytesPerPixel = GetByteStrideFromFormat(Src->GetDesc().Format);
    if (BytesPerPixel == 0 || IsBlockCompressed(Src->GetDesc().Format))
    {
        D3D12_ERROR("CopyTextureSubresourceToBuffer requires a non-block-compressed, supported format. SrcFormat=%s", ToString(Src->GetDesc().Format));
        return;
    }

    FD3D12Resource* DstResource = D3D12Destination->GetResource();
    CHECK(DstResource != nullptr);

    const ETextureDimension TextureDimension = Src->GetDesc().Dimension;

    const uint32 NumArraySlices = RHIDimensionArrayLayers(TextureDimension, Src->GetDesc().NumArraySlices);
    const uint32 SrcSubresource = D3D12CalculateSubresource(SrcMipLevel, SrcArraySlice, 0, Src->GetDesc().NumMipLevels, NumArraySlices);

    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
    SourceLocation.pResource        = D3D12Source->GetResource()->GetD3D12Resource();
    SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    SourceLocation.SubresourceIndex = SrcSubresource;

    const uint32 CopyWidth  = SrcRegion.Width;
    const uint32 CopyHeight = Math::Max(SrcRegion.Height, 1u);
    const uint32 CopyDepth  = Math::Max(SrcRegion.Depth, 1u);
    const uint32 RowPitch   = Math::AlignUp<uint32>(BytesPerPixel * CopyWidth, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.pResource                          = DstResource->GetD3D12Resource();
    DestLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DestLocation.PlacedFootprint.Offset             = DstOffset;
    DestLocation.PlacedFootprint.Footprint.Format   = ConvertFormat(Src->GetDesc().Format);
    DestLocation.PlacedFootprint.Footprint.Width    = CopyWidth;
    DestLocation.PlacedFootprint.Footprint.Height   = CopyHeight;
    DestLocation.PlacedFootprint.Footprint.Depth    = CopyDepth;
    DestLocation.PlacedFootprint.Footprint.RowPitch = RowPitch;

    D3D12_BOX SourceBox = {};
    SourceBox.left   = SrcRegion.PositionX;
    SourceBox.right  = SrcRegion.PositionX + CopyWidth;
    SourceBox.top    = SrcRegion.PositionY;
    SourceBox.bottom = SrcRegion.PositionY + CopyHeight;
    SourceBox.front  = SrcRegion.PositionZ;
    SourceBox.back   = SrcRegion.PositionZ + CopyDepth;

    GetCommandList()->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, &SourceBox);
}

void FD3D12CommandContext::WriteFence(FRHIFence* Fence)
{
    CHECK(Fence != nullptr);

    FD3D12FenceRHI* D3D12Fence = FD3D12DeviceRHI::ResourceCast(Fence);

    SplitCommandList(true, false);
    D3D12Fence->Signal(GetDevice()->GetD3D12CommandQueue(QueueType));
}

void FD3D12CommandContext::DiscardContents(FRHITexture* Texture)
{
    if (FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(Texture))
    {
        GetCommandList()->DiscardResource(D3D12Texture->GetResource()->GetD3D12Resource(), nullptr);
    }
}

void FD3D12CommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    CHECK(RayTracingScene != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12SceneAccelerationStructureRHI* D3D12RayTracingScene = FD3D12DeviceRHI::ResourceCast(RayTracingScene);
    D3D12RayTracingScene->Build(*this, BuildDesc);
}

void FD3D12CommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    CHECK(RayTracingGeometry != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12GeometryAccelerationStructureRHI* D3D12RayTracingGeometry = FD3D12DeviceRHI::ResourceCast(RayTracingGeometry);
    D3D12RayTracingGeometry->Build(*this, BuildDesc);
}

void FD3D12CommandContext::SetRayTracingBindings(FRHISceneAccelerationStructure* /* RayTracingScene */, FRHIRayTracingPipelineState* /* PipelineState */, const FRayTracingShaderResources* /* GlobalResource */, const FRayTracingShaderResources* /* RayGenLocalResources */, const FRayTracingShaderResources* /* MissLocalResources */, const FRayTracingShaderResources* /* HitGroupResources */, uint32 /* NumHitGroupResources */)
{
#if 0
    FD3D12SceneAccelerationStructureRHI* D3D12Scene = FD3D12DeviceRHI::ResourceCast(RayTracingScene);
    D3D12_ERROR_COND(D3D12Scene != nullptr, "RayTracingScene cannot be nullptr");

    FD3D12RayTracingPipelineStateRHI* D3D12PipelineState = FD3D12DeviceRHI::ResourceCast(PipelineState);
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
                FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(GlobalResource->ConstantBuffers[i]);
                ContextState.SetCBV(D3D12Buffer, EShaderVisibility::All, i);
            }
        }
        
        if (!GlobalResource->ShaderResourceViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ShaderResourceViews.Size(); i++)
            {
                FD3D12ShaderResourceViewRHI* D3D12ShaderResourceView = FD3D12DeviceRHI::ResourceCast(GlobalResource->ShaderResourceViews[i]);
                ContextState.DescriptorCache.SetShaderResourceView(EShaderVisibility::All, D3D12ShaderResourceView, i);
            }
        }
        
        if (!GlobalResource->UnorderedAccessViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->UnorderedAccessViews.Size(); i++)
            {
                FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12DeviceRHI::ResourceCast(GlobalResource->UnorderedAccessViews[i]);
                ContextState.DescriptorCache.SetUnorderedAccessView(EShaderVisibility::All, D3D12UnorderedAccessView, i);
            }
        }

        if (!GlobalResource->SamplerStates.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->SamplerStates.Size(); i++)
            {
                FD3D12SamplerStateRHI* DxSampler = FD3D12DeviceRHI::ResourceCast(GlobalResource->SamplerStates[i]);
                ContextState.DescriptorCache.SetSamplerState(EShaderVisibility::All, DxSampler, i);
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
    FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);

    {
        const ETextureDimension Dimension = Texture->GetDesc().Dimension;
        const uint32 NumArrayLayers = RHIDimensionArrayLayers(Dimension, Texture->GetDesc().NumArraySlices);
        CHECK(TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES || TextureTransition.ArraySlice < NumArrayLayers);
    }

    FD3D12Resource* Resource = D3D12Texture->GetResource();

    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(TextureTransition.BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(TextureTransition.AfterState);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    if (TextureTransition.MipLevel != RHI_ALL_MIP_LEVELS || TextureTransition.ArraySlice != RHI_ALL_ARRAY_SLICES)
    {
        const D3D12_RESOURCE_DESC& ResourceDesc = Resource->GetDesc();
        
        const uint32 NumMipLevels   = ResourceDesc.MipLevels;
        const uint32 NumArraySlices = ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? ResourceDesc.DepthOrArraySize : 1u;

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
                    if (CurrentState != D3D12BeforeState)
                    {
                        D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *FString::CreateFormatted("array-slice loop, slice=%u, mip=%u", ArraySlice, TextureTransition.MipLevel),
                            TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                        CHECK(CurrentState == D3D12BeforeState);
                    }
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
                    if (CurrentState != D3D12BeforeState)
                    {
                        D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *FString::CreateFormatted("mip loop, slice=%u, mip=%u", TextureTransition.ArraySlice, MipLevel),
                            TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                        CHECK(CurrentState == D3D12BeforeState);
                    }
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
                if (CurrentState != D3D12BeforeState)
                {
                    D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *FString::CreateFormatted("single subresource, slice=%u, mip=%u", TextureTransition.ArraySlice, TextureTransition.MipLevel),
                        TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                    CHECK(CurrentState == D3D12BeforeState);
                }
            }

            BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
            LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
        }
    }
    else
    {
        if (LocalState.AreAllSubresourcesSameState())
        {
            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, D3D12BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            }
            else
            {
                if (CurrentState != D3D12BeforeState)
                {
                    D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, "all-subresources (uniform state)",
                        TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                    CHECK(CurrentState == D3D12BeforeState);
                }
            }

            BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState);
        }
        else
        {
            for (uint32 i = 0; i < LocalState.GetNumSubresources(); i++)
            {
                const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(i);
                if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
                {
                    AddPendingBarrier(Resource, D3D12BeforeState, i);
                }
                else
                {
                    if (CurrentState != D3D12BeforeState)
                    {
                        D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *FString::CreateFormatted("per-subresource (divergent), subresource=%u", i),
                            TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                        CHECK(CurrentState == D3D12BeforeState);
                    }
                }

                BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, i);
            }
        }

        LocalState.SetResourceState(D3D12AfterState);
    }
}

void FD3D12CommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    FD3D12Resource* Resource = D3D12Buffer->GetResource();

    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);

    const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();
    if (CurrentState == D3D12AfterState)
    {
        return;
    }

    if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
    {
        AddPendingBarrier(Resource, D3D12BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    }
    else
    {
        CHECK(Resource->HasDefaultState() || CurrentState == D3D12BeforeState);
    }

    const D3D12_RESOURCE_STATES EffectiveBeforeState = (CurrentState != D3D12_RESOURCE_STATE_TO_BE_DETERMINED) ? CurrentState : D3D12BeforeState;
    BarrierBatcher.AddTransitionBarrier(Resource, EffectiveBeforeState, D3D12AfterState);
    LocalState.SetResourceState(D3D12AfterState);
}

void FD3D12CommandContext::TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState)
{
    CHECK(Resource != nullptr);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    if (LocalState.AreAllSubresourcesSameState())
    {
        const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();
        if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            AddPendingBarrier(Resource, AfterState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        }
        else
        {
            BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, AfterState);
        }

        LocalState.SetResourceState(AfterState);
    }
    else
    {
        for (uint32 i = 0; i < LocalState.GetNumSubresources(); i++)
        {
            const D3D12_RESOURCE_STATES SubresourceState = LocalState.GetSubresourceState(i);
            if (SubresourceState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, AfterState, i);
            }
            else
            {
                BarrierBatcher.AddTransitionBarrier(Resource, SubresourceState, AfterState, i);
            }

            LocalState.SetSubresourceState(i, AfterState);
        }
    }
}

void FD3D12CommandContext::TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
    CHECK(Resource != nullptr);
    CHECK(BeforeState != D3D12_RESOURCE_STATE_TO_BE_DETERMINED);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    if (LocalState.AreAllSubresourcesSameState())
    {
        const D3D12_RESOURCE_STATES CurrentState = LocalState.GetResourceState();
        if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            AddPendingBarrier(Resource, BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        }

        BarrierBatcher.AddTransitionBarrier(Resource, BeforeState, AfterState);
        LocalState.SetResourceState(AfterState);
    }
    else
    {
        for (uint32 i = 0; i < LocalState.GetNumSubresources(); i++)
        {
            const D3D12_RESOURCE_STATES SubresourceState = LocalState.GetSubresourceState(i);
            if (SubresourceState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, BeforeState, i);
            }

            BarrierBatcher.AddTransitionBarrier(Resource, BeforeState, AfterState, i);
            LocalState.SetSubresourceState(i, AfterState);
        }
    }
}

void FD3D12CommandContext::TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState, uint32 FirstMip, uint32 NumMips, uint32 FirstArraySlice, uint32 NumArraySlices)
{
    CHECK(Resource != nullptr);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    
    const D3D12_RESOURCE_DESC& ResourceDesc = Resource->GetDesc();

    const uint32 MipLevels   = ResourceDesc.MipLevels;
    const uint32 ArraySlices = (ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? ResourceDesc.DepthOrArraySize : 1u;

    for (uint32 Array = FirstArraySlice; Array < FirstArraySlice + NumArraySlices; Array++)
    {
        for (uint32 Mip = FirstMip; Mip < FirstMip + NumMips; Mip++)
        {
            const uint32 SubresourceIndex = D3D12CalculateSubresource(Mip, Array, 0, MipLevels, ArraySlices);

            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(SubresourceIndex);
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, AfterState, SubresourceIndex);
            }
            else
            {
                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, AfterState, SubresourceIndex);
            }

            LocalState.SetSubresourceState(SubresourceIndex, AfterState);
        }
    }
}

void FD3D12CommandContext::TransitionResourceState(FD3D12UnorderedAccessViewRHI* View)
{
    FD3D12Resource* Resource = View->GetViewResource();
    if (!Resource || !Resource->RequiresResourceStateTracking())
    {
        return;
    }

    const D3D12_UNORDERED_ACCESS_VIEW_DESC& ViewDesc = View->GetD3D12Desc();
    switch (ViewDesc.ViewDimension)
    {
        case D3D12_UAV_DIMENSION_TEXTURE1D:
        case D3D12_UAV_DIMENSION_TEXTURE2D:
        case D3D12_UAV_DIMENSION_TEXTURE3D:
        {
            TransitionResourceState(Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, ViewDesc.Texture2D.MipSlice, 1, 0, 1);
            break;
        }

        case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
        {
            TransitionResourceState(
                Resource, 
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
                ViewDesc.Texture2DArray.MipSlice, 
                1, 
                ViewDesc.Texture2DArray.FirstArraySlice, 
                ViewDesc.Texture2DArray.ArraySize);
            break;
        }

        default:
        {
            TransitionResourceState(Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            break;
        }
    }
}

void FD3D12CommandContext::TransitionResourceState(FD3D12ShaderResourceViewRHI* View, D3D12_RESOURCE_STATES State)
{
    FD3D12Resource* Resource = View->GetViewResource();
    if (!Resource || !Resource->RequiresResourceStateTracking())
    {
        return;
    }

    const D3D12_RESOURCE_DESC&             ResourceDesc = Resource->GetDesc();
    const D3D12_SHADER_RESOURCE_VIEW_DESC& ViewDesc     = View->GetD3D12Desc();

    switch (ViewDesc.ViewDimension)
    {
        case D3D12_SRV_DIMENSION_TEXTURE1D:
        case D3D12_SRV_DIMENSION_TEXTURE2D:
        case D3D12_SRV_DIMENSION_TEXTURE3D:
        {
            uint32 NumMips = ViewDesc.Texture2D.MipLevels;
            if (NumMips == uint32(-1))
            {
                NumMips = ResourceDesc.MipLevels - ViewDesc.Texture2D.MostDetailedMip;
            }

            TransitionResourceState(Resource, State, ViewDesc.Texture2D.MostDetailedMip, NumMips, 0, 1);
            break;
        }

        case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        {
            uint32 NumMips = ViewDesc.Texture2DArray.MipLevels;
            if (NumMips == uint32(-1))
            {
                NumMips = ResourceDesc.MipLevels - ViewDesc.Texture2DArray.MostDetailedMip;
            }
        
            uint32 NumSlices = ViewDesc.Texture2DArray.ArraySize;
            if (NumSlices == uint32(-1))
            {
                NumSlices = ResourceDesc.DepthOrArraySize - ViewDesc.Texture2DArray.FirstArraySlice;
            }

            TransitionResourceState(Resource, State, ViewDesc.Texture2DArray.MostDetailedMip, NumMips, ViewDesc.Texture2DArray.FirstArraySlice, NumSlices);
            break;
        }

        case D3D12_SRV_DIMENSION_TEXTURECUBE:
        {
            uint32 NumMips = ViewDesc.TextureCube.MipLevels;
            if (NumMips == uint32(-1))
            {
                NumMips = ResourceDesc.MipLevels - ViewDesc.TextureCube.MostDetailedMip;
            }

            TransitionResourceState(Resource, State, ViewDesc.TextureCube.MostDetailedMip, NumMips, 0, RHI_NUM_CUBE_FACES);
            break;
        }

        case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
        {
            uint32 NumMips = ViewDesc.TextureCubeArray.MipLevels;
            if (NumMips == uint32(-1))
            {
                NumMips = ResourceDesc.MipLevels - ViewDesc.TextureCubeArray.MostDetailedMip;
            }

            TransitionResourceState(
                Resource, 
                State, 
                ViewDesc.TextureCubeArray.MostDetailedMip, 
                NumMips, 
                ViewDesc.TextureCubeArray.First2DArrayFace, 
                RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, ViewDesc.TextureCubeArray.NumCubes));
            break;
        }

        case D3D12_SRV_DIMENSION_TEXTURE2DMS:
        {
            TransitionResourceState(Resource, State, 0, 1, 0, 1);
            break;
        }

        case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
        {
            uint32 NumSlices = ViewDesc.Texture2DMSArray.ArraySize;
            if (NumSlices == uint32(-1))
            {
                NumSlices = ResourceDesc.DepthOrArraySize - ViewDesc.Texture2DMSArray.FirstArraySlice;
            }

            TransitionResourceState(Resource, State, 0, 1, ViewDesc.Texture2DMSArray.FirstArraySlice, NumSlices);
            break;
        }

        default:
        {
            TransitionResourceState(Resource, State);
            break;
        }
    }
}

void FD3D12CommandContext::TransitionResourceState(FD3D12RenderTargetViewRHI* View)
{
    FD3D12Resource* Resource = View->GetViewResource();
    if (!Resource || !Resource->RequiresResourceStateTracking())
    {
        return;
    }

    const D3D12_RENDER_TARGET_VIEW_DESC& ViewDesc = View->GetD3D12Desc();
    switch (ViewDesc.ViewDimension)
    {
        case D3D12_RTV_DIMENSION_TEXTURE1D:
        case D3D12_RTV_DIMENSION_TEXTURE2D:
        case D3D12_RTV_DIMENSION_TEXTURE3D:
        {
            TransitionResourceState(Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, ViewDesc.Texture2D.MipSlice, 1, 0, 1);
            break;
        }

        case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
        {
            TransitionResourceState(
                Resource, 
                D3D12_RESOURCE_STATE_RENDER_TARGET, 
                ViewDesc.Texture2DArray.MipSlice, 
                1, 
                ViewDesc.Texture2DArray.FirstArraySlice, 
                ViewDesc.Texture2DArray.ArraySize);
            break;
        }

        case D3D12_RTV_DIMENSION_TEXTURE2DMS:
        {
            TransitionResourceState(Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, 0, 1, 0, 1);
            break;
        }

        case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
        {
            TransitionResourceState(
                Resource, 
                D3D12_RESOURCE_STATE_RENDER_TARGET, 
                0, 
                1, 
                ViewDesc.Texture2DMSArray.FirstArraySlice, 
                ViewDesc.Texture2DMSArray.ArraySize);
            break;
        }

        default:
        {
            TransitionResourceState(Resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
            break;
        }
    }
}

void FD3D12CommandContext::TransitionResourceState(FD3D12DepthStencilViewRHI* View, D3D12_RESOURCE_STATES State)
{
    FD3D12Resource* Resource = View->GetViewResource();
    if (!Resource || !Resource->RequiresResourceStateTracking())
    {
        return;
    }

    const D3D12_DEPTH_STENCIL_VIEW_DESC& ViewDesc = View->GetD3D12Desc();
    switch (ViewDesc.ViewDimension)
    {
        case D3D12_DSV_DIMENSION_TEXTURE1D:
        case D3D12_DSV_DIMENSION_TEXTURE2D:
        {
            TransitionResourceState(Resource, State, ViewDesc.Texture2D.MipSlice, 1, 0, 1);
            break;
        }

        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
        {
            TransitionResourceState(Resource, State, ViewDesc.Texture2DArray.MipSlice, 1, ViewDesc.Texture2DArray.FirstArraySlice, ViewDesc.Texture2DArray.ArraySize);
            break;
        }

        case D3D12_DSV_DIMENSION_TEXTURE2DMS:
        {
            TransitionResourceState(Resource, State, 0, 1, 0, 1);
            break;
        }

        case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
        {
            TransitionResourceState(Resource, State, 0, 1, ViewDesc.Texture2DMSArray.FirstArraySlice, ViewDesc.Texture2DMSArray.ArraySize);
            break;
        }

	    default:
	    {
		    TransitionResourceState(Resource, State);
		    break;
	    }
    }
}

void FD3D12CommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
    FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);

    {
        const ETextureDimension Dimension = Texture->GetDesc().Dimension;
        const uint32 NumArrayLayers = RHIDimensionArrayLayers(Dimension, Texture->GetDesc().NumArraySlices);
        CHECK(RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES || RequiredState.ArraySlice < NumArrayLayers);
    }

    FD3D12Resource* Resource = D3D12Texture->GetResource();
    CHECK(Resource != nullptr);

    const D3D12_RESOURCE_DESC&  ResourceDesc = Resource->GetDesc();
    const D3D12_RESOURCE_STATES DesiredState = ConvertResourceState(RequiredState.State);
    
    const uint32 NumMipLevels   = ResourceDesc.MipLevels;
    const uint32 NumArraySlices = ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? ResourceDesc.DepthOrArraySize : 1u;
    
    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
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
    FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(Buffer);
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
    FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);

    BarrierBatcher.AddUnorderedAccessBarrier(D3D12Texture->GetResource());
}

void FD3D12CommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    BarrierBatcher.AddUnorderedAccessBarrier(D3D12Buffer->GetResource());
}

void FD3D12CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ConditionalSplitCommandList();

    ContextState.PrepareGraphicsState();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList()->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void FD3D12CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ConditionalSplitCommandList();

    ContextState.PrepareGraphicsState();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList()->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FD3D12CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSplitCommandList();

    ContextState.PrepareGraphicsState();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList()->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSplitCommandList();

    ContextState.PrepareGraphicsState();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList()->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::ConditionalSplitCommandList()
{
    if (ActiveQueryCount > 0)
    {
        return;
    }

    const uint32 MaxCommands = static_cast<uint32>(CVarMaxCommandsPerCommandList.GetValue());
    const uint32 NumCommands = CommandList->GetNumCommands();

    if (NumCommands >= MaxCommands)
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

    ContextState.PrepareComputeState();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindComputeState();

    GetCommandList()->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FD3D12CommandContext::DispatchRays(FRHISceneAccelerationStructure* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    FD3D12SceneAccelerationStructureRHI* D3D12Scene = FD3D12DeviceRHI::ResourceCast(RayTracingScene);
    CHECK(D3D12Scene != nullptr);

    FD3D12RayTracingPipelineStateRHI* D3D12PipelineState = FD3D12DeviceRHI::ResourceCast(PipelineState);
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
    RayDispatchDesc.HitGroupTable             = D3D12Scene->GetHitGroupTable();
    RayDispatchDesc.MissShaderTable           = D3D12Scene->GetMissShaderTable();

    RayDispatchDesc.Width  = Width;
    RayDispatchDesc.Height = Height;
    RayDispatchDesc.Depth  = Depth;

#if D3D12_USE_ID3D12COMMANDLIST_4
    CommandList->GetGraphicsCommandList4()->SetPipelineState1(D3D12PipelineState->GetD3D12StateObject());
    CommandList->GetGraphicsCommandList4()->DispatchRays(&RayDispatchDesc);
#else
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: DispatchRays requires ID3D12GraphicsCommandList4");
#endif
}

void FD3D12CommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    // -------------------------------------------------------------------------------------------
    // Close and submit the active command-list before presenting. In D3D12, 
    // FinishCommandList(true) already hands the paired ID3D12CommandAllocator to the submitted 
    // FD3D12Commands, which enqueues it for recycle/reset after GPU completion.
    // -------------------------------------------------------------------------------------------

    FinishCommandList(true);

    FD3D12SwapChainRHI* D3D12SwapChain = FD3D12DeviceRHI::ResourceCast(SwapChain);
    D3D12SwapChain->Present(bVerticalSync);

    // -------------------------------------------------------------------------------------------
    // Acquire or allocate a fresh command-list so that subsequent GPU work can continue 
    // recording immediately after presenting.
    // -------------------------------------------------------------------------------------------

    ObtainCommandList();
}

void FD3D12CommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    FD3D12SwapChainRHI* D3D12SwapChain = FD3D12DeviceRHI::ResourceCast(SwapChain);
    D3D12SwapChain->Resize(this, Width, Height, Format, ColorSpace);
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

void* FD3D12CommandContext::GetRHINativeCommandList()
{
    return reinterpret_cast<void*>(CommandList->GetCommandList());
}

void FD3D12CommandContext::PushEvent(const FStringView& Name)
{
    EventStack.Emplace(Name.Data());

#if D3D12_ENABLE_PIX_MARKERS
    if (D3D12Functions::PIXBeginEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());
        D3D12Functions::PIXBeginEventOnCommandList(GraphicsCommandList, PIX_COLOR(255, 255, 255), *Name);
    }
#endif
}

void FD3D12CommandContext::PopEvent()
{
    if (!EventStack.IsEmpty())
    {
        EventStack.Pop();
    }

#if D3D12_ENABLE_PIX_MARKERS
    if (D3D12Functions::PIXEndEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());
        D3D12Functions::PIXEndEventOnCommandList(GraphicsCommandList);
    }
#endif
}

void FD3D12CommandContext::CloseEventStack()
{
#if D3D12_ENABLE_PIX_MARKERS
    if (D3D12Functions::PIXEndEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());

        for (int32 i = EventStack.Size() - 1; i >= 0; --i)
        {
            D3D12Functions::PIXEndEventOnCommandList(GraphicsCommandList);
        }
    }
#endif
}

void FD3D12CommandContext::ReopenEventStack()
{
#if D3D12_ENABLE_PIX_MARKERS
    if (D3D12Functions::PIXBeginEventOnCommandList)
    {
        ID3D12GraphicsCommandList* GraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(CommandList->GetCommandList());

        for (int32 i = 0; i < EventStack.Size(); ++i)
        {
            D3D12Functions::PIXBeginEventOnCommandList(GraphicsCommandList, PIX_COLOR(255, 255, 255), *EventStack[i]);
        }
    }
#endif
}
