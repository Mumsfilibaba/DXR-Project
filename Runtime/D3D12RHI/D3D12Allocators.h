#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12Heap.h"

class FD3D12Device;

struct FD3D12GpuFencePoint
{
    uint64                 FenceValue = 0;
    ED3D12CommandQueueType QueueType  = ED3D12CommandQueueType::Direct;
};

enum class ED3D12DeferredAllocatorType : uint8
{
    Pool,
    Buddy,
    MultiBuddy,
    Bucket
};

enum class EAllocationStrategy : uint8
{
    SuballocatedHeap,
    SuballocatedResource
};

struct FD3D12ResourceAllocationRequest
{
    ED3D12ResourceType        ResourceType  = ED3D12ResourceType::Buffer;
    D3D12_HEAP_TYPE           HeapType      = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES     InitialState  = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_FLAGS      ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    const D3D12_CLEAR_VALUE*  ClearValue    = nullptr;
    D3D12_RESOURCE_DESC       ResourceDesc  = {};
    FD3D12GpuFencePoint       FencePoint    = {};
    uint64                    Size          = 0;
    uint64                    Alignment     = 0;

    bool bHasResourceDesc        = false;
    bool bAllowCommittedFallback = true;
    bool bPreferPlaced           = true;
    bool bPersistent             = false;
    bool bShortLived             = false;
};

class FD3D12BuddyAllocator : public FD3D12DeviceChild
{
public:
    FD3D12BuddyAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy);
    ~FD3D12BuddyAllocator();

    bool Initialize();
    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void RecycleAllocation(const FD3D12BuddyAllocatorAllocationData& AllocationData);

    bool IsBackedByHeap() const
    {
        return AllocationStrategy == EAllocationStrategy::SuballocatedHeap;
    }

private:
    uint32 GetOrderForSize(uint64 Size) const;
    uint64 GetOrderBlockSize(uint32 Order) const;
    bool AllocateFromAllocator(uint64 Size, uint64 Alignment, uint64& OutOffset, uint32& OutOrder);
    void FreeToAllocator(uint64 Offset, uint32 Order);
    bool CreateBackingAllocation();

private:
    uint64                    PageSizeBytes;
    uint64                    MinBlockBytes;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_STATES     InitialState;
    EAllocationStrategy       AllocationStrategy;
    FD3D12HeapRef             BackingHeap;
    FD3D12ResourceRef         BackingResource;
    TArray<TArray<uint64>>    FreeOffsets;
    uint8*                    MappedBaseAddress = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS BaseGpuVirtualAddress = 0;
    FCriticalSection          AllocatorCS;
};

class FD3D12MultiBuddyAllocator : public FD3D12DeviceChild
{
public:
    FD3D12MultiBuddyAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy);
    ~FD3D12MultiBuddyAllocator();

    bool Initialize();
    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void RecycleAllocation(const FD3D12MultiBuddyAllocatorAllocationData& AllocationData);

private:
    bool CreateAllocator();

    uint64                        PageSizeBytes;
    uint64                        MinBlockBytes;
    D3D12_HEAP_TYPE               HeapType;
    D3D12_RESOURCE_STATES         InitialState;
    EAllocationStrategy           AllocationStrategy;
    TArray<FD3D12BuddyAllocator*> Allocators;
    FCriticalSection              AllocatorsCS;
};

class FD3D12PoolAllocatorPage : public FD3D12DeviceChild
{
public:
    struct FFreeRange
    {
        uint64 Offset = 0;
        uint64 Size   = 0;
    };

public:
    FD3D12PoolAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy);
    ~FD3D12PoolAllocatorPage();

    bool Initialize();

    bool TryAllocate(uint64 Size, uint64 InAlignment, uint32 InPageIndex, FD3D12ResourceStorage& OutStorage);
    void RecycleAllocation(uint64 Offset, uint64 Size);

    uint64 GetPageSize() const 
    {
        return PageSizeBytes;
    }

    uint64 GetUsedBytes() const
    {
        return UsedBytes;
    }

    const TArray<FFreeRange>& GetFreeRanges() const
    {
        return FreeRanges;
    }

