#pragma once

namespace AlgorithmSortDetail
{
    constexpr int32 InsertionThreshold = 24;
    constexpr int32 NintherThreshold   = 128;
    constexpr int32 PartialInsertLimit = 8;

    template<typename ElementType, typename PredicateType>
    FORCEINLINE void InsertionSort(ElementType* First, ElementType* Last, PredicateType&& Predicate)
    {
        if (First == Last)
        {
            return;
        }

        for (ElementType* Current = First + 1; Current != Last; ++Current)
        {
            if (Predicate(*Current, *(Current - 1)))
            {
                ElementType  Value = Move(*Current);
                ElementType* Sift  = Current;

                do
                {
                    *Sift = Move(*(Sift - 1));
                    --Sift;
                }
                while (Sift != First && Predicate(Value, *(Sift - 1)));

                *Sift = Move(Value);
            }
        }
    }

    template<typename ElementType, typename PredicateType>
    FORCEINLINE void UnguardedInsertionSort(ElementType* First, ElementType* Last, PredicateType&& Predicate)
    {
        if (First == Last)
        {
            return;
        }

        for (ElementType* Current = First + 1; Current != Last; ++Current)
        {
            if (Predicate(*Current, *(Current - 1)))
            {
                ElementType  Value = Move(*Current);
                ElementType* Sift  = Current;

                do
                {
                    *Sift = Move(*(Sift - 1));
                    --Sift;
                }
                while (Predicate(Value, *(Sift - 1)));

                *Sift = Move(Value);
            }
        }
    }

    template<typename ElementType, typename PredicateType>
    FORCEINLINE bool PartialInsertionSort(ElementType* First, ElementType* Last, PredicateType&& Predicate)
    {
        if (First == Last)
        {
            return true;
        }

        int32 Limit = 0;
        for (ElementType* Current = First + 1; Current != Last; ++Current)
        {
            if (Predicate(*Current, *(Current - 1)))
            {
                ElementType  Value = Move(*Current);
                ElementType* Sift  = Current;

                do
                {
                    *Sift = Move(*(Sift - 1));
                    --Sift;
                }
                while (Sift != First && Predicate(Value, *(Sift - 1)));

                *Sift = Move(Value);
                ++Limit;

                if (Limit > PartialInsertLimit)
                {
                    return false;
                }
            }
        }

        return true;
    }

    template<typename ElementType, typename PredicateType>
    FORCEINLINE void HeapSortRange(ElementType* First, ElementType* Last, PredicateType&& Predicate)
    {
        Algorithm::HeapSort(First, static_cast<int32>(Last - First), Forward<PredicateType>(Predicate));
    }

    template<typename ElementType, typename PredicateType>
    FORCEINLINE ElementType* PartitionLeft(ElementType* First, ElementType* Last, PredicateType&& Predicate)
    {
        ElementType* PivotPos = First;
        ElementType* Begin    = First;
        ElementType* End      = Last;

        while (Predicate(*PivotPos, *--End))
        {
        }

        if (End + 1 == Last)
        {
            while (Begin < End && !Predicate(*PivotPos, *++Begin))
            {
            }
        }
        else
        {
            while (!Predicate(*PivotPos, *++Begin))
            {
            }
        }

        while (Begin < End)
        {
            ::Swap(*Begin, *End);

            while (Predicate(*PivotPos, *--End))
            {
            }

            while (!Predicate(*PivotPos, *++Begin))
            {
            }
        }

        ::Swap(*First, *End);
        return End;
    }

    template<typename ElementType, typename PredicateType>
    FORCEINLINE bool PartitionRight(ElementType* First, ElementType* Last, PredicateType&& Predicate, ElementType*& OutPivot)
    {
        ElementType PivotValue = Move(*First);
        ElementType* Begin = First;
        ElementType* End   = Last;

        while (Predicate(*++Begin, PivotValue))
        {
        }

        if (Begin - 1 == First)
        {
            while (Begin < End && !Predicate(*--End, PivotValue))
            {
            }
        }
        else
        {
            while (!Predicate(*--End, PivotValue))
            {
            }
        }

        const bool bAlreadyPartitioned = Begin >= End;
        if (!bAlreadyPartitioned)
        {
            ::Swap(*Begin, *End);
            ++Begin;

            while (Begin < End)
            {
                while (Predicate(*Begin, PivotValue))
                {
                    ++Begin;
                }

                while (!Predicate(*--End, PivotValue))
                {
                }

                if (Begin >= End)
                {
                    break;
                }

                ::Swap(*Begin, *End);
                ++Begin;
            }
        }

        ElementType* PivotPos = Begin - 1;
        *First    = Move(*PivotPos);
        *PivotPos = Move(PivotValue);
        OutPivot  = PivotPos;

        return bAlreadyPartitioned;
    }

