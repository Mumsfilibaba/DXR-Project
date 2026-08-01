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
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12Loader.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/RayTracing/D3D12RayTracing.h"

#include <pix.h>

static TAutoConsoleVariable<int32> CVarMaxCommandsPerCommandList(
    "D3D12RHI.MaxCommandsPerCommandList",
    "Number of commands allowed before submitting the current CommandList to the GPU",
    10000);

static D3D12_RESOURCE_STATES D3D12ResolveRestingState(const FD3D12Resource* Resource, D3D12_RESOURCE_STATES DesiredState)
{
    // COMMON/PRESENT is a state in its own right and must never be promoted into the resting state.
    if (DesiredState == D3D12_RESOURCE_STATE_COMMON)
    {
        return DesiredState;
    }

    if (Resource->HasDefaultState())
    {
        const D3D12_RESOURCE_STATES Default = Resource->GetDefaultState();
        if ((DesiredState & Default) == DesiredState && DesiredState != Default)
        {
            return Default;
        }
    }

    return DesiredState;
}

static D3D12_GPU_VIRTUAL_ADDRESS GetD3D12AccelerationStructureGPUAddress(FRHIRayTracingAccelerationStructure* AccelerationStructure)
{
    if (!AccelerationStructure)
    {
        return 0;
    }

    switch (AccelerationStructure->GetAccelerationStructureType())
    {
        case ERayTracingAccelerationStructureType::Geometry:
        {
            return static_cast<FD3D12GeometryAccelerationStructureRHI*>(AccelerationStructure)->GetGPUVirtualAddress();
        }

        case ERayTracingAccelerationStructureType::Scene:
        {
            return static_cast<FD3D12SceneAccelerationStructureRHI*>(AccelerationStructure)->GetGPUVirtualAddress();
        }

        default:
        {
            return 0;
        }
    }
}

static uint64 GetRayTracingPostBuildInfoStride(EAccelerationStructurePostBuildInfoType InfoType)
{
    switch (InfoType)
    {
        case EAccelerationStructurePostBuildInfoType::Serialization:
            return 16;

        case EAccelerationStructurePostBuildInfoType::CompactedSize:
        case EAccelerationStructurePostBuildInfoType::CurrentSize:
        case EAccelerationStructurePostBuildInfoType::ToolsVisualization:
        default:
            return 8;
    }
}

void FD3D12BarrierBatcher::AddTransitionBarrier(FD3D12Resource* InResource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex)
{
    CHECK(InResource != nullptr);

#if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    {
        String DebugName;
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
        String DebugName;
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
        return;
    }

    int32 CoveringIndex           = -1;
    bool  bCoveringStatesAllMatch = true;

    for (int32 Index = 0; Index < Barriers.Size(); Index++)
    {
        const D3D12_RESOURCE_BARRIER& Existing = Barriers[Index];
        if (Existing.Type != D3D12_RESOURCE_BARRIER_TYPE_TRANSITION || Existing.Transition.pResource != Resource)
        {
            continue;
        }

        const bool bCoversSubresource = (Existing.Transition.Subresource == SubresourceIndex)
            || (Existing.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
            || (SubresourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

        if (!bCoversSubresource)
        {
            continue;
        }

        if (CoveringIndex >= 0 && Barriers[CoveringIndex].Transition.StateAfter != Existing.Transition.StateAfter)
        {
            bCoveringStatesAllMatch = false;
        }

        CoveringIndex = Index;
    }

    if (CoveringIndex >= 0)
    {
        D3D12_RESOURCE_TRANSITION_BARRIER& Covering = Barriers[CoveringIndex].Transition;
        if (Covering.Subresource == SubresourceIndex)
        {
            // Case 1: Redundant barrier (FirstRange->SecondRange then FirstRange->SecondRange again) => ignore new barrier
            if (Covering.StateBefore == BeforeState && Covering.StateAfter == AfterState)
            {
            #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
                LOG_INFO("  Redundant barrier FirstRange->SecondRange kept (SubresourceIndex=%u, %s->%s)",SubresourceIndex, ToString(BeforeState), ToString(AfterState));
            #endif
                return;
            }

            // Case 2: Extend barrier (FirstRange->SecondRange then SecondRange->C) => FirstRange->C
            if (Covering.StateAfter == BeforeState)
            {
                Covering.StateAfter = AfterState;
            #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
                LOG_INFO("  Extended barrier to %s->%s (SubresourceIndex=%u)", ToString(Covering.StateBefore), ToString(Covering.StateAfter), SubresourceIndex);
            #endif

                // If we changed state to the same before- and after-state, remove it
                if (Covering.StateBefore == Covering.StateAfter)
                {
                #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
                    LOG_INFO("  Cancelled barrier (SubresourceIndex=%u, %s<->%s)", SubresourceIndex, ToString(Covering.StateBefore), ToString(Covering.StateAfter));
                #endif
                    Barriers.RemoveAt(CoveringIndex);
                }

                return;
            }

        #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
            D3D12_ERROR("Non-chained transition barrier. Resource=%p Subresource=%u Existing=%s(0x%X)->%s(0x%X) New=%s(0x%X)->%s(0x%X)",
                Resource, SubresourceIndex, ToString(Covering.StateBefore), uint32(Covering.StateBefore), ToString(Covering.StateAfter),
                uint32(Covering.StateAfter), ToString(BeforeState), uint32(BeforeState), ToString(AfterState), uint32(AfterState));
            CHECK(false);
        #endif
        }

        // Case 3: the queued entry covers a different subresource range, so it cannot be rewritten in place.
        // It still determines what state this subresource will be in by the time the new barrier runs, which
        // is only knowable when every covering entry lands on the same state.
        if (bCoveringStatesAllMatch)
        {
            BeforeState = Covering.StateAfter;
            if (BeforeState == AfterState)
            {
                return;
            }
        }
    }

    // Otherwise: Add a new barrier.
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

void FD3D12BarrierBatcher::AddAliasingBarrier(FD3D12Resource* InResourceAfter, ID3D12Resource* ResourceBefore)
{
    CHECK(InResourceAfter != nullptr);

#if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    {
        String DebugName;
        InResourceAfter->GetDebugName(DebugName);
        D3D12_INFO("AddAliasingBarrier Resource=%s", *DebugName);
    }
#endif

    AddAliasingBarrier(InResourceAfter->GetD3D12Resource(), ResourceBefore);
}

void FD3D12BarrierBatcher::AddAliasingBarrier(ID3D12Resource* ResourceAfter, ID3D12Resource* ResourceBefore)
{
    CHECK(ResourceAfter != nullptr);

    // Keyed on the pair: the same resource can legitimately be aliased in from more than one predecessor.
    for (const D3D12_RESOURCE_BARRIER& Existing : Barriers)
    {
        if (Existing.Type == D3D12_RESOURCE_BARRIER_TYPE_ALIASING && Existing.Aliasing.pResourceAfter == ResourceAfter &&
            Existing.Aliasing.pResourceBefore == ResourceBefore)
        {
            return;
        }
    }

    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    Barrier.Flags                    = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Aliasing.pResourceBefore = ResourceBefore;
    Barrier.Aliasing.pResourceAfter  = ResourceAfter;

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
        LocalState.Initialize(Math::Max(Resource->GetNumSubresources(), 1u));
        LocalState.SetState(D3D12_RESOURCE_STATE_TO_BE_DETERMINED);
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

                TransitionTrackedResourceState(Heap->GetReadbackResource(), D3D12_RESOURCE_STATE_COPY_DEST);
                BarrierBatcher.FlushBarriers(GetCommandList());

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
#if D3D12_ENABLE_DESCRIPTOR_HEAP_ROLLOVER_LOGGING
    D3D12_WARNING("[DescriptorRollover] Splitting command list for descriptor-heap rollover (RecordedCommands=%u)",
        CommandList ? CommandList->GetNumCommands() : 0u);
#endif

    SplitCommandList(true, true);
    FD3D12DeviceRHI::Get()->FlushCompletedSubmissions();
}

void FD3D12CommandContext::BeginFrame()
{
    FD3D12DeviceRHI::Get()->BeginFrame(this);
}

void FD3D12CommandContext::EndFrame()
{
    FD3D12DeviceRHI::Get()->EndFrame(this);
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

    D3D12_HEAP_TYPE HeapType = Resource->GetHeapType();
    if (HeapType != D3D12_HEAP_TYPE_UPLOAD)
    {
        TransitionTrackedResourceState(Resource, D3D12_RESOURCE_STATE_COPY_DEST);
    }

    BarrierBatcher.FlushBarriers(GetCommandList());

    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        uint8* BufferData = reinterpret_cast<uint8*>(Resource->MapRange(0, nullptr));
        if (!BufferData)
        {
            D3D12_ERROR("Failed to map buffer data");
            return;
        }

        const D3D12_RANGE WrittenRange =
        {
            BufferRegion.Offset,
            BufferRegion.Offset + BufferRegion.Size
        };

        Memory::Memcpy(BufferData + BufferRegion.Offset, SrcData, BufferRegion.Size);
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

void FD3D12CommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor)
{
    FD3D12RenderTargetViewRHI* D3D12RenderTargetView = FD3D12DeviceRHI::ResourceCast(RenderTargetView);
    CHECK(D3D12RenderTargetView != nullptr);

    TransitionResourceState(D3D12RenderTargetView);

    BarrierBatcher.FlushBarriers(GetCommandList());

    GetCommandList()->ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), ClearColor.XYZW, 0, nullptr);
}

