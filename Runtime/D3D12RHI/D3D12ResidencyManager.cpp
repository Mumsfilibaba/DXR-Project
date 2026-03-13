#include "Core/Generic/GenericThread.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformEvent.h"
#include "D3D12RHI/D3D12ResidencyManager.h"
#include "D3D12RHI/D3D12Device.h"

static TAutoConsoleVariable<int32> CVarResidencyDebugBudgetMB(
    "D3D12RHI.ResidencyDebugBudgetMB",
    "Override residency budget in MB for debugging (0 = use actual DXGI budget)",
    0);

#if D3D12_ENABLE_RESIDENCY_LOGGING
static TAutoConsoleVariable<bool> CVarLogResidencyEvents(
    "D3D12RHI.LogResidencyEvents",
    "Log residency events such as evictions and make-resident calls",
    false);
#endif

FD3D12PagingWorker::FD3D12PagingWorker(ID3D12Device* InDevice)
    : Device(InDevice)
    , LastResult(S_OK)
    , WakeEvent(static_cast<FPlatformEvent*>(FPlatformEvent::Create(false)))
    , CompletionEvent(static_cast<FPlatformEvent*>(FPlatformEvent::Create(false)))
    , bRunning(true)
{
}

FD3D12PagingWorker::~FD3D12PagingWorker()
{
    FPlatformEvent::Recycle(WakeEvent);
    FPlatformEvent::Recycle(CompletionEvent);

    WakeEvent       = nullptr;
    CompletionEvent = nullptr;
}

void FD3D12PagingWorker::RequestMakeResident(TArray<ID3D12Pageable*>&& Pageables)
{
    {
        SCOPED_LOCK(RequestMutex);
        
        PendingPageables = Move(Pageables);
        LastResult       = S_OK;
    }

    WakeEvent->Trigger();
}

bool FD3D12PagingWorker::WaitForCompletion()
{
    CompletionEvent->Wait(UINT64_MAX);

    SCOPED_LOCK(RequestMutex);
    return SUCCEEDED(LastResult);
}

int32 FD3D12PagingWorker::Run()
{
    while (bRunning)
    {
        WakeEvent->Wait(UINT64_MAX);

        if (!bRunning)
        {
            break;
        }

        TArray<ID3D12Pageable*> WorkItems;
        {
            SCOPED_LOCK(RequestMutex);
            WorkItems = Move(PendingPageables);
        }

        HRESULT Result = S_OK;
        if (!WorkItems.IsEmpty())
        {
            Result = Device->MakeResident(WorkItems.Size(), WorkItems.Data());
        }

        {
            SCOPED_LOCK(RequestMutex);
            LastResult = Result;
        }

        CompletionEvent->Trigger();
    }

    return 0;
}

void FD3D12PagingWorker::Stop()
{
    bRunning = false;
    WakeEvent->Trigger();
}

FD3D12ResidencyManager::FD3D12ResidencyManager(FD3D12Device* InDevice, bool bEnableResidency, uint64 TargetBudgetBytes)
    : Device(InDevice)
    , Adapter(Device ? Device->GetAdapter()->GetDXGIAdapter3() : nullptr)
    , GPUFence(nullptr)
    , TargetBudget(TargetBudgetBytes)
    , CurrentFrame(0)
    , bEnable(bEnableResidency)
    , PagingWorker(nullptr)
    , PagingThread(nullptr)
    , PagingFenceValue(0)
    , BudgetChangeEvent(nullptr)
    , BudgetChangeCookie(0)
{
    if (!bEnable || !Device)
    {
        return;
    }

#if D3D12_USE_ID3D12DEVICE_3
    ID3D12Device3* Device3 = Device->GetD3D12Device3();
    if (Device3)
    {
        Device->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&PagingFence));
    }
    else
