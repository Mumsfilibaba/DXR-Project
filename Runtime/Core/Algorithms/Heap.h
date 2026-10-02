#pragma once
#include "Core/Misc/Asserts.h"

template<typename ElementType, typename PredicateType>
void Algorithm::SiftDown(ElementType* Data, int32 HeapSize, int32 Index, PredicateType&& Predicate)
{
    while (true)
    {
        const int32 Left  = (Index * 2) + 1;
        const int32 Right = Left + 1;
        int32       Best  = Index;

        if (Left < HeapSize && Predicate(Data[Best], Data[Left]))
        {
            Best = Left;
        }

        if (Right < HeapSize && Predicate(Data[Best], Data[Right]))
        {
            Best = Right;
        }

        if (Best == Index)
        {
            break;
        }

        ::Swap(Data[Index], Data[Best]);
        Index = Best;
    }
}

template<typename ElementType, typename PredicateType>
void Algorithm::SiftUp(ElementType* Data, int32 Index, PredicateType&& Predicate)
{
    while (Index > 0)
    {
        const int32 Parent = (Index - 1) / 2;
        if (!Predicate(Data[Parent], Data[Index]))
        {
            break;
        }

        ::Swap(Data[Parent], Data[Index]);
        Index = Parent;
    }
}

template<typename ElementType>
void Algorithm::Heapify(ElementType* Data, int32 Count)
{
    Heapify(Data, Count, AlgorithmPrivate::TLess<ElementType>());
}

template<typename ElementType, typename PredicateType>
void Algorithm::Heapify(ElementType* Data, int32 Count, PredicateType&& Predicate)
{
    if (Count <= 1)
    {
        return;
    }

    for (int32 Index = (Count / 2) - 1; Index >= 0; --Index)
    {
        SiftDown(Data, Count, Index, Predicate);
        if (Index == 0)
        {
            break;
        }
    }
}

template<typename RangeType>
void Algorithm::Heapify(RangeType&& Range)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    Heapify<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)));
}

template<typename RangeType, typename PredicateType>
void Algorithm::Heapify(RangeType&& Range, PredicateType&& Predicate)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    Heapify<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<PredicateType>(Predicate));
}

template<typename ElementType>
void Algorithm::HeapSort(ElementType* Data, int32 Count)
{
    HeapSort(Data, Count, AlgorithmPrivate::TLess<ElementType>());
}

template<typename ElementType, typename PredicateType>
void Algorithm::HeapSort(ElementType* Data, int32 Count, PredicateType&& Predicate)
{
    if (Count <= 1)
    {
        return;
    }

    Heapify(Data, Count, Predicate);
    for (int32 Index = Count - 1; Index > 0; --Index)
    {
        ::Swap(Data[0], Data[Index]);
        SiftDown(Data, Index, 0, Predicate);
    }
}

template<typename RangeType>
void Algorithm::HeapSort(RangeType&& Range)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    HeapSort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)));
}

template<typename RangeType, typename PredicateType>
void Algorithm::HeapSort(RangeType&& Range, PredicateType&& Predicate)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    HeapSort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<PredicateType>(Predicate));
}

template<typename RangeType, typename ElementType>
void Algorithm::HeapPush(RangeType& Range, ElementType&& Element)
{
    HeapPush(Range, Forward<ElementType>(Element), AlgorithmPrivate::TLess<typename TRemoveReference<decltype(*ArrayContainer::Data(Range))>::Type>());
}

template<typename RangeType, typename ElementType, typename PredicateType>
void Algorithm::HeapPush(RangeType& Range, ElementType&& Element, PredicateType&& Predicate)
{
    Range.Add(Forward<ElementType>(Element));
    const int32 Count = static_cast<int32>(ArrayContainer::Size(Range));
    SiftUp(ArrayContainer::Data(Range), Count - 1, Forward<PredicateType>(Predicate));
}

template<typename RangeType>
void Algorithm::HeapPop(RangeType& Range)
{
    HeapPop(Range, AlgorithmPrivate::TLess<typename TRemoveReference<decltype(*ArrayContainer::Data(Range))>::Type>());
}

template<typename RangeType, typename PredicateType>
void Algorithm::HeapPop(RangeType& Range, PredicateType&& Predicate)
{
    const int32 Count = static_cast<int32>(ArrayContainer::Size(Range));
    CHECK(Count > 0);

    auto* Data = ArrayContainer::Data(Range);
    if (Count > 1)
    {
        ::Swap(Data[0], Data[Count - 1]);
    }

    Range.Pop();

    if (Count > 1)
    {
        SiftDown(ArrayContainer::Data(Range), Count - 1, 0, Forward<PredicateType>(Predicate));
    }
}