    template<typename ElementType, typename PredicateType>
    void Loop(ElementType* First, ElementType* Last, PredicateType&& Predicate, int32 BadAllowed, bool bLeftmost)
    {
        while (true)
        {
            const int32 Size = static_cast<int32>(Last - First);
            if (Size < InsertionThreshold)
            {
                if (bLeftmost)
                {
                    InsertionSort(First, Last, Predicate);
                }
                else
                {
                    UnguardedInsertionSort(First, Last, Predicate);
                }

                return;
            }

            const int32 Distance = Size / 2;
            if (Size > NintherThreshold)
            {
                AlgorithmPrivate::Sort3(*First, *(First + Distance / 2), *(First + Distance), Predicate);
                AlgorithmPrivate::Sort3(*(Last - 1 - Distance), *(Last - Distance), *(Last - Distance / 2), Predicate);
                AlgorithmPrivate::Sort3(*(First + Distance - Distance / 2), *(First + Distance), *(First + Distance + Distance / 2), Predicate);
                AlgorithmPrivate::Sort3(*(First + Distance), *First, *(Last - 1), Predicate);
            }
            else
            {
                AlgorithmPrivate::Sort3(*(First + Distance), *First, *(Last - 1), Predicate);
            }

            if (!bLeftmost && !Predicate(*(First - 1), *First))
            {
                First = PartitionLeft(First, Last, Predicate) + 1;
                continue;
            }

            ElementType* Pivot = nullptr;

            const bool  bAlreadyPartitioned = PartitionRight(First, Last, Predicate, Pivot);
            const int32 LeftSize            = static_cast<int32>(Pivot - First);
            const int32 RightSize           = static_cast<int32>(Last - (Pivot + 1));

            const bool bHighlyUnbalanced = LeftSize < Size / 8 || RightSize < Size / 8;
            if (bHighlyUnbalanced)
            {
                --BadAllowed;
                if (BadAllowed == 0)
                {
                    HeapSortRange(First, Last, Predicate);
                    return;
                }

                if (LeftSize > InsertionThreshold)
                {
                    AlgorithmPrivate::Sort3(*First, *(First + LeftSize / 4), *(First + LeftSize / 2), Predicate);
                    AlgorithmPrivate::Sort3(*(First + (LeftSize / 2 - LeftSize / 4)), *(First + LeftSize / 2), *(First + (LeftSize / 2 + LeftSize / 4)), Predicate);
                    AlgorithmPrivate::Sort3(*(Pivot - LeftSize / 2), *(Pivot - LeftSize / 4), *(Pivot - 1), Predicate);
                }

                if (RightSize > InsertionThreshold)
                {
                    AlgorithmPrivate::Sort3(*(Pivot + 1), *(Pivot + 1 + RightSize / 4), *(Pivot + 1 + RightSize / 2), Predicate);
                    AlgorithmPrivate::Sort3(*(Pivot + 1 + (RightSize / 2 - RightSize / 4)), *(Pivot + 1 + RightSize / 2), *(Pivot + 1 + (RightSize / 2 + RightSize / 4)), Predicate);
                    AlgorithmPrivate::Sort3(*(Last - RightSize / 2), *(Last - RightSize / 4), *(Last - 1), Predicate);
                }
            }
            else
            {
                if (bAlreadyPartitioned && PartialInsertionSort(First, Pivot, Predicate) && PartialInsertionSort(Pivot + 1, Last, Predicate))
                {
                    return;
                }
            }

            Loop(First, Pivot, Predicate, BadAllowed, bLeftmost);

            First     = Pivot + 1;
            bLeftmost = false;
        }
    }

    template<typename ElementType, typename PredicateType>
    void Merge(ElementType* First, ElementType* Mid, ElementType* Last, ElementType* Scratch, PredicateType&& Predicate)
    {
        ElementType* Left  = First;
        ElementType* Right = Mid;
        ElementType* Out   = Scratch;

        while (Left != Mid && Right != Last)
        {
            if (Predicate(*Right, *Left))
            {
                new (static_cast<void*>(Out)) ElementType(Move(*Right));
                Right->~ElementType();
                ++Right;
            }
            else
            {
                new (static_cast<void*>(Out)) ElementType(Move(*Left));
                Left->~ElementType();
                ++Left;
            }

            ++Out;
        }

        while (Left != Mid)
        {
            new (static_cast<void*>(Out)) ElementType(Move(*Left));
            Left->~ElementType();
            ++Left;
            ++Out;
        }

        while (Right != Last)
        {
            new (static_cast<void*>(Out)) ElementType(Move(*Right));
            Right->~ElementType();
            ++Right;
            ++Out;
        }

        ElementType* Source = Scratch;
        for (ElementType* Dest = First; Dest != Last; ++Dest)
        {
            new (static_cast<void*>(Dest)) ElementType(Move(*Source));
            Source->~ElementType();
            ++Source;
        }
    }
}

