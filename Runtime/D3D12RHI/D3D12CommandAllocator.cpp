#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"

FD3D12CommandAllocator::FD3D12CommandAllocator(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , Allocator(nullptr)
    , QueueType(InQueueType)
{
}

bool FD3D12CommandAllocator::Initialize()
{
    const D3D12_COMMAND_LIST_TYPE Type = ToCommandListType(QueueType);

    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
    if (SUCCEEDED(Result))
    {
        D3D12_INFO("[FD3D12CommandAllocator]: Created CommandAllocator");
        return true;
    }
    else
    {
        D3D12_ERROR("[FD3D12CommandAllocator]: FAILED to create CommandAllocator");
        return false;
    }
}

bool FD3D12CommandAllocator::Reset()
{
    HRESULT Result = Allocator->Reset();
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }

    return SUCCEEDED(Result);
}

bool FD3D12CommandAllocator::IsFinished() const
{
    FD3D12CommandListManager* CommandListManager = GetDevice()->GetCommandListManager(QueueType);
    CHECK(CommandListManager != nullptr);
    return SyncPoint.FenceValue <= CommandListManager->GetFenceManager().GetCompletedValue();
}


FD3D12CommandAllocatorManager::FD3D12CommandAllocatorManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandListType(ToCommandListType(QueueType))
    , AvailableAllocators()
    , CommandAllocators()
{
}

FD3D12CommandAllocatorManager::~FD3D12CommandAllocatorManager()
{
    DestroyAllocators();
}

FD3D12CommandAllocator* FD3D12CommandAllocatorManager::ObtainAllocator()
{
    FD3D12CommandAllocator* CommandAllocator;
    if (!AvailableAllocators.IsEmpty() && (*AvailableAllocators.Peek())->IsFinished())
    {
        AvailableAllocators.Dequeue(CommandAllocator);
        if (!CommandAllocator->Reset())
        {
            DEBUG_BREAK();
            return nullptr;
        }
    }
    else
    {
        CommandAllocator = new FD3D12CommandAllocator(GetDevice(), QueueType);
        if (!CommandAllocator->Initialize())
        {
            DEBUG_BREAK();
            return nullptr;
        }

        CommandAllocators.Add(CommandAllocator);
    }

    return CommandAllocator;
}

void FD3D12CommandAllocatorManager::ReleaseAllocator(FD3D12CommandAllocator* InAllocator)
{
    CHECK(InAllocator != nullptr);

    FD3D12CommandListManager* CommandListManager = GetDevice()->GetCommandListManager(QueueType);
    CHECK(CommandListManager != nullptr);

    const FD3D12FenceSyncPoint SyncPoint(CommandListManager->GetFenceManager().GetFence(), CommandListManager->GetFenceManager().GetCurrentValue());
    InAllocator->SetSyncPoint(SyncPoint);
    AvailableAllocators.Enqueue(InAllocator);
}

void FD3D12CommandAllocatorManager::DestroyAllocators()
{
    for (FD3D12CommandAllocator* CommandAllocator : CommandAllocators)
    {
        delete CommandAllocator;
    }

    CommandAllocators.Clear();
    AvailableAllocators.Clear();
}