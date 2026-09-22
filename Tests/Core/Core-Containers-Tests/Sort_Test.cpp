#include "Sort_Test.h"

#if RUN_SORT_TEST
#include "TestUtils.h"

#include <Core/Algorithms/Algorithm.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/ArrayView.h>
#include <Core/Containers/StaticArray.h>
#include <Core/Math/Random.h>
#include <Core/Templates/NumericLimits.h>

#include <algorithm>

struct FStableItem
{
    int32 Key   = 0;
    int32 Order = 0;
};

static bool ArraysMatch(const TArray<int32>& Left, const TArray<int32>& Right)
{
    if (Left.Size() != Right.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < Left.Size(); ++Index)
    {
        if (Left[Index] != Right[Index])
        {
            return false;
        }
    }

    return true;
}

static TArray<int32> CopyStdSorted(TArray<int32> Values)
{
    std::sort(Values.Data(), Values.Data() + Values.Size());
    return Values;
}

bool Sort_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Algorithm::Sort empty / one / two");
    {
        TArray<int32> Empty;
        Algorithm::Sort(Empty);
        TEST_EXPECT(Empty.IsEmpty());

        TArray<int32> Single = { 42 };
        Algorithm::Sort(Single);
        TEST_EXPECT(Single == TArray<int32>({ 42 }));

        TArray<int32> Pair = { 2, 1 };
        Algorithm::Sort(Pair);
        TEST_EXPECT(Pair == TArray<int32>({ 1, 2 }));
    }

    TEST_SECTION("Algorithm::Sort predicate descending");
    {
        TArray<int32> Values = { 30, 10, 40, 20 };
        Algorithm::Sort(Values, [](int32 Left, int32 Right)
        {
            return Left > Right;
        });
        TEST_EXPECT(Values == TArray<int32>({ 40, 30, 20, 10 }));
    }

    TEST_SECTION("TArray wrapper Sort");
    {
        TArray<int32> Values = { 30, 10, 40, 20 };
        Values.Sort();
        TEST_EXPECT(Values == TArray<int32>({ 10, 20, 30, 40 }));
    }

    TEST_SECTION("TArrayView / TStaticArray / C array");
    {
        TStaticArray<int32, 4> Static = { 4, 1, 3, 2 };
        Algorithm::Sort(Static);
        TEST_EXPECT(Static[0] == 1 && Static[1] == 2 && Static[2] == 3 && Static[3] == 4);

        TArray<int32> Buffer = { 9, 7, 8, 6 };
        TArrayView<int32> View = MakeArrayView(Buffer);
        Algorithm::Sort(View);
        TEST_EXPECT(Buffer == TArray<int32>({ 6, 7, 8, 9 }));

        int32 Raw[5] = { 5, 4, 3, 2, 1 };
        Algorithm::Sort(Raw);
        TEST_EXPECT(Raw[0] == 1 && Raw[4] == 5);
    }

    TEST_SECTION("Random permutations vs std::sort");
    {
        FRandom Random(12345);
        for (int32 Size : { 0, 1, 2, 3, 16, 24, 25, 128, 256, 1024 })
        {
            TArray<int32> Values;
            Values.Reserve(Size);
            for (int32 Index = 0; Index < Size; ++Index)
            {
                Values.Add(static_cast<int32>(Random.Rand()));
            }

            TArray<int32> Expected = CopyStdSorted(Values);
            Algorithm::Sort(Values);
            TEST_EXPECT(ArraysMatch(Values, Expected));
            TEST_EXPECT(Algorithm::IsSorted(Values));
        }
    }

    TEST_SECTION("Adversarial patterns");
    {
        TArray<int32> Sorted;
        TArray<int32> Reverse;
        TArray<int32> Duplicates;
        TArray<int32> AllEqual;
        for (int32 Index = 0; Index < 512; ++Index)
        {
            Sorted.Add(Index);
            Reverse.Add(511 - Index);
            Duplicates.Add(Index % 4);
            AllEqual.Add(7);
        }

        TArray<int32> SortedExpected = CopyStdSorted(Sorted);
        Algorithm::Sort(Sorted);
        TEST_EXPECT(ArraysMatch(Sorted, SortedExpected));

        TArray<int32> ReverseExpected = CopyStdSorted(Reverse);
        Algorithm::Sort(Reverse);
        TEST_EXPECT(ArraysMatch(Reverse, ReverseExpected));

        TArray<int32> DuplicateExpected = CopyStdSorted(Duplicates);
        Algorithm::Sort(Duplicates);
        TEST_EXPECT(ArraysMatch(Duplicates, DuplicateExpected));

        Algorithm::Sort(AllEqual);
        TEST_EXPECT(Algorithm::IsSorted(AllEqual));
    }

    TEST_SECTION("StableSort preserves equal-key order");
    {
        TArray<FStableItem> Items;
        for (int32 Index = 0; Index < 64; ++Index)
        {
            FStableItem Item;
            Item.Key   = Index % 5;
            Item.Order = Index;
            Items.Add(Item);
        }

        Algorithm::StableSort(Items, [](const FStableItem& Left, const FStableItem& Right)
        {
            return Left.Key < Right.Key;
        });

        TEST_EXPECT(Algorithm::IsSorted(Items, [](const FStableItem& Left, const FStableItem& Right)
        {
            return Left.Key < Right.Key;
        }));

        for (int32 Index = 1; Index < Items.Size(); ++Index)
        {
            if (Items[Index].Key == Items[Index - 1].Key)
            {
                TEST_EXPECT(Items[Index - 1].Order < Items[Index].Order);
            }
        }
    }

    TEST_SECTION("Radix / counting / integer sort");
    {
        TArray<int32> Values = { -5, 3, -1, 0, 3, 100, -20, 7 };
        TArray<int32> Radix = Values;
        TArray<int32> Counted = Values;
        TArray<int32> Integer = Values;
        TArray<int32> Expected = CopyStdSorted(Values);

        Algorithm::RadixSort(Radix);
        Algorithm::CountingSortBy(Counted, [](int32 Value) { return Value; });
        Algorithm::IntegerSortBy(Integer, [](int32 Value) { return Value; });

        TEST_EXPECT(ArraysMatch(Radix, Expected));
        TEST_EXPECT(ArraysMatch(Counted, Expected));
        TEST_EXPECT(ArraysMatch(Integer, Expected));

        TArray<int32> Wide;
        for (int32 Index = 0; Index < 256; ++Index)
        {
            Wide.Add(Index * 10000 - 500000);
        }
        TArray<int32> WideExpected = CopyStdSorted(Wide);
        Algorithm::IntegerSortBy(Wide, [](int32 Value) { return Value; });
        TEST_EXPECT(ArraysMatch(Wide, WideExpected));

        TArray<uint64> Big = { 9ull, 1ull, 1ull << 40, 7ull };
        Algorithm::RadixSort(Big);
        TEST_EXPECT(Big[0] == 1ull && Big[3] == (1ull << 40));
    }

    TEST_SECTION("Heapify / HeapPop / HeapSort");
    {
        TArray<int32> Heap = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };
        Algorithm::Heapify(Heap);
        int32 Previous = TNumericLimits<int32>::Max();
        while (Heap.Size() > 0)
        {
            TEST_EXPECT(Heap[0] <= Previous);
            Previous = Heap[0];
            Algorithm::HeapPop(Heap);
        }

        TArray<int32> Sorted = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };
        Algorithm::HeapSort(Sorted);
        TEST_EXPECT(Sorted == TArray<int32>({ 1, 3, 4, 5, 6, 8, 9, 10, 13, 15, 17 }));
    }

    TEST_SECTION("Large reverse-sorted introsort fallback");
    {
        TArray<int32> Reverse;
        Reverse.Reserve(10000);
        for (int32 Index = 10000; Index > 0; --Index)
        {
            Reverse.Add(Index);
        }

        Algorithm::Sort(Reverse);
        TEST_EXPECT(Algorithm::IsSorted(Reverse));
        TEST_EXPECT_EQ(Reverse[0], 1);
        TEST_EXPECT_EQ(Reverse[9999], 10000);
    }

    TEST_END();
}
#endif
