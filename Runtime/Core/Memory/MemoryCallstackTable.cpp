#include "Core/Memory/MemoryCallstackTable.h"
#include "Core/Threading/ScopedLock.h"

static constexpr uint32 INITIAL_RECORD_CAPACITY = 1024;
static constexpr uint32 INITIAL_SLOT_CAPACITY   = 2048;
static constexpr uint64 ARENA_CHUNK_SIZE        = 64 * 1024;

struct FMemoryCallstackTable::FRecord
{
    uint64 Hash;
    int32  NumFrames;
    uint64 Frames[1];
};

static uint64 HashFrames(const uint64* Frames, int32 NumFrames)
{
    uint64 Hash = 0xCBF29CE484222325ull;
    for (int32 Index = 0; Index < NumFrames; ++Index)
    {
        Hash = (Hash ^ Frames[Index]) * 0x100000001B3ull;
    }

    Hash ^= Hash >> 29;
    Hash *= 0xBF58476D1CE4E5B9ull;
    Hash ^= Hash >> 32;
    return Hash;
}

static FORCEINLINE FMemoryCallstackId MakeCallstackId(uint32 ShardIndex, uint32 RecordIndex)
{
    return ((RecordIndex + 1) * FMemoryCallstackTable::NumShards) + ShardIndex;
}

bool FMemoryCallstackTable::GrowRecords(FShard& Shard)
{
    const uint32 NewCapacity = Shard.RecordCapacity == 0 ? INITIAL_RECORD_CAPACITY : Shard.RecordCapacity * 2;

    FRecord** NewRecords = static_cast<FRecord**>(MemoryProfilerPages::Allocate(sizeof(FRecord*) * NewCapacity));
    if (!NewRecords)
    {
        return false;
    }

    if (Shard.Records)
    {
        for (uint32 RecordIndex = 0; RecordIndex < Shard.NumRecords; ++RecordIndex)
        {
            NewRecords[RecordIndex] = Shard.Records[RecordIndex];
        }

        MemoryProfilerPages::Free(Shard.Records, sizeof(FRecord*) * Shard.RecordCapacity);
    }

    Shard.Records        = NewRecords;
    Shard.RecordCapacity = NewCapacity;
    return true;
}

bool FMemoryCallstackTable::GrowSlots(FShard& Shard)
{
    const uint32 NewCapacity = Shard.SlotCapacity == 0 ? INITIAL_SLOT_CAPACITY : Shard.SlotCapacity * 2;

    uint32* NewSlots = static_cast<uint32*>(MemoryProfilerPages::Allocate(sizeof(uint32) * NewCapacity));
    if (!NewSlots)
    {
        return false;
    }

    for (uint32 RecordIndex = 0; RecordIndex < Shard.NumRecords; ++RecordIndex)
    {
        uint32 Slot = static_cast<uint32>(Shard.Records[RecordIndex]->Hash) & (NewCapacity - 1);
        while (NewSlots[Slot] != 0)
        {
            Slot = (Slot + 1) & (NewCapacity - 1);
        }

        NewSlots[Slot] = RecordIndex + 1;
    }

    if (Shard.Slots)
    {
        MemoryProfilerPages::Free(Shard.Slots, sizeof(uint32) * Shard.SlotCapacity);
    }

    Shard.Slots        = NewSlots;
    Shard.SlotCapacity = NewCapacity;
    return true;
}

FMemoryCallstackTable::FRecord* FMemoryCallstackTable::AllocateRecord(FShard& Shard, int32 NumFrames)
{
    const uint64 RecordSize = OFFSETOF(FRecord, Frames) + sizeof(uint64) * static_cast<uint64>(NumFrames);
    if (Shard.ArenaRemaining < RecordSize)
    {
        uint8* Chunk = static_cast<uint8*>(MemoryProfilerPages::Allocate(ARENA_CHUNK_SIZE));
        if (!Chunk)
        {
            return nullptr;
        }

        Shard.ArenaCursor    = Chunk;
        Shard.ArenaRemaining = ARENA_CHUNK_SIZE;
    }

    FRecord* Record = reinterpret_cast<FRecord*>(Shard.ArenaCursor);
    Shard.ArenaCursor    += RecordSize;
    Shard.ArenaRemaining -= RecordSize;
    return Record;
}

