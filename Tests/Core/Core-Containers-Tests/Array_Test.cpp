#include "Array_Test.h"

#if RUN_TARRAY_TEST
#include "TestUtils.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>

#include <vector>

bool TArray_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("TArray::Find / FindLast / Contains"));
    {
        const TArray<int32> Array = { 10, 20, 30, 20, 40 };
        TEST_EXPECT(Array.Find(20) == 1);
        TEST_EXPECT(Array.FindLast(20) == 3);
        TEST_EXPECT(Array.Contains(20));
        TEST_EXPECT(!Array.Contains(99));
    }

    TEST_SECTION(TEXT("TArray::FindWithPredicate / FindLastWithPredicate / ContainsWithPredicate"));
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

    TEST_SECTION(TEXT("TArray::IsValidIndex"));
    {
        const TArray<int32> Array = { 1, 2, 3 };
        TEST_EXPECT(Array.IsValidIndex(0));
        TEST_EXPECT(Array.IsValidIndex(2));
        TEST_EXPECT(!Array.IsValidIndex(3));
        TEST_EXPECT(!Array.IsValidIndex(-1));
    }

    TEST_SECTION(TEXT("TArray::First / Last"));
    {
        TArray<int32> Array = { 10, 20, 40 };
        TEST_EXPECT(Array.First() == 10);
        TEST_EXPECT(Array.Last() == 40);

        const TArray<int32>& ConstArray = Array;
        TEST_EXPECT(ConstArray.First() == 10);
        TEST_EXPECT(ConstArray.Last() == 40);
    }

    TEST_SECTION(TEXT("TArray::LastIndex"));
    {
        const TArray<int32> Array = { 10, 20, 40 };
        TEST_EXPECT_EQ(Array.LastIndex(), 2);

        const TArray<int32> Single = { 10 };
        TEST_EXPECT_EQ(Single.LastIndex(), 0);

        const TArray<int32> Empty;
        TEST_EXPECT_EQ(Empty.LastIndex(), TArray<int32>::InvalidIndex);
        TEST_EXPECT(!Empty.IsValidIndex(Empty.LastIndex()));

        TArray<int32> Cleared = { 1, 2, 3 };
        Cleared.Clear();
        TEST_EXPECT_EQ(Cleared.LastIndex(), TArray<int32>::InvalidIndex);

        int32 Visits = 0;
        for (int32 Index = Empty.LastIndex(); Index >= 0; --Index)
        {
            ++Visits;
        }

        TEST_EXPECT_EQ(Visits, 0);
    }

    TEST_SECTION(TEXT("TArray metadata (Stride/SizeInBytes/CapacityInBytes)"));
    {
        const TArray<int32> Array = { 1, 2, 3, 4 };
        TEST_EXPECT(Array.Stride() == static_cast<int32>(sizeof(int32)));
        TEST_EXPECT(Array.SizeInBytes() == Array.Size() * static_cast<int32>(sizeof(int32)));
        TEST_EXPECT(Array.CapacityInBytes() == Array.Capacity() * static_cast<int32>(sizeof(int32)));
    }

    TEST_SECTION(TEXT("TArray::AddUnique / AddDefault / AddUninitialized"));
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

    TEST_SECTION(TEXT("TArray::Reverse"));
    {
        TArray<int32> Array = { 10, 20, 30, 20, 40 };
        Array.Reverse();
        TEST_EXPECT(Array == TArray<int32>({ 40, 20, 30, 20, 10 }));
    }

    TEST_SECTION(TEXT("TArray::SortWithPredicate"));
    {
        TArray<int32> Array = { 30, 10, 40, 20 };
        Array.SortWithPredicate([](int32 Left, int32 Right)
        {
            return Left > Right;
        });

        TEST_EXPECT(Array == TArray<int32>({ 40, 30, 20, 10 }));
    }

    TEST_SECTION(TEXT("TArray::Sort (empty / single / populated)"));
    {
        TArray<int32> Empty;
        Empty.Sort();
        TEST_EXPECT(Empty.IsEmpty());

        TArray<int32> Single = { 42 };
        Single.Sort();
        TEST_EXPECT(Single == TArray<int32>({ 42 }));

        TArray<int32> Array = { 30, 10, 40, 20 };
        Array.Sort();
        TEST_EXPECT(Array == TArray<int32>({ 10, 20, 30, 40 }));

        TArray<int32> EmptyPredicate;
        EmptyPredicate.SortWithPredicate([](int32 Left, int32 Right)
        {
            return Left > Right;
        });

        TEST_EXPECT(EmptyPredicate.IsEmpty());
    }

    TEST_SECTION(TEXT("TArray::RemoveAtSwap / RemoveSingleSwap / RemoveAllSwap"));
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

    TEST_SECTION(TEXT("TArray::Remove / RemoveAll"));
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

    TEST_SECTION(TEXT("TArray::operator+ / operator+="));
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

    TEST_SECTION(TEXT("TArray::operator== / operator!="));
    {
        const TArray<int32> First = { 1, 2, 3 };
        const TArray<int32> Second = { 1, 2, 3 };
        const TArray<int32> Third = { 1, 2, 4 };
        TEST_EXPECT(First == Second);
        TEST_EXPECT(First != Third);
    }

    TEST_SECTION(TEXT("TArray const iterators"));
    {
        const TArray<int32> Array = { 1, 2, 3, 4 };
        int32 Sum = 0;
        for (const int32& Value : Array)
        {
            Sum += Value;
        }

        TEST_EXPECT(Sum == 10);
    }

    TEST_SECTION(TEXT("TArray construction (count/value, ptr+size, initializer-list)"));
    {
        TArray<FString> Empty;
        TEST_EXPECT(Empty.IsEmpty());

        TArray<FString> Filled(5, TEXT("Hello"));
        TEST_EXPECT_EQ(Filled.Size(), 5);
        TEST_EXPECT(Filled == TArray<FString>({ TEXT("Hello"), TEXT("Hello"), TEXT("Hello"), TEXT("Hello"), TEXT("Hello") }));

        TArray<FString> FromPtr(Filled.Data(), Filled.Size());
        TEST_EXPECT(FromPtr == Filled);

        TArray<FString> FromList = { TEXT("Hello World"), TEXT("TArray"), TEXT("This is a longer teststring") };
        TEST_EXPECT_EQ(FromList.Size(), 3);
    }

    TEST_SECTION(TEXT("TArray copy / move construction"));
    {
        TArray<FString> Source = { TEXT("A"), TEXT("B"), TEXT("C") };
        TArray<FString> Copy = Source;
        TEST_EXPECT(Copy == Source);

        TArray<FString> Moved = ::Move(Copy);
        TEST_EXPECT(Moved == Source);
        TEST_EXPECT(Copy.IsEmpty());
    }

    TEST_SECTION(TEXT("TArray::Reset (count/value, init-list, ptr+size)"));
    {
        TArray<FString> Array;
        Array.Reset(3, TEXT("X"));
        TEST_EXPECT(Array == TArray<FString>({ TEXT("X"), TEXT("X"), TEXT("X") }));

        Array.Reset({ TEXT("1"), TEXT("2") });
        TEST_EXPECT(Array == TArray<FString>({ TEXT("1"), TEXT("2") }));

        TArray<FString> Other = { TEXT("p"), TEXT("q"), TEXT("r") };
        Array.Reset(Other.Data(), Other.Size());
        TEST_EXPECT(Array == Other);
    }

    TEST_SECTION(TEXT("TArray::Resize grow / shrink"));
    {
        TArray<FString> Array = { TEXT("a"), TEXT("b"), TEXT("c"), TEXT("d") };
        Array.Resize(6);
        TEST_EXPECT_EQ(Array.Size(), 6);
        TEST_EXPECT(Array[0] == TEXT("a") && Array[3] == TEXT("d"));
        TEST_EXPECT(Array[4].IsEmpty() && Array[5].IsEmpty());

        Array.Resize(2);
        TEST_EXPECT(Array == TArray<FString>({ TEXT("a"), TEXT("b") }));
        TEST_EXPECT(Array.Capacity() >= Array.Size());
    }

    TEST_SECTION(TEXT("TArray::Reserve / Shrink"));
    {
        TArray<FString> Array = { TEXT("a"), TEXT("b") };
        Array.Reserve(32);
        TEST_EXPECT(Array.Capacity() >= 32);
        TEST_EXPECT(Array == TArray<FString>({ TEXT("a"), TEXT("b") }));

        Array.Shrink();
        TEST_EXPECT(Array.Capacity() == Array.Size());
    }

    TEST_SECTION(TEXT("TArray assignment (copy / move / initializer-list)"));
    {
        TArray<FString> Source = { TEXT("a"), TEXT("b"), TEXT("c") };
        TArray<FString> Copy;
        Copy = Source;
        TEST_EXPECT(Copy == Source);

        TArray<FString> Moved;
        Moved = ::Move(Copy);
        TEST_EXPECT(Moved == Source);
        TEST_EXPECT_EQ(Copy.Size(), 0);

        TArray<FString> FromList;
        FromList = { TEXT("x"), TEXT("y") };
        TEST_EXPECT(FromList == TArray<FString>({ TEXT("x"), TEXT("y") }));
    }

    TEST_SECTION(TEXT("TArray::Add / Emplace / Pop"));
    {
        TArray<FString> Array;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            Array.Add(FString::Printf(TEXT("P%d"), Index));
        }

        Array.Emplace(TEXT("E0"));
        TEST_EXPECT(Array == TArray<FString>({ TEXT("P0"), TEXT("P1"), TEXT("P2"), TEXT("E0") }));

        Array.Pop();
        TEST_EXPECT(Array == TArray<FString>({ TEXT("P0"), TEXT("P1"), TEXT("P2") }));

        Array.Pop(2);
        TEST_EXPECT(Array == TArray<FString>({ TEXT("P0") }));
    }

    TEST_SECTION(TEXT("TArray::Insert (value / initializer-list at front/middle/end)"));
    {
        TArray<FString> Array = { TEXT("a"), TEXT("b"), TEXT("c") };
        Array.Insert(0, TEXT("front"));
        TEST_EXPECT(Array == TArray<FString>({ TEXT("front"), TEXT("a"), TEXT("b"), TEXT("c") }));

        Array.Insert(2, TEXT("mid"));
        TEST_EXPECT(Array == TArray<FString>({ TEXT("front"), TEXT("a"), TEXT("mid"), TEXT("b"), TEXT("c") }));

        Array.Insert(Array.Size(), { TEXT("e1"), TEXT("e2") });
        TEST_EXPECT(Array == TArray<FString>({ TEXT("front"), TEXT("a"), TEXT("mid"), TEXT("b"), TEXT("c"), TEXT("e1"), TEXT("e2") }));

        // Force reallocation, then insert at the front.
        Array.Shrink();
        Array.Insert(0, { TEXT("r1"), TEXT("r2") });
        TEST_EXPECT(Array == TArray<FString>({ TEXT("r1"), TEXT("r2"), TEXT("front"), TEXT("a"), TEXT("mid"), TEXT("b"), TEXT("c"), TEXT("e1"), TEXT("e2") }));
    }

    TEST_SECTION(TEXT("TArray::RemoveAt (single / range)"));
    {
        TArray<FString> Array = { TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5") };
        Array.RemoveAt(0);
        TEST_EXPECT(Array == TArray<FString>({ TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5") }));

        Array.RemoveAt(1, 2);
        TEST_EXPECT(Array == TArray<FString>({ TEXT("1"), TEXT("4"), TEXT("5") }));

        Array.RemoveAt(Array.Size() - 1);
        TEST_EXPECT(Array == TArray<FString>({ TEXT("1"), TEXT("4") }));
    }

    TEST_SECTION(TEXT("TArray::Swap / Fill / Append"));
    {
        TArray<FString> First = { TEXT("a"), TEXT("b") };
        TArray<FString> Second = { TEXT("x"), TEXT("y"), TEXT("z") };
        ::Swap(First, Second);
        TEST_EXPECT(First == TArray<FString>({ TEXT("x"), TEXT("y"), TEXT("z") }));
        TEST_EXPECT(Second == TArray<FString>({ TEXT("a"), TEXT("b") }));

        First.Fill(TEXT("F"));
        TEST_EXPECT(First == TArray<FString>({ TEXT("F"), TEXT("F"), TEXT("F") }));

        First.Append({ TEXT("g"), TEXT("h") });
        TEST_EXPECT(First == TArray<FString>({ TEXT("F"), TEXT("F"), TEXT("F"), TEXT("g"), TEXT("h") }));
    }

    TEST_SECTION(TEXT("TArray heap operations (Heapify/HeapPush/HeapPop/HeapSort)"));
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

    TEST_SECTION(TEXT("TArray with inline allocator"));
    {
        TInlineArray<FString, 4> Array;
        for (int32 Index = 0; Index < 8; ++Index)
        {
            Array.Add(FString::Printf(TEXT("S%d"), Index));
        }

        TEST_EXPECT_EQ(Array.Size(), 8);
        TEST_EXPECT(Array[0] == TEXT("S0") && Array[7] == TEXT("S7"));

        Array.Insert(0, TEXT("front"));
        TEST_EXPECT(Array[0] == TEXT("front"));
        Array.RemoveAt(0);
        TEST_EXPECT(Array[0] == TEXT("S0"));
    }

    TEST_SECTION(TEXT("TArray iterator edge cases (forward / reverse / empty / erase-during-iteration)"));
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

    TEST_SECTION(TEXT("TArray reallocation stress (FInstanced, seeded size sweep)"));
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

    TEST_SECTION(TEXT("TArray heap-string reallocation stress (inline allocator, seeded size sweep)"));
    {
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom                  Random(Seed);
            TInlineArray<FString, 4> Array;
            std::vector<FString>     Oracle;

            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                FString Value = FString::Printf(TEXT("StressString-%d-padding-to-exceed-inline-buffer"), Step);
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
                LOG_ERROR("[STRESS FAIL] TArray<FString> grow seed=%u size=%d", Seed, TargetSize);
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
