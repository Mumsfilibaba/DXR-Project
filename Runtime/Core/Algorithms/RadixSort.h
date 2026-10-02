#pragma once
#include "Core/Misc/Asserts.h"

namespace AlgorithmIntegerSortDetail
{
    template<typename ElementType, typename KeyFnType>
    FORCEINLINE void FindKeyBounds(ElementType* Data, int32 Count, KeyFnType&& KeyFn, auto& OutMin, auto& OutMax)
    {
        OutMin = KeyFn(Data[0]);
        OutMax = OutMin;

        for (int32 Index = 1; Index < Count; ++Index)
        {
            const auto Key = KeyFn(Data[Index]);
            if (Key < OutMin)
            {
                OutMin = Key;
            }
            if (Key > OutMax)
            {
                OutMax = Key;
            }
        }
    }

    template<typename ScratchRangeType, typename ElementType>
    ElementType* ResolveScratch(ScratchRangeType&& Scratch, int32 Count, AlgorithmPrivate::TScratchArray<ElementType>& Fallback)
    {
        if constexpr (requires(ScratchRangeType& Range, int32 Size) { Range.ResizeUninitialized(Size); })
        {
            if (static_cast<int32>(ArrayContainer::Size(Scratch)) < Count)
            {
                Scratch.ResizeUninitialized(Count);
            }

            return ArrayContainer::Data(Scratch);
        }
        else if constexpr (requires { ArrayContainer::Data(Scratch); ArrayContainer::Size(Scratch); })
        {
            if (static_cast<int32>(ArrayContainer::Size(Scratch)) >= Count)
            {
                return ArrayContainer::Data(Scratch);
            }
        }

        Fallback.Reset(Count);
        return Fallback.Data;
    }

    template<typename ElementType, typename KeyFnType>
    void CountingSort(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch)
    {
        using KeyType = decltype(KeyFn(Data[0]));

        KeyType MinKey = KeyType();
        KeyType MaxKey = KeyType();
        FindKeyBounds(Data, Count, KeyFn, MinKey, MaxKey);

        const int64 Span64 = static_cast<int64>(MaxKey) - static_cast<int64>(MinKey) + 1;
        CHECK(Span64 > 0);

        const int32 Span = static_cast<int32>(Span64);
        AlgorithmPrivate::TScratchArray<uint32> CountsStorage;
        uint32 StackCounts[Algorithm::CountingSortMaxSpan];

        uint32* Counts = StackCounts;
        if (Span > Algorithm::CountingSortMaxSpan)
        {
            CountsStorage.Reset(Span);
            Counts = CountsStorage.Data;
        }

        Memory::Memzero(Counts, static_cast<uint64>(Span) * sizeof(uint32));

        for (int32 Index = 0; Index < Count; ++Index)
        {
            ++Counts[static_cast<int32>(KeyFn(Data[Index]) - MinKey)];
        }

        uint32 Offset = 0;
        for (int32 Bucket = 0; Bucket < Span; ++Bucket)
        {
            const uint32 BucketCount = Counts[Bucket];
            Counts[Bucket] = Offset;
            Offset += BucketCount;
        }

        AlgorithmPrivate::TScratchArray<ElementType> LocalScratch;
        if (Scratch == nullptr)
        {
            LocalScratch.Reset(Count);
            Scratch = LocalScratch.Data;
        }

        for (int32 Index = 0; Index < Count; ++Index)
        {
            const int32 Bucket = static_cast<int32>(KeyFn(Data[Index]) - MinKey);
            new (static_cast<void*>(Scratch + Counts[Bucket])) ElementType(Move(Data[Index]));
            Data[Index].~ElementType();
            ++Counts[Bucket];
        }

        for (int32 Index = 0; Index < Count; ++Index)
        {
            new (static_cast<void*>(Data + Index)) ElementType(Move(Scratch[Index]));
            Scratch[Index].~ElementType();
        }
    }

