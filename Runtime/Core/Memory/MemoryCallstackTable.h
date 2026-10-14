#pragma once
#include "Core/Memory/MemoryProfilerTypes.h"
#include "Core/Threading/Spinlock.h"

class CORE_API FMemoryCallstackTable
{
public:

    /** @brief The most frames a callstack keeps, counted from the innermost */
    static constexpr int32 MaxFrames = 32;

    /** @brief How many independently locked shards the callstacks are spread over */
    static constexpr int32 NumShards = 16;

public:
    FMemoryCallstackTable(const FMemoryCallstackTable&) = delete;
    FMemoryCallstackTable& operator=(const FMemoryCallstackTable&) = delete;

    /** @brief Creates an empty table whose storage is mapped from OS pages as each shard first needs it */
    constexpr FMemoryCallstackTable() noexcept
        : Shards()
    {
    }

    /**
     * @brief Interns a callstack, so the same frames always get the same id. Callstacks are never removed.
     *
     * @param Frames The return addresses, innermost first
     * @param NumFrames How many addresses Frames holds, of which at most MaxFrames are kept
     * @return Returns the callstack's id, or MEMORY_CALLSTACK_NONE when it is empty or its storage could not be mapped
     */
    FMemoryCallstackId Intern(const uint64* Frames, int32 NumFrames);

    /**
     * @brief Copies out the frames of an interned callstack
     *
     * @param Id The callstack's id
     * @param OutFrames Receives the return addresses, innermost first, and needs room for MaxFrames
     * @return Returns how many frames were copied, zero for MEMORY_CALLSTACK_NONE and for an unknown id
     */
    int32 GetFrames(FMemoryCallstackId Id, uint64* OutFrames);

    /** @return Returns how many distinct callstacks are held */
    int32 GetNumCallstacks();

private:
    struct FRecord;

    struct alignas(64) FShard
    {
        FSpinLock Lock;
        FRecord** Records        = nullptr;
        uint32    NumRecords     = 0;
        uint32    RecordCapacity = 0;
        uint32*   Slots          = nullptr;
        uint32    SlotCapacity   = 0;
        uint8*    ArenaCursor    = nullptr;
        uint64    ArenaRemaining = 0;
    };

    static bool GrowRecords(FShard& Shard);
    static bool GrowSlots(FShard& Shard);
    static FRecord* AllocateRecord(FShard& Shard, int32 NumFrames);

    FShard Shards[NumShards];
};
