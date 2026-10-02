#include "MultiMap_Test.h"

#if RUN_TMULTIMAP_TEST
#include "TestUtils.h"

#include <Core/Containers/MultiMap.h>
#include <Core/Containers/String.h>

#include <map>

bool TMultiMap_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Add duplicates / Size / Find / Count / Contains");
    {
        TMultiMap<int32, int32> Map;
        TEST_EXPECT_EQ(Map.Size(), 0);

        Map.Add(1, 10);
        Map.Add(1, 20);
        Map.Add(2, 30);

        TEST_EXPECT_EQ(Map.Size(), 3);
        TEST_EXPECT(Map.Contains(1));
        TEST_EXPECT(!Map.Contains(99));
        TEST_EXPECT_EQ(Map.Count(1), 2);
        TEST_EXPECT_EQ(Map.Count(2), 1);
        TEST_EXPECT_EQ(Map.Count(3), 0);

        const int32* Found = Map.Find(1);
        TEST_EXPECT(Found != nullptr);
        TEST_EXPECT(*Found == 10 || *Found == 20);
    }

    TEST_SECTION("MultiFind / FindNext");
    {
        TMultiMap<int32, int32> Map;
        Map.Add(1, 10);
        Map.Add(1, 20);
        Map.Add(1, 30);
        Map.Add(2, 40);

        TArray<int32> Values;
        Map.MultiFind(1, Values);
        TEST_EXPECT_EQ(Values.Size(), 3);

        int32 Sum = 0;
        const int32* Current = Map.Find(1);
        TEST_EXPECT(Current != nullptr);
        while (Current)
        {
            Sum += *Current;
            Current = Map.FindNext(1, Current);
        }

        TEST_EXPECT_EQ(Sum, 60);
    }

    TEST_SECTION("Remove all vs RemoveSingle");
    {
        TMultiMap<int32, int32> Map;
        Map.Add(1, 10);
        Map.Add(1, 20);
        Map.Add(1, 20);
        Map.Add(2, 30);

        TEST_EXPECT(Map.RemoveSingle(1, 20));
        TEST_EXPECT_EQ(Map.Count(1), 2);
        TEST_EXPECT_EQ(Map.Size(), 3);

        TEST_EXPECT_EQ(Map.Remove(1), 2);
        TEST_EXPECT(!Map.Contains(1));
        TEST_EXPECT(Map.Contains(2));
        TEST_EXPECT_EQ(Map.Size(), 1);
    }

    TEST_SECTION("GetKeys / GetValues / Foreach / iterators");
    {
        TMultiMap<int32, int32> Map;
        Map.Add(1, 10);
        Map.Add(1, 11);
        Map.Add(2, 20);

        TEST_EXPECT_EQ(Map.GetKeys().Size(), 3);
        TEST_EXPECT_EQ(Map.GetValues().Size(), 3);

        int32 ValueSum = 0;
        Map.Foreach([&](const int32&, int32& Value)
        {
            ValueSum += Value;
        });
        TEST_EXPECT_EQ(ValueSum, 41);

        int32 IterSum = 0;
        for (auto It = Map.CreateIterator(); !It.IsEnd(); ++It)
        {
            IterSum += It.GetValue();
        }
        TEST_EXPECT_EQ(IterSum, 41);

        int32 RangeSum = 0;
        for (const auto& Pair : Map)
        {
            RangeSum += Pair.Second;
        }
        TEST_EXPECT_EQ(RangeSum, 41);
    }

    TEST_SECTION("copy / move / operator==");
    {
        TMultiMap<int32, int32> First;
        First.Add(1, 10);
        First.Add(1, 20);

        TMultiMap<int32, int32> Second(First);
        TEST_EXPECT(First == Second);

        Second.Add(2, 30);
        TEST_EXPECT(First != Second);

        TMultiMap<int32, int32> Third = ::Move(First);
        TEST_EXPECT_EQ(Third.Size(), 2);
        TEST_EXPECT_EQ(Third.Count(1), 2);
    }

    TEST_SECTION("String keys");
    {
        TMultiMap<String, int32> Map;
        Map.Add(String("a"), 1);
        Map.Add(String("a"), 2);
        TEST_EXPECT_EQ(Map.Count(String("a")), 2);
    }

    TEST_SECTION("TMultiMap stress vs std::multimap");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom                      Random(Seed);
            TMultiMap<int32, FInstanced> Map;
            std::multimap<int32, int32>  Oracle;

            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const int32 Key = static_cast<int32>(Random.RandInt(0, (TargetSize / 2) + 1));
                Map.Add(Key, FInstanced(Step));
                Oracle.insert({ Key, Step });
            }

            bool bMatches = (Map.Size() == static_cast<int32>(Oracle.size()));
            for (auto It = Oracle.begin(); bMatches && (It != Oracle.end()); )
            {
                const int32 Key   = It->first;
                const int32 Count = static_cast<int32>(Oracle.count(Key));
                bMatches = (Map.Count(Key) == Count);
                while (It != Oracle.end() && It->first == Key)
                {
                    ++It;
                }
            }

            if (!bMatches)
            {
                LOG_ERROR("[STRESS FAIL] TMultiMap seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bMatches);
            TEST_EXPECT(FInstanced::LiveCount() == Map.Size());

            Map.Clear();
            TEST_EXPECT(FInstanced::LiveCount() == 0);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_SECTION("TInlineMultiMap stays inline then spills to the heap");
    {
        TInlineMultiMap<int32, int32, 4> Map;
        TEST_EXPECT(!Map.IsHeapAllocated());

        Map.Add(1, 10);
        Map.Add(1, 20);
        Map.Add(2, 30);
        TEST_EXPECT_EQ(Map.Size(), 3);
        TEST_EXPECT(!Map.IsHeapAllocated());
        TEST_EXPECT_EQ(Map.Count(1), 2);
        TEST_EXPECT_EQ(*Map.Find(2), 30);

        for (int32 Index = 0; Index < 32; ++Index)
        {
            Map.Add(Index, Index);
        }

        TEST_EXPECT_EQ(Map.Size(), 35);
        TEST_EXPECT(Map.IsHeapAllocated());
        TEST_EXPECT_EQ(Map.Count(1), 3);

        TInlineMultiMap<int32, int32, 4> Copied = Map;
        TEST_EXPECT_EQ(Copied.Size(), 35);

        TInlineMultiMap<int32, int32, 4> Moved = Move(Map);
        TEST_EXPECT_EQ(Moved.Size(), 35);
        TEST_EXPECT_EQ(Map.Size(), 0);
        TEST_EXPECT(!Map.IsHeapAllocated());

        TArray<int32> Values;
        Moved.MultiFind(1, Values);
        TEST_EXPECT_EQ(Values.Size(), 3);

        Moved.Reset();
        TEST_EXPECT(!Moved.IsHeapAllocated());
    }

    TEST_END();
}
#endif
