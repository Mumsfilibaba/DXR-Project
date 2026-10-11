#pragma once
#include "Core/Core.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FPlatformMemoryStats
{
    /** @brief The memory the system charges the process for: its physical footprint on Mac, its private commit on Windows */
    uint64 FootprintBytes = 0;

    /** @brief The highest footprint the process has reached */
    uint64 PeakFootprintBytes = 0;

    /** @brief The process's pages currently held in physical memory, its working set on Windows */
    uint64 ResidentBytes = 0;

    /** @brief The highest resident size the process has reached */
    uint64 PeakResidentBytes = 0;

    /** @brief Bytes in use in the C runtime heap, which includes allocations made outside GMalloc */
    uint64 AllocatorInUseBytes = 0;

    /** @brief The physical memory installed in the machine */
    uint64 TotalPhysicalBytes = 0;

    /** @brief The physical memory the system can hand out right now without paging anything out */
    uint64 AvailablePhysicalBytes = 0;
};

struct IPlatformMemory
{
    /**
     * @brief Allocates a block from the system heap, bypassing GMalloc and the engine's tracking
     *
     * @param Size Number of bytes to allocate, which must be non-zero
     * @param Alignment Alignment of the block, a power of two of at least STANDARD_ALIGNMENT
     * @return Returns the new block, or nullptr when the allocation failed
     */
    static FORCEINLINE void* Malloc(uint64 Size, uint32 Alignment)
    {
        return nullptr;
    }

    /**
     * @brief Resizes a block from the system heap, keeping the contents up to the smaller of the old and new size
     *
     * @param Block Block returned by Malloc or Realloc, which must not be nullptr
     * @param Size Number of bytes to reallocate
     * @param Alignment Alignment of the block, which must match the alignment Block was allocated with
     * @return Returns the resized block, or nullptr when the allocation failed and Block is left untouched
     */
    static FORCEINLINE void* Realloc(void* Block, uint64 Size, uint32 Alignment)
    {
        return nullptr;
    }

    /**
     * @brief Returns a block to the system heap, whatever alignment it was allocated with
     *
     * @param Block Block returned by Malloc or Realloc, which must not be nullptr
     */
    static FORCEINLINE void Free(void* Block)
    {
    }

    /**
     * @brief Reads the process's memory use from the operating system
     *
     * @param OutStats Receives the snapshot, with any field the platform cannot report left at zero
     * @return Returns true when the snapshot was read
     */
    static FORCEINLINE bool GetStats(FPlatformMemoryStats& OutStats)
    {
        OutStats = FPlatformMemoryStats();
        return false;
    }

    /** @return Returns the size of a virtual memory page, in bytes */
    static FORCEINLINE uint64 GetPageSize()
    {
        return 0;
    }

    /**
     * @brief Maps whole pages of zeroed, readable and writable memory straight from the operating system, bypassing
     * every heap, so that memory can be taken from inside an allocator without recursing into it.
     *
     * @param Size Number of bytes to map, which must be non-zero and is rounded up to whole pages
     * @return Returns the first page, aligned to GetPageSize, or nullptr when the mapping failed
     */
    static FORCEINLINE void* AllocatePages(uint64 Size)
    {
        return nullptr;
    }

    /**
     * @brief Returns pages mapped by AllocatePages to the operating system
     *
     * @param Pages The pointer AllocatePages returned
     * @param Size The size that was passed to AllocatePages
     */
    static FORCEINLINE void FreePages(void* Pages, uint64 Size)
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
