#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/RHIQuery.h"
#include "D3D12RHI/D3D12Queue.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12DeviceDebug.h"

static TAutoConsoleVariable<int32> CVarMaxPendingSubmissions(
    "D3D12RHI.MaxPendingSubmissions",
    "Maximum number of pending GPU submissions before the CPU waits for the GPU to catch up",
    32);

static TAutoConsoleVariable<bool> CVarEnableGPUTimeout(
    "D3D12RHI.EnableGPUTimeout",
    "Enables or disables the GPU timeout on all ID3D12CommandQueues",
    true);

static TAutoConsoleVariable<int32> CVarCommandContextMaxIdleFrames(
    "D3D12RHI.CommandContextPool.MaxIdleFrames",
    "Number of frames a pooled CommandContext may sit unused before it is destroyed",
    16);

static TAutoConsoleVariable<int32> CVarCommandContextMinRetained(
    "D3D12RHI.CommandContextPool.MinRetained",
    "Number of CommandContexts the pool keeps alive regardless of how long they have been idle",
    2);

FD3D12Queue::FD3D12Queue(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandListType(ToCommandListType(InQueueType))
    , Frequency(0)
    , SubmissionFence(nullptr)
    , CommandQueue(nullptr)
{
}

FD3D12Queue::~FD3D12Queue()
{
    WaitForCompletion();
    ProcessCommandQueue();

    CommandContextPool.DestroyAll();
    CommandListPool.DestroyAll();
    AllocatorPool.DestroyAll();
}

bool FD3D12Queue::Initialize()
{
    D3D12_COMMAND_QUEUE_DESC Desc = {};
    Desc.Type     = CommandListType;
    Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    Desc.NodeMask = GetDevice()->GetNodeMask();
    Desc.Flags    = CVarEnableGPUTimeout.GetValue() ? D3D12_COMMAND_QUEUE_FLAG_NONE : D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;

    TComPtr<ID3D12CommandQueue> NewCommandQueue;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandQueue(&Desc, IID_PPV_ARGS(&NewCommandQueue));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Device]: Failed to create CommandQueue '%s'", ToString(QueueType));
        return false;
    }

    if (CommandListType == D3D12_COMMAND_LIST_TYPE_DIRECT || CommandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        UINT64 NewFrequency;
        Result = NewCommandQueue->GetTimestampFrequency(&NewFrequency);
        if (FAILED(Result))
        {
            D3D12_ERROR("[FD3D12Device]: Failed to retrieve TimestampFrequency");
            return false;
        }
        else
        {
            Frequency = NewFrequency;
        }
    }

    const WString WideName = CharToWide(String::CreateFormatted("CommandQueue %s", ToString(QueueType)));
    NewCommandQueue->SetName(*WideName);

    D3D12_INFO("[FD3D12Device]: Created CommandQueue '%s'", ToString(QueueType));
    CommandQueue = NewCommandQueue;

    SubmissionFence = new FD3D12Fence(GetDevice());
    if (!SubmissionFence->Initialize(0))
    {
        return false;
    }

    SubmissionFence->SetDebugName(String::CreateFormatted("SubmissionFence_%s", ToString(QueueType)));
    return true;
}

FD3D12CommandList* FD3D12Queue::ObtainCommandList(FD3D12CommandAllocator* CommandAllocator, ID3D12PipelineState* InitialPipelineState)
{
    FD3D12CommandList* CommandList = CommandListPool.Acquire([&](int32 Index) -> FD3D12CommandList*
    {
        FD3D12CommandList* NewCommandList = new FD3D12CommandList(GetDevice());
        if (!NewCommandList->Initialize(CommandListType, CommandAllocator, InitialPipelineState))
        {
            delete NewCommandList;
            return nullptr;
        }

        NewCommandList->SetDebugName(String::CreateFormatted("%s CommandList %d", ToString(CommandListType), Index));
        return NewCommandList;
    });

    if (!CommandList || !CommandList->Reset(CommandAllocator))
    {
        DEBUG_BREAK();
        return nullptr;
    }

    return CommandList;
}