private:
    void CoalesceFreeRanges();

    uint64                    PageSizeBytes;
    uint64                    Alignment;
    uint64                    UsedBytes;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_STATES     InitialState;
    EAllocationStrategy       AllocationStrategy;
    FD3D12HeapRef             BackingHeap;
    FD3D12ResourceRef         BackingResource;
    uint8*                    MappedBaseAddress;
    D3D12_GPU_VIRTUAL_ADDRESS BaseGpuVirtualAddress;
    TArray<FFreeRange>        FreeRanges;
};

class FD3D12PoolAllocator : public FD3D12DeviceChild
{
    static constexpr uint32 TLSFFirstLevelCount  = 32;
    static constexpr uint32 TLSFSecondLevelCount = 8;

    struct FDefragRecord
    {
        uint32 PageIndex = UINT32_MAX;
        uint64 Offset    = 0;
        uint64 Size      = 0;
    };

public:
    FD3D12PoolAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxResourceSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy);
    ~FD3D12PoolAllocator();

    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void RecycleAllocation(const FD3D12PoolAllocatorAllocationData& AllocationData);

    uint64 GetFragmentedBytes() const
    {
        return FragmentedBytes;
    }

private:
    FD3D12PoolAllocatorPage* CreatePage(uint64 MinimumSize, uint32& OutPageIndex);
    void ComputeTLSFIndices(uint64 Size, uint32& OutFL, uint32& OutSL) const;
    void AddDefragRecord(uint32 PageIndex, uint64 Offset, uint64 Size);

private:
    uint64                           PageSizeBytes;
    uint64                           Alignment;
    uint64                           MaxResourceSize;
    D3D12_HEAP_TYPE                  HeapType;
    D3D12_RESOURCE_STATES            InitialState;
    EAllocationStrategy              AllocationStrategy;
    uint64                           FragmentedBytes;
    TArray<FDefragRecord>            DefragRecords;
    TArray<FD3D12PoolAllocatorPage*> Pages;
    FCriticalSection                 PagesCS;
};

class FD3D12BucketAllocator : public FD3D12DeviceChild
{
    struct FBucket
    {
        uint64            BlockSize = 0;
        uint8*            MappedBaseAddress = nullptr;
        FD3D12ResourceRef BackingResource;
        TArray<FD3D12BucketAllocatorAllocationData> FreeBlocks;
    };

public:
    FD3D12BucketAllocator(FD3D12Device* InDevice, const TArray<uint64>& InBucketSizes, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
    ~FD3D12BucketAllocator();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void RecycleAllocation(const FD3D12BucketAllocatorAllocationData& AllocationData);

private:
    bool CreateBucketResource(uint32 BucketIndex, FBucket& Bucket);

    uint64                PageSizeBytes;
    uint64                Alignment;
    D3D12_HEAP_TYPE       HeapType;
    D3D12_RESOURCE_STATES InitialState;
    TArray<FBucket>       Buckets;
    FCriticalSection      BucketsCS;
};

class FD3D12UploadHeapAllocator : public FD3D12DeviceChild
{
public:
    FD3D12UploadHeapAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallAllocationThreshold = 64ull * 1024ull, uint64 InLargeAllocationThreshold = 2ull * 1024ull * 1024ull);
    ~FD3D12UploadHeapAllocator();

    bool Initialize();
    void Shutdown();

    void* Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    void* AllocateConstants(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage);

    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    uint64                     PageSizeBytes;
    uint64                     DefaultAlignment;
    uint64                     SmallAllocationThreshold;
    uint64                     LargeAllocationThreshold;
    FD3D12MultiBuddyAllocator* SmallAllocator;
    FD3D12PoolAllocator*       LargeAllocator;
    FD3D12MultiBuddyAllocator* ConstantsAllocator;
};

class FD3D12LinearAllocatorPage
{
public:
    FD3D12LinearAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, uint32 InPageIndex);
    ~FD3D12LinearAllocatorPage() = default;

    bool Initialize();
    bool TryAllocate(uint64 Size, uint64 Alignment, uint64& OutOffset);
    void ReleaseAllocation();
    void Reset();

    uint32 GetPageIndex() const { return PageIndex; }
    uint32 GetActiveAllocations() const { return ActiveAllocations; }
    FD3D12ResourceStorage& GetBackingResourceStorage() { return BackingResourceStorage; }
    const FD3D12ResourceStorage& GetBackingResourceStorage() const { return BackingResourceStorage; }

