#include "MetalRHI/MetalResidencyManager.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"

static TAutoConsoleVariable<bool> CVarEnableResidencyEviction(
    "MetalRHI.EnableResidencyEviction",
    "Removes the least recently used Private heaps and standalone resources from the residency set while the resident bytes exceed the budget",
    true);

static TAutoConsoleVariable<int32> CVarResidencyBudgetMB(
    "MetalRHI.ResidencyBudgetMB",
    "Residency budget in megabytes, or the device's recommendedMaxWorkingSetSize when zero",
    0);

#if METAL_ENABLE_RESIDENCY_LOGGING
static TAutoConsoleVariable<bool> CVarLogResidencyEvents(
    "MetalRHI.LogResidencyEvents",
    "Log residency events: evictions, restores of evicted entries, and a summary when the budget forces evictions",
    false);
#endif

static bool IsEntryIdle(const FMetalResidencyEntry& Entry, const uint64 (&CompletedValues)[FMetalResidencyEntry::NumQueues])
{
    for (uint32 Index = 0; Index < FMetalResidencyEntry::NumQueues; ++Index)
    {
        if (Entry.LastUsedValues[Index] > CompletedValues[Index])
        {
            return false;
        }
    }

    return true;
}

FMetalResidencyManager::FMetalResidencyManager(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , EntriesCS()
    , Entries()
    , ResidentBytes(0)
    , EvictedBytes(0)
    , CurrentFrame(1)
{
    static_assert(FMetalResidencyEntry::NumQueues == static_cast<uint32>(EMetalQueueType::Count), "An entry needs one last used value per queue");
}

FMetalResidencyManager::~FMetalResidencyManager()
{
    TScopedLock Lock(EntriesCS);

    for (FMetalResidencyEntry* Entry : Entries)
    {
        Entry->bTracked = false;
    }

    Entries.Clear();
}

void FMetalResidencyManager::BeginTracking(FMetalResidencyEntry& Entry)
{
    CHECK(Entry.Allocation != nil);

    TScopedLock Lock(EntriesCS);
    CHECK(!Entry.bTracked);

    Entry.bTracked      = true;
    Entry.LastUsedFrame = CurrentFrame.Load();
    Entries.Add(&Entry);
    AddToResidencySet(Entry);
}

void FMetalResidencyManager::EndTracking(FMetalResidencyEntry& Entry, bool bRemoveFromSet)
{
    TScopedLock Lock(EntriesCS);

    if (!Entry.bTracked)
    {
        return;
    }

    Entries.RemoveSingleSwap(&Entry);
    Entry.bTracked = false;

    if (Entry.bResident)
    {
        if (bRemoveFromSet)
        {
            FMetalResidencySet& ResidencySet = GetDevice()->GetResidencySet();

            if (Entry.bIsHeap)
            {
                ResidencySet.RemoveHeap(Entry.Allocation);
            }
            else
            {
                ResidencySet.Remove(Entry.Allocation);
            }
        }

        Entry.bResident = false;
        ResidentBytes.Subtract(Entry.SizeInBytes);
        STAT_SUBTRACT(STAT_Metal_ResidentBytes, Entry.SizeInBytes);
    }
    else
    {
        EvictedBytes.Subtract(Entry.SizeInBytes);
        STAT_SUBTRACT(STAT_Metal_EvictedBytes, Entry.SizeInBytes);
    }
}

void FMetalResidencyManager::Pin(FMetalResidencyEntry* Entry)
{
    if (!Entry)
    {
        return;
    }

    if (Entry->BindlessPins.Increment() == 1)
    {
        STAT_ADD(STAT_Metal_PinnedBytes, Entry->SizeInBytes);
    }

    MakeResident(*Entry);
}

void FMetalResidencyManager::Unpin(FMetalResidencyEntry* Entry)
{
    if (!Entry)
    {
        return;
    }

    const int32 Remaining = Entry->BindlessPins.Decrement();
    CHECK(Remaining >= 0);

    if (Remaining == 0)
    {
        STAT_SUBTRACT(STAT_Metal_PinnedBytes, Entry->SizeInBytes);
    }
}

void FMetalResidencyManager::PrepareForExecution(const FMetalResidencyList& List, const FMetalQueue& Queue, uint64 SubmissionValue)
{
    const TArray<FMetalResidencyEntry*>& ListEntries = List.GetEntries();
    if (ListEntries.IsEmpty())
    {
        return;
    }

    const uint32 QueueIndex = static_cast<uint32>(Queue.GetType());

    TScopedLock Lock(EntriesCS);

    for (FMetalResidencyEntry* Entry : ListEntries)
    {
        if (Entry->bTracked)
        {
            Entry->LastUsedValues[QueueIndex] = SubmissionValue;
            Restore(*Entry);
        }
    }
}

void FMetalResidencyManager::EndFrame()
{
    CurrentFrame.Increment();
}

void FMetalResidencyManager::EvictIfNeeded()
{
    const uint64 Budget = GetBudget();
    STAT_SET(STAT_Metal_ResidencyBudget, Budget);

    if (!CVarEnableResidencyEviction.GetValue() || GetResidentBytes() <= Budget)
    {
        return;
    }

    GetDevice()->TrimAllocatorCaches();

    uint64 CompletedValues[FMetalResidencyEntry::NumQueues] = {};
    GetDevice()->ForEachQueue([&CompletedValues](FMetalQueue& Queue)
    {
        CompletedValues[static_cast<uint32>(Queue.GetType())] = Queue.GetCompletedValue();
    });

    TScopedLock Lock(EntriesCS);

    uint32 NumEvicted = 0;
    while (ResidentBytes.Load() > Budget)
    {
        FMetalResidencyEntry* Victim = nullptr;

        for (FMetalResidencyEntry* Entry : Entries)
        {
            const bool bCandidate = Entry->bResident && Entry->BindlessPins.Load() == 0 && IsEntryIdle(*Entry, CompletedValues);

            if (bCandidate && (!Victim || Entry->LastUsedFrame < Victim->LastUsedFrame))
            {
                Victim = Entry;
            }
        }

        if (!Victim)
        {
            break;
        }

        Evict(*Victim);
        ++NumEvicted;
    }

#if METAL_ENABLE_RESIDENCY_LOGGING
    if (NumEvicted > 0 && CVarLogResidencyEvents.GetValue())
    {
        METAL_INFO("[ResidencyManager] EvictIfNeeded: evicted %u entries (Resident=%llu Budget=%llu)", NumEvicted, static_cast<uint64>(ResidentBytes.Load()), Budget);
    }
#else
    UNREFERENCED_VARIABLE(NumEvicted);
#endif
}

uint64 FMetalResidencyManager::GetBudget() const
{
    const int32 BudgetMB = CVarResidencyBudgetMB.GetValue();

    if (BudgetMB > 0)
    {
        return static_cast<uint64>(BudgetMB) * 1024ull * 1024ull;
    }

    return GetDevice()->GetProperties().RecommendedMaxWorkingSetSize;
}

uint64 FMetalResidencyManager::GetPinnedBytes() const
{
    TScopedLock Lock(EntriesCS);

    uint64 PinnedBytes = 0;
    for (const FMetalResidencyEntry* Entry : Entries)
    {
        if (Entry->BindlessPins.Load() > 0)
        {
            PinnedBytes += Entry->SizeInBytes;
        }
    }

    return PinnedBytes;
}

void FMetalResidencyManager::MakeResident(FMetalResidencyEntry& Entry)
{
    TScopedLock Lock(EntriesCS);

    if (Entry.bTracked)
    {
        Restore(Entry);
    }
}

void FMetalResidencyManager::MakeNonResident(FMetalResidencyEntry& Entry)
{
    TScopedLock Lock(EntriesCS);
    CHECK(Entry.BindlessPins.Load() == 0);

    if (!Entry.bTracked || !Entry.bResident)
    {
        return;
    }

    Evict(Entry);
}

void FMetalResidencyManager::Restore(FMetalResidencyEntry& Entry)
{
    Entry.LastUsedFrame = CurrentFrame.Load();

    if (!Entry.bResident)
    {
        EvictedBytes.Subtract(Entry.SizeInBytes);
        STAT_SUBTRACT(STAT_Metal_EvictedBytes, Entry.SizeInBytes);
        AddToResidencySet(Entry);

    #if METAL_ENABLE_RESIDENCY_LOGGING
        if (CVarLogResidencyEvents.GetValue())
        {
            METAL_INFO("[ResidencyManager] Restore: %s Size=%llu bytes", Entry.bIsHeap ? "heap" : "resource", Entry.SizeInBytes);
        }
    #endif
    }
}

void FMetalResidencyManager::AddToResidencySet(FMetalResidencyEntry& Entry)
{
    FMetalResidencySet& ResidencySet = GetDevice()->GetResidencySet();

    if (Entry.bIsHeap)
    {
        ResidencySet.AddHeap(Entry.Allocation);
    }
    else
    {
        ResidencySet.Add(Entry.Allocation, Entry.bBindlessReachable);
    }

    Entry.bResident = true;
    ResidentBytes.Add(Entry.SizeInBytes);
    STAT_ADD(STAT_Metal_ResidentBytes, Entry.SizeInBytes);
}

void FMetalResidencyManager::Evict(FMetalResidencyEntry& Entry)
{
    FMetalResidencySet& ResidencySet = GetDevice()->GetResidencySet();

    if (Entry.bIsHeap)
    {
        ResidencySet.RemoveHeap(Entry.Allocation);
    }
    else
    {
        ResidencySet.Remove(Entry.Allocation);
    }

    Entry.bResident = false;
    ResidentBytes.Subtract(Entry.SizeInBytes);
    EvictedBytes.Add(Entry.SizeInBytes);

    STAT_SUBTRACT(STAT_Metal_ResidentBytes, Entry.SizeInBytes);
    STAT_ADD(STAT_Metal_EvictedBytes, Entry.SizeInBytes);
    STAT_ADD(STAT_Metal_Evictions, 1);

#if METAL_ENABLE_RESIDENCY_LOGGING
    if (CVarLogResidencyEvents.GetValue())
    {
        METAL_INFO("[ResidencyManager] Evict: %s Size=%llu bytes (Resident=%llu Budget=%llu)",
            Entry.bIsHeap ? "heap" : "resource", Entry.SizeInBytes, static_cast<uint64>(ResidentBytes.Load()), GetBudget());
    }
#endif
}
