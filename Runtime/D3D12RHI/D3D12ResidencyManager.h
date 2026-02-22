#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "D3D12RHI/D3D12Core.h"

struct IDXGIAdapter3;
class FD3D12Device;
class FD3D12ResidencyManager;

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

    FORCEINLINE bool IsInitialized() const { return Pageable != nullptr; }
    FORCEINLINE bool IsResident()    const { return bIsResident; }

    FORCEINLINE ID3D12Pageable* GetPageable()  const { return Pageable; }
    FORCEINLINE uint64          GetSizeBytes() const { return SizeBytes; }

private:
    ID3D12Pageable* Pageable      = nullptr;
    uint64          SizeBytes     = 0;
    uint64          LastUsedFrame = 0;
    bool            bIsResident   = true;
    bool            bIsTracked    = false;
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

private:
    uint64 GetCurrentUsage() const;

    FD3D12Device*                    Device;
    IDXGIAdapter3*                   Adapter;
    uint64                           TargetBudget;
    uint64                           CurrentFrame;
    bool                             bEnable;
    TArray<FD3D12ResidencyHandle*>   TrackedObjects;
    FCriticalSection                 Mutex;
};