#endif
    {
        PagingWorker = new FD3D12PagingWorker(Device->GetD3D12Device());
        PagingThread = FGenericThread::Create(PagingWorker, "D3D12 Paging Worker");
        PagingThread->Start();
    }

    if (Adapter)
    {
        BudgetChangeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (BudgetChangeEvent)
        {
            if (FAILED(Adapter->RegisterVideoMemoryBudgetChangeNotificationEvent(BudgetChangeEvent, &BudgetChangeCookie)))
            {
                CloseHandle(BudgetChangeEvent);
                BudgetChangeEvent  = nullptr;
                BudgetChangeCookie = 0;
            }
        }
    }

    if (Device)
    {
        FD3D12Queue* DirectQueue = Device->GetQueue(ED3D12CommandQueueType::Direct);
        if (DirectQueue)
        {
            GPUFence = &DirectQueue->GetSubmissionFence();
        }
    }
}

FD3D12ResidencyManager::~FD3D12ResidencyManager()
{
    if (BudgetChangeCookie && Adapter)
    {
        Adapter->UnregisterVideoMemoryBudgetChangeNotification(BudgetChangeCookie);
        BudgetChangeCookie = 0;
    }

    if (BudgetChangeEvent)
    {
        CloseHandle(BudgetChangeEvent);
        BudgetChangeEvent = nullptr;
    }

    if (PagingThread)
    {
        PagingThread->Kill(true);
        delete PagingThread;
        PagingThread = nullptr;
    }

    delete PagingWorker;
    PagingWorker = nullptr;

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

    {
        SCOPED_LOCK(Mutex);
        ++CurrentFrame;
    }

    if (BudgetChangeEvent && WaitForSingleObject(BudgetChangeEvent, 0) == WAIT_OBJECT_0)
    {
        EvictIfNeeded();
    }
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

    TArray<ID3D12Pageable*> Pageables;
    Pageables.Add(Handle->GetPageable());

    if (!MakeResidentAsync(Pageables))
    {
        D3D12_ERROR("[FD3D12ResidencyManager] MakeResident failed for Pageable=%p", Handle->GetPageable());
        return;
    }

    SCOPED_LOCK(Mutex);
    Handle->bIsResident   = true;
    Handle->LastUsedFrame = CurrentFrame;

#if D3D12_ENABLE_RESIDENCY_LOGGING
    if (CVarLogResidencyEvents.GetValue())
    {
        D3D12_INFO("[ResidencyManager] MakeResident: Pageable=%p Size=%llu bytes", Handle->GetPageable(), Handle->SizeBytes);
    }
#endif
}

void FD3D12ResidencyManager::EvictIfNeeded()
{
    if (!bEnable || !Device)
    {
        return;
    }

    const uint64 Budget = GetBudget();
    if (Budget == 0)
    {
        return;
    }

    const uint64 CompletedFenceValue = GPUFence ? GPUFence->GetCompletedValue() : UINT64_MAX;

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

            if (Handle->LastUsedFenceValue > CompletedFenceValue)
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

#if D3D12_ENABLE_RESIDENCY_LOGGING
        if (CVarLogResidencyEvents.GetValue())
        {
            D3D12_INFO("[ResidencyManager] Evict: Pageable=%p Size=%llu bytes (Usage=%llu Budget=%llu)", Victim->Pageable, Victim->SizeBytes, Usage, Budget);
        }
#endif

        Victim->bIsResident = false;
        Usage = Usage > Victim->SizeBytes ? (Usage - Victim->SizeBytes) : 0;
    }
}

