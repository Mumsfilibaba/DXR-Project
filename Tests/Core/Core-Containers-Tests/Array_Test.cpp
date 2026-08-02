#include "Array_Test.h"

#if RUN_TARRAY_TEST
#include "TestUtils.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>

#include <vector>

bool TArray_Test()
{
    TEST_BEGIN();

    TEST_SECTION("TArray::Find / FindLast / Contains");
    {
        const TArray<int32> Array = { 10, 20, 30, 20, 40 };
        TEST_EXPECT(Array.Find(20) == 1);
        TEST_EXPECT(Array.FindLast(20) == 3);
        TEST_EXPECT(Array.Contains(20));
        TEST_EXPECT(!Array.Contains(99));
    }

    TEST_SECTION("TArray::FindWithPredicate / FindLastWithPredicate / ContainsWithPredicate");
    {
        const TArray<int32> Array = { 10, 20, 30, 20, 40 };
        TEST_EXPECT(Array.FindWithPredicate([](int32 Value)
        {
            return Value == 30;
        }) == 2);
        TEST_EXPECT(Array.FindLastWithPredicate([](int32 Value)
        {
            return Value == 20;
        }) == 3);
        TEST_EXPECT(Array.ContainsWithPredicate([](int32 Value)
        {
            return Value > 35;
        }));
        TEST_EXPECT(!Array.ContainsWithPredicate([](int32 Value)
        {
            return Value > 100;
        }));
    }

    TEST_SECTION("TArray::IsValidIndex");
    {
        const TArray<int32> Array = { 1, 2, 3 };
        TEST_EXPECT(Array.IsValidIndex(0));
        TEST_EXPECT(Array.IsValidIndex(2));
        TEST_EXPECT(!Array.IsValidIndex(3));
        TEST_EXPECT(!Array.IsValidIndex(-1));
    }

    TEST_SECTION("TArray::FirstElement / LastElement");
    {
        TArray<int32> Array = { 10, 20, 40 };
        TEST_EXPECT(Array.FirstElement() == 10);
        TEST_EXPECT(Array.LastElement() == 40);

        const TArray<int32>& ConstArray = Array;
        TEST_EXPECT(ConstArray.FirstElement() == 10);
        TEST_EXPECT(ConstArray.LastElement() == 40);
    }

    TEST_SECTION("TArray metadata (Stride/SizeInBytes/CapacityInBytes)");
    {
        const TArray<int32> Array = { 1, 2, 3, 4 };
        TEST_EXPECT(Array.Stride() == static_cast<int32>(sizeof(int32)));
        TEST_EXPECT(Array.SizeInBytes() == Array.Size() * static_cast<int32>(sizeof(int32)));
        TEST_EXPECT(Array.CapacityInBytes() == Array.Capacity() * static_cast<int32>(sizeof(int32)));
    }

    TEST_SECTION("TArray::AddUnique / AddDefault / AddUninitialized");
    {
        TArray<int32> Array = { 10, 20 };
        TEST_EXPECT(Array.AddUnique(20) == 1);
        TEST_EXPECT(Array.Size() == 2);

        TEST_EXPECT(Array.AddUnique(50) == 2);
        TEST_EXPECT(Array.Size() == 3);

        const int32 DefaultIndex = Array.Size();
        Array.AddDefault();
        TEST_EXPECT(Array[DefaultIndex] == 0);

        const int32 SizeBefore = Array.Size();
        Array.AddUninitialized();
        TEST_EXPECT(Array.Size() == SizeBefore + 1);
    }

    TEST_SECTION("TArray::Reverse");
    {
        TArray<int32> Array = { 10, 20, 30, 20, 40 };
        Array.Reverse();
        TEST_EXPECT(Array == TArray<int32>({ 40, 20, 30, 20, 10 }));
    }

    TEST_SECTION("TArray::SortWithPredicate");
    {
        TArray<int32> Array = { 30, 10, 40, 20 };
        Array.SortWithPredicate([](int32 Left, int32 Right)
        {
            return Left > Right;
        });

        TEST_EXPECT(Array == TArray<int32>({ 40, 30, 20, 10 }));
    }

    TEST_SECTION("TArray::RemoveAtSwap / RemoveSingleSwap / RemoveAllSwap");
    {
        TArray<int32> Array = { 1, 2, 3, 4, 5 };
        Array.RemoveAtSwap(1);
        TEST_EXPECT(Array.Size() == 4);
        TEST_EXPECT(!Array.Contains(2));

        TArray<int32> Single = { 1, 2, 3, 2 };
        TEST_EXPECT(Single.RemoveSingleSwap(2));
        TEST_EXPECT(Single.Size() == 3);

        TArray<int32> Multi = { 1, 2, 3, 4, 5, 6 };
        Multi.RemoveAllSwap([](int32 Value)
        {
            return (Value % 2) == 0;
        });

        TEST_EXPECT(Multi.Size() == 3);
        TEST_EXPECT(!Multi.ContainsWithPredicate([](int32 Value)
        {
            return (Value % 2) == 0;
        }));
    }

    TEST_SECTION("TArray::Remove / RemoveAll");
    {
        // Remove deletes every matching instance.
        TArray<int32> Array = { 1, 2, 2, 3, 2 };
        TEST_EXPECT(Array.Remove(2));
        TEST_EXPECT(Array == TArray<int32>({ 1, 3 }));
        TEST_EXPECT(!Array.Remove(99));

        TArray<int32> All = { 1, 2, 2, 3, 2 };
        All.RemoveAll([](int32 Value)
        {
            return Value == 2;
        });

        TEST_EXPECT(All == TArray<int32>({ 1, 3 }));
    }

    TEST_SECTION("TArray::operator+ / operator+=");
    {
        TArray<int32> First = { 1, 2 };
        const TArray<int32> Second = { 3, 4 };
        const TArray<int32> Sum = First + Second;
        TEST_EXPECT(Sum == TArray<int32>({ 1, 2, 3, 4 }));

        First += Second;
        TEST_EXPECT(First == TArray<int32>({ 1, 2, 3, 4 }));

        First += { 5, 6 };
        TEST_EXPECT(First == TArray<int32>({ 1, 2, 3, 4, 5, 6 }));
    }

    TEST_SECTION("TArray::operator== / operator!=");
    {
        const TArray<int32> First = { 1, 2, 3 };
        const TArray<int32> Second = { 1, 2, 3 };
        const TArray<int32> Third = { 1, 2, 4 };
        TEST_EXPECT(First == Second);
        TEST_EXPECT(First != Third);
    }

    TEST_SECTION("TArray const iterators");
    {
        const TArray<int32> Array = { 1, 2, 3, 4 };
        int32 Sum = 0;
        for (const int32& Value : Array)
        {
            Sum += Value;
        }

        TEST_EXPECT(Sum == 10);
    }

    TEST_SECTION("TArray construction (count/value, ptr+size, initializer-list)");
    {
        TArray<String> Empty;
        TEST_EXPECT(Empty.IsEmpty());

        TArray<String> Filled(5, "Hello");
        TEST_EXPECT_EQ(Filled.Size(), 5);
        TEST_EXPECT(Filled == TArray<String>({ "Hello", "Hello", "Hello", "Hello", "Hello" }));

        TArray<String> FromPtr(Filled.Data(), Filled.Size());
        TEST_EXPECT(FromPtr == Filled);

        TArray<String> FromList = { "Hello World", "TArray", "This is a longer teststring" };
        TEST_EXPECT_EQ(FromList.Size(), 3);
    }

    TEST_SECTION("TArray copy / move construction");
    {
        TArray<String> Source = { "A", "B", "C" };
        TArray<String> Copy = Source;
        TEST_EXPECT(Copy == Source);

        TArray<String> Moved = ::Move(Copy);
        TEST_EXPECT(Moved == Source);
        TEST_EXPECT(Copy.IsEmpty());
    }

    TEST_SECTION("TArray::Reset (count/value, init-list, ptr+size)");
    {
        TArray<String> Array;
        Array.Reset(3, "X");
        TEST_EXPECT(Array == TArray<String>({ "X", "X", "X" }));

        Array.Reset({ "1", "2" });
        TEST_EXPECT(Array == TArray<String>({ "1", "2" }));

        TArray<String> Other = { "p", "q", "r" };
        Array.Reset(Other.Data(), Other.Size());
        TEST_EXPECT(Array == Other);
    }

    TEST_SECTION("TArray::Resize grow / shrink");
    {
        TArray<String> Array = { "a", "b", "c", "d" };
        Array.Resize(6);
        TEST_EXPECT_EQ(Array.Size(), 6);
        TEST_EXPECT(Array[0] == "a" && Array[3] == "d");
        TEST_EXPECT(Array[4].IsEmpty() && Array[5].IsEmpty());

        Array.Resize(2);
        TEST_EXPECT(Array == TArray<String>({ "a", "b" }));
        TEST_EXPECT(Array.Capacity() >= Array.Size());
    }

    TEST_SECTION("TArray::Reserve / Shrink");
    {
        TArray<String> Array = { "a", "b" };
        Array.Reserve(32);
        TEST_EXPECT(Array.Capacity() >= 32);
        TEST_EXPECT(Array == TArray<String>({ "a", "b" }));

        Array.Shrink();
        TEST_EXPECT(Array.Capacity() == Array.Size());
    }

    TEST_SECTION("TArray assignment (copy / move / initializer-list)");
    {
        TArray<String> Source = { "a", "b", "c" };
        TArray<String> Copy;
        Copy = Source;
        TEST_EXPECT(Copy == Source);

        TArray<String> Moved;
        Moved = ::Move(Copy);
        TEST_EXPECT(Moved == Source);
        TEST_EXPECT_EQ(Copy.Size(), 0);

        TArray<String> FromList;
        FromList = { "x", "y" };
        TEST_EXPECT(FromList == TArray<String>({ "x", "y" }));
    }

    TEST_SECTION("TArray::Add / Emplace / Pop");
    {
        TArray<String> Array;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            Array.Add(String::CreateFormatted("P%d", Index));
        }

        Array.Emplace("E0");
        TEST_EXPECT(Array == TArray<String>({ "P0", "P1", "P2", "E0" }));

        Array.Pop();
        TEST_EXPECT(Array == TArray<String>({ "P0", "P1", "P2" }));

        Array.Pop(2);
        TEST_EXPECT(Array == TArray<String>({ "P0" }));
    }

    TEST_SECTION("TArray::Insert (value / initializer-list at front/middle/end)");
    {
        TArray<String> Array = { "a", "b", "c" };
        Array.Insert(0, "front");
        TEST_EXPECT(Array == TArray<String>({ "front", "a", "b", "c" }));

        Array.Insert(2, "mid");
        TEST_EXPECT(Array == TArray<String>({ "front", "a", "mid", "b", "c" }));

        Array.Insert(Array.Size(), { "e1", "e2" });
        TEST_EXPECT(Array == TArray<String>({ "front", "a", "mid", "b", "c", "e1", "e2" }));

        // Force reallocation, then insert at the front.
        Array.Shrink();
        Array.Insert(0, { "r1", "r2" });
        TEST_EXPECT(Array == TArray<String>({ "r1", "r2", "front", "a", "mid", "b", "c", "e1", "e2" }));
    }

    TEST_SECTION("TArray::RemoveAt (single / range)");
    {
        TArray<String> Array = { "0", "1", "2", "3", "4", "5" };
        Array.RemoveAt(0);
        TEST_EXPECT(Array == TArray<String>({ "1", "2", "3", "4", "5" }));

        Array.RemoveAt(1, 2);
        TEST_EXPECT(Array == TArray<String>({ "1", "4", "5" }));

        Array.RemoveAt(Array.Size() - 1);
        TEST_EXPECT(Array == TArray<String>({ "1", "4" }));
    }

    TEST_SECTION("TArray::Swap / Fill / Append");
    {
        TArray<String> First = { "a", "b" };
        TArray<String> Second = { "x", "y", "z" };
        ::Swap(First, Second);
        TEST_EXPECT(First == TArray<String>({ "x", "y", "z" }));
        TEST_EXPECT(Second == TArray<String>({ "a", "b" }));

        First.Fill("F");
        TEST_EXPECT(First == TArray<String>({ "F", "F", "F" }));

        First.Append({ "g", "h" });
        TEST_EXPECT(First == TArray<String>({ "F", "F", "F", "g", "h" }));
    }

    TEST_SECTION("TArray heap operations (Heapify/HeapPush/HeapPop/HeapSort)");
    {
        TArray<int32> Heap = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };
        Heap.Heapify();
        TEST_EXPECT_EQ(Heap.Size(), 11);

        Heap.HeapPush(19);
        Heap.HeapPush(0);
        TEST_EXPECT_EQ(Heap.Size(), 13);
        Heap.HeapPop();
        TEST_EXPECT_EQ(Heap.Size(), 12);

        TArray<int32> Sorted = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };
        Sorted.HeapSort();
        TEST_EXPECT(Sorted == TArray<int32>({ 1, 3, 4, 5, 6, 8, 9, 10, 13, 15, 17 }));
    }

    TEST_SECTION("TArray with inline allocator");
    {
        TInlineArray<String, 4> Array;
        for (int32 Index = 0; Index < 8; ++Index)
        {
            Array.Add(String::CreateFormatted("S%d", Index));
        }

        TEST_EXPECT_EQ(Array.Size(), 8);
        TEST_EXPECT(Array[0] == "S0" && Array[7] == "S7");

        Array.Insert(0, "front");
        TEST_EXPECT(Array[0] == "front");
        Array.RemoveAt(0);
        TEST_EXPECT(Array[0] == "S0");
    }

    TEST_SECTION("TArray iterator edge cases (forward / reverse / empty / erase-during-iteration)");
    {
        // Empty container: range-for and explicit iterators must not execute the body.
        TArray<int32> Empty;
        int32 EmptyVisits = 0;
        for (int32 Value : Empty)
        {
            (void)Value;
            ++EmptyVisits;
        }

        TEST_EXPECT(EmptyVisits == 0);
        TEST_EXPECT(Empty.Iterator().IsEnd());
        TEST_EXPECT(Empty.ReverseIterator().IsEnd());

        // Forward explicit iterator.
        TArray<int32> Array = { 1, 2, 3, 4, 5 };
        int32 ForwardSum = 0;
        for (auto It = Array.Iterator(); !It.IsEnd(); ++It)
        {
            ForwardSum += *It;
        }

        TEST_EXPECT(ForwardSum == 15);

        // Reverse iterator visits elements last-to-first.
        TArray<int32> Reversed;
        for (auto It = Array.ReverseIterator(); !It.IsEnd(); ++It)
        {
            Reversed.Add(*It);
        }

        TEST_EXPECT(Reversed == TArray<int32>({ 5, 4, 3, 2, 1 }));

        // Erase-during-iteration: walk by index removing evens; survivors keep their relative order.
        TArray<int32> Erase = { 1, 2, 3, 4, 5, 6, 7, 8 };
        for (int32 Index = 0; Index < Erase.Size(); )
        {
            if ((Erase[Index] % 2) == 0)
            {
                Erase.RemoveAt(Index);
            }
            else
            {
                ++Index;
            }
        }

        TEST_EXPECT(Erase == TArray<int32>({ 1, 3, 5, 7 }));
    }

    TEST_SECTION("TArray reallocation stress (FInstanced, seeded size sweep)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom Random(Seed);
            TArray<FInstanced> Array;
            std::vector<int32> Oracle;

            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const int32 Id = static_cast<int32>(Random.RandInt(0, 1000000));
                if (!Array.IsEmpty() && Random.RandBool())
                {
                    const int32 At = static_cast<int32>(Random.RandInt(0, Array.Size() - 1));
                    Array.Insert(At, FInstanced(Id));
                    Oracle.insert(Oracle.begin() + At, Id);
                }
                else
                {
                    Array.Add(FInstanced(Id));
                    Oracle.push_back(Id);
                }

                if ((Step % 17) == 0)
                {
                    Array.Reserve(Array.Size() + 64);
                }

                if ((Step % 23) == 0)
                {
                    Array.Shrink();
                }
            }

            bool bMatches = (Array.Size() == static_cast<int32>(Oracle.size()));
            for (int32 Index = 0; bMatches && (Index < Array.Size()); ++Index)
            {
                bMatches = (Array[Index].GetId() == Oracle[Index]) && Array[Index].IsPayloadValid();
            }

            if (!bMatches)
            {
                LOG_ERROR("[STRESS FAIL] TArray grow seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bMatches);
            TEST_EXPECT(FInstanced::LiveCount() == Array.Size());

            // Remove from random positions down to empty; every live element must be destroyed once.
            while (!Array.IsEmpty())
            {
                const int32 At = static_cast<int32>(Random.RandInt(0, Array.Size() - 1));
                Array.RemoveAt(At);
                Oracle.erase(Oracle.begin() + At);
            }

            TEST_EXPECT(static_cast<int32>(Oracle.size()) == 0);
            TEST_EXPECT(FInstanced::LiveCount() == 0);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_SECTION("TArray heap-string reallocation stress (inline allocator, seeded size sweep)");
    {
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom                 Random(Seed);
            TInlineArray<String, 4> Array;
            std::vector<String>     Oracle;

            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                String Value = String::CreateFormatted("StressString-%d-padding-to-exceed-inline-buffer", Step);
                if (!Array.IsEmpty() && Random.RandBool())
                {
                    const int32 At = static_cast<int32>(Random.RandInt(0, Array.Size() - 1));
                    Array.Insert(At, Value);
                    Oracle.insert(Oracle.begin() + At, Value);
                }
                else
                {
                    Array.Add(Value);
                    Oracle.push_back(Value);
                }

                if ((Step % 17) == 0)
                {
                    Array.Reserve(Array.Size() + 64);
                }

                if ((Step % 23) == 0)
                {
                    Array.Shrink();
                }
            }

            bool bMatches = (Array.Size() == static_cast<int32>(Oracle.size()));
            for (int32 Index = 0; bMatches && (Index < Array.Size()); ++Index)
            {
                bMatches = (Array[Index] == Oracle[Index]);
            }

            if (!bMatches)
            {
                LOG_ERROR("[STRESS FAIL] TArray<String> grow seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bMatches);

            // Remove from random positions down to empty to stress shifting of heap strings.
            while (!Array.IsEmpty())
            {
                const int32 At = static_cast<int32>(Random.RandInt(0, Array.Size() - 1));
                Array.RemoveAt(At);
                Oracle.erase(Oracle.begin() + At);
            }

            TEST_EXPECT(static_cast<int32>(Oracle.size()) == 0);
            TEST_EXPECT(Array.IsEmpty());
        }
    }

    TEST_END();
}
#endif
