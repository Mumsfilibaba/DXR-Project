#pragma once
#include "Array.h"

#include "Core/Threading/AtomicInt.h"
#include "Core/Templates/UnderlyingType.h"
#include "Core/Templates/IsFundamental.h"
#include "Core/Templates/AddCV.h"
#include "Core/Templates/AddReference.h"
#include "Core/Templates/NumericLimits.h"

enum class EQueuePriority
{
    Highest = 0,
    High    = 1,
    Normal  = 2,
    Low     = 3,
    Lowest  = 4,
    Count
};

template<typename T>
class TPriorityQueue
{
public:

    using ElementType      = T;
    using ElementInputType = typename TConditional<
        TOr<TIsPointer<ElementType>, TIsFundamental<T>>::Value,
        ElementType,
        typename TAddReference<typename TAddConst<ElementType>::Type>::LValue>::Type;

    /**
     * @brief - Constructor
     */
    FORCEINLINE TPriorityQueue()
        : PriorityQueues(0)
        , NumElements(0)
        , LowestPopulatedPriorityIndex(0)
    { }

    /**
     * @brief          - Enqueue a new element in the queue
     * @param Element  - New element in the queue
     * @param Priority - Priority of the new element
     */
    inline void Enqueue(ElementInputType Element, EQueuePriority Priority = EQueuePriority::Normal)
    {
        const int32 QueueIndex = ToUnderlying(Priority);
        if (!PriorityQueues.IsValidIndex(QueueIndex))
        {
            PriorityQueues.Resize(QueueIndex + 1);
        }

        NumElements++;

        if (QueueIndex < LowestPopulatedPriorityIndex)
        {
            LowestPopulatedPriorityIndex = QueueIndex;
        }

        PriorityQueues[QueueIndex].Emplace(Element);
    }

    /**
     * @brief         - Removes an element from the queue
     * @param Element - Element to search for
     * @return        - Returns true if the element was found and removed, false otherwise
     */
    inline bool Remove(ElementInputType Element)
    {
        for (int32 Index = LowestPopulatedPriorityIndex; Index < PriorityQueues.GetSize(); ++Index)
        {
            if (PriorityQueues[Index].Remove(Element))
            {
                NumElements--;
                return true;
            }
        }

        return false;
    }

    /**
     * @brief             - Removes the first element off the queue and returns it
     * @param OutElement  - Pointer the storage of the element
     * @param OutPriority - Optional pointer to a variable that will store the priority of the element
     * @return            - Returns true if an element was popped, false otherwise
     */
    inline bool Dequeue(ElementType* OutElement, EQueuePriority* OutPriority = nullptr)
    {
        for (int32 Index = LowestPopulatedPriorityIndex; Index < PriorityQueues.GetSize(); ++Index)
        {
            auto& CurrentQueue = PriorityQueues[Index];
            if (!CurrentQueue.IsEmpty())
            {
                CHECK(OutElement != nullptr);
                *OutElement = ::Move(CurrentQueue.FirstElement());

                CurrentQueue.RemoveAt(0);
                NumElements--;

                // Start searching in this queue next time
                LowestPopulatedPriorityIndex = Index;

                if (OutPriority)
                {
                    *OutPriority = EQueuePriority(Index);
                }

                return true;
            }
        }

        return false;
    }

    /**
     * @brief             - Peeks the first element in the queue, without removing
     * @param OutPriority - Optional pointer to a variable that will store the priority of the element
     * @return            - Returns a pointer to the first element if the queue is not empty, false otherwise
     */
    inline ElementType* Peek(EQueuePriority* OutPriority = nullptr)
    {
        for (int32 Index = LowestPopulatedPriorityIndex; Index < PriorityQueues.GetSize(); ++Index)
        {
            auto& CurrentQueue = PriorityQueues[Index];
            if (!CurrentQueue.IsEmpty())
            {
                if (OutPriority)
                {
                    *OutPriority = EQueuePriority(Index);
                }

                return &CurrentQueue[0];
            }
        }

        return nullptr;
    }

    /**
     * @brief - Resets the queue
     */
    inline void Reset()
    {
        PriorityQueues.Clear();
        LowestPopulatedPriorityIndex = 0;
        NumElements = 0;
    }

    /**
     * @return - Returns the size of the queue
     */
    FORCEINLINE int32 GetSize() const
    {
        return NumElements.Load();
    }

private:
    static constexpr auto NumPriorities = static_cast<int32>(ToUnderlying(EQueuePriority::Count));

    using ElementArray   = TArray<ElementType>;
    using ArrayAllocator = TInlineArrayAllocator<ElementArray, NumPriorities + 1>;

    TArray<ElementArray, ArrayAllocator> PriorityQueues;
    
    FAtomicInt32 NumElements;
    int32        LowestPopulatedPriorityIndex;
};