void FD3D12Queue::RecycleCommandList(FD3D12CommandList* InCommandList)
{
    CommandListPool.Release(InCommandList);
}

FD3D12CommandAllocator* FD3D12Queue::ObtainAllocator()
{
    return AllocatorPool.Acquire([this](int32 Index) -> FD3D12CommandAllocator*
    {
        FD3D12CommandAllocator* NewAllocator = new FD3D12CommandAllocator(GetDevice(), QueueType);
        if (!NewAllocator->Initialize())
        {
            DEBUG_BREAK();
            delete NewAllocator;
            return nullptr;
        }

        NewAllocator->SetDebugName(String::CreateFormatted("%s CommandAllocator %d", ToString(CommandListType), Index));
        return NewAllocator;
    });
}

void FD3D12Queue::RecycleAllocator(FD3D12CommandAllocator* InAllocator)
{
    CHECK(InAllocator != nullptr);

    // Unlike a command list, an allocator resets on release rather than acquire.
    if (!InAllocator->Reset())
    {
        DEBUG_BREAK();
    }

    AllocatorPool.Release(InAllocator);
}

FD3D12CommandContext* FD3D12Queue::ObtainCommandContext()
{
    FD3D12CommandContext* CommandContext = CommandContextPool.Acquire([this](int32) -> FD3D12CommandContext*
    {
        FD3D12CommandContext* NewCommandContext = new FD3D12CommandContext(GetDevice(), *this);
        if (!NewCommandContext->Initialize())
        {
            DEBUG_BREAK();
            delete NewCommandContext;
            return nullptr;
        }

        return NewCommandContext;
    });

    if (!CommandContext)
    {
        D3D12_ERROR_CRITICAL("Failed to Obtain CommandContext");
    }

    return CommandContext;
}

void FD3D12Queue::ReleaseCommandContext(FD3D12CommandContext* InContext)
{
    CHECK(InContext != nullptr);
    CHECK(!InContext->IsRecording());

    InContext->RetireTransientObjects();

    InContext->SetLastUsedFrame(CurrentFrame.Load());
    CommandContextPool.Release(InContext);
}

void FD3D12Queue::PruneCommandContexts(uint64 InCurrentFrame)
{
    CurrentFrame.Store(InCurrentFrame);

    const uint64 MaxIdleFrames = static_cast<uint64>(Math::Max<int32>(0, CVarCommandContextMaxIdleFrames.GetValue()));
    const int32  MinRetained   = Math::Max<int32>(0, CVarCommandContextMinRetained.GetValue());

    CommandContextPool.PruneFree(MinRetained, [InCurrentFrame, MaxIdleFrames](FD3D12CommandContext* Context)
    {
        return (InCurrentFrame - Context->GetLastUsedFrame()) > MaxIdleFrames;
    });
}

FD3D12FenceSyncPoint FD3D12Queue::ExecuteCommandList(FD3D12CommandList* InCommandList, bool bWaitForCompletion)
{
	if (!InCommandList)
	{
		const uint64 FenceValue = SubmissionFence->Signal(CommandQueue.Get());
		if (bWaitForCompletion)
		{
			SubmissionFence->WaitForValue(FenceValue);
		}

		return FD3D12FenceSyncPoint(SubmissionFence.Get(), FenceValue);
	}

    ID3D12CommandList* CommandList = InCommandList->GetCommandList();
    CommandQueue->ExecuteCommandLists(1, &CommandList);

    const uint64 FenceValue = SubmissionFence->Signal(CommandQueue.Get());
    if (bWaitForCompletion)
    {
        SubmissionFence->WaitForValue(FenceValue);
    }

    return FD3D12FenceSyncPoint(SubmissionFence.Get(), FenceValue);
}

