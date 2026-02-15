#pragma once
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Threading/Runnable.h"
#include "Core/Threading/Atomic.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Allocators.h"

class FD3D12OnlineDescriptorHeap;
struct FD3D12OnlineDescriptorBlock;
class FD3D12Heap;

struct FD3D12DeferredObject
{
    // Go through an array of deferred resources and delete them
    static void ProcessItems(const TArray<FD3D12DeferredObject>& Items);

    enum class EType
    {
        D3DResource           = 1,
        Resource              = 2,
        RHIResource           = 3,
        OnlineDescriptorBlock = 4,
        Heap                  = 5,
        AllocatorBlock        = 6,
    };

    FD3D12DeferredObject(FRHIResource* InResource)
        : Type(EType::RHIResource)
    {
        CHECK(InResource != nullptr);
        RHIResource = InResource;
    }

    FD3D12DeferredObject(ID3D12Resource* InResource)
        : Type(EType::D3DResource)
    {
        CHECK(InResource != nullptr);
        D3DResource = InResource;
        InResource->AddRef();
    }

    FD3D12DeferredObject(FD3D12Resource* InResource)
        : Type(EType::Resource)
    {
        CHECK(InResource != nullptr);
        Resource = InResource;
        InResource->AddRef();
    }

    FD3D12DeferredObject(FD3D12OnlineDescriptorHeap* InHeap, FD3D12OnlineDescriptorBlock* InBlock)
        : Type(EType::OnlineDescriptorBlock)
    {
        CHECK(InHeap != nullptr);
        CHECK(InBlock != nullptr);
        OnlineDescriptorBlock.Heap = InHeap;
        OnlineDescriptorBlock.Block = InBlock;
    }

    FD3D12DeferredObject(FD3D12Heap* InHeap)
        : Type(EType::Heap)
    {
        CHECK(InHeap != nullptr);
        D3D12Heap = InHeap;
        InHeap->AddRef();
    }

    FD3D12DeferredObject(ED3D12DeferredAllocatorType InAllocatorType, void* InAllocator, const FD3D12BuddyAllocatorAllocationData& InAllocationData)
        : Type(EType::AllocatorBlock)
    {
        CHECK(InAllocator != nullptr);

        AllocatorBlock.AllocatorType       = InAllocatorType;
        AllocatorBlock.Allocator           = InAllocator;
        AllocatorBlock.BuddyAllocationData = InAllocationData;
    }

    FD3D12DeferredObject(ED3D12DeferredAllocatorType InAllocatorType, void* InAllocator, const FD3D12PoolAllocatorAllocationData& InAllocationData)
        : Type(EType::AllocatorBlock)
    {
        CHECK(InAllocator != nullptr);

        AllocatorBlock.AllocatorType      = InAllocatorType;
        AllocatorBlock.Allocator          = InAllocator;
        AllocatorBlock.PoolAllocationData = InAllocationData;
    }

    FD3D12DeferredObject(ED3D12DeferredAllocatorType InAllocatorType, void* InAllocator, const FD3D12BucketAllocatorAllocationData& InAllocationData)
        : Type(EType::AllocatorBlock)
    {
        CHECK(InAllocator != nullptr);

        AllocatorBlock.AllocatorType        = InAllocatorType;
        AllocatorBlock.Allocator            = InAllocator;
        AllocatorBlock.BucketAllocationData = InAllocationData;
    }

    EType const Type;
    struct FOnlineDescriptorBlockData
    {
        FD3D12OnlineDescriptorHeap*  Heap  = nullptr;
        FD3D12OnlineDescriptorBlock* Block = nullptr;
    } OnlineDescriptorBlock;

    struct FAllocatorBlockData
    {
        ED3D12DeferredAllocatorType         AllocatorType        = ED3D12DeferredAllocatorType::Pool;
        void*                               Allocator            = nullptr;
        FD3D12BuddyAllocatorAllocationData  BuddyAllocationData  = {};
        FD3D12PoolAllocatorAllocationData   PoolAllocationData   = {};
        FD3D12BucketAllocatorAllocationData BucketAllocationData = {};
    } AllocatorBlock;

    FRHIResource*   RHIResource = nullptr;
    FD3D12Resource* Resource    = nullptr;
    ID3D12Resource* D3DResource = nullptr;
    FD3D12Heap*     D3D12Heap   = nullptr;
};
