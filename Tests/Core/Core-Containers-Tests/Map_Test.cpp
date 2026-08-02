#include "Map_Test.h"

#if RUN_TMAP_TEST
#include "TestUtils.h"

#include <Core/Containers/Map.h>
#include <Core/Containers/String.h>

#include <map>

bool TMap_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Add / Size / Find / Contains");
    {
        TMap<int32, int32> Map;
        TEST_EXPECT_EQ(Map.Size(), 0);

        Map.Add(1, 100);
        Map.Add(2, 200);
        Map.Add(3, 300);
        
        TEST_EXPECT_EQ(Map.Size(), 3);

        const int32* Found = Map.Find(2);
        TEST_EXPECT(Found != nullptr);

        if (Found)
        {
            TEST_EXPECT_EQ(*Found, 200);
        }

        TEST_EXPECT(Map.Find(42) == nullptr);
        TEST_EXPECT(Map.Contains(1));
        TEST_EXPECT(!Map.Contains(99));
    }

    TEST_SECTION("operator[] update / insert default");
    {
        TMap<int32, int32> Map;
        Map.Add(2, 200);
        Map[2] = 250;

        TEST_EXPECT_EQ(Map[2], 250);

        const int32 Inserted = Map[10];
        TEST_EXPECT_EQ(Inserted, 0);
        TEST_EXPECT(Map.Contains(10));
    }

    TEST_SECTION("Add(Key) reference / RemoveKey");
    {
        TMap<int32, int32> Map;
        Map.Add(1, 1);
        Map.Add(1) = 111;

        TEST_EXPECT_EQ(Map[1], 111);

        TEST_EXPECT(Map.RemoveKey(1));
        TEST_EXPECT(!Map.Contains(1));
        TEST_EXPECT(!Map.RemoveKey(1));
    }

    TEST_SECTION("Growth / rehash (1000 entries) / Clear");
    {
        TMap<int32, int32> Big;
        for (int32 Index = 0; Index < 1000; ++Index)
        {
            Big.Add(Index, Index * 2);
        }

        TEST_EXPECT_EQ(Big.Size(), 1000);

        bool bAllFound = true;
        for (int32 Index = 0; Index < 1000; ++Index)
        {
            const int32* Value = Big.Find(Index);
            bAllFound = bAllFound && (Value != nullptr) && (*Value == Index * 2);
        }

        TEST_EXPECT(bAllFound);

        Big.Clear();
        TEST_EXPECT_EQ(Big.Size(), 0);
    }

    TEST_SECTION("String keys (non-trivial key hashing)");
    {
        TMap<String, int32> StringMap;
        StringMap.Add(String("one"), 1);
        StringMap.Add(String("two"), 2);

        TEST_EXPECT(StringMap.Contains(String("one")));

        const int32* OneValue = StringMap.Find(String("one"));
        TEST_EXPECT(OneValue != nullptr && *OneValue == 1);
    }

    TEST_SECTION("Emplace / Append / FindOrAdd / Remove");
    {
        TMap<int32, int32> Map;
        Map.Emplace(1, 10);
        Map.Emplace(2); // default-constructed value

        TEST_EXPECT_EQ(Map[1], 10);
        TEST_EXPECT_EQ(Map[2], 0);

        int32& Ref = Map.FindOrAdd(3); // inserts a default then returns a reference
        Ref = 30;

        TEST_EXPECT_EQ(Map[3], 30);
        TEST_EXPECT_EQ(Map.FindOrAdd(3), 30); // existing key keeps its value

        TMap<int32, int32> Other;
        Other.Add(4, 40);
        Other.Add(5, 50);
        Map.Append(Other);

        TEST_EXPECT(Map.Contains(4) && Map.Contains(5));

        Map.Remove(1);
        TEST_EXPECT(!Map.Contains(1));
    }

    TEST_SECTION("GetKeys / GetValues / Foreach / iterators");
    {
        TMap<int32, int32> Map;
        for (int32 Index = 1; Index <= 5; ++Index)
        {
            Map.Add(Index, Index * 10);
        }

        TArray<int32> Keys   = Map.GetKeys();
        TArray<int32> Values = Map.GetValues();

        TEST_EXPECT_EQ(Keys.Size(), 5);
        TEST_EXPECT_EQ(Values.Size(), 5);

        int32 KeySum   = 0;
        int32 ValueSum = 0;

        Map.Foreach([&](const int32& Key, int32& Value)
        {
            KeySum += Key;
            ValueSum += Value;
        });

        TEST_EXPECT_EQ(KeySum, 15);
        TEST_EXPECT_EQ(ValueSum, 150);

        int32 IterValueSum = 0;
        for (auto It = Map.CreateIterator(); !It.IsEnd(); ++It)
        {
            IterValueSum += It.GetValue();
        }

        TEST_EXPECT_EQ(IterValueSum, 150);

        int32 RangeKeySum = 0;
        for (const auto& Pair : Map)
        {
            RangeKeySum += Pair.First;
        }

        TEST_EXPECT_EQ(RangeKeySum, 15);
    }

    TEST_SECTION("operator== / operator!= / copy / move / assignment");
    {
        TMap<int32, int32> First;
        First.Add(1, 1);
        First.Add(2, 2);

        TMap<int32, int32> Second(First);
        TEST_EXPECT(First == Second);

        Second.Add(3, 3);
        TEST_EXPECT(First != Second);

        TMap<int32, int32> Third;
        Third = First;
        TEST_EXPECT(Third == First);

        TMap<int32, int32> Fourth = ::Move(Third);
        TEST_EXPECT(Fourth == First);
    }

    TEST_SECTION("Count / RemoveKey(out) / Reserve / Rehash / load factor");
    {
        TMap<int32, int32> Map;
        Map.Add(1, 100);

        TEST_EXPECT_EQ(Map.Count(1), 1);
        TEST_EXPECT_EQ(Map.Count(2), 0);

        int32 Removed = -1;
        TEST_EXPECT(Map.RemoveKey(1, &Removed));
        TEST_EXPECT_EQ(Removed, 100);
        TEST_EXPECT(!Map.RemoveKey(1, &Removed));

        Map.Reserve(256);
        TEST_EXPECT(Map.BucketCount() >= 1);

        Map.Add(5, 5);
        Map.SetMaxLoadFactor(0.5f);
        Map.Rehash(512);

        TEST_EXPECT(Map.LoadFactor() <= Map.MaxLoadFactor());
        TEST_EXPECT(Map.Contains(5));
    }

    TEST_SECTION("TMap rehash stress (FInstanced values, seeded size sweep vs std::map)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom                 Random(Seed);
            TMap<int32, FInstanced> Map;
            std::map<int32, int32>  Oracle;

            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const int32 Key = static_cast<int32>(Random.RandInt(0, (TargetSize * 2) + 1));
                Map.Add(Key, FInstanced(Key));
                Oracle[Key] = Key;
            }

            if (TargetSize > 16)
            {
                Map.Reserve(TargetSize * 2);
            }

            bool bMatches = (Map.Size() == static_cast<int32>(Oracle.size()));
            for (auto It = Oracle.begin(); bMatches && (It != Oracle.end()); ++It)
            {
                const FInstanced* Value = Map.Find(It->first);
                bMatches = (Value != nullptr) && (Value->GetId() == It->second) && Value->IsPayloadValid();
            }

            if (!bMatches)
            {
                LOG_ERROR("[STRESS FAIL] TMap seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bMatches);
            TEST_EXPECT(FInstanced::LiveCount() == Map.Size());

            // Remove roughly half the keys and re-verify both size and live count.
            const TArray<int32> RemainingKeys = Map.GetKeys();
            for (int32 Index = 0; Index < RemainingKeys.Size(); Index += 2)
            {
                const bool bRemoved = Map.RemoveKey(RemainingKeys[Index]);
                (void)bRemoved;
                Oracle.erase(RemainingKeys[Index]);
            }

            TEST_EXPECT(Map.Size() == static_cast<int32>(Oracle.size()));
            TEST_EXPECT(FInstanced::LiveCount() == Map.Size());
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
