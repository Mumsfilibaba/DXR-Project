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
class FD3D12CommandContext;
class FD3D12FenceManager;

enum class ED3D12DeferredAllocatorType : uint8
{
    Pool,
    Buddy,
    Bucket
};

enum class EAllocationStrategy : uint8
{
    SuballocatedHeap,
    SuballocatedResource
};

static constexpr uint64 D3D12_MIN_BUDDY_ALLOCATOR_BLOCK_SIZE = 16ull;

class FD3D12BuddyAllocator : public FD3D12DeviceChild
{
public:
    FD3D12BuddyAllocator(FD3D12Device* InDevice, uint64 InBackingStorageSize, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags = D3D12_RESOURCE_FLAG_NONE);
    ~FD3D12BuddyAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags) const;
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void RecycleAllocation(const FD3D12BuddyAllocatorAllocationData& AllocationData);

    bool Initialize();
    void Destroy();
    
    bool IsEmpty() const;

    bool IsBackedByHeap() const
    {
        return AllocationStrategy == EAllocationStrategy::SuballocatedHeap;
    }

    FD3D12Heap* GetBackingHeap() const
    {
        return AllocationStrategy == EAllocationStrategy::SuballocatedHeap ? BackingHeap.Get() : nullptr;
    }

    FD3D12Resource* GetBackingResource() const
    {
        return AllocationStrategy == EAllocationStrategy::SuballocatedResource ? BackingResource.Get() : nullptr;
    }

private:
    uint32 GetOrderForSize(uint64 SizeInBytes) const;
    uint64 GetOrderBlockSize(uint32 Order) const;

    uint64                   BackingStorageSize;
    uint64                   MinBlockBytes;
    D3D12_HEAP_TYPE          HeapType;
    D3D12_RESOURCE_STATES    InitialState;
    EAllocationStrategy      AllocationStrategy;
    D3D12_RESOURCE_FLAGS     ResourceFlags;
    FD3D12HeapRef            BackingHeap;
    FD3D12ResourceRef        BackingResource;
    TArray<TArray<uint64>>   FreeOffsets;
    uint8*                   MappedBaseAddress;
    mutable FCriticalSection AllocatorCS;
};

class FD3D12MultiBuddyAllocator : public FD3D12DeviceChild
{
public:
    FD3D12MultiBuddyAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy);
    ~FD3D12MultiBuddyAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage, D3D12_RESOURCE_FLAGS InResourceFlags = D3D12_RESOURCE_FLAG_NONE);
    bool Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags) const;
    
    bool Initialize();
    void Destroy();
    void CleanUp();
    
private:
    bool CreateAllocator(D3D12_RESOURCE_FLAGS InResourceFlags);

    uint64                        PageSizeBytes;
    uint64                        MinBlockBytes;
    D3D12_HEAP_TYPE               HeapType;
    D3D12_RESOURCE_STATES         InitialState;
    EAllocationStrategy           AllocationStrategy;
    TArray<FD3D12BuddyAllocator*> Allocators;
    mutable FCriticalSection      AllocatorsCS;
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
    FD3D12PoolAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags = D3D12_RESOURCE_FLAG_NONE);
    ~FD3D12PoolAllocatorPage();

    bool TryAllocate(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FD3D12ResourceStorage& OutStorage);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, FD3D12PoolAllocatorAllocationData& OutData);
    void RecycleAllocation(uint64 Offset, uint64 SizeInBytes);
    void TransferOwnership(uint64 Offset, FD3D12ResourceStorage* NewStorage);
    
    bool Initialize();

    bool IsEmpty() const { return UsedBytes == 0; }

    FD3D12Heap* GetBackingHeap() const
    {
        return BackingHeap.Get();
    }

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

    const TArray<FD3D12PoolAllocatorAllocationData>& GetLiveAllocations() const
    {
        return LiveAllocations;
    }

private:
    void CoalesceFreeRanges();

    uint64                                    PageSizeBytes;
    uint64                                    Alignment;
    uint64                                    UsedBytes;
    D3D12_HEAP_TYPE                           HeapType;
    D3D12_RESOURCE_STATES                     InitialState;
    EAllocationStrategy                       AllocationStrategy;
    D3D12_RESOURCE_FLAGS                      ResourceFlags;
    FD3D12HeapRef                             BackingHeap;
    FD3D12ResourceRef                         BackingResource;
    uint8*                                    MappedBaseAddress;
    D3D12_GPU_VIRTUAL_ADDRESS                 BaseGpuVirtualAddress;
    TArray<FFreeRange>                        FreeRanges;
    TArray<FD3D12PoolAllocatorAllocationData> LiveAllocations;
};

