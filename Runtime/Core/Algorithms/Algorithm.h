#pragma once
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Templates/ArrayContainer.h"
#include "Core/Templates/Utility.h"
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"
#include "Core/Algorithms/AlgorithmPrivate.h"

struct Algorithm : public FNonConstructible
{
    template<typename ElementType>
    static void Sort(ElementType* Data, int32 Count);

    template<typename ElementType, typename PredicateType>
    static void Sort(ElementType* Data, int32 Count, PredicateType&& Predicate);

    template<typename RangeType>
    static void Sort(RangeType&& Range);

    template<typename RangeType, typename PredicateType>
    static void Sort(RangeType&& Range, PredicateType&& Predicate);

    template<typename ElementType>
    static void StableSort(ElementType* Data, int32 Count);

    template<typename ElementType, typename PredicateType>
    static void StableSort(ElementType* Data, int32 Count, PredicateType&& Predicate);

    template<typename RangeType>
    static void StableSort(RangeType&& Range);

    template<typename RangeType, typename PredicateType>
    static void StableSort(RangeType&& Range, PredicateType&& Predicate);

    template<typename ElementType>
    static void InsertionSort(ElementType* Data, int32 Count);

    template<typename ElementType, typename PredicateType>
    static void InsertionSort(ElementType* Data, int32 Count, PredicateType&& Predicate);

    template<typename RangeType>
    static void InsertionSort(RangeType&& Range);

    template<typename RangeType, typename PredicateType>
    static void InsertionSort(RangeType&& Range, PredicateType&& Predicate);

    template<typename ElementType>
    NODISCARD static bool IsSorted(const ElementType* Data, int32 Count);

    template<typename ElementType, typename PredicateType>
    NODISCARD static bool IsSorted(const ElementType* Data, int32 Count, PredicateType&& Predicate);

    template<typename RangeType>
    NODISCARD static bool IsSorted(RangeType&& Range);

    template<typename RangeType, typename PredicateType>
    NODISCARD static bool IsSorted(RangeType&& Range, PredicateType&& Predicate);

    template<typename ElementType, typename PredicateType>
    static void SiftDown(ElementType* Data, int32 HeapSize, int32 Index, PredicateType&& Predicate);

    template<typename ElementType, typename PredicateType>
    static void SiftUp(ElementType* Data, int32 Index, PredicateType&& Predicate);

    template<typename ElementType>
    static void Heapify(ElementType* Data, int32 Count);

    template<typename ElementType, typename PredicateType>
    static void Heapify(ElementType* Data, int32 Count, PredicateType&& Predicate);

    template<typename RangeType>
    static void Heapify(RangeType&& Range);

    template<typename RangeType, typename PredicateType>
    static void Heapify(RangeType&& Range, PredicateType&& Predicate);

    template<typename ElementType>
    static void HeapSort(ElementType* Data, int32 Count);

    template<typename ElementType, typename PredicateType>
    static void HeapSort(ElementType* Data, int32 Count, PredicateType&& Predicate);

    template<typename RangeType>
    static void HeapSort(RangeType&& Range);

    template<typename RangeType, typename PredicateType>
    static void HeapSort(RangeType&& Range, PredicateType&& Predicate);

    template<typename RangeType, typename ElementType>
    static void HeapPush(RangeType& Range, ElementType&& Element);

    template<typename RangeType, typename ElementType, typename PredicateType>
    static void HeapPush(RangeType& Range, ElementType&& Element, PredicateType&& Predicate);

    template<typename RangeType>
    static void HeapPop(RangeType& Range);

    template<typename RangeType, typename PredicateType>
    static void HeapPop(RangeType& Range, PredicateType&& Predicate);

    template<typename ElementType>
    static void RadixSort(ElementType* Data, int32 Count);

    template<typename RangeType>
    static void RadixSort(RangeType&& Range);

    template<typename ElementType, typename KeyFnType>
    static void RadixSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn);

    template<typename ElementType, typename KeyFnType>
    static void RadixSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch);

    template<typename RangeType, typename KeyFnType>
    static void RadixSortBy(RangeType&& Range, KeyFnType&& KeyFn);

    template<typename RangeType, typename KeyFnType, typename ScratchRangeType>
    static void RadixSortBy(RangeType&& Range, KeyFnType&& KeyFn, ScratchRangeType&& Scratch);

    template<typename ElementType, typename KeyFnType>
    static void CountingSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn);

    template<typename ElementType, typename KeyFnType>
    static void CountingSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch);

    template<typename RangeType, typename KeyFnType>
    static void CountingSortBy(RangeType&& Range, KeyFnType&& KeyFn);

    template<typename RangeType, typename KeyFnType, typename ScratchRangeType>
    static void CountingSortBy(RangeType&& Range, KeyFnType&& KeyFn, ScratchRangeType&& Scratch);

    template<typename ElementType, typename KeyFnType>
    static void IntegerSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn);

    template<typename ElementType, typename KeyFnType>
    static void IntegerSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch);

    template<typename RangeType, typename KeyFnType>
    static void IntegerSortBy(RangeType&& Range, KeyFnType&& KeyFn);

    template<typename RangeType, typename KeyFnType, typename ScratchRangeType>
    static void IntegerSortBy(RangeType&& Range, KeyFnType&& KeyFn, ScratchRangeType&& Scratch);

    static constexpr int32 CountingSortMaxSpan = 1024;
};

#include "Core/Algorithms/Heap.h"
#include "Core/Algorithms/Sort.h"
#include "Core/Algorithms/RadixSort.h"
