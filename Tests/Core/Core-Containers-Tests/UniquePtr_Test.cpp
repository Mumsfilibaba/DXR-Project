#include "UniquePtr_Test.h"

#if RUN_TUNIQUEPTR_TEST
#include "TestUtils.h"

#include <Core/Containers/UniquePtr.h>
#include <Core/Containers/Array.h>

namespace
{
    struct FDeleterCalls
    {
        static int32& Count()
        {
            static int32 GCount = 0;
            return GCount;
        }
    };

    struct FDeleter
    {
        FORCEINLINE void Call(FInstanced* Pointer) noexcept
        {
            FDeleterCalls::Count()++;
            delete Pointer;
        }
    };
}

bool TUniquePtr_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Default / nullptr / IsValid / Get / dereference");
    {
        TUniquePtr<int32> Empty;
        TEST_EXPECT(!Empty.IsValid());
        TEST_EXPECT(Empty.Get() == nullptr);

        TUniquePtr<int32> Null = nullptr;
        TEST_EXPECT(!Null.IsValid());

        TUniquePtr<int32> Ptr = MakeUniquePtr<int32>(7);
        TEST_EXPECT(Ptr.IsValid());
        TEST_EXPECT(*Ptr == 7);
        TEST_EXPECT(*Ptr.Get() == 7);
    }

    TEST_SECTION("Reset / Release / Swap");
    {
        TUniquePtr<int32> Ptr = MakeUniquePtr<int32>(1);
        Ptr.Reset(new int32(2));
        TEST_EXPECT(*Ptr == 2);

        int32* Released = Ptr.Release();
        TEST_EXPECT(!Ptr.IsValid());
        TEST_EXPECT(Released != nullptr && *Released == 2);
        delete Released;

        TUniquePtr<int32> First  = MakeUniquePtr<int32>(10);
        TUniquePtr<int32> Second = MakeUniquePtr<int32>(20);
        First.Swap(Second);

        TEST_EXPECT(*First == 20);
        TEST_EXPECT(*Second == 10);
    }

    TEST_SECTION("Move construction / move assignment");
    {
        TUniquePtr<int32> Ptr   = MakeUniquePtr<int32>(5);
        TUniquePtr<int32> Moved = ::Move(Ptr);
        TEST_EXPECT(Moved.IsValid());
        TEST_EXPECT(!Ptr.IsValid());
        TEST_EXPECT(*Moved == 5);

        TUniquePtr<int32> Assigned;
        Assigned = ::Move(Moved);

        TEST_EXPECT(Assigned.IsValid());
        TEST_EXPECT(!Moved.IsValid());
        TEST_EXPECT(*Assigned == 5);
    }

    TEST_SECTION("Array unique pointer indexing");
    {
        TUniquePtr<int32[]> Array = MakeUniquePtr<int32[]>(4);
        for (int32 Index = 0; Index < 4; ++Index)
        {
            Array[Index] = Index * 2;
        }

        TEST_EXPECT(Array[0] == 0);
        TEST_EXPECT(Array[3] == 6);
    }

    TEST_SECTION("FInstanced ownership (single destruction, no leaks)");
    {
        FInstanced::Reset();
        {
            TUniquePtr<FInstanced> Owner = MakeUniquePtr<FInstanced>(99);
            TEST_EXPECT(FInstanced::LiveCount() == 1);
            TEST_EXPECT(Owner->GetId() == 99);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_SECTION("Custom deleter is invoked exactly once");
    {
        FInstanced::Reset();
        FDeleterCalls::Count() = 0;
        {
            TUniquePtr<FInstanced, FDeleter> Owner(new FInstanced(1), FDeleter());
            TEST_EXPECT(Owner.IsValid());
        }

        TEST_EXPECT(FDeleterCalls::Count() == 1);
        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_SECTION("TUniquePtr lifetime stress (create / move / reset cycles)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom Random(Seed);

            TArray<TUniquePtr<FInstanced>> Owners;
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                Owners.Emplace(MakeUniquePtr<FInstanced>(Step));
            }

            TEST_EXPECT(FInstanced::LiveCount() == TargetSize);

            // Moving ownership into a second array creates/destroys no instances.
            TArray<TUniquePtr<FInstanced>> Moved = ::Move(Owners);
            TEST_EXPECT(FInstanced::LiveCount() == TargetSize);

            // Resetting a random subset destroys exactly one instance per reset.
            int32 Expected = TargetSize;
            for (int32 Index = 0; Index < Moved.Size(); ++Index)
            {
                if (Random.RandBool() && Moved[Index].IsValid())
                {
                    Moved[Index].Reset();
                    --Expected;
                }
            }

            TEST_EXPECT(FInstanced::LiveCount() == Expected);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
