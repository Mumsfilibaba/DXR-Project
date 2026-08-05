#pragma once
#include "Core/Containers/Array.h"
#include "Core/Threading/Atomic.h"

// Slots a single thread can protect at once. Two is enough for a linked queue: the node it is
// inspecting and the node it is about to claim.
constexpr int32 HAZARD_POINTERS_PER_THREAD = 2;

// Retired pointers a thread accumulates before it attempts a reclamation scan.
constexpr int32 HAZARD_RETIRE_THRESHOLD = 64;

class FHazardPointerGuard;

class FHazardPointerDomain
{
    friend class FHazardPointerGuard;

    struct FRetiredPointer
    {
        void* Pointer;
        void (*Deleter)(void*);
    };

    struct FRecord
    {
        TAtomicPointer<void*>    Hazards[HAZARD_POINTERS_PER_THREAD];
        TAtomicPointer<FRecord*> NextRecord;
        AtomicBool               bInUse;
        TArray<FRetiredPointer>  Retired;
    };

    struct FOrphanBatch
    {
        TArray<FRetiredPointer> Retired;
        FOrphanBatch*           NextBatch;
    };

    /** @brief Holds a record for the lifetime of a thread, returning it on thread exit. */
    struct FRecordOwner
    {
        FRecordOwner()
            : Record(FHazardPointerDomain::Get().AcquireRecord())
        {
        }

        ~FRecordOwner()
        {
            FHazardPointerDomain::Get().ReleaseRecord(Record);
        }

        FRecord* Record;
    };

public:
    FHazardPointerDomain(const FHazardPointerDomain&) = delete;
    FHazardPointerDomain& operator=(const FHazardPointerDomain&) = delete;

    /**
     * @return Returns the process-wide domain. Deliberately never destroyed. A thread returns 
     * its record from a thread_local destructor, and the order of that against static destruction
     * is not something the standard pins down, so a domain that went away at exit could be
     * written to after it was gone. The records that leak are one per thread that ever used a 
     * lock-free container, and Collect frees everything the containers themselves retired.
     */
    static FHazardPointerDomain& Get()
    {
        static FHazardPointerDomain* Instance = new FHazardPointerDomain();
        return *Instance;
    }

    /**
     * @brief Defers deletion of a pointer until no thread has it published as a hazard.
     * @param Pointer Pointer to reclaim.
     * @param Deleter Function that performs the actual deletion.
     */
    void Retire(void* Pointer, void (*Deleter)(void*))
    {
        FRecord* Record = GetThreadRecord();
        Record->Retired.Add(FRetiredPointer{ Pointer, Deleter });

        if (Record->Retired.Size() >= HAZARD_RETIRE_THRESHOLD)
        {
            Scan(Record);
        }
    }

    /**
     * @brief Forces a reclamation pass, freeing everything that is no longer protected
     * Containers call this when they are destroyed so nothing is left pending. Pointers retired by
     * threads that have already exited are picked up here as well.
     */
    void Collect()
    {
        Scan(GetThreadRecord());
    }

private:
    FHazardPointerDomain()
        : Records(nullptr)
        , OrphanBatches(nullptr)
    {
    }

    /**
     * @return Returns the calling thread's record, claiming one on first use and releasing it when
     * the thread exits.
     */
    static FRecord* GetThreadRecord()
    {
        static thread_local FRecordOwner Owner;
        return Owner.Record;
    }

    /** @return Returns a record for the calling thread, reusing one left by an exited thread */
    FRecord* AcquireRecord()
    {
        for (FRecord* Record = Records.Load(EMemoryOrder::Acquire); Record != nullptr; Record = Record->NextRecord.Load(EMemoryOrder::Acquire))
        {
            if (Record->bInUse.CompareExchange(true, false))
            {
                return Record;
            }
        }

        FRecord* NewRecord = new FRecord();
        NewRecord->bInUse.Store(true);

        for (;;)
        {
            FRecord* CurrentHead = Records.Load(EMemoryOrder::Acquire);
            NewRecord->NextRecord.Store(CurrentHead, EMemoryOrder::Relaxed);

            if (Records.CompareExchange(NewRecord, CurrentHead))
            {
                return NewRecord;
            }
        }
    }

