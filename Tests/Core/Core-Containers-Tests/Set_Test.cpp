#include "Set_Test.h"

#if RUN_TSET_TEST
#include "TestUtils.h"

#include <Core/Containers/Set.h>
#include <Core/Containers/String.h>

#include <set>

bool TSet_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Add (dedup) / Size / IsEmpty / Contains / Find"));
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

    TEST_SECTION(TEXT("RemoveKey"));
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

    TEST_SECTION(TEXT("Growth / rehash (1000 entries) / dedup / Clear"));
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

    TEST_SECTION(TEXT("FString elements (non-trivial element hashing)"));
    {
        TSet<FString> Strings;
        Strings.Add(FString(TEXT("alpha")));
        Strings.Add(FString(TEXT("beta")));
        Strings.Add(FString(TEXT("alpha")));

        TEST_EXPECT_EQ(Strings.Size(), 2);
        TEST_EXPECT(Strings.Contains(FString(TEXT("beta"))));
    }

    TEST_SECTION(TEXT("Add(out) / Emplace / FindOrAdd / Append / Remove"));
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

    TEST_SECTION(TEXT("GetValues / iterators / RemoveKey(out)"));
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

    TEST_SECTION(TEXT("operator== / operator!= / copy / move / Reserve"));
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

    TEST_SECTION(TEXT("Reserve keeps Find pointers until rehash"));
    {
        TSet<int32> Set;
        Set.Reserve(256);
        for (int32 Index = 0; Index < 32; ++Index)
        {
            Set.Add(Index);
        }

        const int32* Pointer = Set.Find(7);
        TEST_EXPECT(Pointer != nullptr);

        for (int32 Index = 32; Index < 64; ++Index)
        {
            Set.Add(Index);
        }

        TEST_EXPECT(Set.Find(7) == Pointer);
        TEST_EXPECT(*Pointer == 7);
    }

    TEST_SECTION(TEXT("TSet rehash stress (FInstanced, seeded size sweep vs std::set)"));
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

    TEST_SECTION(TEXT("Aligned pointers allocated one after another spread over the table's home slots"));
    {
        for (uintptr_t Stride : { uintptr_t(16), uintptr_t(64), uintptr_t(256), uintptr_t(4096) })
        {
            static constexpr uint32 NumSlots = 1024;

            TSet<uint32> Homes;
            for (uint32 Index = 0; Index < NumSlots; ++Index)
            {
                uint8* const Pointer = reinterpret_cast<uint8*>(uintptr_t(0x1D2CA760000) + uintptr_t(Index) * Stride);
                Homes.Add(TDefaultKeyFuncs<uint8*>::GetKeyHash(Pointer) & (NumSlots - 1));
            }

            if (Homes.Size() < static_cast<int32>(NumSlots / 2))
            {
                LOG_ERROR("[STRESS FAIL] %d aligned pointers at stride %llu reached only %d of %u home slots", NumSlots, static_cast<unsigned long long>(Stride), Homes.Size(), NumSlots);
            }

            TEST_EXPECT(Homes.Size() >= static_cast<int32>(NumSlots / 2));
        }
    }

    TEST_SECTION(TEXT("TSet removes every element of a full pointer set in random order"));
    {
        for (uint32 Seed = 1; Seed <= 4; ++Seed)
        {
            for (int32 Count : { 1000, 10000, 50000 })
            {
                for (uintptr_t Stride : { uintptr_t(16), uintptr_t(48), uintptr_t(256), uintptr_t(4096) })
                {
                FRandom Random(Seed);

                TArray<uint8*> Pointers;
                TSet<uint8*>   Set;
                for (int32 Index = 0; Index < Count; ++Index)
                {
                    uint8* const Pointer = reinterpret_cast<uint8*>(uintptr_t(0x1D2CA760000) + uintptr_t(Index) * Stride);
                    Set.Add(Pointer);
                    Pointers.Add(Pointer);
                }

                for (int32 Index = Pointers.Size() - 1; Index > 0; --Index)
                {
                    ::Swap(Pointers[Index], Pointers[static_cast<int32>(Random.RandInt(0, Index))]);
                }

                bool bMatches = true;
                for (int32 Index = 0; Index < Pointers.Size() && bMatches; ++Index)
                {
                    Set.Remove(Pointers[Index]);
                    bMatches = !Set.Contains(Pointers[Index]) && Set.Size() == Pointers.Size() - Index - 1;

                    if (bMatches && (Index % 97) == 0 && Index + 1 < Pointers.Size())
                    {
                        bMatches = Set.Contains(Pointers[Index + 1 + static_cast<int32>(Random.RandInt(0, Pointers.Size() - Index - 2))]);
                    }
                }

                if (!bMatches)
                {
                    LOG_ERROR("[STRESS FAIL] TSet pointer removal seed=%u count=%d stride=%llu", Seed, Count, static_cast<unsigned long long>(Stride));
                }

                TEST_EXPECT(bMatches);
                TEST_EXPECT(Set.IsEmpty());
                }
            }
        }
    }

    TEST_SECTION(TEXT("TSet keeps pointers right through random adds and removes over reused addresses"));
    {
        for (uint32 Seed = 1; Seed <= 4; ++Seed)
        {
            for (uintptr_t Stride : { uintptr_t(16), uintptr_t(64), uintptr_t(256), uintptr_t(4096) })
            {
                FRandom Random(Seed);

                static constexpr int32 PoolSize = 20000;

                TArray<uint8*> Live;
                TArray<uint8*> Free;
                for (int32 Index = 0; Index < PoolSize; ++Index)
                {
                    Free.Add(reinterpret_cast<uint8*>(uintptr_t(0x1D2CA760000) + uintptr_t(Index) * Stride));
                }

                TSet<uint8*> Set;
                bool         bMatches = true;
                for (int32 Step = 0; Step < 400000 && bMatches; ++Step)
                {
                    const bool bAdd = !Free.IsEmpty() && (Live.IsEmpty() || Random.RandInt(0, 99) < 52);

                    TArray<uint8*>& From  = bAdd ? Free : Live;
                    const int32     Index = static_cast<int32>(Random.RandInt(0, From.Size() - 1));
                    uint8* const    Pointer = From[Index];
                    From[Index] = From.Last();
                    From.Pop();

                    if (bAdd)
                    {
                        Set.Add(Pointer);
                        Live.Add(Pointer);
                    }
                    else
                    {
                        Set.Remove(Pointer);
                        Free.Add(Pointer);
                    }

                    bMatches = (Set.Contains(Pointer) == bAdd) && (Set.Size() == Live.Size());
                }

                for (int32 Index = 0; Index < Live.Size() && bMatches; ++Index)
                {
                    bMatches = Set.Contains(Live[Index]);
                }

                if (!bMatches)
                {
                    LOG_ERROR("[STRESS FAIL] TSet pointer churn seed=%u stride=%llu", Seed, static_cast<unsigned long long>(Stride));
                }

                TEST_EXPECT(bMatches);
            }
        }
    }

    TEST_SECTION(TEXT("TInlineSet stays inline then spills to the heap"));
    {
        TInlineSet<int32, 4> Set;
        TEST_EXPECT(!Set.IsHeapAllocated());

        Set.Add(1);
        Set.Add(2);
        Set.Add(2);
        Set.Add(3);
        TEST_EXPECT_EQ(Set.Size(), 3);
        TEST_EXPECT(!Set.IsHeapAllocated());
        TEST_EXPECT(Set.Contains(2));

        for (int32 Index = 0; Index < 32; ++Index)
        {
            Set.Add(Index);
        }

        TEST_EXPECT_EQ(Set.Size(), 32);
        TEST_EXPECT(Set.IsHeapAllocated());
        TEST_EXPECT(Set.Contains(31));

        TInlineSet<int32, 4> Copied = Set;
        TEST_EXPECT(Copied.Contains(10));

        TInlineSet<int32, 4> Moved = Move(Set);
        TEST_EXPECT_EQ(Moved.Size(), 32);
        TEST_EXPECT_EQ(Set.Size(), 0);
        TEST_EXPECT(!Set.IsHeapAllocated());

        TInlineSet<FString, 4> Strings = { FString(TEXT("alpha")), FString(TEXT("beta")), FString(TEXT("alpha")) };
        TEST_EXPECT_EQ(Strings.Size(), 2);
        TEST_EXPECT(!Strings.IsHeapAllocated());
        TEST_EXPECT(Strings.Contains(FString(TEXT("beta"))));

        Moved.Reset();
        TEST_EXPECT(!Moved.IsHeapAllocated());
    }

    TEST_END();
}
#endif