    template<typename ElementType, typename KeyFnType>
    void RadixSort(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch)
    {
        using KeyType     = decltype(KeyFn(Data[0]));
        using UnsignedKey = decltype(AlgorithmPrivate::ToUnsignedSortKey(KeyFn(Data[0])));

        AlgorithmPrivate::TScratchArray<ElementType> LocalScratch;
        if (Scratch == nullptr)
        {
            LocalScratch.Reset(Count);
            Scratch = LocalScratch.Data;
        }

        ElementType* Source = Data;
        ElementType* Dest   = Scratch;

        const int32 ByteCount = static_cast<int32>(sizeof(UnsignedKey));
        for (int32 ByteIndex = 0; ByteIndex < ByteCount; ++ByteIndex)
        {
            uint32 Counts[256] = {};
            const uint32 Shift = static_cast<uint32>(ByteIndex) * 8u;

            for (int32 Index = 0; Index < Count; ++Index)
            {
                const UnsignedKey Key = AlgorithmPrivate::ToUnsignedSortKey(KeyFn(Source[Index]));
                ++Counts[(Key >> Shift) & 0xffu];
            }

            uint32 Offset = 0;
            for (uint32 Bucket = 0; Bucket < 256; ++Bucket)
            {
                const uint32 BucketCount = Counts[Bucket];
                Counts[Bucket] = Offset;
                Offset += BucketCount;
            }

            for (int32 Index = 0; Index < Count; ++Index)
            {
                const UnsignedKey Key = AlgorithmPrivate::ToUnsignedSortKey(KeyFn(Source[Index]));
                const uint32 DestIndex = Counts[(Key >> Shift) & 0xffu]++;
                new (static_cast<void*>(Dest + DestIndex)) ElementType(Move(Source[Index]));
                Source[Index].~ElementType();
            }

            ElementType* Temp = Source;
            Source = Dest;
            Dest   = Temp;
        }

        if (Source != Data)
        {
            for (int32 Index = 0; Index < Count; ++Index)
            {
                new (static_cast<void*>(Data + Index)) ElementType(Move(Source[Index]));
                Source[Index].~ElementType();
            }
        }
    }
}

template<typename ElementType, typename KeyFnType>
void Algorithm::CountingSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn)
{
    CountingSortBy(Data, Count, Forward<KeyFnType>(KeyFn), static_cast<ElementType*>(nullptr));
}

template<typename ElementType, typename KeyFnType>
void Algorithm::CountingSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch)
{
    if (Count <= 1 || Data == nullptr)
    {
        return;
    }

    AlgorithmIntegerSortDetail::CountingSort(Data, Count, Forward<KeyFnType>(KeyFn), Scratch);
}

template<typename RangeType, typename KeyFnType>
void Algorithm::CountingSortBy(RangeType&& Range, KeyFnType&& KeyFn)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    CountingSortBy<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<KeyFnType>(KeyFn));
}

template<typename RangeType, typename KeyFnType, typename ScratchRangeType>
void Algorithm::CountingSortBy(RangeType&& Range, KeyFnType&& KeyFn, ScratchRangeType&& Scratch)
{
    const int32 Count = static_cast<int32>(ArrayContainer::Size(Range));

    auto* Data = ArrayContainer::Data(Range);
    AlgorithmPrivate::TScratchArray<typename TRemoveReference<decltype(*Data)>::Type> Fallback;

    auto* ScratchData = AlgorithmIntegerSortDetail::ResolveScratch(Forward<ScratchRangeType>(Scratch), Count, Fallback);
    CountingSortBy<typename TRemoveReference<decltype(*Data)>::Type>(Data, Count, Forward<KeyFnType>(KeyFn), ScratchData);
}

template<typename ElementType, typename KeyFnType>
void Algorithm::RadixSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn)
{
    RadixSortBy(Data, Count, Forward<KeyFnType>(KeyFn), static_cast<ElementType*>(nullptr));
}

