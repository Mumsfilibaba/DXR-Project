#include "Core/Memory/MemoryAllocationMap.h"
#include "Core/Threading/ScopedLock.h"

static constexpr uint32 INITIAL_SHARD_CAPACITY = 1024;

static void* const TOMBSTONE = reinterpret_cast<void*>(UPTR_INT(1));

static FORCEINLINE uint64 HashBlock(const void* Block)
{
    return (static_cast<uint64>(reinterpret_cast<UPTR_INT>(Block)) >> 4) * 0x9E3779B97F4A7C15ull;
}

static FORCEINLINE uint32 GetShardIndex(uint64 Hash)
{
    return static_cast<uint32>(Hash >> 58);
}

bool FMemoryAllocationMap::Grow(FShard& Shard)
{
    const uint32 NewCapacity = (Shard.Capacity == 0)
        ? INITIAL_SHARD_CAPACITY
        : ((Shard.NumEntries + 1) * 2 > Shard.Capacity / 2 ? Shard.Capacity * 2 : Shard.Capacity);

    FMemoryAllocationEntry* NewEntries = static_cast<FMemoryAllocationEntry*>(MemoryProfilerPages::Allocate(sizeof(FMemoryAllocationEntry) * NewCapacity));
    if (!NewEntries)
    {
        return false;
    }

    for (uint32 Index = 0; Index < Shard.Capacity; ++Index)
    {
        const FMemoryAllocationEntry& Entry = Shard.Entries[Index];
        if (Entry.Block == nullptr || Entry.Block == TOMBSTONE)
        {
            continue;
        }

        uint32 Slot = static_cast<uint32>(HashBlock(Entry.Block)) & (NewCapacity - 1);
        while (NewEntries[Slot].Block != nullptr)
        {
            Slot = (Slot + 1) & (NewCapacity - 1);
        }

        NewEntries[Slot] = Entry;
    }

    if (Shard.Entries)
    {
        MemoryProfilerPages::Free(Shard.Entries, sizeof(FMemoryAllocationEntry) * Shard.Capacity);
    }

    Shard.Entries       = NewEntries;
    Shard.Capacity      = NewCapacity;
    Shard.NumTombstones = 0;
    return true;
}

EMemoryMapInsertResult FMemoryAllocationMap::Insert(const FMemoryAllocationEntry& Entry, FMemoryAllocationEntry& OutReplaced)
{
    const uint64 Hash  = HashBlock(Entry.Block);
    FShard&      Shard = Shards[GetShardIndex(Hash)];

    SCOPED_LOCK(Shard.Lock);

    if ((Shard.NumEntries + Shard.NumTombstones + 1) * 2 > Shard.Capacity && !Grow(Shard))
    {
        return EMemoryMapInsertResult::Failed;
    }

    uint32 FirstTombstone = Shard.Capacity;
    for (uint32 Slot = static_cast<uint32>(Hash) & (Shard.Capacity - 1);; Slot = (Slot + 1) & (Shard.Capacity - 1))
    {
        FMemoryAllocationEntry& Current = Shard.Entries[Slot];
        if (Current.Block == Entry.Block)
        {
            OutReplaced = Current;
            Current     = Entry;
            return EMemoryMapInsertResult::Replaced;
        }

        if (Current.Block == TOMBSTONE)
        {
            FirstTombstone = (FirstTombstone == Shard.Capacity) ? Slot : FirstTombstone;
            continue;
        }

        if (Current.Block == nullptr)
        {
            if (FirstTombstone != Shard.Capacity)
            {
                Shard.Entries[FirstTombstone] = Entry;
                Shard.NumTombstones--;
            }
            else
            {
                Current = Entry;
            }

            Shard.NumEntries++;
            return EMemoryMapInsertResult::Inserted;
        }
    }
}

bool FMemoryAllocationMap::Remove(const void* Block, FMemoryAllocationEntry& OutEntry)
{
    const uint64 Hash  = HashBlock(Block);
    FShard&      Shard = Shards[GetShardIndex(Hash)];

    SCOPED_LOCK(Shard.Lock);

    if (Shard.Capacity == 0)
    {
        return false;
    }

    for (uint32 Slot = static_cast<uint32>(Hash) & (Shard.Capacity - 1);; Slot = (Slot + 1) & (Shard.Capacity - 1))
    {
        FMemoryAllocationEntry& Current = Shard.Entries[Slot];
        if (Current.Block == nullptr)
        {
            return false;
        }

        if (Current.Block == Block)
        {
            OutEntry      = Current;
            Current       = FMemoryAllocationEntry();
            Current.Block = TOMBSTONE;

            Shard.NumEntries--;
            Shard.NumTombstones++;
            return true;
        }
    }
}

void FMemoryAllocationMap::Clear()
{
    for (FShard& Shard : Shards)
    {
        SCOPED_LOCK(Shard.Lock);

        if (Shard.Entries)
        {
            MemoryProfilerPages::Free(Shard.Entries, sizeof(FMemoryAllocationEntry) * Shard.Capacity);
        }

        Shard.Entries       = nullptr;
        Shard.Capacity      = 0;
        Shard.NumEntries    = 0;
        Shard.NumTombstones = 0;
    }
}

int64 FMemoryAllocationMap::CountEntries(uint64 MinSize, uint32 MinMarker)
{
    int64 NumEntries = 0;
    for (FShard& Shard : Shards)
    {
        SCOPED_LOCK(Shard.Lock);

        for (uint32 Index = 0; Index < Shard.Capacity; ++Index)
        {
            const FMemoryAllocationEntry& Entry = Shard.Entries[Index];
            if (Entry.Block != nullptr && Entry.Block != TOMBSTONE && Entry.Size >= MinSize && Entry.Marker >= MinMarker)
            {
                NumEntries++;
            }
        }
    }

    return NumEntries;
}

int64 FMemoryAllocationMap::CopyEntries(uint64 MinSize, uint32 MinMarker, FMemoryAllocationEntry* OutEntries, int64 MaxEntries)
{
    int64 NumCopied = 0;
    for (FShard& Shard : Shards)
    {
        SCOPED_LOCK(Shard.Lock);

        for (uint32 Index = 0; Index < Shard.Capacity && NumCopied < MaxEntries; ++Index)
        {
            const FMemoryAllocationEntry& Entry = Shard.Entries[Index];
            if (Entry.Block != nullptr && Entry.Block != TOMBSTONE && Entry.Size >= MinSize && Entry.Marker >= MinMarker)
            {
                OutEntries[NumCopied++] = Entry;
            }
        }
    }

    return NumCopied;
}

int64 FMemoryAllocationMap::GetNumEntries()
{
    int64 NumEntries = 0;
    for (FShard& Shard : Shards)
    {
        SCOPED_LOCK(Shard.Lock);
        NumEntries += Shard.NumEntries;
    }

    return NumEntries;
}