FD3D12FenceSyncPoint FD3D12Queue::ExecuteCommandLists(FD3D12CommandList* const* InCommandLists, uint32 NumCommandLists, bool bWaitForCompletion)
{
	if (!InCommandLists || NumCommandLists == 0)
	{
        const uint64 FenceValue = SubmissionFence->Signal(CommandQueue.Get());
        if (bWaitForCompletion)
        {
            SubmissionFence->WaitForValue(FenceValue);
        }

        return FD3D12FenceSyncPoint(SubmissionFence.Get(), FenceValue);
    }

    TArray<ID3D12CommandList*> D3DCommandLists;
    D3DCommandLists.Reserve(NumCommandLists);

    for (uint32 Index = 0; Index < NumCommandLists; Index++)
    {
        D3DCommandLists.Add(InCommandLists[Index]->GetCommandList());
    }

    CommandQueue->ExecuteCommandLists(D3DCommandLists.Size(), D3DCommandLists.Data());

#if D3D12_ENABLE_DEVICE_LOST_CHECK
    if (GetDevice()->GetD3D12Device()->GetDeviceRemovedReason() != S_OK)
    {
        D3D12RHIDeviceRemovedHandler(GetDevice(), "ExecuteCommandLists");
    }
#endif

    const uint64 FenceValue = SubmissionFence->Signal(CommandQueue.Get());
    if (bWaitForCompletion)
    {
        SubmissionFence->WaitForValue(FenceValue);
    }

    return FD3D12FenceSyncPoint(SubmissionFence.Get(), FenceValue);
}

FD3D12FenceSyncPoint FD3D12Queue::SubmitCommands(FD3D12Commands* Commands)
{
    CHECK(Commands != nullptr);
    if (Commands->IsEmpty())
    {
        return FD3D12FenceSyncPoint();
    }

    TScopedLock Lock(SubmissionCS);

    Commands->PreExecute();

    const bool bResolveQueries = IsEnumFlagSet(Commands->Flags, ED3D12CommandsFlags::ResolveQueries);
    if (bResolveQueries)
    {
        if (!PendingQueryRanges.IsEmpty())
        {
            Commands->QueryRanges.Insert(0, PendingQueryRanges);
            PendingQueryRanges.Clear();
        }

        if (!PendingTimestampQueries.IsEmpty())
        {
            Commands->TimestampQueries.Insert(0, PendingTimestampQueries);
            PendingTimestampQueries.Clear();
        }

        if (!PendingOcclusionQueries.IsEmpty())
        {
            Commands->OcclusionQueries.Insert(0, PendingOcclusionQueries);
            PendingOcclusionQueries.Clear();
        }

        if (!PendingPipelineStatsQueries.IsEmpty())
        {
            Commands->PipelineStatsQueries.Insert(0, PendingPipelineStatsQueries);
            PendingPipelineStatsQueries.Clear();
        }

        if (!PendingQueryRHIs.IsEmpty())
        {
            Commands->PendingQueries.Insert(0, PendingQueryRHIs);
            PendingQueryRHIs.Clear();
        }
    }
    else
    {
        PendingTimestampQueries.Append(Commands->TimestampQueries);
        PendingOcclusionQueries.Append(Commands->OcclusionQueries);
        PendingPipelineStatsQueries.Append(Commands->PipelineStatsQueries);
        PendingQueryRHIs.Append(Commands->PendingQueries);

        Commands->TimestampQueries.Clear();
        Commands->OcclusionQueries.Clear();
        Commands->PipelineStatsQueries.Clear();
        Commands->PendingQueries.Clear();
    }

    Commands->Execute();

    const FD3D12FenceSyncPoint SyncPoint = Commands->SyncPoint;
    PendingSubmissions.Enqueue(Commands);

    const int32 MaxPending = CVarMaxPendingSubmissions.GetValue();
    {
        SCOPED_LOCK(ConsumerCS);

        while (PendingSubmissions.Size() > MaxPending)
        {
            FD3D12Commands* Oldest = nullptr;
            if (PendingSubmissions.Peek(Oldest) && Oldest)
            {
                Oldest->SyncPoint.Wait();
                PendingSubmissions.Dequeue();
                Oldest->PostExecute();
            }
            else
            {
                break;
            }
        }
    }

    return SyncPoint;
}

