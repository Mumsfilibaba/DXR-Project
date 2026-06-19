#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "Core/Threading/Atomic/AtomicPointer.h"
#include "Core/Threading/Atomic/AtomicMemoryOrder.h"
#include "Core/Templates/Utility/NonCopyable.h"

template<typename ElementType>
class TWorkStealingDeque : private FNonCopyable
{
    struct FRingBuffer
    {
        explicit FRingBuffer(int64 InCapacity)
            : Capacity(InCapacity)
            , Mask(InCapacity - 1)
            , Items(new ElementType[InCapacity])
        {
        }

        ~FRingBuffer()
        {
            delete[] Items;
        }

        FORCEINLINE ElementType Get(int64 Index) const
        {
            return Items[Index & Mask];
        }

        FORCEINLINE void Put(int64 Index, ElementType Item)
        {
            Items[Index & Mask] = Item;
        }

        FRingBuffer* Grow(int64 BottomIndex, int64 TopIndex) const
        {
            FRingBuffer* NewBuffer = new FRingBuffer(Capacity * 2);
            for (int64 Index = TopIndex; Index < BottomIndex; ++Index)
            {
                NewBuffer->Put(Index, Get(Index));
            }

            return NewBuffer;
        }

        int64        Capacity;
        int64        Mask;
        ElementType* Items;
    };

public:
    explicit TWorkStealingDeque(int64 InitialCapacity = 1024)
        : Top(0)
        , Bottom(0)
        , Buffer(new FRingBuffer(InitialCapacity))
        , RetiredBuffers()
    {
    }

    ~TWorkStealingDeque()
    {
        if (FRingBuffer* CurrentBuffer = Buffer.Load(EMemoryOrder::Relaxed))
        {
            delete CurrentBuffer;
        }

        for (FRingBuffer* Retired : RetiredBuffers)
        {
            delete Retired;
        }

        RetiredBuffers.Clear();
    }

    /**
     * @brief Owner-only. Pushes an item onto the bottom of the deque. Never blocks; may grow.
     */
    void PushBottom(ElementType Item)
    {
        const int64  BottomIndex = Bottom.Load(EMemoryOrder::Relaxed);
        const int64  TopIndex    = Top.Load(EMemoryOrder::Acquire);
        FRingBuffer* CurrentBuffer = Buffer.Load(EMemoryOrder::Relaxed);

        if ((BottomIndex - TopIndex) >= (CurrentBuffer->Capacity - 1))
        {
            // Full: grow into a larger buffer. The old buffer is retained (not freed) so any thief
            // still reading it stays valid. It is reclaimed in the destructor.

            FRingBuffer* NewBuffer = CurrentBuffer->Grow(BottomIndex, TopIndex);
            RetiredBuffers.Add(CurrentBuffer);
            Buffer.Store(NewBuffer, EMemoryOrder::Release);
            CurrentBuffer = NewBuffer;
        }

        CurrentBuffer->Put(BottomIndex, Item);

        // Publish the item before advancing Bottom so thieves never observe a stale cell.
        Bottom.Store(BottomIndex + 1, EMemoryOrder::Release);
    }

    /**
     * @brief Owner-only. Pops an item from the bottom (LIFO).
     * @return Returns false if the deque was empty or a thief won the race for the last item.
     */
    bool PopBottom(ElementType& OutItem)
    {
        const int64  BottomIndex = Bottom.Load(EMemoryOrder::Relaxed) - 1;
        FRingBuffer* CurrentBuffer = Buffer.Load(EMemoryOrder::Relaxed);

        // Full fence: makes the Bottom decrement visible before reading Top, closing the race
        // against a concurrent Steal.
        Bottom.Store(BottomIndex, EMemoryOrder::SequentiallyConsistent);
        const int64 TopIndex = Top.Load(EMemoryOrder::SequentiallyConsistent);

        if (TopIndex > BottomIndex)
        {
            // Empty - restore Bottom.
            Bottom.Store(BottomIndex + 1, EMemoryOrder::Relaxed);
            return false;
        }

        ElementType Item = CurrentBuffer->Get(BottomIndex);
        if (TopIndex != BottomIndex)
        {
            // More than one element remaining; no race possible.
            OutItem = Item;
            return true;
        }

        // Exactly one element: race with thieves for it.
        const bool bResult = Top.CompareExchange(TopIndex + 1, TopIndex, EMemoryOrder::SequentiallyConsistent);
        Bottom.Store(BottomIndex + 1, EMemoryOrder::Relaxed);

        if (!bResult)
        {
            return false;
        }

        OutItem = Item;
        return true;
    }

    /**
     * @brief Thief side. Steals an item from the top (FIFO).
     * @return Returns false on empty or if another thread won the CAS; the caller should retry on a different victim.
     */
    bool Steal(ElementType& OutItem)
    {
        const int64 TopIndex    = Top.Load(EMemoryOrder::Acquire);
        const int64 BottomIndex = Bottom.Load(EMemoryOrder::Acquire);

        if (TopIndex >= BottomIndex)
        {
            return false;
        }

        FRingBuffer* CurrentBuffer = Buffer.Load(EMemoryOrder::Acquire);
        ElementType Item = CurrentBuffer->Get(TopIndex);

        if (!Top.CompareExchange(TopIndex + 1, TopIndex, EMemoryOrder::SequentiallyConsistent))
        {
            return false;
        }

        OutItem = Item;
        return true;
    }

    /** @return Cheap, unsynchronized snapshot of the number of queued items. */
    FORCEINLINE int64 ApproxSize() const
    {
        return Bottom.Load(EMemoryOrder::Relaxed) - Top.Load(EMemoryOrder::Relaxed);
    }

private:
    AtomicInt64                  Top;
    AtomicInt64                  Bottom;
    TAtomicPointer<FRingBuffer*> Buffer;
    TArray<FRingBuffer*>         RetiredBuffers;
};
