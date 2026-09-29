#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Math.h"
#include "Core/Templates/Utility.h"

template<typename ElementType>
class TRingBuffer
{
public:
    explicit TRingBuffer(int32 InCapacity = 0)
        : Elements()
        , Head(0)
        , Count(0)
        , Capacity(Math::Max(InCapacity, 0))
    {
    }

    /**
     * @brief Appends an element, dropping the oldest once the buffer is full.
     *
     * @return The appended element.
     */
    template<typename... ArgTypes>
    ElementType& Emplace(ArgTypes&&... Args)
    {
        CHECK(Capacity > 0);

        if (Count < Capacity)
        {
            // Until the buffer wraps the storage grows in order, so the next slot is always the end
            Elements.Emplace(Forward<ArgTypes>(Args)...);
            ++Count;
            return Elements.Last();
        }

        ElementType& Slot = Elements[Head];
        Slot = ElementType(Forward<ArgTypes>(Args)...);
        Head = (Head + 1) % Capacity;
        return Slot;
    }

    /** @brief Drops every element, keeping the storage for reuse. */
    void Clear()
    {
        Elements.Clear();
        Head  = 0;
        Count = 0;
    }

    /**
     * @brief Changes how many elements the buffer keeps, dropping the oldest when it shrinks.
     *
     * @param InCapacity The new capacity, clamped to at least zero.
     */
    void SetCapacity(int32 InCapacity)
    {
        const int32 NewCapacity = Math::Max(InCapacity, 0);
        const int32 Kept        = Math::Min(Count, NewCapacity);

        TArray<ElementType> Linear;
        Linear.Reserve(Kept);

        for (int32 Index = Count - Kept; Index < Count; ++Index)
        {
            Linear.Emplace(Move((*this)[Index]));
        }

        Elements = Move(Linear);
        Head     = 0;
        Count    = Kept;
        Capacity = NewCapacity;
    }

    NODISCARD FORCEINLINE ElementType& operator[](int32 Index)
    {
        CHECK(Index >= 0 && Index < Count);
        return Elements[(Head + Index) % Capacity];
    }

    NODISCARD FORCEINLINE const ElementType& operator[](int32 Index) const
    {
        CHECK(Index >= 0 && Index < Count);
        return Elements[(Head + Index) % Capacity];
    }

    NODISCARD FORCEINLINE int32 Size() const
    {
        return Count;
    }

    NODISCARD FORCEINLINE int32 GetCapacity() const
    {
        return Capacity;
    }

    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Count == 0;
    }

    NODISCARD FORCEINLINE bool IsFull() const
    {
        return Count == Capacity;
    }

private:
    TArray<ElementType> Elements;
    int32               Head;
    int32               Count;
    int32               Capacity;
};
