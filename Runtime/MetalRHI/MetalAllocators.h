#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalHeap.h"
#include "MetalRHI/MetalResource.h"

class FMetalQueue;

#if METAL_ENABLE_STATS
struct FMetalAllocatorUsage
{
    uint64 AllocatedBytes  = 0;
    uint64 UsedBytes       = 0;
    uint64 FragmentedBytes = 0;
};
#endif

class FMetalHeapPool : public FMetalDeviceChild
{
public:
    explicit FMetalHeapPool(FMetalDevice* InDevice);
    ~FMetalHeapPool();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap);
    void Deallocate(uint32 HeapIndex, uint64 Offset, uint64 Size);
    void CleanUp();
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

    struct FHeapBlock
    {
        FMetalHeap*            Heap;
        uint64                 Size;
        uint64                 UsedBytes;
        uint32                 AllocCount;
        TArray<FHeapFreeRange> FreeRanges;
    };

    struct FPendingHeapFree
    {
        uint32 HeapIndex;
        uint64 Offset;
        uint64 Size;
        uint64 DirectFenceValue;
        uint64 ComputeFenceValue;
        uint64 CopyFenceValue;
    };

    FHeapBlock* CreateHeapBlock(uint64 MinimumSize);
    bool        TrySuballocate(FHeapBlock& Block, uint32 HeapIndex, uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset);
    void        ReturnHeapRange(uint32 HeapIndex, uint64 Offset, uint64 Size);
    void        RecyclePendingHeapFrees();
    void        DropUnusedHeaps();
    void        StampPendingHeapFree(FPendingHeapFree& PendingFree) const;
    bool        IsHeapFreeEligible(const FPendingHeapFree& PendingFree) const;

    static constexpr uint64 DefaultHeapSize    = 64ull * 1024ull * 1024ull;
    static constexpr uint64 MaxUnusedHeapBytes = 64ull * 1024ull * 1024ull;

    FCriticalSection         PoolCS;
    TArray<FHeapBlock>       HeapBlocks;
    TArray<FPendingHeapFree> PendingHeapFrees;
};

class FMetalLinearAllocator : public FMetalDeviceChild
{
public:
    FMetalLinearAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InLargeAllocationThreshold);
    ~FMetalLinearAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage);
    void  RetireAllocations(FMetalQueue* Queue, uint64 SubmissionValue);
    void  CleanUp();
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
        bool          bDedicated;
        bool          bPendingRetire;
    };

    FPage* CreatePage(uint64 SizeInBytes, bool bDedicated);
    FPage* FindFreePage(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue);
    void   ReleasePage(FPage* Page);
    void   DropUnusedPages();

    static constexpr uint64 MaxUnusedBytes = 32ull * 1024ull * 1024ull;

    FCriticalSection AllocatorsCS;
    TArray<FPage*>   Pages;
    FPage*           ActivePage;
    uint64           PageSizeBytes;
    uint64           LargeAllocationThreshold;
};

class FMetalUploadHeapAllocator : public FMetalDeviceChild
{
public:
    FMetalUploadHeapAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InConstantPageSizeBytes, uint64 InLargeAllocationThreshold);
    ~FMetalUploadHeapAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage);
    void* AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage);
    void  RetireAllocations(FMetalQueue* Queue, uint64 SubmissionValue);
    void  CleanUp();
    void  Destroy();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    FMetalLinearAllocator UploadAllocator;
    FMetalLinearAllocator ConstantsAllocator;
};

class FMetalBufferAllocator : public FMetalDeviceChild
{
public:
    explicit FMetalBufferAllocator(FMetalDevice* InDevice);
    ~FMetalBufferAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, MTLResourceOptions Options, FMetalResourceStorage& OutStorage);
    void Deallocate(FMetalResourceStorage& Storage);
    void CleanUp();
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
    void CleanUp();
    void Destroy();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    FMetalHeapPool HeapPool;
};
