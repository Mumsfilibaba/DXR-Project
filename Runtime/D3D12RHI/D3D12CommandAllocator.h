#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Fence.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"

class FD3D12CommandAllocator : public FD3D12DeviceChild
{
public:
    FD3D12CommandAllocator(const FD3D12CommandAllocator&) = delete;
    FD3D12CommandAllocator& operator=(const FD3D12CommandAllocator&) = delete;

    FD3D12CommandAllocator(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandAllocator() = default;

    bool Initialize();

    bool Reset();

    bool IsFinished() const;

    FORCEINLINE ID3D12CommandAllocator* GetD3D12Allocator() const
    {
        return Allocator.Get();
    }

    FORCEINLINE void SetSyncPoint(const FD3D12FenceSyncPoint& InSyncPoint)
    {
        SyncPoint = InSyncPoint;
    }

    FORCEINLINE void SetDebugName(const FString& Name)
    {
        FStringWide WideName = CharToWide(Name);
        Allocator->SetName(WideName.GetCString());
    }

private:
    ED3D12CommandQueueType          QueueType;
    TComPtr<ID3D12CommandAllocator> Allocator;
    FD3D12FenceSyncPoint            SyncPoint;
};

class FD3D12CommandAllocatorManager : public FD3D12DeviceChild
{
public:
    FD3D12CommandAllocatorManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandAllocatorManager();

    FD3D12CommandAllocator* ObtainAllocator();
    void ReleaseAllocator(FD3D12CommandAllocator* InAllocator);

    void DestroyAllocators();

    ED3D12CommandQueueType GetQueueType() const
    {
        return QueueType;
    }

private:
    ED3D12CommandQueueType          QueueType;
    D3D12_COMMAND_LIST_TYPE         CommandListType;
    TQueue<FD3D12CommandAllocator*> AvailableAllocators;
    TArray<FD3D12CommandAllocator*> CommandAllocators;
};