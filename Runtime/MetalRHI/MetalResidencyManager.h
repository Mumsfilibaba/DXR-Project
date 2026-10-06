#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "MetalRHI/MetalDeviceChild.h"

class FMetalQueue;

struct FMetalResidencyEntry
{
    static constexpr uint32 NumQueues = 3;

    id                Allocation                = nil;
    uint64            SizeInBytes               = 0;
    uint64            LastUsedFrame             = 0;
    uint64            LastUsedValues[NumQueues] = {};
    TAtomicInt<int32> BindlessPins;
    bool              bIsHeap                   = false;
    bool              bBindlessReachable        = false;
    bool              bResident                 = false;
    bool              bTracked                  = false;
};

class FMetalResidencyList
{
public:
    void Reset()
    {
        Entries.Clear();
    }

    FORCEINLINE void Insert(FMetalResidencyEntry* Entry)
    {
        if (Entry)
        {
            Entries.AddUnique(Entry);
        }
    }

    void Append(const FMetalResidencyList& Other)
    {
        for (FMetalResidencyEntry* Entry : Other.Entries)
        {
            Entries.AddUnique(Entry);
        }
    }

    FORCEINLINE const TArray<FMetalResidencyEntry*>& GetEntries() const
    {
        return Entries;
    }

private:
    TArray<FMetalResidencyEntry*> Entries;
};

class FMetalResidencyManager : public FMetalDeviceChild, public FNonCopyAndNonMovable
{
public:
    explicit FMetalResidencyManager(FMetalDevice* InDevice);
    virtual ~FMetalResidencyManager();

    void BeginTracking(FMetalResidencyEntry& Entry);
    void EndTracking(FMetalResidencyEntry& Entry, bool bRemoveFromSet);

    void MakeResident(FMetalResidencyEntry& Entry);
    void MakeNonResident(FMetalResidencyEntry& Entry);

    void PrepareForExecution(const FMetalResidencyList& List, const FMetalQueue& Queue, uint64 SubmissionValue);

    void Pin(FMetalResidencyEntry* Entry);
    void Unpin(FMetalResidencyEntry* Entry);

    void EndFrame();
    void EvictIfNeeded();

    uint64 GetBudget() const;
    uint64 GetPinnedBytes() const;

    uint64 GetResidentBytes() const
    {
        return ResidentBytes.Load();
    }

    uint64 GetEvictedBytes() const
    {
        return EvictedBytes.Load();
    }

private:
    void Restore(FMetalResidencyEntry& Entry);
    void AddToResidencySet(FMetalResidencyEntry& Entry);
    void Evict(FMetalResidencyEntry& Entry);

    mutable FCriticalSection      EntriesCS;
    TArray<FMetalResidencyEntry*> Entries;
    TAtomicInt<uint64>            ResidentBytes;
    TAtomicInt<uint64>            EvictedBytes;
    TAtomicInt<uint64>            CurrentFrame;
};
