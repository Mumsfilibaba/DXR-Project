#pragma once
#include "Core/Memory/MemoryProfilerTypes.h"
#include "Core/Threading/Spinlock.h"

struct FMemoryAllocationEntry
{
    /** @brief The block the entry describes, which is the map's key */
    void* Block = nullptr;

    /** @brief The size requested for the block, in bytes */
    uint64 Size = 0;

    /** @brief The module tag that was current when the block was allocated */
    FMemoryNameId Tag = MEMORY_NAME_UNTAGGED;

    /** @brief The scope that was current when the block was allocated */
    FMemoryScopeId Scope = MEMORY_SCOPE_ROOT;

    /** @brief The allocating callstack, zero until callstacks are captured */
    uint32 CallstackId = 0;
};

enum class EMemoryMapInsertResult : uint8
{
    Failed   = 0,
    Inserted = 1,
    Replaced = 2,
};

class CORE_API FMemoryAllocationMap
{
public:

    /** @brief How many independently locked shards the blocks are spread over */
    static constexpr int32 NumShards = 64;

public:
    FMemoryAllocationMap(const FMemoryAllocationMap&) = delete;
    FMemoryAllocationMap& operator=(const FMemoryAllocationMap&) = delete;

    /** @brief Creates an empty map whose tables are mapped from OS pages as each shard first needs one */
    constexpr FMemoryAllocationMap() noexcept
        : Shards()
    {
    }

    /**
     * @brief Records a live block, replacing any entry already held for the same address
     *
     * @param Entry The block and what it is attributed to
     * @param OutReplaced Receives the entry that was replaced, when the result is Replaced
     * @return Returns Failed when the shard's table could not grow, so the block is not tracked
     */
    EMemoryMapInsertResult Insert(const FMemoryAllocationEntry& Entry, FMemoryAllocationEntry& OutReplaced);

    /**
     * @brief Removes the entry for a block that is being freed
     *
     * @param Block The block's address
     * @param OutEntry Receives the removed entry
     * @return Returns true when the block was tracked, false for a block allocated before tracking began
     */
    bool Remove(const void* Block, FMemoryAllocationEntry& OutEntry);

    /** @brief Forgets every block and unmaps every table */
    void Clear();

    /** @return Returns how many blocks are tracked */
    int64 GetNumEntries();

private:
    struct alignas(64) FShard
    {
        FSpinLock               Lock;
        FMemoryAllocationEntry* Entries       = nullptr;
        uint32                  Capacity      = 0;
        uint32                  NumEntries    = 0;
        uint32                  NumTombstones = 0;
    };

    static bool Grow(FShard& Shard);

    FShard Shards[NumShards];
};