void FD3D12ResidencyManager::PrepareForExecution(FD3D12ResidencySet* const* Sets, uint32 NumSets)
{
    if (!bEnable || !Device)
    {
        return;
    }

    SCOPED_LOCK(Mutex);

    TArray<FD3D12ResidencyHandle*> ObjectsToMakeResident;

    for (uint32 SetIndex = 0; SetIndex < NumSets; ++SetIndex)
    {
        if (!Sets[SetIndex])
        {
            continue;
        }

        for (FD3D12ResidencyHandle* Handle : Sets[SetIndex]->GetHandles())
        {
            if (!Handle || !Handle->IsInitialized())
            {
                continue;
            }

            Handle->LastUsedFrame = CurrentFrame;

            if (!Handle->bIsResident)
            {
                ObjectsToMakeResident.AddUnique(Handle);
            }
        }
    }

    if (!ObjectsToMakeResident.IsEmpty())
    {
        TArray<ID3D12Pageable*> Pageables;
        Pageables.Reserve(ObjectsToMakeResident.Size());

        for (FD3D12ResidencyHandle* Handle : ObjectsToMakeResident)
        {
            Pageables.Add(Handle->Pageable);
        }

        if (MakeResidentAsync(Pageables))
        {
            for (FD3D12ResidencyHandle* Handle : ObjectsToMakeResident)
            {
                Handle->bIsResident = true;
            }

#if D3D12_ENABLE_RESIDENCY_LOGGING
            if (CVarLogResidencyEvents.GetValue())
            {
                D3D12_INFO("[ResidencyManager] PrepareForExecution: Made %d objects resident", ObjectsToMakeResident.Size());
            }
#endif
        }
        else
        {
            D3D12_ERROR("[FD3D12ResidencyManager] PrepareForExecution: MakeResident failed for %d objects", ObjectsToMakeResident.Size());
        }
    }
}

void FD3D12ResidencyManager::NotifySubmitted(FD3D12ResidencySet* const* Sets, uint32 NumSets, uint64 FenceValue)
{
    if (!bEnable)
    {
        return;
    }

    for (uint32 SetIndex = 0; SetIndex < NumSets; ++SetIndex)
    {
        if (!Sets[SetIndex])
        {
            continue;
        }

        for (FD3D12ResidencyHandle* Handle : Sets[SetIndex]->GetHandles())
        {
            if (Handle && Handle->IsInitialized())
            {
                Handle->LastUsedFenceValue = FenceValue;
            }
        }
    }
}

bool FD3D12ResidencyManager::MakeResidentAsync(TArray<ID3D12Pageable*>& Pageables)
{
    if (Pageables.IsEmpty())
    {
        return true;
    }

#if D3D12_USE_ID3D12DEVICE_3
    if (ID3D12Device3* Device3 = Device->GetD3D12Device3())
    {
        const uint64 FenceValue = ++PagingFenceValue;
        HRESULT Result = Device3->EnqueueMakeResident(D3D12_RESIDENCY_FLAG_NONE, Pageables.Size(), Pageables.Data(), PagingFence.Get(), FenceValue);
        if (FAILED(Result))
        {
            return false;
        }

        if (PagingFence->GetCompletedValue() < FenceValue)
        {
            HANDLE Event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            PagingFence->SetEventOnCompletion(FenceValue, Event);
            WaitForSingleObject(Event, INFINITE);
            CloseHandle(Event);
        }

        return true;
    }
#endif

    if (PagingWorker)
    {
        PagingWorker->RequestMakeResident(Move(Pageables));
        return PagingWorker->WaitForCompletion();
    }

    return SUCCEEDED(Device->GetD3D12Device()->MakeResident(Pageables.Size(), Pageables.Data()));
}

uint64 FD3D12ResidencyManager::GetBudget() const
{
#if !RELEASE_BUILD
    const int32 DebugBudgetMB = CVarResidencyDebugBudgetMB.GetValue();
    if (DebugBudgetMB > 0)
    {
        return static_cast<uint64>(DebugBudgetMB) * 1024ull * 1024ull;
    }
#endif

    uint64 Budget = TargetBudget;
    if (Budget == 0 && Adapter)
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO Info = {};
        if (SUCCEEDED(Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &Info)))
        {
            Budget = Info.Budget;
        }
    }

    return Budget;
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