void FD3D12Queue::ProcessCommandQueue()
{
    SCOPED_LOCK(ConsumerCS);

    bool bProcess = true;
    while (bProcess)
    {
        FD3D12Commands* Commands = nullptr;
        if (PendingSubmissions.Peek(Commands))
        {
            CHECK(Commands != nullptr);
            if (!Commands->SyncPoint.IsReached())
            {
                bProcess = false;
            }
            else
            {
                PendingSubmissions.Dequeue();
                Commands->PostExecute();
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FD3D12Queue::WaitForCompletion()
{
    SubmissionFence->Signal(CommandQueue.Get());
    SubmissionFence->WaitForValue(SubmissionFence->GetLastSignaledValue());
}

FD3D12Commands::FD3D12Commands(FD3D12Device* InDevice, FD3D12Queue* InQueue)
    : Queue(InQueue)
    , Device(InDevice)
    , Flags(ED3D12CommandsFlags::None)
    , SyncPoint()
    , CommandAllocators()
    , CommandLists()
    , QueryRanges()
    , TimestampQueries()
    , OcclusionQueries()
    , PipelineStatsQueries()
    , PendingQueries()
    , DeferredObjects()
{
}

void FD3D12Commands::PreExecute()
{
    for (int32 i = 0; i < CommandLists.Size(); i++)
    {
        FD3D12CommandList* CommandList = CommandLists[i];
        TimestampQueries.Append(CommandList->TimestampQueries);
        OcclusionQueries.Append(CommandList->OcclusionQueries);
        PipelineStatsQueries.Append(CommandList->PipelineStatsQueries);
        
        CommandList->TimestampQueries.Clear();
        CommandList->OcclusionQueries.Clear();
        CommandList->PipelineStatsQueries.Clear();
    }

    FD3D12BarrierBatcher BarrierBatcher;

    const auto ResolveBeforeState = [](FD3D12Resource* Resource, D3D12_RESOURCE_STATES TrackedState) -> D3D12_RESOURCE_STATES
    {
        if (TrackedState != D3D12_RESOURCE_STATE_TO_BE_DETERMINED)
        {
            return TrackedState;
        }

        return Resource->HasDefaultState() ? Resource->GetDefaultState() : D3D12_RESOURCE_STATE_COMMON;
    };

    const auto AddPendingFixup = [&BarrierBatcher](const FD3D12PendingBarrier& Pending, D3D12_RESOURCE_STATES BeforeState, uint32 Subresource)
    {
        if (BeforeState == Pending.DesiredState)
        {
            return;
        }

    #if D3D12_ENABLE_RESOURCE_STATE_VALIDATION
        String DebugName;
        Pending.Resource->GetDebugName(DebugName);
        D3D12_INFO(
            "[PendingFixup] Resource=%s Subresource=%u Global=%s(0x%X) Desired=%s(0x%X)",
            DebugName.IsEmpty() ? "<unnamed>" : *DebugName,
            Subresource,
            ToString(BeforeState),
            uint32(BeforeState),
            ToString(Pending.DesiredState),
            uint32(Pending.DesiredState));
    #endif

        BarrierBatcher.AddTransitionBarrier(Pending.Resource, BeforeState, Pending.DesiredState, Subresource);
    };

    for (const FD3D12PendingBarrier& Pending : PendingBarriers)
    {
        FD3D12ResourceState& GlobalState = Pending.Resource->GetResourceState();
        if (Pending.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            if (GlobalState.IsSingleState())
            {
                const D3D12_RESOURCE_STATES BeforeState = ResolveBeforeState(Pending.Resource, GlobalState.GetState());
                AddPendingFixup(Pending, BeforeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            }
            else
            {
                const uint32 NumSubresources = GlobalState.GetNumSubresources();
                for (uint32 i = 0; i < NumSubresources; i++)
                {
                    const D3D12_RESOURCE_STATES BeforeState = ResolveBeforeState(Pending.Resource, GlobalState.GetSubresourceState(i));
                    AddPendingFixup(Pending, BeforeState, i);
                }
            }
        }
        else
        {
            const D3D12_RESOURCE_STATES BeforeState = ResolveBeforeState(Pending.Resource, GlobalState.GetSubresourceState(Pending.Subresource));
            AddPendingFixup(Pending, BeforeState, Pending.Subresource);
        }
    }

    if (BarrierBatcher.HasPendingBarriers())
    {
        FD3D12CommandAllocator* FixupAllocator = Queue->ObtainAllocator();

        FD3D12CommandList* FixupCommandList = Queue->ObtainCommandList(FixupAllocator, nullptr);
        BarrierBatcher.FlushBarriers(*FixupCommandList);

        if (!FixupCommandList->Close())
        {
            D3D12_ERROR_CRITICAL("Failed to close fixup CommandList");
        }
        else
        {
            CommandLists.Insert(0, FixupCommandList);
            AddCommandAllocator(FixupAllocator);
        }
    }

    for (auto It = PendingResourceStates.CreateIterator(); !It.IsEnd(); ++It)
    {
        FD3D12Resource*      Resource   = It.GetKey();
        FD3D12ResourceState& LocalState = It.GetValue();
        
        Resource->GetResourceState().ApplyResolvedStates(LocalState);
    }

    PendingBarriers.Clear();
    PendingResourceStates.Clear();
}

void FD3D12Commands::Execute()
{
    TArray<FD3D12ResidencySet*> ResidencySets;
    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
    {
        ResidencySets.Reserve(CommandLists.Size());

        for (int32 CmdListIdx = 0; CmdListIdx < CommandLists.Size(); CmdListIdx++)
        {
            ResidencySets.Add(&CommandLists[CmdListIdx]->GetResidencySet());
        }

        ResidencyManager->PrepareForExecution(ResidencySets.Data(), ResidencySets.Size());
    }

    SyncPoint = Queue->ExecuteCommandLists(CommandLists.Data(), CommandLists.Size(), false);

    for (int32 QueryIdx = 0; QueryIdx < PendingQueries.Size(); QueryIdx++)
    {
        PendingQueries[QueryIdx]->SyncPoint = SyncPoint;
    }

    PendingQueries.Clear();

    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
    {
        ResidencyManager->NotifySubmitted(ResidencySets.Data(), ResidencySets.Size(), SyncPoint.GetFenceValue());
    }
}

void FD3D12Commands::PostExecute()
{
    const uint64 Frequency = Queue->GetFrequency();

    TArray<uint64> RawTicksArray;
    RawTicksArray.Resize(TimestampQueries.Size());

    for (int32 i = 0; i < TimestampQueries.Size(); i++)
    {
        RawTicksArray[i] = 0;
        const FD3D12Query& Query = TimestampQueries[i];
        if (Query.QueryHeap)
        {
            Query.CopyResult(&RawTicksArray[i]);
        }
    }

    uint64 AccumulatedIdleTicks    = 0;
    uint64 LastCommandListEndTicks = 0;
    
    bool bHaveLastCommandListEnd = false;
    for (int32 i = 0; i < TimestampQueries.Size(); i++)
    {
        const FD3D12Query& Query = TimestampQueries[i];
        
        const uint64 Ticks = RawTicksArray[i];
        if (Query.Type == ED3D12QueryType::CommandListEnd)
        {
            LastCommandListEndTicks = Ticks;
            bHaveLastCommandListEnd = true;
        }
        else if (Query.Type == ED3D12QueryType::CommandListBegin && bHaveLastCommandListEnd)
        {
            if (Ticks > LastCommandListEndTicks)
            {
                AccumulatedIdleTicks += Ticks - LastCommandListEndTicks;
            }

            bHaveLastCommandListEnd = false;
        }

        if (Query.Type == ED3D12QueryType::Timestamp && Query.ResultTarget)
        {
            const uint64 AdjustedTicks = (Ticks > AccumulatedIdleTicks) ? (Ticks - AccumulatedIdleTicks) : 0;
            const uint64 Nanoseconds   = (AdjustedTicks * 1000000000ULL) / Frequency;
            *Query.ResultTarget = Nanoseconds;
        }
    }

    TimestampQueries.Clear();

    for (int32 QueryIdx = 0; QueryIdx < OcclusionQueries.Size(); QueryIdx++)
    {
        const FD3D12Query& Query = OcclusionQueries[QueryIdx];
        if (!Query.QueryHeap || !Query.ResultTarget)
        {
            continue;
        }

        uint64 OcclusionResult = 0;
        Query.CopyResult(&OcclusionResult);
        *Query.ResultTarget = OcclusionResult;
    }

    OcclusionQueries.Clear();

    for (int32 QueryIdx = 0; QueryIdx < PipelineStatsQueries.Size(); QueryIdx++)
    {
        const FD3D12Query& Query = PipelineStatsQueries[QueryIdx];
        FD3D12QueryHeap* Heap = Query.QueryHeap;
        if (!Heap || !Query.ResultTarget)
        {
            continue;
        }

        FRHIPipelineStatistics* Stats = reinterpret_cast<FRHIPipelineStatistics*>(Query.ResultTarget);
        const D3D12_QUERY_HEAP_TYPE HeapType = Heap->QueryHeapType;

        if (HeapType == D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS)
        {
            D3D12_QUERY_DATA_PIPELINE_STATISTICS Src = {};
            Query.CopyResult(&Src);

            Stats->IAVertices    = Src.IAVertices;
            Stats->IAPrimitives  = Src.IAPrimitives;
            Stats->VSInvocations = Src.VSInvocations;
            Stats->GSInvocations = Src.GSInvocations;
            Stats->GSPrimitives  = Src.GSPrimitives;
            Stats->CInvocations  = Src.CInvocations;
            Stats->CPrimitives   = Src.CPrimitives;
            Stats->PSInvocations = Src.PSInvocations;
            Stats->HSInvocations = Src.HSInvocations;
            Stats->DSInvocations = Src.DSInvocations;
            Stats->CSInvocations = Src.CSInvocations;
            Stats->ASInvocations = 0;
            Stats->MSInvocations = 0;
            Stats->MSPrimitives  = 0;
        }
    #if D3D12_SUPPORT_PIPELINE_STATISTICS1
        else if (HeapType == D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1)
        {
            D3D12_QUERY_DATA_PIPELINE_STATISTICS1 Src = {};
            Query.CopyResult(&Src);

            Stats->IAVertices    = Src.IAVertices;
            Stats->IAPrimitives  = Src.IAPrimitives;
            Stats->VSInvocations = Src.VSInvocations;
            Stats->GSInvocations = Src.GSInvocations;
            Stats->GSPrimitives  = Src.GSPrimitives;
            Stats->CInvocations  = Src.CInvocations;
            Stats->CPrimitives   = Src.CPrimitives;
            Stats->PSInvocations = Src.PSInvocations;
            Stats->HSInvocations = Src.HSInvocations;
            Stats->DSInvocations = Src.DSInvocations;
            Stats->CSInvocations = Src.CSInvocations;
            Stats->ASInvocations = Src.ASInvocations;
            Stats->MSInvocations = Src.MSInvocations;
            Stats->MSPrimitives  = Src.MSPrimitives;
        }
    #endif
    }

    PipelineStatsQueries.Clear();

    for (int32 RangeIdx = 0; RangeIdx < QueryRanges.Size(); RangeIdx++)
    {
        FD3D12QueryHeap* Heap = QueryRanges[RangeIdx].Heap;
        Device->RecycleQueryHeap(Heap);
    }

    QueryRanges.Clear();

    for (int32 CmdListIdx = 0; CmdListIdx < CommandLists.Size(); CmdListIdx++)
    {
        Queue->RecycleCommandList(CommandLists[CmdListIdx]);
    }

    CommandLists.Clear();

    for (int32 AllocIdx = 0; AllocIdx < CommandAllocators.Size(); AllocIdx++)
    {
        Queue->RecycleAllocator(CommandAllocators[AllocIdx]);
    }

    CommandAllocators.Clear();

    FD3D12DeferredObject::ProcessItems(DeferredObjects);
    DeferredObjects.Clear();

    PendingBarriers.Clear();
    PendingResourceStates.Clear();

    delete this;
}