class FD3D12PoolAllocator : public FD3D12DeviceChild
{
    static constexpr uint32 TLSFFirstLevelCount  = 32;
    static constexpr uint32 TLSFSecondLevelCount = 8;

public:
    FD3D12PoolAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxResourceSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags = D3D12_RESOURCE_FLAG_NONE);
    ~FD3D12PoolAllocator();

    bool TryAllocate(const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceStorage& OutStorage);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 ExcludePageIndex, FD3D12PoolAllocatorAllocationData& OutData);
    bool Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_FLAGS InResourceFlags) const;
    void Deallocate(const FD3D12ResourceStorage& Storage);
    bool GetDefragCandidate(FD3D12PoolAllocatorAllocationData& OutCandidate) const;
    void RecycleAllocation(const FD3D12PoolAllocatorAllocationData& AllocationData);
    void TransferOwnership(const FD3D12PoolAllocatorAllocationData& Data, FD3D12ResourceStorage* NewStorage);

    bool Initialize();
    void Destroy();
    void CleanUp();

    FD3D12Heap* GetBackingHeap(uint32 PageIndex);

    uint64 GetFragmentedBytes() const
    {
        return FragmentedBytes;
    }

    D3D12_RESOURCE_STATES GetInitialState() const
    {
        return InitialState;
    }

    uint64 GetAlignment() const
    {
        return Alignment;
    }

private:
    FD3D12PoolAllocatorPage* CreatePage(uint64 MinimumSize, uint32& OutPageIndex);
    void ComputeTLSFIndices(uint64 SizeInBytes, uint32& OutFL, uint32& OutSL) const;
    void RebuildFragmentationData();

    uint64                           PageSizeBytes;
    uint64                           Alignment;
    uint64                           MaxResourceSize;
    D3D12_HEAP_TYPE                  HeapType;
    D3D12_RESOURCE_STATES            InitialState;
    EAllocationStrategy              AllocationStrategy;
    D3D12_RESOURCE_FLAGS             ResourceFlags;
    uint64                           FragmentedBytes;
    TArray<FD3D12PoolAllocatorPage*> Pages;
    mutable FCriticalSection         PagesCS;
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

    bool TryAllocate(uint64 SizeInBytes, FD3D12ResourceStorage& OutStorage);
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
    
    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    void* AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage);

    bool Initialize();
    void Destroy();
    void CleanUp();

private:
    uint64                    PageSizeBytes;
    uint64                    DefaultAlignment;
    uint64                    SmallAllocationThreshold;
    uint64                    LargeAllocationThreshold;
    FD3D12MultiBuddyAllocator SmallAllocator;
    FD3D12PoolAllocator       LargeAllocator;
    FD3D12MultiBuddyAllocator ConstantsAllocator;
};

class FD3D12LinearAllocatorPage : public FD3D12DeviceChild
{
public:
    FD3D12LinearAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
    ~FD3D12LinearAllocatorPage() = default;

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset);
    bool Initialize();
    void Reset();
    
    bool IsExhausted() const
    {
        return CurrentOffset >= PageSizeBytes;
    }

    FD3D12ResourceStorage& GetBackingResourceStorage()
    {
        return BackingResourceStorage;
    }

    const FD3D12ResourceStorage& GetBackingResourceStorage() const
    {
        return BackingResourceStorage;
    }

private:
    uint64                PageSizeBytes;
    D3D12_HEAP_TYPE       HeapType;
    D3D12_RESOURCE_STATES InitialState;
    uint64                CurrentOffset;
    FD3D12ResourceStorage BackingResourceStorage;
};

class FD3D12LinearAllocator : public FD3D12DeviceChild
{

public:
    FD3D12LinearAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
    ~FD3D12LinearAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    void CleanUp();

private:
    FD3D12LinearAllocatorPage* CreatePage();
    void RetirePage(FD3D12LinearAllocatorPage* Page);

