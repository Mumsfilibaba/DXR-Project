#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Platform/CriticalSection.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalHeap.h"
#include "MetalRHI/MetalResource.h"

class FMetalQueue;
class FMetalCommandContext;

#if METAL_ENABLE_STATS
struct FMetalAllocatorUsage
{
    uint64 AllocatedBytes  = 0;
    uint64 UsedBytes       = 0;
    uint64 FragmentedBytes = 0;
    uint32 NumBlocks       = 0;
};
#endif

struct FMetalDefragCandidate
{
    FMetalResourceStorage* Storage = nullptr;
    uint32                 HeapIndex = UINT32_MAX;
};

struct FMetalDefragMove
{
    FMetalResourceStorage*            Storage = nullptr;
    TUniquePtr<FMetalResourceStorage> Target;
    bool                              bCancelled = false;
};

class FMetalHeapPool : public FMetalDeviceChild
{
public:
    explicit FMetalHeapPool(FMetalDevice* InDevice);
    ~FMetalHeapPool();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FMetalResourceStorage* Storage, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 SourceHeapIndex, FMetalResourceStorage* Storage, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap);

    void SelectDefragCandidates(uint32 MaxMoves, uint64 EligibleBeforeFrame, TArray<FMetalDefragCandidate>& OutCandidates);
    void RetargetAllocation(uint32 HeapIndex, uint64 Offset, FMetalResourceStorage* NewStorage);

    void Deallocate(uint32 HeapIndex, uint64 Offset, uint64 Size);
    void CleanUp();
    void Trim();

    void Destroy();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats(FMetalAllocatorUsage& OutUsage) const;
#endif

private:
    struct FHeapFreeRange
    {
        uint64 Offset;
        uint64 Size;
    };

    struct FHeapAllocation
    {
        uint64                 Offset;
        uint64                 Size;
        FMetalResourceStorage* Storage;
        uint64                 CreatedFrame;
    };

    struct FHeapBlock
    {
        FMetalHeap*             Heap;
        uint64                  Size;
        uint64                  UsedBytes;
        TArray<FHeapFreeRange>  FreeRanges;
        TArray<FHeapAllocation> Allocations;
        bool                    bVolatile;
    };

    struct FPendingHeapFree
    {
        uint32       HeapIndex;
        uint64       Offset;
        uint64       Size;
        uint64       DirectFenceValue;
        uint64       ComputeFenceValue;
        uint64       CopyFenceValue;
    };

    uint32 CreateHeapBlock(uint64 MinimumSize);
    void   ReleaseHeapBlock(FHeapBlock& Block);
    bool   TryAllocateFromBlocks(uint64 SizeInBytes, uint64 Alignment, FMetalResourceStorage* Storage, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap);
    bool   TrySuballocate(FHeapBlock& Block, uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset);
    void   RecordAllocation(FHeapBlock& Block, uint64 Offset, uint64 Size, FMetalResourceStorage* Storage);
    void   ReturnHeapRange(uint32 HeapIndex, uint64 Offset, uint64 Size);
    void   RecyclePendingHeapFrees();
    void   DropUnusedHeaps(uint64 MaxUnusedBytes);
    bool   IsHeapFreeEligible(const FPendingHeapFree& PendingFree) const;
    bool   ReviveHeapBlock(FHeapBlock& Block);

    static constexpr uint64 DefaultHeapSize    = 64ull * 1024ull * 1024ull;
    static constexpr uint64 MaxUnusedHeapBytes = 64ull * 1024ull * 1024ull;

    mutable FCriticalSection PoolCS;
    TArray<FHeapBlock>       HeapBlocks;
    TArray<FPendingHeapFree> PendingHeapFrees;
};

enum class EMetalAllocationLifetime : uint8
{
    Submission,
    Frame,
};

class FMetalLinearAllocator : public FMetalDeviceChild
{
public:
    FMetalLinearAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InLargeAllocationThreshold, MTLResourceOptions InOptions, bool bInBindlessReachable, EMetalAllocationLifetime InLifetime);
    ~FMetalLinearAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage);
    void  RetireAllocations(FMetalQueue* Queue, uint64 SubmissionValue);

    void  EndFrame();
    void  CleanUp();
    void  Trim();
    void  Destroy();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats(FMetalAllocatorUsage& OutUsage) const;
#endif

private:
    struct FPage
    {
        id<MTLBuffer> Buffer;
        uint64        Size;
        uint64        Offset;
        uint64        UsedBytes;
        FMetalQueue*  Queue;
        uint64        EligibleFromFenceValue;
        uint64        DirtyBegin;
        uint64        DirtyEnd;
        bool          bDedicated;
        bool          bPendingRetire;
    };

    FPage* CreatePage(uint64 SizeInBytes, bool bDedicated);
    FPage* FindFreePage(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue);
    void   ReleasePage(FPage* Page);
    void   RecycleRetiredPages();
    void   DropUnusedPages(uint64 MaxUnusedPageBytes);

    static constexpr uint64 MaxUnusedBytes = 32ull * 1024ull * 1024ull;

    mutable FCriticalSection AllocatorsCS;
    TArray<FPage*>           Pages;
    FPage*                   ActivePage;
    uint64                   PageSizeBytes;
    uint64                   LargeAllocationThreshold;
    MTLResourceOptions       Options;
    EMetalAllocationLifetime Lifetime;
    bool                     bManaged;
    bool                     bBindlessReachable;
};

class FMetalUploadHeapAllocator : public FMetalDeviceChild
{
public:
    FMetalUploadHeapAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InLargeAllocationThreshold);
    ~FMetalUploadHeapAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage);
    void  RetireAllocations(FMetalQueue* Queue, uint64 SubmissionValue);
    void  EndFrame();
    void  CleanUp();
    void  Trim();
    void  Destroy();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    FMetalLinearAllocator UploadAllocator;
};

class FMetalBufferAllocator : public FMetalDeviceChild
{
public:
    explicit FMetalBufferAllocator(FMetalDevice* InDevice);
    ~FMetalBufferAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, MTLResourceOptions Options, bool bBindlessReachable, FMetalResourceStorage& OutStorage);
    void Deallocate(FMetalResourceStorage& Storage);

    uint32 RecordDefragMoves(FMetalCommandContext& Context, uint32 MaxMoves, uint64 EligibleBeforeFrame, TArray<FMetalDefragMove>& OutMoves);
    void   RetargetAllocation(FMetalResourceStorage& Storage);

    void CleanUp();
    void Trim();
    void Destroy();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    FMetalHeapPool HeapPool;
};

class FMetalTextureAllocator : public FMetalDeviceChild
{
public:
    explicit FMetalTextureAllocator(FMetalDevice* InDevice);
    ~FMetalTextureAllocator();

    bool TryAllocate(MTLTextureDescriptor* TextureDescriptor, FMetalResourceStorage& OutStorage);
    void Deallocate(FMetalResourceStorage& Storage);

    uint32 RecordDefragMoves(FMetalCommandContext& Context, uint32 MaxMoves, uint64 EligibleBeforeFrame, TArray<FMetalDefragMove>& OutMoves);
    void   RetargetAllocation(FMetalResourceStorage& Storage);

    void CleanUp();
    void Trim();
    void Destroy();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    FMetalHeapPool HeapPool;
};