void FD3D12CommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
    FD3D12DepthStencilViewRHI* D3D12DepthStencilView = FD3D12DeviceRHI::ResourceCast(DepthStencilView);
    CHECK(D3D12DepthStencilView != nullptr);

    TransitionResourceState(D3D12DepthStencilView, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    BarrierBatcher.FlushBarriers(GetCommandList());

    D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
    if (IsStencilFormat(D3D12DepthStencilView->GetD3D12Desc().Format))
    {
        ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
    }

    GetCommandList()->ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), ClearFlags, Depth, Stencil, 0, nullptr);
}

void FD3D12CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
    FD3D12UnorderedAccessViewRHI* D3D12UnorderedAccessView = FD3D12DeviceRHI::ResourceCast(UnorderedAccessView);
    CHECK(D3D12UnorderedAccessView != nullptr);

    TransitionResourceState(D3D12UnorderedAccessView);

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

    TransitionResourceState(D3D12UnorderedAccessView);

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
    FD3D12RenderTargetViewRHI* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    FD3D12DepthStencilViewRHI* DepthStencilView = nullptr;

    for (uint32 Index = 0; Index < BeginRenderPassDesc.NumRenderTargets; ++Index)
    {
        const FRHIRenderPassAttachment& CurrentAttachment = BeginRenderPassDesc.RenderTargets[Index];

        RenderTargetViews[Index] = FD3D12DeviceRHI::ResourceCast(CurrentAttachment.View);
        if (RenderTargetViews[Index] && CurrentAttachment.LoadAction == EAttachmentLoadAction::Clear)
        {
            TransitionResourceState(RenderTargetViews[Index]);
        }
    }

    const FRHIDepthStencilAttachment& CurrentDSAttachment = BeginRenderPassDesc.DepthStencilAttachment;
    DepthStencilView = FD3D12DeviceRHI::ResourceCast(CurrentDSAttachment.View);

    const bool bClearDepthStencil = DepthStencilView && CurrentDSAttachment.LoadAction == EAttachmentLoadAction::Clear;
    if (bClearDepthStencil)
    {
        TransitionResourceState(DepthStencilView, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    BarrierBatcher.FlushBarriers(GetCommandList());


    for (uint32 Index = 0; Index < BeginRenderPassDesc.NumRenderTargets; ++Index)
    {
        const FRHIRenderPassAttachment& CurrentAttachment = BeginRenderPassDesc.RenderTargets[Index];
        if (RenderTargetViews[Index] && CurrentAttachment.LoadAction == EAttachmentLoadAction::Clear)
        {
            GetCommandList()->ClearRenderTargetView(
                RenderTargetViews[Index]->GetOfflineHandle(), 
                CurrentAttachment.ClearValue.RGBA, 
                0, 
                nullptr);
        }
    }

    if (bClearDepthStencil)
    {
        D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
        if (IsStencilFormat(DepthStencilView->GetD3D12Desc().Format))
        {
            ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
        }

        GetCommandList()->ClearDepthStencilView(
            DepthStencilView->GetOfflineHandle(),
            ClearFlags,
            CurrentDSAttachment.ClearValue.Depth,
            static_cast<uint8>(CurrentDSAttachment.ClearValue.Stencil),
            0,
            nullptr);
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

void FD3D12CommandContext::SetBlendFactor(const Vector4& Color)
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

void FD3D12CommandContext::SetMeshletPipelineState(class FRHIMeshletPipelineState* PipelineState)
{
    FD3D12MeshletPipelineStateRHI* MeshletPipelineState = FD3D12DeviceRHI::ResourceCast(PipelineState);
    ContextState.SetMeshletPipelineState(MeshletPipelineState);
}

void FD3D12CommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    MAYBE_UNUSED FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
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

    FD3D12TextureRHI* D3D12Source      = FD3D12DeviceRHI::ResourceCast(Src);
    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);

    const DXGI_FORMAT DstFormat = D3D12CastShaderResourceFormat(D3D12Destination->GetDXGIFormat());
    const DXGI_FORMAT SrcFormat = D3D12CastShaderResourceFormat(D3D12Source->GetDXGIFormat());

    if (DstFormat != SrcFormat)
    {
        D3D12_ERROR("Dst and Src must have compatible formats for resolve");
        return;
    }

    TransitionTrackedResourceState(D3D12Source, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    TransitionTrackedResourceState(D3D12Destination, D3D12_RESOURCE_STATE_RESOLVE_DEST);

    BarrierBatcher.FlushBarriers(GetCommandList());

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

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Resource* D3D12Resource = D3D12Destination->GetResource();
    CHECK(D3D12Resource != nullptr);

    TransitionTrackedResourceState(D3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(GetCommandList());

    D3D12_RESOURCE_DESC Desc = D3D12Resource->GetDesc();
#if D3D12_USE_TIGHT_ALIGNMENT
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) != 0)
    {
        // Query APIs require tight-alignment resources to use Alignment=0 in the desc.
        Desc.Alignment = 0;
    }
#endif

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

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12Resource* D3D12Resource = D3D12Destination->GetResource();
    CHECK(D3D12Resource != nullptr);

    TransitionTrackedResourceState(D3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(GetCommandList());

    D3D12_RESOURCE_DESC Desc = D3D12Resource->GetDesc();
#if D3D12_USE_TIGHT_ALIGNMENT
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) != 0)
    {
        Desc.Alignment = 0;
    }
#endif

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

    FD3D12BufferRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12BufferRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    TransitionTrackedResourceState(D3D12Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionTrackedResourceState(D3D12Destination, D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(GetCommandList());

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

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    TransitionTrackedResourceState(D3D12Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionTrackedResourceState(D3D12Destination, D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(GetCommandList());

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
        
        MAYBE_UNUSED const uint32 SrcNumArrayLayers = RHIDimensionArrayLayers(SrcDimension, Src->GetDesc().NumArraySlices);
        MAYBE_UNUSED const uint32 DstNumArrayLayers = RHIDimensionArrayLayers(DstDimension, Dst->GetDesc().NumArraySlices);
        CHECK(InCopyDesc.SrcArraySlice + InCopyDesc.NumArraySlices <= SrcNumArrayLayers);
        CHECK(InCopyDesc.DstArraySlice + InCopyDesc.NumArraySlices <= DstNumArrayLayers);
    }

    FD3D12TextureRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);
    
    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    GetCommandList().UpdateResidency(D3D12Destination->GetResource()->GetResidencyHandle());
    GetCommandList().UpdateResidency(D3D12Source->GetResource()->GetResidencyHandle());

    TransitionTrackedResourceState(D3D12Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionTrackedResourceState(D3D12Destination, D3D12_RESOURCE_STATE_COPY_DEST);

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
 
    FD3D12BufferRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr); 
 
    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src); 
    CHECK(D3D12Source != nullptr);

    TransitionTrackedResourceState(D3D12Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionTrackedResourceState(D3D12Destination, D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(GetCommandList()); 

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

    const FD3D12ResourceStorage& DstStorage = D3D12Destination->GetResourceStorage();
    const uint64 EffectiveOffset = DstOffset + DstStorage.GetResourceOffset();

    if ((EffectiveOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) != 0)
    {
        D3D12_ERROR("CopyTextureRegionToBuffer requires a %u-byte aligned destination offset. Offset=%llu SuballocationOffset=%llu",
            D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, DstOffset, DstStorage.GetResourceOffset());
        return;
    }

    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.pResource                        = DstResource->GetD3D12Resource();
    DestLocation.Type                             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DestLocation.PlacedFootprint.Offset           = EffectiveOffset;
    DestLocation.PlacedFootprint.Footprint.Format = ConvertFormat(Src->GetDesc().Format);

    const uint32 SrcLeft      = SrcRegion.PositionX >> SrcMipLevel;
    const uint32 SrcTop       = SrcRegion.PositionY >> SrcMipLevel;
    const uint32 SrcRight     = Math::Max((SrcRegion.PositionX + SrcRegion.Width) >> SrcMipLevel, SrcLeft + 1);
    const uint32 SrcBottom    = Math::Max((SrcRegion.PositionY + SrcRegion.Height) >> SrcMipLevel, SrcTop + 1);
    const uint32 CopyWidth    = SrcRight - SrcLeft;
    const uint32 CopyHeight   = SrcBottom - SrcTop;
    const uint32 RowPitch     = Math::AlignUp<uint32>(BytesPerPixel * CopyWidth, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    MAYBE_UNUSED const uint64 RequiredSize = uint64(RowPitch) * uint64(CopyHeight);

    CHECK(DstOffset + RequiredSize <= DstStorage.GetSize());

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
        MAYBE_UNUSED const uint32 SrcNumArrayLayers = RHIDimensionArrayLayers(SrcDimension, Src->GetDesc().NumArraySlices);
        CHECK(SrcArraySlice < SrcNumArrayLayers);
    }

    FD3D12BufferRHI* D3D12Destination = FD3D12DeviceRHI::ResourceCast(Dst);
    CHECK(D3D12Destination != nullptr);

    FD3D12TextureRHI* D3D12Source = FD3D12DeviceRHI::ResourceCast(Src);
    CHECK(D3D12Source != nullptr);

    TransitionTrackedResourceState(D3D12Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionTrackedResourceState(D3D12Destination, D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(GetCommandList());

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

    const FD3D12ResourceStorage& DstStorage = D3D12Destination->GetResourceStorage();
    const uint64 EffectiveOffset = DstOffset + DstStorage.GetResourceOffset();

    if ((EffectiveOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) != 0)
    {
        D3D12_ERROR("CopyTextureSubresourceToBuffer requires a %u-byte aligned destination offset. Offset=%llu SuballocationOffset=%llu", 
            D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, DstOffset, DstStorage.GetResourceOffset());
        return;
    }

    MAYBE_UNUSED const uint64 RequiredSize = uint64(RowPitch) * uint64(CopyHeight) * uint64(CopyDepth);
    CHECK(DstOffset + RequiredSize <= DstStorage.GetSize());

    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.pResource                          = DstResource->GetD3D12Resource();
    DestLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DestLocation.PlacedFootprint.Offset             = EffectiveOffset;
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
    FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(Texture);
    if (!D3D12Texture)
    {
        return;
    }

    // DiscardResource only accepts a resource in one of the three writable states.
    const FRHITextureDesc& TextureDesc = D3D12Texture->GetDesc();

    D3D12_RESOURCE_STATES DiscardState = D3D12_RESOURCE_STATE_COMMON;
    if (TextureDesc.IsDepthStencil())
    {
        DiscardState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    else if (TextureDesc.IsRenderTarget())
    {
        DiscardState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    else if (TextureDesc.IsUnorderedAccessTexture())
    {
        DiscardState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    else
    {
        D3D12_WARNING("DiscardContents ignored: texture is neither a render target, depth-stencil nor unordered-access texture");
        return;
    }

    TransitionTrackedResourceState(D3D12Texture, DiscardState);
    BarrierBatcher.FlushBarriers(GetCommandList());

    GetCommandList()->DiscardResource(D3D12Texture->GetResource()->GetD3D12Resource(), nullptr);
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

void FD3D12CommandContext::TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(Texture);
    CHECK(D3D12Texture != nullptr);

    {
        const ETextureDimension Dimension = Texture->GetDesc().Dimension;
        MAYBE_UNUSED const uint32 NumArrayLayers = RHIDimensionArrayLayers(Dimension, Texture->GetDesc().NumArraySlices);
        CHECK(TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES || TextureTransition.ArraySlice < NumArrayLayers);
    }

    FD3D12Resource* Resource = D3D12Texture->GetResource();
    CHECK(Resource != nullptr);

    if (!Resource->IsHeapTypeDefault())
    {
        return;
    }

    const D3D12_RESOURCE_STATES D3D12BeforeState = D3D12ResolveRestingState(Resource, ConvertResourceState(TextureTransition.BeforeState));
    const D3D12_RESOURCE_STATES D3D12AfterState  = D3D12ResolveRestingState(Resource, ConvertResourceState(TextureTransition.AfterState));

    const bool bIsPartialTransition = (TextureTransition.MipLevel != RHI_ALL_MIP_LEVELS || TextureTransition.ArraySlice != RHI_ALL_ARRAY_SLICES) && !Resource->HasMultiplePlanes();

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    if (bIsPartialTransition)
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
                    BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
                    LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
                }
                else if (!D3D12IsReadStateSatisfied(CurrentState, D3D12AfterState))
                {
                    if (!D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState))
                    {
                        D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *String::CreateFormatted("array-slice loop, slice=%u, mip=%u", ArraySlice, TextureTransition.MipLevel),
                            TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                        CHECK(D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState));
                    }

                    BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, D3D12AfterState, SubresourceIndex);
                    LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
                }
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
                    BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
                    LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
                }
                else if (!D3D12IsReadStateSatisfied(CurrentState, D3D12AfterState))
                {
                    if (!D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState))
                    {
                        D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *String::CreateFormatted("mip loop, slice=%u, mip=%u", TextureTransition.ArraySlice, MipLevel),
                            TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                        CHECK(D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState));
                    }

                    BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, D3D12AfterState, SubresourceIndex);
                    LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
                }
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
                BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, SubresourceIndex);
                LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
            }
            else if (!D3D12IsReadStateSatisfied(CurrentState, D3D12AfterState))
            {
                if (!D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState))
                {
                    D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *String::CreateFormatted("single subresource, slice=%u, mip=%u", TextureTransition.ArraySlice, TextureTransition.MipLevel),
                        TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                    CHECK(D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState));
                }

                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, D3D12AfterState, SubresourceIndex);
                LocalState.SetSubresourceState(SubresourceIndex, D3D12AfterState);
            }
        }
    }
    else
    {
        if (LocalState.IsSingleState())
        {
            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetState();
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, D3D12BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
                BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState);
                LocalState.SetState(D3D12AfterState);
            }
            else if (!D3D12IsReadStateSatisfied(CurrentState, D3D12AfterState))
            {
                if (!D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState))
                {
                    D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, "all-subresources (uniform state)",
                        TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                    CHECK(D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState));
                }

                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, D3D12AfterState);
                LocalState.SetState(D3D12AfterState);
            }
        }
        else
        {
            for (uint32 i = 0; i < LocalState.GetNumSubresources(); i++)
            {
                const D3D12_RESOURCE_STATES CurrentState = LocalState.GetSubresourceState(i);
                if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
                {
                    AddPendingBarrier(Resource, D3D12BeforeState, i);
                    BarrierBatcher.AddTransitionBarrier(Resource, D3D12BeforeState, D3D12AfterState, i);
                }
                else
                {
                    if (!D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState))
                    {
                        D3D12_LOG_TRANSITION_MISMATCH(D3D12Texture, *String::CreateFormatted("per-subresource (divergent), subresource=%u", i),
                            TextureTransition.BeforeState, D3D12BeforeState, D3D12AfterState, CurrentState);
                        CHECK(D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState));
                    }

                    BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, D3D12AfterState, i);
                }
            }

            LocalState.SetState(D3D12AfterState);
        }
    }
}

