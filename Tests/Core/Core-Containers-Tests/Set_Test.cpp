#include "Set_Test.h"

#if RUN_TSET_TEST
#include "TestUtils.h"

#include <Core/Containers/Set.h>
#include <Core/Containers/String.h>

#include <set>

bool TSet_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Add (dedup) / Size / IsEmpty / Contains / Find");
    {
        TSet<int32> Set;
        TEST_EXPECT_EQ(Set.Size(), 0);
        TEST_EXPECT(Set.IsEmpty());

        Set.Add(1);
        Set.Add(2);
        Set.Add(2); // duplicate, should not grow
        Set.Add(3);

        TEST_EXPECT_EQ(Set.Size(), 3);
        TEST_EXPECT(!Set.IsEmpty());

        TEST_EXPECT(Set.Contains(2));
        TEST_EXPECT(!Set.Contains(99));

        const int32* Found = Set.Find(3);
        TEST_EXPECT(Found != nullptr && *Found == 3);
        TEST_EXPECT(Set.Find(99) == nullptr);
    }

    TEST_SECTION("RemoveKey");
    {
        TSet<int32> Set;
        Set.Add(1);
        Set.Add(2);
        Set.Add(3);

        TEST_EXPECT(Set.RemoveKey(2));
        TEST_EXPECT(!Set.Contains(2));
        TEST_EXPECT_EQ(Set.Size(), 2);
        TEST_EXPECT(!Set.RemoveKey(2));
    }

    TEST_SECTION("Growth / rehash (1000 entries) / dedup / Clear");
    {
        TSet<int32> Big;
        for (int32 Index = 0; Index < 1000; ++Index)
        {
            Big.Add(Index);
        }

        TEST_EXPECT_EQ(Big.Size(), 1000);

        bool bAllFound = true;
        for (int32 Index = 0; Index < 1000; ++Index)
        {
            bAllFound = bAllFound && Big.Contains(Index);
        }

        TEST_EXPECT(bAllFound);

        Big.Add(500); // existing element, no growth
        TEST_EXPECT_EQ(Big.Size(), 1000);

        Big.Clear();
        TEST_EXPECT(Big.IsEmpty());
    }

    TEST_SECTION("String elements (non-trivial element hashing)");
    {
        TSet<String> Strings;
        Strings.Add(String("alpha"));
        Strings.Add(String("beta"));
        Strings.Add(String("alpha"));

        TEST_EXPECT_EQ(Strings.Size(), 2);
        TEST_EXPECT(Strings.Contains(String("beta")));
    }

    TEST_SECTION("Add(out) / Emplace / FindOrAdd / Append / Remove");
    {
        TSet<int32> Set;
        
        bool bAlready = true;
        Set.Add(1, &bAlready);
        TEST_EXPECT(!bAlready);

        Set.Add(1, &bAlready);
        TEST_EXPECT(bAlready);

        Set.Emplace(2);
        TEST_EXPECT(Set.Contains(2));

        const int32& Found = Set.FindOrAdd(3);
        TEST_EXPECT_EQ(Found, 3);

        TSet<int32> Other;
        Other.Add(4);
        Other.Add(5);
        Set.Append(Other);
        TEST_EXPECT(Set.Contains(4) && Set.Contains(5));

        Set.Remove(1);
        TEST_EXPECT(!Set.Contains(1));
    }

    TEST_SECTION("GetValues / iterators / RemoveKey(out)");
    {
        TSet<int32> Set;
        for (int32 Index = 1; Index <= 5; ++Index)
        {
            Set.Add(Index);
        }

        TArray<int32> Values = Set.GetValues();
        TEST_EXPECT_EQ(Values.Size(), 5);

        int32 IterSum = 0;
        for (auto It = Set.CreateIterator(); !It.IsEnd(); ++It)
        {
            IterSum += It.GetElement();
        }

        TEST_EXPECT_EQ(IterSum, 15);

        int32 RangeSum = 0;
        for (const int32& Value : Set)
        {
            RangeSum += Value;
        }

        TEST_EXPECT_EQ(RangeSum, 15);

        int32 Removed = -1;
        TEST_EXPECT(Set.RemoveKey(3, &Removed));
        TEST_EXPECT_EQ(Removed, 3);
        TEST_EXPECT(!Set.RemoveKey(3, &Removed));
    }

    TEST_SECTION("operator== / operator!= / copy / move / Reserve");
    {
        TSet<int32> First;
        First.Add(1);
        First.Add(2);

        TSet<int32> Second(First);
        TEST_EXPECT(First == Second);
        
        Second.Add(3);
        TEST_EXPECT(First != Second);

        TSet<int32> Third;
        Third = First;
        TEST_EXPECT(Third == First);

        TSet<int32> Fourth = ::Move(Third);
        TEST_EXPECT(Fourth == First);

        First.Reserve(128);
        TEST_EXPECT(First.BucketCount() >= 1);
    }

    TEST_SECTION("TSet rehash stress (FInstanced, seeded size sweep vs std::set)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom          Random(Seed);
            TSet<FInstanced> Set;
            std::set<int32>  Oracle;

            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const int32 Id = static_cast<int32>(Random.RandInt(0, (TargetSize * 2) + 1));
                Set.Add(FInstanced(Id));
                Oracle.insert(Id);
            }

            if (TargetSize > 16)
            {
                Set.Reserve(TargetSize * 2); // explicit mid-life rehash
            }

            bool bMatches = (Set.Size() == static_cast<int32>(Oracle.size()));
            for (auto It = Oracle.begin(); bMatches && (It != Oracle.end()); ++It)
            {
                const FInstanced* Element = Set.Find(FInstanced(*It));
                bMatches = (Element != nullptr) && (Element->GetId() == *It) && Element->IsPayloadValid();
            }

            if (!bMatches)
            {
                LOG_ERROR("[STRESS FAIL] TSet seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bMatches);
            TEST_EXPECT(FInstanced::LiveCount() == Set.Size());

            // Remove roughly half the elements (scoped copy so it does not perturb the live count check).
            {
                const TArray<FInstanced> Values = Set.GetValues();
                for (int32 Index = 0; Index < Values.Size(); Index += 2)
                {
                    const bool bRemoved = Set.RemoveKey(Values[Index]);
                    (void)bRemoved;
                    Oracle.erase(Values[Index].GetId());
                }
            }

            TEST_EXPECT(Set.Size() == static_cast<int32>(Oracle.size()));
            TEST_EXPECT(FInstanced::LiveCount() == Set.Size());
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
