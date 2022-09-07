#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"

#include "Core/Platform/CriticalSection.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class FD3D12CommandAllocator> FD3D12CommandAllocatorRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandAllocator 

class FD3D12CommandAllocator 
    : public FD3D12DeviceChild
    , public FD3D12RefCounted
{
public:
    FD3D12CommandAllocator(FD3D12Device* InDevice);
    ~FD3D12CommandAllocator() = default;

    bool Create(D3D12_COMMAND_LIST_TYPE Type);
    bool Reset();

    FORCEINLINE void SetName(const FString& Name)
    {
        FStringWide WideName = CharToWide(Name);
        Allocator->SetName(WideName.GetCString());
    }

    FORCEINLINE ID3D12CommandAllocator* GetAllocator() const { return Allocator.Get(); }

private:
    TComPtr<ID3D12CommandAllocator> Allocator;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandAllocatorManager

class FD3D12CommandAllocatorManager
    : public FD3D12DeviceChild
{
public:
    FD3D12CommandAllocatorManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandAllocatorManager() = default;

    FD3D12CommandAllocatorRef ObtainAllocator();
    void ReleaseAllocator(FD3D12CommandAllocatorRef InAllocator);

private:
    TArray<FD3D12CommandAllocatorRef> Allocators;
    FCriticalSection                  AllocatorsCS;
};