void FD3D12CommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    FD3D12Resource* Resource = D3D12Buffer->GetResource();
    CHECK(Resource != nullptr);

    if (!Resource->IsHeapTypeDefault())
    {
        return;
    }

    const D3D12_RESOURCE_STATES D3D12AfterState  = D3D12ResolveRestingState(Resource, ConvertResourceState(AfterState));
    const D3D12_RESOURCE_STATES D3D12BeforeState = D3D12ResolveRestingState(Resource, ConvertResourceState(BeforeState));

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);

    const D3D12_RESOURCE_STATES CurrentState = LocalState.GetState();
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
        if (D3D12IsReadStateSatisfied(CurrentState, D3D12AfterState))
        {
            return;
        }

        CHECK(D3D12IsBeforeStateValid(CurrentState, D3D12BeforeState));
    }

    const D3D12_RESOURCE_STATES EffectiveBeforeState = (CurrentState != D3D12_RESOURCE_STATE_TO_BE_DETERMINED) ? CurrentState : D3D12BeforeState;
    BarrierBatcher.AddTransitionBarrier(Resource, EffectiveBeforeState, D3D12AfterState);
    LocalState.SetState(D3D12AfterState);
}

void FD3D12CommandContext::TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState)
{
    CHECK(Resource != nullptr);

    if (!Resource->IsHeapTypeDefault())
    {
        return;
    }

    AfterState = D3D12ResolveRestingState(Resource, AfterState);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    if (LocalState.IsSingleState())
    {
        const D3D12_RESOURCE_STATES CurrentState = LocalState.GetState();
        if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            AddPendingBarrier(Resource, AfterState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        }
        else
        {
            if (D3D12IsReadStateSatisfied(CurrentState, AfterState))
            {
                return;
            }

            BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, AfterState);
        }

        LocalState.SetState(AfterState);
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
                if (D3D12IsReadStateSatisfied(SubresourceState, AfterState))
                {
                    continue;
                }

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

    if (!Resource->IsHeapTypeDefault())
    {
        return;
    }

    AfterState = D3D12ResolveRestingState(Resource, AfterState);

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    if (LocalState.IsSingleState())
    {
        const D3D12_RESOURCE_STATES CurrentState = LocalState.GetState();
        if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            AddPendingBarrier(Resource, BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        }

        const D3D12_RESOURCE_STATES EffectiveBeforeState = (CurrentState != D3D12_RESOURCE_STATE_TO_BE_DETERMINED) ? CurrentState : BeforeState;
        BarrierBatcher.AddTransitionBarrier(Resource, EffectiveBeforeState, AfterState);
        LocalState.SetState(AfterState);
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

            const D3D12_RESOURCE_STATES EffectiveBeforeState = (SubresourceState != D3D12_RESOURCE_STATE_TO_BE_DETERMINED) ? SubresourceState : BeforeState;
            BarrierBatcher.AddTransitionBarrier(Resource, EffectiveBeforeState, AfterState, i);
            LocalState.SetSubresourceState(i, AfterState);
        }
    }
}

