#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/PlatformEvent.h"
#include "Core/Threading/Runnable.h"
#include "D3D12RHI/D3D12Core.h"

struct IDXGIAdapter3;
class FD3D12Device;
class FD3D12Fence;
class FD3D12Resource;
class FD3D12ResidencyManager;
class FGenericThread;

class FD3D12ResidencyHandle
{
    friend class FD3D12ResidencyManager;

public:
    FD3D12ResidencyHandle() = default;

    void Initialize(ID3D12Pageable* InPageable, uint64 InSizeBytes)
    {
        Pageable    = InPageable;
        SizeBytes   = InSizeBytes;
        bIsResident = true;
    }

    void Deinitialize()
    {
        Pageable    = nullptr;
        SizeBytes   = 0;
        bIsResident = false;
        bIsTracked  = false;
    }

    FORCEINLINE bool IsResident()    const { return bIsResident; }
    FORCEINLINE bool IsInitialized() const { return Pageable != nullptr; }

    FORCEINLINE ID3D12Pageable* GetPageable() const
    {
        return Pageable;
    }

    FORCEINLINE uint64 GetSizeBytes() const
    {
        return SizeBytes;
    }

private:
    ID3D12Pageable* Pageable            = nullptr;
    uint64          SizeBytes           = 0;
    uint64          LastUsedFrame       = 0;
    uint64          LastUsedFenceValue  = 0;
    bool            bIsResident         = true;
    bool            bIsTracked          = false;
};

class FD3D12ResidencySet
{
public:
    void Reset()
    {
        Handles.Clear();
    }

    FORCEINLINE void Insert(FD3D12ResidencyHandle* Handle)
    {
        if (Handle && Handle->IsInitialized())
        {
            Handles.AddUnique(Handle);
        }
    }

    FORCEINLINE const TArray<FD3D12ResidencyHandle*>& GetHandles() const
    {
        return Handles;
    }

    FORCEINLINE int32 GetNumHandles() const
    {
        return Handles.Size();
    }

private:
    TArray<FD3D12ResidencyHandle*> Handles;
};

class FD3D12PagingWorker : public FRunnable
{
public:
    FD3D12PagingWorker(ID3D12Device* InDevice);
    ~FD3D12PagingWorker();

    void RequestMakeResident(TArray<ID3D12Pageable*>&& Pageables);
    bool WaitForCompletion();

    // FRunnable
    virtual int32 Run() override;
    virtual void Stop() override;

private:
    ID3D12Device*           Device;
    TArray<ID3D12Pageable*> PendingPageables;
    HRESULT                 LastResult;
    FPlatformEvent*         WakeEvent;
    FPlatformEvent*         CompletionEvent;
    FCriticalSection        RequestMutex;
    bool                    bRunning;
};

class FD3D12ResidencyManager
{
public:
    FD3D12ResidencyManager(FD3D12Device* InDevice, bool bEnableResidency = false, uint64 TargetBudgetBytes = 0);
    ~FD3D12ResidencyManager();

    void Tick();

    void BeginTrackingObject(FD3D12ResidencyHandle* Handle);
    void EndTrackingObject(FD3D12ResidencyHandle* Handle);

    void UpdateResidency(FD3D12ResidencyHandle* Handle);

    void MakeResident(FD3D12ResidencyHandle* Handle);
    void EvictIfNeeded();

    void PrepareForExecution(FD3D12ResidencySet* const* Sets, uint32 NumSets);
    void NotifySubmitted(FD3D12ResidencySet* const* Sets, uint32 NumSets, uint64 FenceValue);

private:
    bool MakeResidentAsync(TArray<ID3D12Pageable*>& Pageables);

    uint64 GetCurrentUsage() const;
    uint64 GetBudget() const;

    FD3D12Device*                  Device;
    IDXGIAdapter3*                 Adapter;
    FD3D12Fence*                   GPUFence;
    uint64                         TargetBudget;
    uint64                         CurrentFrame;
    bool                           bEnable;
    TArray<FD3D12ResidencyHandle*> TrackedObjects;
    FCriticalSection               Mutex;
    FD3D12PagingWorker*            PagingWorker;
    FGenericThread*                PagingThread;
    TComPtr<ID3D12Fence>           PagingFence;
    uint64                         PagingFenceValue;
    HANDLE                         BudgetChangeEvent;
    DWORD                          BudgetChangeCookie;
};
