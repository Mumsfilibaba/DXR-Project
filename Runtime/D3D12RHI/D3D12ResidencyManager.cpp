#include "D3D12RHI/D3D12ResidencyManager.h"
#include "D3D12RHI/D3D12Device.h"

FD3D12ResidencyManager::FD3D12ResidencyManager(FD3D12Device* InDevice, bool bEnableResidency, uint64 TargetBudgetBytes)
    : Device(InDevice)
    , Adapter(Device ? Device->GetAdapter()->GetDXGIAdapter3() : nullptr)
    , TargetBudget(TargetBudgetBytes)
    , CurrentFrame(0)
    , bEnable(bEnableResidency)
{
}

FD3D12ResidencyManager::~FD3D12ResidencyManager()
{
    SCOPED_LOCK(Mutex);

    TrackedIndexByPageable.Clear();
    Tracked.Clear(true);
    Adapter = nullptr;
    bEnable = false;
    TargetBudget = 0;
    CurrentFrame = 0;
}

void FD3D12ResidencyManager::Tick()
{
    if (!bEnable)
    {
        return;
    }

    SCOPED_LOCK(Mutex);
    ++CurrentFrame;
}

int32 FD3D12ResidencyManager::FindTrackedIndexByPageable(ID3D12Pageable* InPageable) const
{
    if (!InPageable)
    {
        return -1;
    }

    const int32* FoundIndex = TrackedIndexByPageable.Find(InPageable);
    if (!FoundIndex)
    {
        return -1;
    }

    const int32 Index = *FoundIndex;
    if (Index < 0 || Index >= Tracked.Size())
    {
        return -1;
    }

    const FD3D12ResidencyTrackedPageable& Entry = Tracked[Index];
    if (!Entry.bAllocated || Entry.Pageable != InPageable)
    {
        return -1;
    }

    return Index;
}

bool FD3D12ResidencyManager::IsValidHandle(const FD3D12ResidencyHandle& Handle) const
{
    return Handle.Slot < static_cast<uint32>(Tracked.Size()) && Tracked[Handle.Slot].bAllocated && Tracked[Handle.Slot].Generation == Handle.Generation;
}

uint64 FD3D12ResidencyManager::GetCurrentUsage() const
{
    uint64 Usage = 0;
    for (const FD3D12ResidencyTrackedPageable& Entry : Tracked)
    {
        if (Entry.bAllocated && Entry.bIsResident)
        {
            Usage += Entry.SizeBytes;
        }
    }

    return Usage;
}

FD3D12ResidencyHandle FD3D12ResidencyManager::RegisterPageable(ID3D12Pageable* InPageable, uint64 InSizeBytes, bool InIsAlwaysResident)
{
    if (!bEnable || !InPageable)
    {
        return {};
    }

    SCOPED_LOCK(Mutex);

    const int32 ExistingIndex = FindTrackedIndexByPageable(InPageable);
    if (ExistingIndex >= 0)
    {
        FD3D12ResidencyTrackedPageable& Entry = Tracked[ExistingIndex];
        Entry.SizeBytes       = InSizeBytes;
        Entry.bAlwaysResident = InIsAlwaysResident;
        Entry.bIsResident     = true;
        Entry.LastUsedFrame   = CurrentFrame;
        TrackedIndexByPageable[InPageable] = ExistingIndex;
        return { static_cast<uint32>(ExistingIndex), Entry.Generation };
    }

    int32 FreeIndex = -1;
    for (int32 Index = 0; Index < Tracked.Size(); ++Index)
    {
        if (!Tracked[Index].bAllocated)
        {
            FreeIndex = Index;
            break;
        }
    }

    if (FreeIndex < 0)
    {
        FD3D12ResidencyTrackedPageable Entry = {};
        Entry.Pageable        = InPageable;
        Entry.SizeBytes       = InSizeBytes;
        Entry.LastUsedFrame   = CurrentFrame;
        Entry.Generation      = 1;
        Entry.bAlwaysResident = InIsAlwaysResident;
        Entry.bIsResident     = true;
        Entry.bAllocated      = true;
        Tracked.Add(Entry);

        const uint32 Slot = static_cast<uint32>(Tracked.Size() - 1);
        TrackedIndexByPageable[InPageable] = static_cast<int32>(Slot);
        return { Slot, Tracked[Slot].Generation };
    }

    FD3D12ResidencyTrackedPageable& FreeEntry = Tracked[FreeIndex];
    FreeEntry.Pageable        = InPageable;
    FreeEntry.SizeBytes       = InSizeBytes;
    FreeEntry.LastUsedFrame   = CurrentFrame;
    FreeEntry.bAlwaysResident = InIsAlwaysResident;
    FreeEntry.bIsResident     = true;
    FreeEntry.bAllocated      = true;
    ++FreeEntry.Generation;
    TrackedIndexByPageable[InPageable] = FreeIndex;

    return { static_cast<uint32>(FreeIndex), FreeEntry.Generation };
}