void FD3D12CommandContext::TransitionResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState, uint32 FirstMip, uint32 NumMips, uint32 FirstArraySlice, uint32 NumArraySlices)
{
    CHECK(Resource != nullptr);

    if (!Resource->IsHeapTypeDefault())
    {
        return;
    }

    // Per-subresource indices only address plane 0 of a multi-plane format, so transition the whole resource.
    if (Resource->HasMultiplePlanes())
    {
        TransitionResourceState(Resource, AfterState);
        return;
    }

    AfterState = D3D12ResolveRestingState(Resource, AfterState);

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
                if (D3D12IsReadStateSatisfied(CurrentState, AfterState))
                {
                    continue;
                }

                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, AfterState, SubresourceIndex);
            }

            LocalState.SetSubresourceState(SubresourceIndex, AfterState);
        }
    }
}

void FD3D12CommandContext::TransitionTrackedResourceState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES AfterState)
{
    if (!Resource || !Resource->RequiresResourceStateTracking())
    {
        EnsureResourceState(Resource, AfterState);
        return;
    }

    TransitionResourceState(Resource, AfterState);
}

void FD3D12CommandContext::TransitionTrackedResourceState(FD3D12BufferRHI* Buffer, D3D12_RESOURCE_STATES AfterState)
{
    TransitionTrackedResourceState(Buffer ? Buffer->GetResource() : nullptr, AfterState);
}

void FD3D12CommandContext::TransitionTrackedResourceState(FD3D12TextureRHI* Texture, D3D12_RESOURCE_STATES AfterState)
{
    TransitionTrackedResourceState(Texture ? Texture->GetResource() : nullptr, AfterState);
}

void FD3D12CommandContext::EnsureResourceState(const FD3D12Resource* Resource, D3D12_RESOURCE_STATES RequiredState) const
{
#if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    if (Resource && Resource->IsHeapTypeDefault())
    {
        const D3D12_RESOURCE_STATES CurrentState = GetTrackedResourceState(Resource);
        if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            return;
        }

        const D3D12_RESOURCE_STATES ResolvedState = D3D12ResolveRestingState(Resource, RequiredState);
        if (CurrentState == ResolvedState || D3D12IsReadStateSatisfied(CurrentState, ResolvedState))
        {
            return;
        }

        String DebugName;
        Resource->GetDebugName(DebugName);

        D3D12_ERROR("Untracked resource '%s' is in %s (0x%08X) where %s (0x%08X) is required",
            *DebugName, ToString(CurrentState), uint32(CurrentState), ToString(ResolvedState), uint32(ResolvedState));

        CHECK(false);
    }
#else
    UNREFERENCED_VARIABLE(Resource);
    UNREFERENCED_VARIABLE(RequiredState);
#endif
}

D3D12_RESOURCE_STATES FD3D12CommandContext::GetTrackedResourceState(const FD3D12Resource* Resource) const
{
    const FD3D12ResourceState* Pending     = PendingResourceStates.Find(const_cast<FD3D12Resource*>(Resource));
    const bool                 bUsePending = Pending && Pending->IsInitialized() && Pending->IsSingleState();
    const FD3D12ResourceState& GlobalState = Resource->GetResourceState();

    return bUsePending
        ? Pending->GetState()
        : (GlobalState.IsSingleState() ? GlobalState.GetState() : GlobalState.GetSubresourceState(0));
}