FMemoryCallstackId FMemoryCallstackTable::Intern(const uint64* Frames, int32 NumFrames)
{
    NumFrames = NumFrames < MaxFrames ? NumFrames : MaxFrames;
    if (!Frames || NumFrames <= 0)
    {
        return MEMORY_CALLSTACK_NONE;
    }

    const uint64 Hash       = HashFrames(Frames, NumFrames);
    const uint32 ShardIndex = static_cast<uint32>(Hash >> 60);
    FShard&      Shard      = Shards[ShardIndex];

    SCOPED_LOCK(Shard.Lock);

    if ((Shard.NumRecords + 1) * 2 > Shard.SlotCapacity && !GrowSlots(Shard))
    {
        return MEMORY_CALLSTACK_NONE;
    }

    uint32 Slot = static_cast<uint32>(Hash) & (Shard.SlotCapacity - 1);
    for (; Shard.Slots[Slot] != 0; Slot = (Slot + 1) & (Shard.SlotCapacity - 1))
    {
        const uint32   RecordIndex = Shard.Slots[Slot] - 1;
        const FRecord* Record      = Shard.Records[RecordIndex];
        if (Record->Hash != Hash || Record->NumFrames != NumFrames)
        {
            continue;
        }

        bool bSameFrames = true;
        for (int32 Index = 0; Index < NumFrames && bSameFrames; ++Index)
        {
            bSameFrames = Record->Frames[Index] == Frames[Index];
        }

        if (bSameFrames)
        {
            return MakeCallstackId(ShardIndex, RecordIndex);
        }
    }

    if (Shard.NumRecords == Shard.RecordCapacity && !GrowRecords(Shard))
    {
        return MEMORY_CALLSTACK_NONE;
    }

    FRecord* Record = AllocateRecord(Shard, NumFrames);
    if (!Record)
    {
        return MEMORY_CALLSTACK_NONE;
    }

    Record->Hash      = Hash;
    Record->NumFrames = NumFrames;
    for (int32 Index = 0; Index < NumFrames; ++Index)
    {
        Record->Frames[Index] = Frames[Index];
    }

    const uint32 RecordIndex = Shard.NumRecords++;
    Shard.Records[RecordIndex] = Record;
    Shard.Slots[Slot]          = RecordIndex + 1;
    return MakeCallstackId(ShardIndex, RecordIndex);
}

int32 FMemoryCallstackTable::GetFrames(FMemoryCallstackId Id, uint64* OutFrames)
{
    if (Id == MEMORY_CALLSTACK_NONE || !OutFrames)
    {
        return 0;
    }

    const uint32 ShardIndex  = Id % NumShards;
    const uint32 RecordIndex = (Id / NumShards) - 1;
    FShard&      Shard       = Shards[ShardIndex];

    SCOPED_LOCK(Shard.Lock);

    if (RecordIndex >= Shard.NumRecords)
    {
        return 0;
    }

    const FRecord* Record = Shard.Records[RecordIndex];
    for (int32 Index = 0; Index < Record->NumFrames; ++Index)
    {
        OutFrames[Index] = Record->Frames[Index];
    }

    return Record->NumFrames;
}

int32 FMemoryCallstackTable::GetNumCallstacks()
{
    int32 NumCallstacks = 0;
    for (FShard& Shard : Shards)
    {
        SCOPED_LOCK(Shard.Lock);
        NumCallstacks += static_cast<int32>(Shard.NumRecords);
    }

    return NumCallstacks;
}
