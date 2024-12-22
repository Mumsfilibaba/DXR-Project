#pragma once
#include "VulkanDeviceChild.h"

struct FVulkanMemoryBlock;
class FVulkanMemoryHeap;

struct FVulkanMemoryAllocation
{
    FVulkanMemoryAllocation()
    {
        Reset();
    }
    
    bool IsValid() const
    {
        // Block is nullptr in the case of a dedicated allocation
        return Memory != VK_NULL_HANDLE;
    }
    
    void Reset()
    {
        Memory        = VK_NULL_HANDLE;
        Offset        = 0;
        DeviceAddress = 0;
        Block         = nullptr;
        bIsDedicated  = false;
    }
    
    VkDeviceMemory      Memory;
    VkDeviceSize        Offset;
    VkDeviceAddress     DeviceAddress;
    FVulkanMemoryBlock* Block;
    bool                bIsDedicated;
};

struct FVulkanMemoryBlock
{
    FVulkanMemoryBlock()
        : Page(nullptr)
        , Next(nullptr)
        , Previous(nullptr)
        , SizeInBytes(0)
        , TotalSizeInBytes(0)
        , Offset(0)
        , bIsFree(true)
    {
    }

    FVulkanMemoryHeap* Page;

    // Linked list of blocks
    FVulkanMemoryBlock* Next;
    FVulkanMemoryBlock* Previous;

    // Size of the allocation
    uint64 SizeInBytes;

    // Total size of the block (TotalSizeInBytes - SizeInBytes = AlignmentOffset)
    uint64 TotalSizeInBytes;

    // Offset of the DeviceMemory
    uint64 Offset;
    bool   bIsFree;
};

class FVulkanMemoryHeap : public FVulkanDeviceChild, private FNonCopyable
{
public:
    FVulkanMemoryHeap(FVulkanDevice* InDevice, VkMemoryAllocateFlags InAllocationFlags, uint32 InHeapIndex, uint32 InMemoryIndex);
    ~FVulkanMemoryHeap();

    bool Initialize(uint64 InSizeInBytes);
    bool Allocate(FVulkanMemoryAllocation& OutAllocation, VkDeviceSize InSizeInBytes, VkDeviceSize Alignment, VkDeviceSize PageGranularity);
    bool Free(FVulkanMemoryAllocation& OutAllocation);
    void* Map(const FVulkanMemoryAllocation& OutAllocation);
    void Unmap(const FVulkanMemoryAllocation& OutAllocation);
    void SetDebugName(const FString& InName);

    FORCEINLINE bool IsEmpty() const
    {
        if (Head)
        {
            return Head->bIsFree && Head->TotalSizeInBytes == SizeInBytes;
        }

        return true;
    }

    FORCEINLINE uint32 GetMemoryIndex() const
    {
        return MemoryIndex;
    }
    
    FORCEINLINE VkMemoryAllocateFlags GetAllocationFlags() const
    {
        return AllocationFlags;
    }

    FORCEINLINE uint32 GetHeapIndex() const
    {
        return HeapIndex;
    }

private:
    bool IsAliasing(VkDeviceSize FirstBlockOffset, VkDeviceSize FirstBlockSize, VkDeviceSize SecondBlockOffset, VkDeviceSize PageGranularity);

    bool ValidateNoOverlap() const;
    bool ValidateChain() const;
    bool ValidateBlock(FVulkanMemoryBlock* Block) const;

    uint64                SizeInBytes;
    VkMemoryAllocateFlags AllocationFlags;
    uint32                MemoryIndex;
    uint32                HeapIndex;
    FVulkanMemoryBlock*   Head;
    uint8*                HostMemory;
    VkDeviceMemory        DeviceMemory;
    uint32                MappingCount;
    FString               DebugName;
    FCriticalSection      HeapCS;

#ifdef DEBUG_BUILD
    TArray<FVulkanMemoryBlock*> AllBlocks;
#endif
};

class FVulkanMemoryManager : public FVulkanDeviceChild
{
public:
    FVulkanMemoryManager(FVulkanDevice* InDevice);
    ~FVulkanMemoryManager();
    
    bool AllocateBufferMemory(VkBuffer Buffer, VkMemoryPropertyFlags PropertyFlags, VkMemoryAllocateFlags AllocateFlags, bool bForceDedicatedAllocation, FVulkanMemoryAllocation& OutAllocation);
    bool AllocateImageMemory(VkImage Image, VkMemoryPropertyFlags PropertyFlags, VkMemoryAllocateFlags AllocateFlags, bool bForceDedicatedAllocation, FVulkanMemoryAllocation& OutAllocation); 
    bool AllocateMemoryDedicated(VkDeviceMemory& OutDeviceMemory, const VkMemoryAllocateInfo& AllocateInfo);
    bool AllocateMemoryDedicated(VkDeviceMemory& OutDeviceMemory, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint32 MemoryIndex);
    bool AllocateMemoryFromHeap(FVulkanMemoryAllocation& OutAllocation, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, uint32 MemoryIndex);
    bool Free(FVulkanMemoryAllocation& OutAllocation);
    void FreeMemory(VkDeviceMemory& OutDeviceMemory);
    void* Map(const FVulkanMemoryAllocation& Allocation);
    void Unmap(const FVulkanMemoryAllocation& Allocation);
    
private:
    TArray<FVulkanMemoryHeap*> MemoryHeaps;
    VkDeviceSize               HeapSize;
    FAtomicInt32               NumAllocations;
    uint32                     MaxMemoryAllocationCount;
    uint32                     BufferImageGranularity;
    FCriticalSection           ManagerCS;
};