void FD3D12CommandContext::EnsureDefaultState(const FD3D12Resource* Resource) const
{
#if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    if (Resource && Resource->HasDefaultState())
    {
        const FD3D12ResourceState* Pending     = PendingResourceStates.Find(const_cast<FD3D12Resource*>(Resource));
        const bool                 bUsePending = Pending && Pending->IsInitialized() && Pending->IsSingleState();

        const D3D12_RESOURCE_STATES CurrentState = GetTrackedResourceState(Resource);
        const D3D12_RESOURCE_STATES DefaultState = Resource->GetDefaultState();
        if (CurrentState != D3D12_RESOURCE_STATE_TO_BE_DETERMINED && CurrentState != DefaultState)
        {
            String DebugName;
            Resource->GetDebugName(DebugName);

            D3D12_ERROR("EnsureDefaultState mismatch on '%s': CurrentState=%s (0x%08X, Source=%s), DefaultState=%s (0x%08X)",
                *DebugName, ToString(CurrentState), uint32(CurrentState), bUsePending ? "Pending" : "Global", ToString(DefaultState), uint32(DefaultState));
        }

        CHECK(CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED || CurrentState == DefaultState);
    }
#else
    UNREFERENCED_VARIABLE(Resource);
#endif
}

void FD3D12CommandContext::TransitionResourceState(FD3D12UnorderedAccessViewRHI* View)
{
    TransitionTrackedResourceState(View->GetCounterResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    FD3D12Resource* Resource = View->GetViewResource();
    if (!Resource || !Resource->RequiresResourceStateTracking())
    {
        EnsureDefaultState(Resource);
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
        EnsureDefaultState(Resource);
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

        case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
        {
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
        MAYBE_UNUSED const uint32 NumArrayLayers = RHIDimensionArrayLayers(Dimension, Texture->GetDesc().NumArraySlices);
        CHECK(RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES || RequiredState.ArraySlice < NumArrayLayers);
    }

    FD3D12Resource* Resource = D3D12Texture->GetResource();
    CHECK(Resource != nullptr);

    if (!Resource->RequiresResourceStateTracking() || !Resource->IsHeapTypeDefault())
    {
        EnsureDefaultState(Resource);
        return;
    }

    const D3D12_RESOURCE_DESC&  ResourceDesc = Resource->GetDesc();
    const D3D12_RESOURCE_STATES DesiredState = D3D12ResolveRestingState(Resource, ConvertResourceState(RequiredState.State));
    
    const uint32 NumMipLevels   = ResourceDesc.MipLevels;
    const uint32 NumArraySlices = ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? ResourceDesc.DepthOrArraySize : 1u;
    
    const bool bIsWholeResource = (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS && RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES) || Resource->HasMultiplePlanes();

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);
    if (bIsWholeResource)
    {
        if (LocalState.IsSingleState())
        {
            const D3D12_RESOURCE_STATES CurrentState = LocalState.GetState();
            if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
            {
                AddPendingBarrier(Resource, DesiredState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
                LocalState.SetState(DesiredState);
            }
            else if (!D3D12IsReadStateSatisfied(CurrentState, DesiredState))
            {
                if (CurrentState != DesiredState)
                {
                    BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState);
                }

                LocalState.SetState(DesiredState);
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

            LocalState.SetState(DesiredState);
        }
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
                LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
            }
            else if (!D3D12IsReadStateSatisfied(CurrentState, DesiredState))
            {
                if (CurrentState != DesiredState)
                {
                    BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState, SubresourceIndex);
                }

                LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
            }
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
                LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
            }
            else if (!D3D12IsReadStateSatisfied(CurrentState, DesiredState))
            {
                if (CurrentState != DesiredState)
                {
                    BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState, SubresourceIndex);
                }

                LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
            }
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
            LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
        }
        else if (!D3D12IsReadStateSatisfied(CurrentState, DesiredState))
        {
            if (CurrentState != DesiredState)
            {
                BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState, SubresourceIndex);
            }
            
            LocalState.SetSubresourceState(SubresourceIndex, DesiredState);
        }
    }
}

void FD3D12CommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
    FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(Buffer);
    CHECK(D3D12Buffer != nullptr);

    FD3D12Resource* Resource = D3D12Buffer->GetResource();
    CHECK(Resource != nullptr);

    if (!Resource->RequiresResourceStateTracking() || !Resource->IsHeapTypeDefault())
    {
        EnsureDefaultState(Resource);
        return;
    }

    FD3D12ResourceState& LocalState = RetrievePendingResourceState(Resource);

    const D3D12_RESOURCE_STATES DesiredState = D3D12ResolveRestingState(Resource, ConvertResourceState(RequiredState));
    const D3D12_RESOURCE_STATES CurrentState = LocalState.GetState();

    if (CurrentState == D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
    {
        AddPendingBarrier(Resource, DesiredState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        LocalState.SetState(DesiredState);
    }
    else if (CurrentState != DesiredState && !D3D12IsReadStateSatisfied(CurrentState, DesiredState))
    {
        BarrierBatcher.AddTransitionBarrier(Resource, CurrentState, DesiredState);
        LocalState.SetState(DesiredState);
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

void FD3D12CommandContext::AliasingBarrier(FD3D12Resource* ResourceAfter, ID3D12Resource* ResourceBefore)
{
    CHECK(ResourceAfter != nullptr);
    BarrierBatcher.AddAliasingBarrier(ResourceAfter, ResourceBefore);
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

void FD3D12CommandContext::DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    if (ThreadGroupCountX == 0 || ThreadGroupCountY == 0 || ThreadGroupCountZ == 0)
    {
        return;
    }

#if D3D12_USE_ID3D12COMMANDLIST_6
    if (GD3D12MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    {
        D3D12_ERROR("DispatchMesh called but mesh shaders are not supported on this device");
        return;
    }

    ConditionalSplitCommandList();

    ContextState.PrepareMeshletState();
    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindMeshletState();

    GetCommandList().GetGraphicsCommandList6()->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
#else
    D3D12_ERROR("DispatchMesh requires ID3D12GraphicsCommandList6 support");
#endif
}

void FD3D12CommandContext::DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    FD3D12BufferRHI* Arguments = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ID3D12CommandSignature* Signature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::Draw);
    CHECK(Signature != nullptr);

    ConditionalSplitCommandList();
    ContextState.PrepareGraphicsState();

    FD3D12Resource* ArgumentResource = Arguments->GetResource();
    CHECK(ArgumentResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());

    const uint64 NativeOffset = Arguments->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    GetCommandList()->ExecuteIndirect(Signature, CommandCount, ArgumentResource->GetD3D12Resource(), NativeOffset, nullptr, 0);
}

void FD3D12CommandContext::DrawIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
    FD3D12BufferRHI* Arguments = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    FD3D12BufferRHI* Count = FD3D12DeviceRHI::ResourceCast(CountBuffer);
    CHECK(Count != nullptr);

    ID3D12CommandSignature* Signature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::Draw);
    CHECK(Signature != nullptr);

    ConditionalSplitCommandList();
    ContextState.PrepareGraphicsState();

    FD3D12Resource* ArgumentResource = Arguments->GetResource();
    CHECK(ArgumentResource != nullptr);

    FD3D12Resource* CountResource = Count->GetResource();
    CHECK(CountResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    TransitionTrackedResourceState(CountResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());
    GetCommandList().UpdateResidency(CountResource->GetResidencyHandle());

    const uint64 NativeArgumentOffset = Arguments->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    const uint64 NativeCountOffset    = Count->GetResourceStorage().GetResourceOffset() + CountBufferOffset;

    GetCommandList()->ExecuteIndirect(
        Signature,
        MaxCommandCount,
        ArgumentResource->GetD3D12Resource(),
        NativeArgumentOffset,
        CountResource->GetD3D12Resource(),
        NativeCountOffset);
}

void FD3D12CommandContext::DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    FD3D12BufferRHI* Arguments = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ID3D12CommandSignature* Signature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::DrawIndexed);
    CHECK(Signature != nullptr);

    ConditionalSplitCommandList();
    ContextState.PrepareGraphicsState();

    FD3D12Resource* ArgumentResource = Arguments->GetResource();
    CHECK(ArgumentResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());

    const uint64 NativeOffset = Arguments->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    GetCommandList()->ExecuteIndirect(Signature, CommandCount, ArgumentResource->GetD3D12Resource(), NativeOffset, nullptr, 0);
}