template<typename ElementType>
void Algorithm::Sort(ElementType* Data, int32 Count)
{
    Sort(Data, Count, AlgorithmPrivate::TLess<ElementType>());
}

template<typename ElementType, typename PredicateType>
void Algorithm::Sort(ElementType* Data, int32 Count, PredicateType&& Predicate)
{
    if (Count <= 1 || Data == nullptr)
    {
        return;
    }

    AlgorithmSortDetail::Loop(Data, Data + Count, Forward<PredicateType>(Predicate), AlgorithmPrivate::FloorLog2(Count), true);
}

template<typename RangeType>
void Algorithm::Sort(RangeType&& Range)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    Sort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)));
}

template<typename RangeType, typename PredicateType>
void Algorithm::Sort(RangeType&& Range, PredicateType&& Predicate)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    Sort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<PredicateType>(Predicate));
}

template<typename ElementType>
void Algorithm::InsertionSort(ElementType* Data, int32 Count)
{
    InsertionSort(Data, Count, AlgorithmPrivate::TLess<ElementType>());
}

template<typename ElementType, typename PredicateType>
void Algorithm::InsertionSort(ElementType* Data, int32 Count, PredicateType&& Predicate)
{
    if (Count <= 1 || Data == nullptr)
    {
        return;
    }

    AlgorithmSortDetail::InsertionSort(Data, Data + Count, Forward<PredicateType>(Predicate));
}

template<typename RangeType>
void Algorithm::InsertionSort(RangeType&& Range)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    InsertionSort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)));
}

template<typename RangeType, typename PredicateType>
void Algorithm::InsertionSort(RangeType&& Range, PredicateType&& Predicate)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    InsertionSort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<PredicateType>(Predicate));
}

template<typename ElementType>
bool Algorithm::IsSorted(const ElementType* Data, int32 Count)
{
    return IsSorted(Data, Count, AlgorithmPrivate::TLess<ElementType>());
}

template<typename ElementType, typename PredicateType>
bool Algorithm::IsSorted(const ElementType* Data, int32 Count, PredicateType&& Predicate)
{
    if (Count <= 1 || Data == nullptr)
    {
        return true;
    }

    for (int32 Index = 1; Index < Count; ++Index)
    {
        if (Predicate(Data[Index], Data[Index - 1]))
        {
            return false;
        }
    }

    return true;
}

template<typename RangeType>
bool Algorithm::IsSorted(RangeType&& Range)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    return IsSorted<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)));
}

template<typename RangeType, typename PredicateType>
bool Algorithm::IsSorted(RangeType&& Range, PredicateType&& Predicate)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    return IsSorted<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<PredicateType>(Predicate));
}

template<typename ElementType>
void Algorithm::StableSort(ElementType* Data, int32 Count)
{
    StableSort(Data, Count, AlgorithmPrivate::TLess<ElementType>());
}

template<typename ElementType, typename PredicateType>
void Algorithm::StableSort(ElementType* Data, int32 Count, PredicateType&& Predicate)
{
    if (Count <= 1 || Data == nullptr)
    {
        return;
    }

    AlgorithmPrivate::TScratchArray<ElementType> Scratch(Count);

    int32 Width = 1;
    while (Width < Count)
    {
        for (int32 Index = 0; Index < Count; Index += Width * 2)
        {
            const int32 Mid  = Index + Width < Count ? Index + Width : Count;
            const int32 Last = Index + Width * 2 < Count ? Index + Width * 2 : Count;
            if (Mid < Last)
            {
                AlgorithmSortDetail::Merge(Data + Index, Data + Mid, Data + Last, Scratch.Data, Predicate);
            }
        }

        Width *= 2;
    }
}

template<typename RangeType>
void Algorithm::StableSort(RangeType&& Range)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    StableSort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)));
}

template<typename RangeType, typename PredicateType>
void Algorithm::StableSort(RangeType&& Range, PredicateType&& Predicate)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    StableSort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<PredicateType>(Predicate));
}
