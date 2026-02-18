#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"
#include "D3D12RHI/D3D12Core.h"

class FD3D12Device;

class FD3D12ResidencyManager
{
    struct FD3D12ResidencyTrackedPageable
    {
        ID3D12Pageable* Pageable        = nullptr;
        uint64          SizeBytes       = 0;
        uint64          LastUsedFrame   = 0;
        uint32          Generation      = 0;
        bool            bAlwaysResident = false;
        bool            bIsResident     = true;
        bool            bAllocated      = false;
    };

public:
    FD3D12ResidencyManager(FD3D12Device* InDevice, bool bEnableResidency = false, uint64 TargetBudgetBytes = 0);
    ~FD3D12ResidencyManager();

    void Tick();

    FD3D12ResidencyHandle RegisterPageable(ID3D12Pageable* InPageable, uint64 InSizeBytes, bool InIsAlwaysResident);
    void UnregisterPageable(const FD3D12ResidencyHandle& Handle);

    void TouchPageable(const FD3D12ResidencyHandle& Handle);
    void TouchPageable(ID3D12Pageable* InPageable);

    void MakeResident(ID3D12CommandQueue* InQueue, ID3D12Pageable* InPageable);
    void EvictIfNeeded(ID3D12CommandQueue* InQueue);

private:
    int32 FindTrackedIndexByPageable(ID3D12Pageable* InPageable) const;
    bool IsValidHandle(const FD3D12ResidencyHandle& Handle) const;
    uint64 GetCurrentUsage() const;

private:
    FD3D12Device*                          Device;
    IDXGIAdapter3*                         Adapter;
    uint64                                 TargetBudget;
    uint64                                 CurrentFrame;
    bool                                   bEnable;
    TArray<FD3D12ResidencyTrackedPageable> Tracked;
    TMap<ID3D12Pageable*, int32>           TrackedIndexByPageable;
    FCriticalSection                       Mutex;
};