void FD3D12CommandContext::DrawIndexedIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
    FD3D12BufferRHI* Arguments = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    FD3D12BufferRHI* Count = FD3D12DeviceRHI::ResourceCast(CountBuffer);
    CHECK(Count != nullptr);

    ID3D12CommandSignature* Signature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::DrawIndexed);
    CHECK(Signature != nullptr);

    ConditionalSplitCommandList();
    ContextState.PrepareGraphicsState();

    FD3D12Resource* ArgumentResource = Arguments->GetResource();
    CHECK(ArgumentResource != nullptr);

    FD3D12Resource* CountResource = Count->GetResource();
    CHECK(CountResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    TransitionTrackedResourceState(CountResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindGraphicsState();

    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());
    GetCommandList().UpdateResidency(CountResource->GetResidencyHandle());

    const uint64 NativeArgumentOffset = Arguments->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    const uint64 NativeCountOffset    = Count->GetResourceStorage().GetResourceOffset() + CountBufferOffset;

    GetCommandList()->ExecuteIndirect(
        Signature,
        MaxCommandCount,
        ArgumentResource->GetD3D12Resource(),
        NativeArgumentOffset,
        CountResource->GetD3D12Resource(),
        NativeCountOffset);
}

void FD3D12CommandContext::DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
    FD3D12BufferRHI* Arguments = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ID3D12CommandSignature* Signature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::Dispatch);
    CHECK(Signature != nullptr);

    ConditionalSplitCommandList();
    ContextState.PrepareComputeState();

    FD3D12Resource* ArgumentResource = Arguments->GetResource();
    CHECK(ArgumentResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindComputeState();

    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());

    const uint64 NativeOffset = Arguments->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    GetCommandList()->ExecuteIndirect(Signature, 1, ArgumentResource->GetD3D12Resource(), NativeOffset, nullptr, 0);
}

void FD3D12CommandContext::DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
#if D3D12_USE_ID3D12COMMANDLIST_6
    if (GD3D12MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    {
        D3D12_ERROR("DispatchMeshIndirect called but mesh shaders are not supported on this device");
        return;
    }

    FD3D12BufferRHI* Arguments = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ID3D12CommandSignature* Signature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::DispatchMesh);
    CHECK(Signature != nullptr);

    ConditionalSplitCommandList();
    ContextState.PrepareMeshletState();

    FD3D12Resource* ArgumentResource = Arguments->GetResource();
    CHECK(ArgumentResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindMeshletState();

    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());

    const uint64 NativeOffset = Arguments->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    GetCommandList().GetGraphicsCommandList6()->ExecuteIndirect(Signature, CommandCount, ArgumentResource->GetD3D12Resource(), NativeOffset, nullptr, 0);
#else
    UNREFERENCED_VARIABLE(ArgumentBuffer);
    UNREFERENCED_VARIABLE(ArgumentBufferOffset);
    UNREFERENCED_VARIABLE(CommandCount);
    D3D12_ERROR("DispatchMeshIndirect requires ID3D12GraphicsCommandList6 support");
#endif
}

void FD3D12CommandContext::DispatchMeshIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
#if D3D12_USE_ID3D12COMMANDLIST_6
    if (GD3D12MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    {
        D3D12_ERROR("DispatchMeshIndirectCount called but mesh shaders are not supported on this device");
        return;
    }

    FD3D12BufferRHI* Arguments = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    FD3D12BufferRHI* Count = FD3D12DeviceRHI::ResourceCast(CountBuffer);
    CHECK(Count != nullptr);

    ID3D12CommandSignature* Signature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::DispatchMesh);
    CHECK(Signature != nullptr);

    ConditionalSplitCommandList();
    ContextState.PrepareMeshletState();

    FD3D12Resource* ArgumentResource = Arguments->GetResource();
    CHECK(ArgumentResource != nullptr);

    FD3D12Resource* CountResource = Count->GetResource();
    CHECK(CountResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    TransitionTrackedResourceState(CountResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    ContextState.BindMeshletState();

    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());
    GetCommandList().UpdateResidency(CountResource->GetResidencyHandle());

    const uint64 NativeArgumentOffset = Arguments->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    const uint64 NativeCountOffset    = Count->GetResourceStorage().GetResourceOffset() + CountBufferOffset;

    GetCommandList().GetGraphicsCommandList6()->ExecuteIndirect(
        Signature,
        MaxCommandCount,
        ArgumentResource->GetD3D12Resource(),
        NativeArgumentOffset,
        CountResource->GetD3D12Resource(),
        NativeCountOffset);
#else
    UNREFERENCED_VARIABLE(ArgumentBuffer);
    UNREFERENCED_VARIABLE(ArgumentBufferOffset);
    UNREFERENCED_VARIABLE(CountBuffer);
    UNREFERENCED_VARIABLE(CountBufferOffset);
    UNREFERENCED_VARIABLE(MaxCommandCount);
    D3D12_ERROR("DispatchMeshIndirectCount requires ID3D12GraphicsCommandList6 support");
#endif
}

void FD3D12CommandContext::SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    if (!ShaderBindingTable)
    {
        return;
    }

    FD3D12ShaderBindingTable* D3D12ShaderBindingTable = FD3D12DeviceRHI::ResourceCast(ShaderBindingTable);
    D3D12ShaderBindingTable->SetBindings(RecordKind, RecordIndex, Bindings, NumBindings);
}

void FD3D12CommandContext::BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (!ShaderBindingTable)
    {
        return;
    }

    FD3D12ShaderBindingTable* D3D12ShaderBindingTable = FD3D12DeviceRHI::ResourceCast(ShaderBindingTable);
    D3D12ShaderBindingTable->Build(*this);
}

void FD3D12CommandContext::ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (!ShaderBindingTable)
    {
        return;
    }

    FD3D12ShaderBindingTable* D3D12ShaderBindingTable = FD3D12DeviceRHI::ResourceCast(ShaderBindingTable);
    D3D12ShaderBindingTable->ClearTableRecords();
}

void FD3D12CommandContext::SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState)
{
    FD3D12RayTracingPipelineStateRHI* D3D12PipelineState = FD3D12DeviceRHI::ResourceCast(PipelineState);
    ContextState.SetRayTracingPipelineState(D3D12PipelineState);
}

