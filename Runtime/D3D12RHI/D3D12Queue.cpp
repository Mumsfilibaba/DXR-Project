#include "D3D12Queue.h"
#include "D3D12Device.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"

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
    D3D12_COMMAND_QUEUE_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Type     = CommandListType;
    Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    Desc.NodeMask = GetDevice()->GetNodeMask();
    Desc.Flags    = CVarEnableGPUTimeout.GetValue() ? D3D12_COMMAND_QUEUE_FLAG_NONE : D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;

    TComPtr<ID3D12CommandQueue> NewCommandQueue;
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandQueue(&Desc, IID_PPV_ARGS(&NewCommandQueue));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to create CommandQueue '%s'", ToString(QueueType));
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
    NewCommandQueue->SetName(WideName.GetCString());

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
    CHECK(InCommandList != nullptr);

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
    CHECK(InCommandLists != nullptr);

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

FD3D12CommandPayload::FD3D12CommandPayload(FD3D12Device* InDevice, FD3D12Queue* InQueue)
    : Queue(InQueue)
    , Device(InDevice)
    , SyncPoint()
    , CommandAllocators()
    , CommandLists()
    , QueryHeaps()
    , DeletionQueue()
{
}

void FD3D12CommandPayload::Finish()
{
    // Delete all the resources that has been queued up for destruction
    FD3D12DeferredObject::ProcessItems(DeletionQueue);
    DeletionQueue.Clear();

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
    
    // Destroy this instance after execution is finished
    delete this;
}