template<typename ElementType, typename KeyFnType>
void Algorithm::RadixSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch)
{
    if (Count <= 1 || Data == nullptr)
    {
        return;
    }

    AlgorithmIntegerSortDetail::RadixSort(Data, Count, Forward<KeyFnType>(KeyFn), Scratch);
}

template<typename RangeType, typename KeyFnType>
void Algorithm::RadixSortBy(RangeType&& Range, KeyFnType&& KeyFn)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    RadixSortBy<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<KeyFnType>(KeyFn));
}

template<typename RangeType, typename KeyFnType, typename ScratchRangeType>
void Algorithm::RadixSortBy(RangeType&& Range, KeyFnType&& KeyFn, ScratchRangeType&& Scratch)
{
    const int32 Count = static_cast<int32>(ArrayContainer::Size(Range));

    auto* Data = ArrayContainer::Data(Range);
    AlgorithmPrivate::TScratchArray<typename TRemoveReference<decltype(*Data)>::Type> Fallback;

    auto* ScratchData = AlgorithmIntegerSortDetail::ResolveScratch(Forward<ScratchRangeType>(Scratch), Count, Fallback);
    RadixSortBy<typename TRemoveReference<decltype(*Data)>::Type>(Data, Count, Forward<KeyFnType>(KeyFn), ScratchData);
}

template<typename ElementType>
void Algorithm::RadixSort(ElementType* Data, int32 Count)
{
    RadixSortBy(Data, Count, [](const ElementType& Value) { return Value; });
}

template<typename RangeType>
void Algorithm::RadixSort(RangeType&& Range)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    RadixSort<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)));
}

template<typename ElementType, typename KeyFnType>
void Algorithm::IntegerSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn)
{
    IntegerSortBy(Data, Count, Forward<KeyFnType>(KeyFn), static_cast<ElementType*>(nullptr));
}

template<typename ElementType, typename KeyFnType>
void Algorithm::IntegerSortBy(ElementType* Data, int32 Count, KeyFnType&& KeyFn, ElementType* Scratch)
{
    if (Count <= 1 || Data == nullptr)
    {
        return;
    }

    using KeyType = decltype(KeyFn(Data[0]));

    KeyType MinKey = KeyType();
    KeyType MaxKey = KeyType();
    AlgorithmIntegerSortDetail::FindKeyBounds(Data, Count, KeyFn, MinKey, MaxKey);

    const int64 Span64 = static_cast<int64>(MaxKey) - static_cast<int64>(MinKey) + 1;
    if (Span64 > 0 && Span64 <= CountingSortMaxSpan)
    {
        CountingSortBy(Data, Count, Forward<KeyFnType>(KeyFn), Scratch);
        return;
    }

    RadixSortBy(Data, Count, Forward<KeyFnType>(KeyFn), Scratch);
}

template<typename RangeType, typename KeyFnType>
void Algorithm::IntegerSortBy(RangeType&& Range, KeyFnType&& KeyFn)
{
    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    IntegerSortBy<ElementType>(Data, static_cast<int32>(ArrayContainer::Size(Range)), Forward<KeyFnType>(KeyFn));
}

template<typename RangeType, typename KeyFnType, typename ScratchRangeType>
void Algorithm::IntegerSortBy(RangeType&& Range, KeyFnType&& KeyFn, ScratchRangeType&& Scratch)
{
    const int32 Count = static_cast<int32>(ArrayContainer::Size(Range));

    auto* Data = ArrayContainer::Data(Range);
    using ElementType = typename AlgorithmPrivate::TElementFromPointer<decltype(Data)>::Type;
    AlgorithmPrivate::TScratchArray<ElementType> Fallback;

    auto* ScratchData = AlgorithmIntegerSortDetail::ResolveScratch(Forward<ScratchRangeType>(Scratch), Count, Fallback);
    IntegerSortBy<ElementType>(Data, Count, Forward<KeyFnType>(KeyFn), ScratchData);
}