void FD3D12CommandContext::PrepareShaderBindingTableForDispatch(FD3D12ShaderBindingTable* ShaderBindingTable)
{
    CHECK(ShaderBindingTable != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandList());

    if (FD3D12Resource* TableResource = ShaderBindingTable->GetResource())
    {
        GetCommandList().UpdateResidency(TableResource->GetResidencyHandle());
    }

    const uint32 NumLocalTableDescriptors   = ShaderBindingTable->GetNumPendingLocalTableDescriptors();
    const uint32 NumLocalSamplerDescriptors = ShaderBindingTable->GetNumPendingLocalSamplerDescriptors();

    if (NumLocalTableDescriptors > 0)
    {
        FD3D12LocalDescriptorHeap& ResourceHeap = ContextState.GetDescriptorCache().GetResourceHeap();
        if (!ResourceHeap.HasSpace(NumLocalTableDescriptors))
        {
            ResourceHeap.Realloc();
        }
    }

    if (NumLocalSamplerDescriptors > 0)
    {
        FD3D12LocalDescriptorHeap& SamplerHeap = ContextState.GetDescriptorCache().GetSamplerHeap();
        if (!SamplerHeap.HasSpace(NumLocalSamplerDescriptors))
        {
            SamplerHeap.Realloc();
        }
    }

    ContextState.BindRayTracingState();

    if (NumLocalTableDescriptors > 0 || NumLocalSamplerDescriptors > 0)
    {
        FD3D12LocalDescriptorHeap& ResourceHeap = ContextState.GetDescriptorCache().GetResourceHeap();
        FD3D12LocalDescriptorHeap& SamplerHeap  = ContextState.GetDescriptorCache().GetSamplerHeap();

        ShaderBindingTable->ResolveLocalDescriptorTables(*this, ResourceHeap, SamplerHeap);
        BarrierBatcher.FlushBarriers(GetCommandList());
    }
}

void FD3D12CommandContext::DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    FD3D12RayTracingPipelineStateRHI* D3D12PipelineState = ContextState.GetRayTracingPipelineState();
    CHECK(D3D12PipelineState != nullptr);

    FD3D12ShaderBindingTable* D3D12ShaderBindingTable = FD3D12DeviceRHI::ResourceCast(ShaderBindingTable);
    CHECK(D3D12ShaderBindingTable != nullptr);
    CHECK(D3D12ShaderBindingTable->GetPipeline() == D3D12PipelineState);

    if (Width == 0 || Height == 0 || Depth == 0)
    {
        return;
    }

    ConditionalSplitCommandList();
    PrepareShaderBindingTableForDispatch(D3D12ShaderBindingTable);
    CommandList->GetGraphicsCommandList4()->SetPipelineState1(D3D12PipelineState->GetD3D12StateObject());

    D3D12_DISPATCH_RAYS_DESC RayDispatchDesc = {};
    RayDispatchDesc.RayGenerationShaderRecord = D3D12ShaderBindingTable->GetRayGenRecord();
    RayDispatchDesc.MissShaderTable           = D3D12ShaderBindingTable->GetMissTable();
    RayDispatchDesc.HitGroupTable             = D3D12ShaderBindingTable->GetHitGroupTable();
    RayDispatchDesc.CallableShaderTable       = D3D12ShaderBindingTable->GetCallableTable();
    RayDispatchDesc.Width                     = Width;
    RayDispatchDesc.Height                    = Height;
    RayDispatchDesc.Depth                     = Depth;

    CommandList->GetGraphicsCommandList4()->DispatchRays(&RayDispatchDesc);
#else
    UNREFERENCED_VARIABLE(ShaderBindingTable);
    UNREFERENCED_VARIABLE(Width);
    UNREFERENCED_VARIABLE(Height);
    UNREFERENCED_VARIABLE(Depth);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: DispatchRays requires ID3D12GraphicsCommandList4");
#endif
}

void FD3D12CommandContext::BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc)
{
#if D3D12_ENABLE_OPACITY_MICROMAPS
    if (FD3D12OpacityMicromapRHI* D3D12OpacityMicromap = FD3D12DeviceRHI::ResourceCast(OpacityMicromap))
    {
        D3D12OpacityMicromap->Build(*this, BuildDesc);
    }
#else
    UNREFERENCED_VARIABLE(OpacityMicromap);
    UNREFERENCED_VARIABLE(BuildDesc);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: BuildOpacityMicromap is not supported - opacity micromaps require a newer Agility SDK.");
#endif
}

void FD3D12CommandContext::ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations)
{
#if D3D12_ENABLE_INDIRECT_RTAS_OPERATIONS
    #error "D3D12_ENABLE_INDIRECT_RTAS_OPERATIONS is on but the ExecuteIndirectRTASOperations path is not implemented for the integrated SDK yet."
#else
    UNREFERENCED_VARIABLE(Operations);
    UNREFERENCED_VARIABLE(NumOperations);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: ExecuteIndirectRayTracingAccelerationStructureOperations is not supported - Clusters/PTLAS require a DXR 2.0 SDK.");
#endif
}

void FD3D12CommandContext::DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    FD3D12RayTracingPipelineStateRHI* D3D12PipelineState = ContextState.GetRayTracingPipelineState();
    CHECK(D3D12PipelineState != nullptr);

    FD3D12ShaderBindingTable* D3D12ShaderBindingTable = FD3D12DeviceRHI::ResourceCast(ShaderBindingTable);
    CHECK(D3D12ShaderBindingTable != nullptr);
    CHECK(D3D12ShaderBindingTable->GetPipeline() == D3D12PipelineState);

    FD3D12BufferRHI* D3D12ArgumentBuffer = FD3D12DeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(D3D12ArgumentBuffer != nullptr);

    ID3D12CommandSignature* CommandSignature = GetDevice()->GetCommandSignature(ED3D12CommandSignatureType::DispatchRays);
    if (!CommandSignature)
    {
        return;
    }

    ConditionalSplitCommandList();
    PrepareShaderBindingTableForDispatch(D3D12ShaderBindingTable);

    FD3D12Resource* ArgumentResource = D3D12ArgumentBuffer->GetResource();
    CHECK(ArgumentResource != nullptr);

    TransitionTrackedResourceState(ArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    BarrierBatcher.FlushBarriers(GetCommandList());
    GetCommandList().UpdateResidency(ArgumentResource->GetResidencyHandle());

    CommandList->GetGraphicsCommandList4()->SetPipelineState1(D3D12PipelineState->GetD3D12StateObject());

    const uint64 NativeArgumentOffset = D3D12ArgumentBuffer->GetResourceStorage().GetResourceOffset() + ArgumentBufferOffset;
    CommandList->GetGraphicsCommandList4()->ExecuteIndirect(
        CommandSignature,
        1,
        ArgumentResource->GetD3D12Resource(),
        NativeArgumentOffset,
        nullptr,
        0);
#else
    UNREFERENCED_VARIABLE(ShaderBindingTable);
    UNREFERENCED_VARIABLE(ArgumentBuffer);
    UNREFERENCED_VARIABLE(ArgumentBufferOffset);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: DispatchRaysIndirect requires ID3D12GraphicsCommandList4");
#endif
}