void FD3D12ResidencyManager::UnregisterPageable(const FD3D12ResidencyHandle& Handle)
{
    if (!bEnable || !Handle.IsValid())
    {
        return;
    }

    SCOPED_LOCK(Mutex);

    if (!IsValidHandle(Handle))
    {
        return;
    }

    FD3D12ResidencyTrackedPageable& Entry = Tracked[Handle.Slot];
    if (Entry.Pageable)
    {
        TrackedIndexByPageable.Remove(Entry.Pageable);
    }
    
    Entry.Pageable        = nullptr;
    Entry.SizeBytes       = 0;
    Entry.LastUsedFrame   = CurrentFrame;
    Entry.bAlwaysResident = false;
    Entry.bIsResident     = false;
    Entry.bAllocated      = false;
    ++Entry.Generation;
}

void FD3D12ResidencyManager::TouchPageable(const FD3D12ResidencyHandle& Handle)
{
    if (!bEnable || !Handle.IsValid())
    {
        return;
    }

    SCOPED_LOCK(Mutex);

    if (IsValidHandle(Handle))
    {
        Tracked[Handle.Slot].LastUsedFrame = CurrentFrame;
    }
}

void FD3D12ResidencyManager::TouchPageable(ID3D12Pageable* InPageable)
{
    if (!bEnable || !InPageable)
    {
        return;
    }

    SCOPED_LOCK(Mutex);
    const int32 Index = FindTrackedIndexByPageable(InPageable);
    if (Index >= 0)
    {
        Tracked[Index].LastUsedFrame = CurrentFrame;
    }
}

void FD3D12ResidencyManager::MakeResident(ID3D12CommandQueue* InQueue, ID3D12Pageable* InPageable)
{
    UNREFERENCED_VARIABLE(InQueue);

    if (!bEnable || !Device || !InPageable)
    {
        return;
    }

    ID3D12Pageable* Pageables[] = { InPageable };
    if (FAILED(Device->GetD3D12Device()->MakeResident(1, Pageables)))
    {
        D3D12_ERROR("[FD3D12ResidencyManager] MakeResident failed");
        return;
    }

    SCOPED_LOCK(Mutex);
    const int32 Index = FindTrackedIndexByPageable(InPageable);
    if (Index >= 0)
    {
        Tracked[Index].bIsResident = true;
        Tracked[Index].LastUsedFrame = CurrentFrame;
    }
}

void FD3D12ResidencyManager::EvictIfNeeded(ID3D12CommandQueue* InQueue)
{
    UNREFERENCED_VARIABLE(InQueue);

    if (!bEnable || !Device)
    {
        return;
    }

    uint64 Budget = TargetBudget;
    if (Budget == 0 && Adapter)
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO Info = {};
        if (SUCCEEDED(Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &Info)))
        {
            Budget = Info.Budget;
        }
    }

    if (Budget == 0)
    {
        return;
    }

    SCOPED_LOCK(Mutex);
    
    uint64 Usage = GetCurrentUsage();
    while (Usage > Budget)
    {
        int32 OldestIndex = -1;
        uint64 OldestFrame = UINT64_MAX;

        for (int32 Index = 0; Index < Tracked.Size(); ++Index)
        {
            const FD3D12ResidencyTrackedPageable& Entry = Tracked[Index];
            if (!Entry.bAllocated || Entry.bAlwaysResident || !Entry.bIsResident || !Entry.Pageable)
            {
                continue;
            }

            if (Entry.LastUsedFrame < OldestFrame)
            {
                OldestFrame = Entry.LastUsedFrame;
                OldestIndex = Index;
            }
        }

        if (OldestIndex < 0)
        {
            break;
        }

        FD3D12ResidencyTrackedPageable& Victim = Tracked[OldestIndex];
        
        ID3D12Pageable* Pageables[] = { Victim.Pageable };
        if (FAILED(Device->GetD3D12Device()->Evict(1, Pageables)))
        {
            D3D12_ERROR("[FD3D12ResidencyManager] Evict failed");
            break;
        }

        Victim.bIsResident = false;
        Usage = Usage > Victim.SizeBytes ? (Usage - Victim.SizeBytes) : 0;
    }
}