    uint64                             PageSizeBytes;
    D3D12_HEAP_TYPE                    HeapType;
    D3D12_RESOURCE_STATES              InitialState;
    TArray<FD3D12LinearAllocatorPage*> Pages;
    TArray<FD3D12LinearAllocatorPage*> FullPages;
    FCriticalSection                   PagesCS;
};

class FD3D12DynamicConstantsAllocator : public FD3D12DeviceChild
{
public:
    FD3D12DynamicConstantsAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes);
    ~FD3D12DynamicConstantsAllocator() = default;

    void* Allocate(uint64 SizeInBytes, FD3D12ResourceStorage& OutStorage);
    void CleanUp();

private:
    FD3D12LinearAllocator LinearAllocator;
};

class FD3D12BufferAllocatorPool : public FD3D12DeviceChild
{
public:
    static D3D12_RESOURCE_STATES GetInitialResourceStateForHeapType(D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES RequestedInitialState);
    
public:
    FD3D12BufferAllocatorPool(FD3D12Device* InDevice, D3D12_HEAP_TYPE InHeapType, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy);
    ~FD3D12BufferAllocatorPool();
    
    bool TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, const D3D12_RESOURCE_DESC& ResourceDesc) const;

    bool Initialize();
    void CleanUp();

private:
    void Destroy();

    D3D12_HEAP_TYPE            HeapType;
    D3D12_RESOURCE_STATES      InitialState;
    EAllocationStrategy        AllocationStrategy;
    uint64                     PageSizeBytes;
    uint64                     MinBlockBytes;
    uint64                     MaxSuballocationSize;
    FD3D12MultiBuddyAllocator  MultiBuddyAllocator;
};

class FD3D12BufferAllocator : public FD3D12DeviceChild
{
public:
    static EAllocationStrategy GetAllocationStrategy(D3D12_HEAP_TYPE HeapType);

public:
    FD3D12BufferAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize);
    ~FD3D12BufferAllocator();
    
    bool TryAllocate(D3D12_HEAP_TYPE HeapType, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, const D3D12_RESOURCE_DESC& ResourceDesc) const;

    bool Initialize();
    void Destroy();
    void CleanUp();

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
        SmallReadOnly            = 0,
        ReadOnly                 = 1,
        RenderTargetDepthStencil = 2,
        UAVOnly                  = 3,
        Count
    };

    static constexpr uint32 TexturePoolClassCount = static_cast<uint32>(ETexturePoolClass::Count);

    struct FPendingDefragMove
    {
        FD3D12ResourceStorage*            SourceStorage;
        FD3D12Resource*                   NewResource;
        FD3D12PoolAllocator*              Allocator;
        FD3D12PoolAllocatorAllocationData OldAllocationData;
        FD3D12PoolAllocatorAllocationData NewAllocationData;
        D3D12_RESOURCE_STATES             ResourceState;
        uint64                            FenceValueAtCreation;
    };

public:
    FD3D12TextureAllocator(FD3D12Device* InDevice, uint64 InDefaultPageSizeBytes, uint64 InCommittedThreshold);
    ~FD3D12TextureAllocator();

    bool TryAllocate(const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceStorage& OutStorage);
    bool Supports(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc) const;

    void DefragmentAllocations(FD3D12CommandContext* InCommandContext, int32 MaxMovesPerFrame, FD3D12FenceManager& FenceManager);
    void CancelPendingDefragMoves(FD3D12GenericResource* Owner);

    bool Initialize();
    void Destroy();
    void CleanUp();    

private:
    ETexturePoolClass ClassifyTexture(const D3D12_RESOURCE_DESC& Desc, uint64 Alignment) const;
    bool CanUseSmallResourcePlacementAlignment(const D3D12_RESOURCE_DESC& Desc) const;
    bool GetDefragCandidate(FD3D12PoolAllocatorAllocationData& OutCandidate, FD3D12PoolAllocator*& OutAllocator);
    void ReleasePools();

    uint64                     CommittedThreshold;
    uint64                     DefaultPageSizeBytes;
    uint64                     SmallPoolAlignment;
    FD3D12PoolAllocator*       Pools[TexturePoolClassCount];
    TArray<FPendingDefragMove> PendingDefragMoves;
    FCriticalSection           PoolsCS;
};