void FD3D12CommandContext::WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    CHECK(DstBuffer != nullptr);
    CHECK(Sources != nullptr && NumSources > 0);

    FD3D12BufferRHI* D3D12DestinationBuffer = FD3D12DeviceRHI::ResourceCast(DstBuffer);
    CHECK(D3D12DestinationBuffer != nullptr);

    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE D3D12InfoType = ConvertAccelerationStructurePostBuildInfoType(InfoType);

    TArray<D3D12_GPU_VIRTUAL_ADDRESS> SourceAddresses;
    SourceAddresses.Reserve(NumSources);

    for (uint32 Index = 0; Index < NumSources; ++Index)
    {
        SourceAddresses.Emplace(GetD3D12AccelerationStructureGPUAddress(Sources[Index]));
    }

    const uint64 Stride           = GetRayTracingPostBuildInfoStride(InfoType);
    const uint64 TotalSize        = static_cast<uint64>(NumSources) * Stride;
    const uint64 IntermediateSize = Math::Max<uint64>(Math::AlignUp<uint64>(TotalSize, 16ull), 16ull);

    FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
    if (!Allocator)
    {
        D3D12_ERROR("[FD3D12CommandContext]: WriteAccelerationStructurePostBuildInfo has no buffer allocator");
        return;
    }

    D3D12_RESOURCE_DESC IntermediateDesc = {};
    IntermediateDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    IntermediateDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    IntermediateDesc.Format           = DXGI_FORMAT_UNKNOWN;
    IntermediateDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    IntermediateDesc.Width            = IntermediateSize;
    IntermediateDesc.Height           = 1;
    IntermediateDesc.DepthOrArraySize = 1;
    IntermediateDesc.MipLevels        = 1;
    IntermediateDesc.SampleDesc.Count = 1;

    FD3D12ResourceStorage IntermediateStorage(GetDevice());
    const bool bAllocated = Allocator->TryAllocate(
        D3D12_HEAP_TYPE_DEFAULT,
        IntermediateDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        ED3D12ResourceStateMode::MultipleStates,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
        IntermediateStorage);

    if (!bAllocated || IntermediateStorage.GetResource() == nullptr)
    {
        D3D12_ERROR("[FD3D12CommandContext]: WriteAccelerationStructurePostBuildInfo failed to allocate intermediate UAV buffer");
        return;
    }

    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());

    FD3D12CommandList& CmdList = GetCommandList();
    CmdList.UpdateResidency(IntermediateStorage.GetResource()->GetResidencyHandle());
    CmdList.UpdateResidency(D3D12DestinationBuffer->GetResource()->GetResidencyHandle());

    if (IntermediateStorage.GetResource()->IsPlacedResource())
    {
        BarrierBatcher.AddAliasingBarrier(IntermediateStorage.GetResource());
        BarrierBatcher.FlushBarriers(CmdList);
    }

    TransitionResourceState(IntermediateStorage.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    BarrierBatcher.FlushBarriers(CmdList);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC PostBuildInfoDesc = {};
    PostBuildInfoDesc.DestBuffer = IntermediateStorage.GetGPUVirtualAddress();
    PostBuildInfoDesc.InfoType   = D3D12InfoType;

    CmdList.GetGraphicsCommandList4()->EmitRaytracingAccelerationStructurePostbuildInfo(&PostBuildInfoDesc, NumSources, SourceAddresses.Data());

    BarrierBatcher.AddUnorderedAccessBarrier(IntermediateStorage.GetResource());
    TransitionResourceState(IntermediateStorage.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionTrackedResourceState(D3D12DestinationBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
    BarrierBatcher.FlushBarriers(CmdList);

    const FD3D12ResourceStorage& DstStorage = D3D12DestinationBuffer->GetResourceStorage();
    const uint64 EffectiveDstOffset = DstOffset + DstStorage.GetResourceOffset();
    const uint64 SrcOffset = IntermediateStorage.GetResourceOffset();

    CmdList->CopyBufferRegion(
        D3D12DestinationBuffer->GetResource()->GetD3D12Resource(),
        EffectiveDstOffset,
        IntermediateStorage.GetResource()->GetD3D12Resource(),
        SrcOffset,
        TotalSize);
#else
    UNREFERENCED_VARIABLE(DstBuffer);
    UNREFERENCED_VARIABLE(DstOffset);
    UNREFERENCED_VARIABLE(InfoType);
    UNREFERENCED_VARIABLE(Sources);
    UNREFERENCED_VARIABLE(NumSources);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: WriteAccelerationStructurePostBuildInfo requires ID3D12GraphicsCommandList4");
#endif
}

void FD3D12CommandContext::CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    CHECK(Destination != nullptr && Source != nullptr);
    
    const D3D12_GPU_VIRTUAL_ADDRESS SourceAddress                         = GetD3D12AccelerationStructureGPUAddress(Source);
    const D3D12_GPU_VIRTUAL_ADDRESS DestinationAddress                    = GetD3D12AccelerationStructureGPUAddress(Destination);
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE D3D12CopyMode = ConvertAccelerationStructureCopyMode(CopyMode);

    ConditionalSplitCommandList();
    BarrierBatcher.FlushBarriers(GetCommandList());

    CommandList->GetGraphicsCommandList4()->CopyRaytracingAccelerationStructure(DestinationAddress, SourceAddress, D3D12CopyMode);
#else
    UNREFERENCED_VARIABLE(Destination);
    UNREFERENCED_VARIABLE(Source);
    UNREFERENCED_VARIABLE(CopyMode);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: CopyAccelerationStructure requires ID3D12GraphicsCommandList4");
#endif
}

void FD3D12CommandContext::CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    if (!AccelerationStructure || CompactedSizeInBytes == 0)
    {
        return;
    }

    FD3D12AccelerationStructure* D3D12AccelerationStructure = nullptr;
    switch (AccelerationStructure->GetAccelerationStructureType())
    {
        case ERayTracingAccelerationStructureType::Geometry:
        {
            D3D12AccelerationStructure = static_cast<FD3D12GeometryAccelerationStructureRHI*>(AccelerationStructure);
            break;
        }

        case ERayTracingAccelerationStructureType::Scene:
        {
            D3D12AccelerationStructure = static_cast<FD3D12SceneAccelerationStructureRHI*>(AccelerationStructure);
            break;
        }

        default:
        {
            break;
        }
    }

    if (D3D12AccelerationStructure)
    {
        D3D12AccelerationStructure->CompactInPlace(*this, CompactedSizeInBytes);
    }
#else
    UNREFERENCED_VARIABLE(AccelerationStructure);
    UNREFERENCED_VARIABLE(CompactedSizeInBytes);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: CompactAccelerationStructure requires ID3D12GraphicsCommandList4");
#endif
}

void FD3D12CommandContext::SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    CHECK(DstBuffer != nullptr && Source != nullptr);

    FD3D12BufferRHI* D3D12DestinationBuffer = FD3D12DeviceRHI::ResourceCast(DstBuffer);
    CHECK(D3D12DestinationBuffer != nullptr);

    const D3D12_GPU_VIRTUAL_ADDRESS DestinationAddress = D3D12DestinationBuffer->GetGPUVirtualAddress() + DstOffset;
    const D3D12_GPU_VIRTUAL_ADDRESS SourceAddress      = GetD3D12AccelerationStructureGPUAddress(Source);

    ConditionalSplitCommandList();

    TransitionTrackedResourceState(D3D12DestinationBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    BarrierBatcher.FlushBarriers(GetCommandList());

    CommandList->GetGraphicsCommandList4()->CopyRaytracingAccelerationStructure(DestinationAddress, SourceAddress, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE);
#else
    UNREFERENCED_VARIABLE(Source);
    UNREFERENCED_VARIABLE(DstBuffer);
    UNREFERENCED_VARIABLE(DstOffset);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: SerializeAccelerationStructure requires ID3D12GraphicsCommandList4");
#endif
}

void FD3D12CommandContext::DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    CHECK(Destination != nullptr && SourceBuffer != nullptr);

    FD3D12BufferRHI* D3D12SourceBuffer = FD3D12DeviceRHI::ResourceCast(SourceBuffer);
    CHECK(D3D12SourceBuffer != nullptr);

    const D3D12_GPU_VIRTUAL_ADDRESS DestinationAddress = GetD3D12AccelerationStructureGPUAddress(Destination);
    const D3D12_GPU_VIRTUAL_ADDRESS SourceAddress      = D3D12SourceBuffer->GetGPUVirtualAddress() + SourceOffset;

    ConditionalSplitCommandList();

    TransitionTrackedResourceState(D3D12SourceBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    BarrierBatcher.FlushBarriers(GetCommandList());

    CommandList->GetGraphicsCommandList4()->CopyRaytracingAccelerationStructure(DestinationAddress, SourceAddress, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE);
#else
    UNREFERENCED_VARIABLE(Destination);
    UNREFERENCED_VARIABLE(SourceBuffer);
    UNREFERENCED_VARIABLE(SourceOffset);
    D3D12_ERROR_CRITICAL("[FD3D12CommandContext]: DeserializeAccelerationStructure requires ID3D12GraphicsCommandList4");
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

void FD3D12CommandContext::PushEvent(const StringView& Name)
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
