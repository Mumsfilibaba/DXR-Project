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
class FD3D12ResidencyManager;

struct FD3D12GpuFencePoint
{
    uint64                 FenceValue = 0;
    ED3D12CommandQueueType QueueType  = ED3D12CommandQueueType::Direct;
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

enum class ED3D12DeferredAllocatorType : uint8
{
    Pool,
    Buddy,
    Bucket
};

class FD3D12BuddyAllocator : public FD3D12DeviceChild
{
    struct FPage
    {
        FD3D12HeapRef             Heap;
        FD3D12ResourceRef         Resource;
        TArray<TArray<uint64>>    FreeOffsets;
        uint8*                    MappedBaseAddress = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS BaseGpuVirtualAddress = 0;
        uint64                    Size = 0;
        uint32                    PageIndex = UINT32_MAX;
        FD3D12ResidencyHandle     ResidencyHandle = {};
    };

public:
    FD3D12BuddyAllocator(FD3D12Device* InDevice);
    ~FD3D12BuddyAllocator();

    bool Initialize(uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, bool bInBackWithHeap);
    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void ReturnBlockToAllocator(const FD3D12ResourceStorage& Storage);

    bool IsBackedByHeap() const
    {
        return bBackWithHeap;
    }

private:
    uint32 GetOrderForSize(uint64 Size) const;
    uint64 GetOrderBlockSize(uint32 Order) const;
    bool AllocateFromPage(FPage& Page, uint64 Size, uint64 Alignment, uint64& OutOffset, uint32& OutOrder);
    void FreeToPage(FPage& Page, uint64 Offset, uint32 Order);
    bool CreatePage(FPage& OutPage, uint64 RequestedSize);

private:
    uint64                PageSizeBytes;
    uint64                MinBlockBytes;
    D3D12_HEAP_TYPE       HeapType;
    D3D12_RESOURCE_STATES InitialState;
    bool                  bBackWithHeap;
    uint32                NextPageIndex;
    TArray<FPage>         Pages;
    FCriticalSection      PagesCS;
};

class FD3D12MultiBuddyAllocator : public FD3D12DeviceChild
{
public:
    FD3D12MultiBuddyAllocator(FD3D12Device* InDevice);
    ~FD3D12MultiBuddyAllocator();

    bool Initialize(uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, bool bInBackWithHeap);
    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    bool CreateAllocator();

    uint64                        PageSizeBytes;
    uint64                        MinBlockBytes;
    D3D12_HEAP_TYPE               HeapType;
    D3D12_RESOURCE_STATES         InitialState;
    bool                          bBackWithHeap;
    TArray<FD3D12BuddyAllocator*> Allocators;
    FCriticalSection              AllocatorsCS;
};

class FD3D12PoolAllocator : public FD3D12DeviceChild
{
    static constexpr uint32 TLSFFirstLevelCount  = 32;
    static constexpr uint32 TLSFSecondLevelCount = 8;

    struct FFreeRange
    {
        uint64 Offset = 0;
        uint64 Size   = 0;
    };

    struct FDefragRecord
    {
        uint32 PageIndex = UINT32_MAX;
        uint64 Offset    = 0;
        uint64 Size      = 0;
    };

    struct FPage
    {
        FD3D12HeapRef             Heap;
        FD3D12ResourceRef         Resource;
        uint8*                    MappedBaseAddress = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS BaseGpuVirtualAddress = 0;
        uint64                    Size      = 0;
        uint64                    UsedBytes = 0;
        uint32                    PageIndex = UINT32_MAX;
        FD3D12ResidencyHandle     ResidencyHandle = {};
        TArray<FFreeRange>        FreeRanges;
    };

public:
    FD3D12PoolAllocator(FD3D12Device* InDevice);
    ~FD3D12PoolAllocator();

    bool Initialize(uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxResourceSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, bool bInBackWithHeap);
    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void ReturnBlockToAllocator(const FD3D12ResourceStorage& Storage);

    uint64 GetFragmentedBytes() const { return FragmentedBytes; }

private:
    bool CreatePage(uint64 MinimumSize, FPage& OutPage);
    bool AllocateFromPage(FPage& Page, uint64 Size, uint64 Alignment, uint64& OutOffset);
    void CoalesceFreeRanges(TArray<FFreeRange>& FreeRanges);
    void ComputeTLSFIndices(uint64 Size, uint32& OutFL, uint32& OutSL) const;
    void AddDefragRecord(uint32 PageIndex, uint64 Offset, uint64 Size);

private:
    uint64                PageSizeBytes;
    uint64                Alignment;
    uint64                MaxResourceSize;
    D3D12_HEAP_TYPE       HeapType;
    D3D12_RESOURCE_STATES InitialState;
    bool                  bBackWithHeap;
    uint32                NextPageIndex;
    uint64                FragmentedBytes;
    TArray<FDefragRecord> DefragRecords;
    TArray<FPage>         Pages;
    FCriticalSection      PagesCS;
};

class FD3D12BucketAllocator : public FD3D12DeviceChild
{
    struct FBucket
    {
        uint64 BlockSize = 0;
        FD3D12PoolAllocator* Pool = nullptr;
    };

public:
    FD3D12BucketAllocator(FD3D12Device* InDevice);
    ~FD3D12BucketAllocator();

