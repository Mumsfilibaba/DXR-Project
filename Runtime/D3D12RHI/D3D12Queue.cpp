#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "D3D12RHI/D3D12Queue.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12CommandContext.h"

static TAutoConsoleVariable<bool> CVarEnableGPUTimeout(
    "D3D12RHI.EnableGPUTimeout",
    "Enables or disables the GPU timeout on all ID3D12CommandQueues",
    true);

FD3D12Queue::FD3D12Queue(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandListType(ToCommandListType(InQueueType))
    , Frequency(0)
    , FenceManager(InDevice)
    , CommandQueue(nullptr)
    , CommandLists()
{
}

FD3D12Queue::~FD3D12Queue()
{
    TScopedLock Lock(CommandListsCS);

    for (FD3D12CommandList* CommandList : CommandLists)
    {
        delete CommandList;
    }
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

    const FStringWide WideName = CharToWide(FString::CreateFormatted("CommandQueue %s", ToString(QueueType)));
    NewCommandQueue->SetName(*WideName);

    D3D12_INFO("[FD3D12Device]: Created CommandQueue '%s'", ToString(QueueType));
    CommandQueue = NewCommandQueue;

    // Fences
    if (!FenceManager.Initialize())
    {
        return false;
    }

    return true;
}

FD3D12CommandList* FD3D12Queue::ObtainCommandList(FD3D12CommandAllocator* CommandAllocator, ID3D12PipelineState* InitialPipelineState)
{
    TScopedLock Lock(CommandListsCS);

    FD3D12CommandList* CommandList;
    if (AvailableCommandLists.IsEmpty())
    {
        CommandList = new FD3D12CommandList(GetDevice());
        if (!CommandList->Initialize(CommandListType, CommandAllocator, InitialPipelineState))
        {
            return nullptr;
        }

        if (!CommandList->Reset(CommandAllocator))
        {
            return nullptr;
        }

        CommandLists.Add(CommandList);
    }
    else
    {
        AvailableCommandLists.Dequeue(CommandList);
        if (!CommandList->Reset(CommandAllocator))
        {
            DEBUG_BREAK();
            return nullptr;
        }
    }

    return CommandList;
}

void FD3D12Queue::RecycleCommandList(FD3D12CommandList* InCommandList)
{
    CHECK(InCommandList != nullptr);
    
    TScopedLock Lock(CommandListsCS);
    AvailableCommandLists.Enqueue(InCommandList);
}

FD3D12FenceSyncPoint FD3D12Queue::ExecuteCommandList(FD3D12CommandList* InCommandList, bool bWaitForCompletion)
{
	if (!InCommandList)
	{
		// Still return a sync point on the current fence value for symmetry.
		const uint64 FenceValue = FenceManager.SignalGPU(QueueType);
		if (bWaitForCompletion)
		{
			FenceManager.WaitForFence(FenceValue);
		}

		return FD3D12FenceSyncPoint(FenceManager.GetFence(), FenceValue);
	}

    ID3D12CommandList* CommandList = InCommandList->GetCommandList();
    CommandQueue->ExecuteCommandLists(1, &CommandList);

    const uint64 FenceValue = FenceManager.SignalGPU(QueueType);
    if (bWaitForCompletion)
    {
        FenceManager.WaitForFence(FenceValue);
    }

    return FD3D12FenceSyncPoint(FenceManager.GetFence(), FenceValue);
}

FD3D12FenceSyncPoint FD3D12Queue::ExecuteCommandLists(FD3D12CommandList* const* InCommandLists, uint32 NumCommandLists, bool bWaitForCompletion)
{
	if (!InCommandLists || NumCommandLists == 0)
	{
	    // Still return a sync point on the current fence value for symmetry.
        const uint64 FenceValue = FenceManager.SignalGPU(QueueType);
        if (bWaitForCompletion)
        {
            FenceManager.WaitForFence(FenceValue);
        }

        return FD3D12FenceSyncPoint(FenceManager.GetFence(), FenceValue);
    }

    TArray<ID3D12CommandList*> D3DCommandLists;
    D3DCommandLists.Reserve(NumCommandLists);

    for (uint32 Index = 0; Index < NumCommandLists; Index++)
    {
        D3DCommandLists.Add(InCommandLists[Index]->GetCommandList());
    }

    CommandQueue->ExecuteCommandLists(D3DCommandLists.Size(), D3DCommandLists.Data());

    const uint64 FenceValue = FenceManager.SignalGPU(QueueType);
    if (bWaitForCompletion)
    {
        FenceManager.WaitForFence(FenceValue);
    }

    return FD3D12FenceSyncPoint(FenceManager.GetFence(), FenceValue);
}

FD3D12Commands::FD3D12Commands(FD3D12Device* InDevice, FD3D12Queue* InQueue)
    : Queue(InQueue)
    , Device(InDevice)
    , SyncPoint()
    , CommandAllocators()
    , CommandLists()
    , QueryHeaps()
    , DeletionQueue()
{
}

void FD3D12Commands::PreExecute()
{
    FD3D12BarrierBatcher BarrierBatcher;

    for (const FD3D12PendingBarrier& Pending : PendingBarriers)
    {
        FD3D12ResourceState& GlobalState = Pending.Resource->GetResourceState();

        if (Pending.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            if (GlobalState.AreAllSubresourcesSameState())
            {
                BarrierBatcher.AddTransitionBarrier(Pending.Resource, GlobalState.GetResourceState(), Pending.DesiredState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            }
            else
            {
                const uint32 NumSubresources = GlobalState.GetNumSubresources();
                for (uint32 i = 0; i < NumSubresources; i++)
                {
                    BarrierBatcher.AddTransitionBarrier(Pending.Resource, GlobalState.GetSubresourceState(i), Pending.DesiredState, i);
                }
            }
        }
        else
        {
            BarrierBatcher.AddTransitionBarrier(Pending.Resource, GlobalState.GetSubresourceState(Pending.Subresource), Pending.DesiredState, Pending.Subresource);
        }
    }

    if (BarrierBatcher.HasPendingBarriers())
    {
        FD3D12CommandAllocator* FixupAllocator = Device->GetCommandAllocatorManager(Queue->GetQueueType())->ObtainAllocator();

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
        Resource->GetResourceState() = LocalState;
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

        for (FD3D12CommandList* CmdList : CommandLists)
        {
            ResidencySets.Add(&CmdList->GetResidencySet());
        }

        ResidencyManager->PrepareForExecution(ResidencySets.Data(), ResidencySets.Size());
    }

    SyncPoint = Queue->ExecuteCommandLists(CommandLists.Data(), CommandLists.Size(), false);

    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
    {
        ResidencyManager->NotifySubmitted(ResidencySets.Data(), ResidencySets.Size(), SyncPoint.FenceValue);
    }
}

void FD3D12Commands::Finish()
{
    for (FD3D12QueryHeap* QueryHeap : QueryHeaps)
    {
        FD3D12QueryHeapManager* QueryHeapManager = QueryHeap->GetQueryHeapManager();
        QueryHeap->ReadBackResults(*Queue);
        QueryHeapManager->RecycleQueryHeap(QueryHeap);
    }

    QueryHeaps.Clear();

    // Recycle all the CommandLists
    for (FD3D12CommandList* CommandList : CommandLists)
    {
        Queue->RecycleCommandList(CommandList);
    }

    CommandLists.Clear();

    // Recycle all the CommandAllocators
    for (FD3D12CommandAllocator* CommandAllocator : CommandAllocators)
    {
        FD3D12CommandAllocatorManager* CommandAllocatorManager = Device->GetCommandAllocatorManager(CommandAllocator->GetQueueType());
        CommandAllocatorManager->RecycleAllocator(CommandAllocator);
    }
    
    CommandAllocators.Clear();

    // Delete all the resources that has been queued up for destruction
    FD3D12DeferredObject::ProcessItems(DeletionQueue);
    DeletionQueue.Clear();
    
    // Destroy this instance after execution is finished
    delete this;
}
