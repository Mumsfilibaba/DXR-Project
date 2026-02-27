#pragma once
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Threading/Runnable.h"
#include "Core/Threading/Atomic.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Allocators.h"

class FD3D12Heap;
class FD3D12OnlineDescriptorHeap;
struct FD3D12OnlineDescriptorBlock;

struct FD3D12DeferredObject
{
    // Go through an array of deferred resources and delete them
    static void ProcessItems(const TArray<FD3D12DeferredObject>& Items);

    enum class EType
    {
        D3DResource           = 1,
        D3DHeap               = 2,
        Resource              = 3,
        RHIResource           = 4,
        OnlineDescriptorBlock = 5,
        Heap                  = 6,
        BuddyAllocatorBlock   = 7,
        PoolAllocatorBlock    = 8,
        BucketAllocatorBlock  = 9,
        LinearAllocatorPage   = 10,
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

    FD3D12DeferredObject(ID3D12Heap* InHeap)
        : Type(EType::D3DHeap)
    {
        CHECK(InHeap != nullptr);
        D3DHeap = InHeap;
        InHeap->AddRef();
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

        OnlineDescriptorBlock.Heap  = InHeap;
        OnlineDescriptorBlock.Block = InBlock;
    }

    FD3D12DeferredObject(FD3D12Heap* InHeap)
        : Type(EType::Heap)
    {
        CHECK(InHeap != nullptr);
        D3D12Heap = InHeap;
        InHeap->AddRef();
    }

    FD3D12DeferredObject(FD3D12BuddyAllocator* InAllocator, const FD3D12BuddyAllocatorAllocationData& InAllocationData)
        : Type(EType::BuddyAllocatorBlock)
    {
        CHECK(InAllocator != nullptr);
        BuddyAllocatorBlock.Allocator      = InAllocator;
        BuddyAllocatorBlock.AllocationData = InAllocationData;
    }

    FD3D12DeferredObject(FD3D12PoolAllocator* InAllocator, const FD3D12PoolAllocatorAllocationData& InAllocationData)
        : Type(EType::PoolAllocatorBlock)
    {
        CHECK(InAllocator != nullptr);
        PoolAllocatorBlock.Allocator      = InAllocator;
        PoolAllocatorBlock.AllocationData = InAllocationData;
    }

    FD3D12DeferredObject(FD3D12BucketAllocator* InAllocator, const FD3D12BucketAllocatorAllocationData& InAllocationData)
        : Type(EType::BucketAllocatorBlock)
    {
        CHECK(InAllocator != nullptr);
        BucketAllocatorBlock.Allocator      = InAllocator;
        BucketAllocatorBlock.AllocationData = InAllocationData;
    }

    FD3D12DeferredObject(FD3D12LinearAllocator* InAllocator, FD3D12LinearAllocatorPage* InPage)
        : Type(EType::LinearAllocatorPage)
    {
        CHECK(InAllocator != nullptr);
        CHECK(InPage != nullptr);
        LinearAllocatorPage.Allocator = InAllocator;
        LinearAllocatorPage.Page      = InPage;
    }

    EType const Type;

    struct FOnlineDescriptorBlockData
    {
        FD3D12OnlineDescriptorHeap*  Heap  = nullptr;
        FD3D12OnlineDescriptorBlock* Block = nullptr;
    };

    struct FBuddyAllocatorBlockData
    {
        FD3D12BuddyAllocator*              Allocator      = nullptr;
        FD3D12BuddyAllocatorAllocationData AllocationData = {};
    };

    struct FPoolAllocatorBlockData
    {
        FD3D12PoolAllocator*              Allocator      = nullptr;
        FD3D12PoolAllocatorAllocationData AllocationData = {};
    };

    struct FBucketAllocatorBlockData
    {
        FD3D12BucketAllocator*              Allocator      = nullptr;
        FD3D12BucketAllocatorAllocationData AllocationData = {};
    };

    struct FLinearAllocatorPageData
    {
        FD3D12LinearAllocator*     Allocator = nullptr;
        FD3D12LinearAllocatorPage* Page      = nullptr;
    };

    union
    {
        FRHIResource*              RHIResource;
        FD3D12Resource*            Resource;
        ID3D12Heap*                D3DHeap;
        ID3D12Resource*            D3DResource;
        FD3D12Heap*                D3D12Heap;
        FOnlineDescriptorBlockData OnlineDescriptorBlock;
        FBuddyAllocatorBlockData   BuddyAllocatorBlock;
        FPoolAllocatorBlockData    PoolAllocatorBlock;
        FBucketAllocatorBlockData  BucketAllocatorBlock;
        FLinearAllocatorPageData   LinearAllocatorPage;
    };
};