    /** @brief Returns a record to the domain, keeping anything still protected alive */
    void ReleaseRecord(FRecord* Record)
    {
        for (int32 Slot = 0; Slot < HAZARD_POINTERS_PER_THREAD; ++Slot)
        {
            Record->Hazards[Slot].Store(nullptr);
        }

        Scan(Record);

        if (!Record->Retired.IsEmpty())
        {
            FOrphanBatch* Batch = new FOrphanBatch();
            Batch->Retired = Move(Record->Retired);

            for (;;)
            {
                FOrphanBatch* CurrentHead = OrphanBatches.Load(EMemoryOrder::Acquire);
                Batch->NextBatch = CurrentHead;

                if (OrphanBatches.CompareExchange(Batch, CurrentHead))
                {
                    break;
                }
            }
        }

        Record->bInUse.Store(false);
    }

    /** @brief Frees every retired pointer in the record's list that no thread has published */
    void Scan(FRecord* Record)
    {
        AdoptOrphans(Record);

        if (Record->Retired.IsEmpty())
        {
            return;
        }

        TArray<void*> Protected;
        Protected.Reserve(Record->Retired.Size());

        for (FRecord* Current = Records.Load(EMemoryOrder::Acquire); Current != nullptr; Current = Current->NextRecord.Load(EMemoryOrder::Acquire))
        {
            for (int32 Slot = 0; Slot < HAZARD_POINTERS_PER_THREAD; ++Slot)
            {
                void* Hazard = Current->Hazards[Slot].Load(EMemoryOrder::Acquire);
                if (Hazard != nullptr)
                {
                    Protected.Add(Hazard);
                }
            }
        }

        int32 NumSurvivors = 0;
        for (int32 Index = 0; Index < Record->Retired.Size(); ++Index)
        {
            const FRetiredPointer Retired = Record->Retired[Index];
            if (Protected.Contains(Retired.Pointer))
            {
                Record->Retired[NumSurvivors++] = Retired;
            }
            else
            {
                Retired.Deleter(Retired.Pointer);
            }
        }

        Record->Retired.Pop(Record->Retired.Size() - NumSurvivors);
    }

    /** @brief Takes over the pointers left behind by threads that have exited */
    void AdoptOrphans(FRecord* Record)
    {
        if (OrphanBatches.Load(EMemoryOrder::Relaxed) == nullptr)
        {
            return;
        }

        FOrphanBatch* Batch = OrphanBatches.Exchange(nullptr);
        while (Batch != nullptr)
        {
            Record->Retired.Append(Batch->Retired);

            FOrphanBatch* NextBatch = Batch->NextBatch;
            delete Batch;
            Batch = NextBatch;
        }
    }

private:
    TAtomicPointer<FRecord*>      Records;
    TAtomicPointer<FOrphanBatch*> OrphanBatches;
};

/**
 * @brief Publishes hazards for the calling thread and clears them on scope exit
 */
class FHazardPointerGuard
{
public:
    FHazardPointerGuard(const FHazardPointerGuard&) = delete;
    FHazardPointerGuard& operator=(const FHazardPointerGuard&) = delete;

    FORCEINLINE FHazardPointerGuard()
        : Record(FHazardPointerDomain::GetThreadRecord())
    {
    }

    FORCEINLINE ~FHazardPointerGuard()
    {
        ClearAll();
    }

    /**
     * @brief Publishes a pointer, making it safe to dereference until the slot is overwritten
     * @param Slot Slot to publish into
     * @param Pointer Pointer to protect
     *
     * The store is sequentially consistent because a scanning thread has to observe it before this
     * thread dereferences the pointer. A weaker store could sink past the load that validates the
     * pointer is still reachable, which would let a concurrent scan miss it.
     */
    FORCEINLINE void Protect(int32 Slot, void* Pointer)
    {
        CHECK(Slot >= 0 && Slot < HAZARD_POINTERS_PER_THREAD);
        Record->Hazards[Slot].Store(Pointer);
    }

    /** @brief Releases every pointer this thread has published */
    FORCEINLINE void ClearAll()
    {
        for (int32 Slot = 0; Slot < HAZARD_POINTERS_PER_THREAD; ++Slot)
        {
            Record->Hazards[Slot].Store(nullptr);
        }
    }

private:
    FHazardPointerDomain::FRecord* Record;
};