    bool Initialize(const TArray<uint64>& InBucketSizes, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
    void Shutdown();

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);
    void ReturnBlockToAllocator(const FD3D12ResourceStorage& Storage);

private:
    uint64                PageSizeBytes;
    uint64                Alignment;
    D3D12_HEAP_TYPE       HeapType;
    D3D12_RESOURCE_STATES InitialState;
    TArray<FBucket>       Buckets;
};

class FD3D12UploadHeapAllocator : public FD3D12DeviceChild
{
public:
    FD3D12UploadHeapAllocator(FD3D12Device* InDevice);
    ~FD3D12UploadHeapAllocator();

    bool Initialize(uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallAllocationThreshold = 64ull * 1024ull, uint64 InLargeAllocationThreshold = 2ull * 1024ull * 1024ull);
    void Shutdown();

    void* Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    void* AllocateConstants(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage);

    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    uint64                    PageSizeBytes;
    uint64                    DefaultAlignment;
    uint64                    SmallAllocationThreshold;
    uint64                    LargeAllocationThreshold;
    FD3D12MultiBuddyAllocator SmallAllocator;
    FD3D12PoolAllocator       LargeAllocator;
    FD3D12MultiBuddyAllocator ConstantsAllocator;
};

class FD3D12LinearAllocator : public FD3D12DeviceChild
{
public:
    FD3D12LinearAllocator(FD3D12Device* InDevice);
    ~FD3D12LinearAllocator();

    bool Initialize(uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, FD3D12UploadHeapAllocator* InUploadHeapAllocator = nullptr);
    void Shutdown();

    void* Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    struct FPage
    {
        FD3D12ResourceStorage BackingResourceStorage;
        uint64                CurrentOffset        = 0;
        uint32                ActiveAllocations    = 0;
        uint32                PageIndex            = UINT32_MAX;
    };

private:
    bool CreatePage(FPage& OutPage);
    void RetirePage(FPage& Page);

private:
    uint64                   PageSizeBytes;
    D3D12_HEAP_TYPE          HeapType;
    D3D12_RESOURCE_STATES    InitialState;
    uint32                   NextPageIndex;
    FD3D12UploadHeapAllocator* UploadHeapAllocator;
    TArray<FPage>            Pages;
    FCriticalSection         PagesCS;
};

class FD3D12DynamicConstantsAllocator : public FD3D12DeviceChild
{
public:
    FD3D12DynamicConstantsAllocator(FD3D12Device* InDevice);
    ~FD3D12DynamicConstantsAllocator();

    bool Initialize(uint64 InPageSizeBytes, FD3D12UploadHeapAllocator* InUploadHeapAllocator);
    void Shutdown();

    void* Allocate(uint64 Size, FD3D12ResourceStorage& OutStorage);
    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    FD3D12LinearAllocator LinearAllocator;
};

class FD3D12BufferAllocatorPool : public FD3D12DeviceChild
{
public:
    FD3D12BufferAllocatorPool(FD3D12Device* InDevice);
    ~FD3D12BufferAllocatorPool();

    bool Initialize(D3D12_HEAP_TYPE InHeapType, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, D3D12_RESOURCE_STATES InInitialState);
    void Shutdown();

    bool Supports(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc) const;
    bool TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, EBufferFlags BufferUsage, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    D3D12_HEAP_TYPE          HeapType;
    D3D12_RESOURCE_STATES    InitialState;
    uint64                   MaxSuballocationSize;
    FD3D12MultiBuddyAllocator MultiBuddyAllocator;
};

class FD3D12BufferAllocator : public FD3D12DeviceChild
{
public:
    FD3D12BufferAllocator(FD3D12Device* InDevice);
    ~FD3D12BufferAllocator();

    bool Initialize(uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize);
    void Shutdown();

    bool TryAllocate(D3D12_HEAP_TYPE HeapType, const D3D12_RESOURCE_DESC& ResourceDesc, EBufferFlags BufferUsage, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage);
    bool TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage);
    void Deallocate(const FD3D12ResourceStorage& Storage);

private:
    uint64                     PageSizeBytes;
    uint64                     MinBlockBytes;
    uint64                     MaxSuballocationSize;
    TArray<FD3D12BufferAllocatorPool*> Pools;
    FCriticalSection           PoolsCS;
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
