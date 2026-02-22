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

    for (FD3D12ResidencyHandle* Handle : TrackedObjects)
    {
        if (Handle)
        {
            Handle->bIsTracked = false;
        }
    }

    TrackedObjects.Clear(true);
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

void FD3D12ResidencyManager::BeginTrackingObject(FD3D12ResidencyHandle* Handle)
{
    if (!bEnable || !Handle || !Handle->IsInitialized())
    {
        return;
    }

    SCOPED_LOCK(Mutex);

    if (Handle->bIsTracked)
    {
        return;
    }

    Handle->bIsTracked    = true;
    Handle->LastUsedFrame = CurrentFrame;
    TrackedObjects.Add(Handle);
}

void FD3D12ResidencyManager::EndTrackingObject(FD3D12ResidencyHandle* Handle)
{
    if (!bEnable || !Handle)
    {
        return;
    }

    SCOPED_LOCK(Mutex);

    if (!Handle->bIsTracked)
    {
        return;
    }

    TrackedObjects.Remove(Handle);
    Handle->bIsTracked = false;
}

void FD3D12ResidencyManager::UpdateResidency(FD3D12ResidencyHandle* Handle)
{
    if (!bEnable || !Handle || !Handle->bIsTracked)
    {
        return;
    }

    SCOPED_LOCK(Mutex);
    Handle->LastUsedFrame = CurrentFrame;
}

void FD3D12ResidencyManager::MakeResident(FD3D12ResidencyHandle* Handle)
{
    if (!bEnable || !Device || !Handle || !Handle->IsInitialized())
    {
        return;
    }

    ID3D12Pageable* Pageables[] = { Handle->GetPageable() };
    if (FAILED(Device->GetD3D12Device()->MakeResident(1, Pageables)))
    {
        D3D12_ERROR("[FD3D12ResidencyManager] MakeResident failed for Pageable=%p", Handle->GetPageable());
        return;
    }

    SCOPED_LOCK(Mutex);
    Handle->bIsResident   = true;
    Handle->LastUsedFrame = CurrentFrame;
}

void FD3D12ResidencyManager::EvictIfNeeded()
{
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
        FD3D12ResidencyHandle* Victim = nullptr;
        uint64 OldestFrame = UINT64_MAX;

        for (FD3D12ResidencyHandle* Handle : TrackedObjects)
        {
            if (!Handle || !Handle->bIsResident || !Handle->Pageable)
            {
                continue;
            }

            if (Handle->LastUsedFrame < OldestFrame)
            {
                OldestFrame = Handle->LastUsedFrame;
                Victim      = Handle;
            }
        }

        if (!Victim)
        {
            break;
        }

        ID3D12Pageable* Pageables[] = { Victim->Pageable };
        if (FAILED(Device->GetD3D12Device()->Evict(1, Pageables)))
        {
            D3D12_ERROR("[FD3D12ResidencyManager] Evict failed");
            break;
        }

        Victim->bIsResident = false;
        Usage = Usage > Victim->SizeBytes ? (Usage - Victim->SizeBytes) : 0;
    }
}

uint64 FD3D12ResidencyManager::GetCurrentUsage() const
{
    uint64 Usage = 0;
    for (const FD3D12ResidencyHandle* Handle : TrackedObjects)
    {
        if (Handle && Handle->bIsResident)
        {
            Usage += Handle->SizeBytes;
        }
    }

    return Usage;
}