private:
    FD3D12Device*         Device;
    uint64                PageSizeBytes;
    D3D12_HEAP_TYPE       HeapType;
    D3D12_RESOURCE_STATES InitialState;
    uint64                CurrentOffset;
    uint32                ActiveAllocations;
    uint32                PageIndex;
    FD3D12ResourceStorage BackingResourceStorage;
};

class FD3D12LinearAllocator : public FD3D12DeviceChild
{
public:
    FD3D12LinearAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
    ~FD3D12LinearAllocator();

    void* Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    FD3D12LinearAllocatorPage* CreatePage();
    void RetirePage(FD3D12LinearAllocatorPage* Page);

private:
    uint64                             PageSizeBytes;
    D3D12_HEAP_TYPE                    HeapType;
    D3D12_RESOURCE_STATES              InitialState;
    uint32                             NextPageIndex;
    TArray<FD3D12LinearAllocatorPage*> Pages;
    FCriticalSection                   PagesCS;
};

class FD3D12DynamicConstantsAllocator : public FD3D12DeviceChild
{
public:
    FD3D12DynamicConstantsAllocator(FD3D12Device* InDevice);
    ~FD3D12DynamicConstantsAllocator();

    bool Initialize(uint64 InPageSizeBytes, FD3D12UploadHeapAllocator* InUploadHeapAllocator);
    void Shutdown();

    void* Allocate(uint64 Size, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    FD3D12LinearAllocator* LinearAllocator;
};

class FD3D12BufferAllocatorPool : public FD3D12DeviceChild
{
public:
    FD3D12BufferAllocatorPool(FD3D12Device* InDevice, D3D12_HEAP_TYPE InHeapType, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, D3D12_RESOURCE_STATES InInitialState);
    ~FD3D12BufferAllocatorPool();

    bool Initialize();

    bool Supports(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc) const;
    bool TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, EBufferFlags BufferUsage, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    void ReleaseAllocator();

    D3D12_HEAP_TYPE            HeapType;
    D3D12_RESOURCE_STATES      InitialState;
    uint64                     PageSizeBytes;
    uint64                     MinBlockBytes;
    uint64                     MaxSuballocationSize;
    FD3D12MultiBuddyAllocator* MultiBuddyAllocator;
};

class FD3D12BufferAllocator : public FD3D12DeviceChild
{
public:
    FD3D12BufferAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize);
    ~FD3D12BufferAllocator();

    bool Initialize();

    bool TryAllocate(D3D12_HEAP_TYPE HeapType, const D3D12_RESOURCE_DESC& ResourceDesc, EBufferFlags BufferUsage, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    void ReleasePools();

    uint64                             PageSizeBytes;
    uint64                             MinBlockBytes;
    uint64                             MaxSuballocationSize;
    TArray<FD3D12BufferAllocatorPool*> Pools;
    FCriticalSection                   PoolsCS;
};

class FD3D12TextureAllocator : public FD3D12DeviceChild
{
    enum class ETexturePoolClass : uint32
    {
        Small4K                  = 0,
        ReadOnly                 = 1,
        RenderTargetDepthStencil = 2,
        UAVOnly                  = 3
    };
    
    struct FPool
    {
        ETexturePoolClass    PoolClass = ETexturePoolClass::ReadOnly;
        FD3D12PoolAllocator* Pool      = nullptr;
    };

public:
    FD3D12TextureAllocator(FD3D12Device* InDevice);
    ~FD3D12TextureAllocator();

    bool Initialize(uint64 InDefaultPageSizeBytes, uint64 InCommittedThreshold);
    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    ETexturePoolClass ClassifyTexture(const D3D12_RESOURCE_DESC& Desc, uint64 Size, uint64 Alignment) const;

    uint64             CommittedThreshold;
    uint64             DefaultPageSizeBytes;
    TArray<FPool>      Pools;
    FCriticalSection   PoolsCS;
};
