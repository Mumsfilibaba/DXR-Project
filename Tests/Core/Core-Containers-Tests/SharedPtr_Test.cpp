#include "SharedPtr_Test.h"

#if RUN_TSHAREDPTR_TEST
#include "TestUtils.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Containers/SharedRef.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Containers/Array.h>
#include <Core/Threading/Atomic/AtomicInt.h>

namespace
{
    struct FBase
    {
        virtual ~FBase() = default;
        int32 X = 1;
    };

    struct FDerived : public FBase
    {
        int32 Y = 2;
    };

    struct FShared : public TSharedFromThis<FShared>
    {
        int32 Value = 42;
    };

    struct FTestRefCounted : public IRefCounted
    {
        virtual int32 AddRef() const override
        {
            ++Refs;
            return Refs.Load();
        }

        virtual int32 Release() const override
        {
            const int32 Count = --Refs;
            if (Count < 1)
            {
                delete this;
            }

            return Count;
        }

        virtual int32 GetRefCount() const override
        {
            return Refs.Load();
        }

        int32 Tag = 7;
        mutable AtomicInt32 Refs = 1;
    };

    struct FTestRefCountedDerived : public FTestRefCounted
    {
        int32 Extra = 21;
    };
}

bool TSharedPtr_Test()
{
    TEST_BEGIN();

    TEST_SECTION("TSharedPtr::IsValid / operator bool / Get");
    {
        TSharedPtr<int32> Empty;
        TEST_EXPECT(!Empty.IsValid());
        TEST_EXPECT(!static_cast<bool>(Empty));
        TEST_EXPECT(Empty.Get() == nullptr);

        TSharedPtr<int32> Ptr = MakeSharedPtr<int32>(5);
        TEST_EXPECT(Ptr.IsValid());
        TEST_EXPECT(static_cast<bool>(Ptr));
        TEST_EXPECT(*Ptr == 5);
        TEST_EXPECT(*Ptr.Get() == 5);
    }

    TEST_SECTION("TSharedPtr::IsUnique");
    {
        TSharedPtr<int32> Ptr = MakeSharedPtr<int32>(1);
        TEST_EXPECT(Ptr.IsUnique());

        TSharedPtr<int32> Shared = Ptr;
        TEST_EXPECT(!Ptr.IsUnique());
    }

    TEST_SECTION("TSharedPtr::Reset");
    {
        TSharedPtr<int32> Ptr = MakeSharedPtr<int32>(9);
        TEST_EXPECT(Ptr.IsValid());

        Ptr.Reset();
        TEST_EXPECT(!Ptr.IsValid());
    }

    TEST_SECTION("TSharedPtr::Swap");
    {
        TSharedPtr<int32> First = MakeSharedPtr<int32>(1);
        TSharedPtr<int32> Second = MakeSharedPtr<int32>(2);
        First.Swap(Second);
        
        TEST_EXPECT(*First == 2);
        TEST_EXPECT(*Second == 1);
    }

    TEST_SECTION("TSharedPtr assignment operators");
    {
        TSharedPtr<int32> Ptr = MakeSharedPtr<int32>(3);

        TSharedPtr<int32> Copy;
        Copy = Ptr;
        
        TEST_EXPECT(Copy.Get() == Ptr.Get());
        TEST_EXPECT(Ptr.GetStrongReferenceCount() == 2);

        TSharedPtr<int32> Moved;
        Moved = ::Move(Copy);
        
        TEST_EXPECT(Moved.Get() == Ptr.Get());
        TEST_EXPECT(!Copy.IsValid());

        Moved = nullptr;
        TEST_EXPECT(!Moved.IsValid());
    }

    TEST_SECTION("TSharedPtr converting ctor (derived -> base)");
    {
        TSharedPtr<FDerived> Derived = MakeSharedPtr<FDerived>();
        TSharedPtr<FBase> Base = Derived;
        TEST_EXPECT(Base.IsValid());
        TEST_EXPECT(Base.Get() == static_cast<FBase*>(Derived.Get()));
        TEST_EXPECT(Base.GetStrongReferenceCount() == 2);
    }

    TEST_SECTION("TWeakPtr Reset/Swap/IsExpired/IsValid/ToSharedPtr");
    {
        TSharedPtr<int32> Shared = MakeSharedPtr<int32>(11);
        TWeakPtr<int32> Weak = Shared;
        TEST_EXPECT(Weak.IsValid());
        TEST_EXPECT(!Weak.IsExpired());
        TEST_EXPECT(*Weak == 11);

        TSharedPtr<int32> Promoted = Weak.ToSharedPtr();
        TEST_EXPECT(Promoted.Get() == Shared.Get());

        TWeakPtr<int32> Other;
        Weak.Swap(Other);

        TEST_EXPECT(Other.IsValid());
        TEST_EXPECT(!Weak.IsValid());

        Other.Reset();
        TEST_EXPECT(!Other.IsValid());
    }

    TEST_SECTION("TWeakPtr expiration");
    {
        TWeakPtr<int32> Weak;
        {
            TSharedPtr<int32> Shared = MakeSharedPtr<int32>(99);
            Weak = Shared;
            TEST_EXPECT(!Weak.IsExpired());
        }

        TEST_EXPECT(Weak.IsExpired());
        TEST_EXPECT(!Weak.IsValid());
    }

    TEST_SECTION("TSharedFromThis AsSharedPtr/AsWeakPtr");
    {
        TSharedPtr<FShared> Shared = MakeSharedPtr<FShared>();
        TSharedPtr<FShared> FromThis = Shared->AsSharedPtr();
        TEST_EXPECT(FromThis.Get() == Shared.Get());
        TEST_EXPECT(Shared.GetStrongReferenceCount() == 2);

        TWeakPtr<FShared> WeakThis = Shared->AsWeakPtr();
        TEST_EXPECT(WeakThis.IsValid());
        TEST_EXPECT(WeakThis.Get() == Shared.Get());
    }

    TEST_SECTION("TSharedRef MakeSharedRef / Get / IsValid / operator bool");
    {
        TSharedRef<FTestRefCounted> Empty;
        TEST_EXPECT(!Empty.IsValid());
        TEST_EXPECT(!static_cast<bool>(Empty));

        FTestRefCounted* Object = new FTestRefCounted();
        TSharedRef<FTestRefCounted> Ref = MakeSharedRef<FTestRefCounted>(Object);
        TEST_EXPECT(Ref.IsValid());
        TEST_EXPECT(Ref.Get() == Object);
        TEST_EXPECT(Ref->GetRefCount() == 2);

        Object->Release();
        TEST_EXPECT(Ref->GetRefCount() == 1);
    }

    TEST_SECTION("TSharedRef AddRef / Reset / Swap");
    {
        TSharedRef<FTestRefCounted> Ref = new FTestRefCounted();
        TEST_EXPECT(Ref->GetRefCount() == 1);

        Ref.AddRef();
        TEST_EXPECT(Ref->GetRefCount() == 2);
        Ref->Release();

        TSharedRef<FTestRefCounted> Other = new FTestRefCounted();
        Other->Tag = 13;
        Ref.Swap(Other);
        TEST_EXPECT(Ref->Tag == 13);

        Ref.Reset();
        TEST_EXPECT(!Ref.IsValid());
    }

    TEST_SECTION("TSharedRef ReleaseOwnership / GetAs / assignment");
    {
        TSharedRef<FTestRefCounted> Ref = new FTestRefCounted();
        TSharedRef<FTestRefCounted> Copy;
        Copy = Ref;
        TEST_EXPECT(Ref->GetRefCount() == 2);

        FTestRefCounted* Owned = Copy.ReleaseOwnership();
        TEST_EXPECT(!Copy.IsValid());
        TEST_EXPECT(Owned != nullptr);
        Owned->Release();

        TSharedRef<FTestRefCounted> BaseRef = new FTestRefCountedDerived();
        FTestRefCountedDerived* AsDerived = BaseRef.GetAs<FTestRefCountedDerived>();
        TEST_EXPECT(AsDerived == static_cast<FTestRefCountedDerived*>(BaseRef.Get()));
    }

    TEST_SECTION("TSharedRef StaticCastSharedRef");
    {
        TSharedRef<FTestRefCounted> Ref = new FTestRefCounted();
        TSharedRef<IRefCounted> AsBase = StaticCastSharedRef<IRefCounted>(Ref);
        TEST_EXPECT(AsBase.Get() == static_cast<IRefCounted*>(Ref.Get()));
        TEST_EXPECT(Ref->GetRefCount() == 2);
    }

    TEST_SECTION("Cast helpers: Static / Const / Reinterpret (scalar)");
    {
        TSharedPtr<FDerived> Derived = MakeSharedPtr<FDerived>();
        TSharedPtr<FBase> Base = Derived;
        TEST_EXPECT(Base.GetStrongReferenceCount() == 2);

        TSharedPtr<FDerived> BackToDerived = StaticCastSharedPtr<FDerived>(Base);
        TEST_EXPECT(BackToDerived.Get() == Derived.Get());
        TEST_EXPECT(Base.GetStrongReferenceCount() == 3);

        TSharedPtr<FDerived> MovedDerived = StaticCastSharedPtr<FDerived>(::Move(BackToDerived));
        TEST_EXPECT(MovedDerived.Get() == Derived.Get());

        TSharedPtr<const uint32> ConstPtr = MakeSharedPtr<const uint32>(5);
        TSharedPtr<uint32> NonConst = ConstCastSharedPtr<uint32>(ConstPtr);
        TEST_EXPECT(*NonConst == 5);
        TEST_EXPECT(ConstPtr.GetStrongReferenceCount() == 2);

        TSharedPtr<int32> AsInt   = MakeSharedPtr<int32>(1065353216);
        TSharedPtr<float> AsFloat = ReinterpretCastSharedPtr<float>(AsInt);
        TEST_EXPECT(*AsFloat == 1.0f);
    }

    TEST_SECTION("Array TSharedPtr: MakeSharedPtr / operator[] / cast / weak array");
    {
        TSharedPtr<uint32[]> Array = MakeSharedPtr<uint32[]>(5);
        TEST_EXPECT(Array.GetStrongReferenceCount() == 1);
        
        for (uint32 Index = 0; Index < 5; ++Index)
        {
            Array[Index] = Index;
        }

        TEST_EXPECT(Array[0] == 0u);
        TEST_EXPECT(Array[4] == 4u);

        TSharedPtr<const uint32[]> ConstArray = ConstCastSharedPtr<const uint32[]>(Array);
        TEST_EXPECT(ConstArray.GetStrongReferenceCount() == 2);
        TEST_EXPECT(ConstArray[3] == 3u);

        TSharedPtr<FDerived[]> DerivedArr = MakeSharedPtr<FDerived[]>(4);
        TSharedPtr<FBase[]> BaseArr = DerivedArr;
        TEST_EXPECT(BaseArr.GetStrongReferenceCount() == 2);

        TSharedPtr<FDerived[]> BackArr = StaticCastSharedPtr<FDerived[]>(BaseArr);
        TEST_EXPECT(BaseArr.GetStrongReferenceCount() == 3);

        TWeakPtr<uint32[]> WeakArr = Array;
        WeakArr[1] = 6;
        TEST_EXPECT(WeakArr[1] == 6u);
        
        TSharedPtr<uint32[]> Promoted = WeakArr.ToSharedPtr();
        TEST_EXPECT(Promoted.Get() == Array.Get());
    }

    TEST_SECTION("Custom deleter / TUniquePtr -> TSharedPtr");
    {
        struct FMyDeleter
        {
            FORCEINLINE void Call(uint32* Pointer) noexcept
            {
                delete Pointer;
            }
        };

        TSharedPtr<uint32> WithDeleter = TSharedPtr<uint32>(new uint32(5), FMyDeleter());
        TEST_EXPECT(*WithDeleter == 5);

        TUniquePtr<uint32, FMyDeleter> Unique = TUniquePtr<uint32, FMyDeleter>(new uint32(7), FMyDeleter());
        TSharedPtr<uint32> FromUnique = TSharedPtr<uint32>(::Move(Unique));
        TEST_EXPECT(*FromUnique == 7);
    }

    TEST_SECTION("TUniquePtr scalar / array / Reset / Release / IsValid");
    {
        TUniquePtr<uint32> Scalar = MakeUniquePtr<uint32>(5);
        TEST_EXPECT(Scalar.IsValid());
        TEST_EXPECT(*Scalar == 5);
        
        Scalar.Reset(new uint32(15));
        TEST_EXPECT(*Scalar == 15);

        uint32* Released = Scalar.Release();
        TEST_EXPECT(!Scalar.IsValid());
        delete Released;

        TUniquePtr<uint32> Null = nullptr;
        TEST_EXPECT(!Null.IsValid());

        TUniquePtr<uint32[]> ArrayUnique = MakeUniquePtr<uint32[]>(5);
        TEST_EXPECT(ArrayUnique.IsValid());
        
        ArrayUnique[0] = 1;
        TEST_EXPECT(ArrayUnique[0] == 1u);

        uint32* ReleasedArray = ArrayUnique.Release();
        delete[] ReleasedArray;

        TUniquePtr<uint32> UniqueInt        = MakeUniquePtr<uint32>(5);
        TSharedPtr<uint32> SharedFromUnique = TSharedPtr<uint32>(::Move(UniqueInt));

        TEST_EXPECT(SharedFromUnique.IsValid());
        TEST_EXPECT(!UniqueInt.IsValid());
    }

    TEST_SECTION("TUniquePtr inside TArray (move)");
    {
        TArray<TUniquePtr<int32>> UniqueArray;
        for (int32 Index = 0; Index < 64; ++Index)
        {
            UniqueArray.Emplace(MakeUniquePtr<int32>(Index));
        }

        TArray<TUniquePtr<int32>> Moved = ::Move(UniqueArray);
        TEST_EXPECT(Moved.Size() == 64);
        TEST_EXPECT(*Moved[10] == 10);
    }

    TEST_SECTION("Weak/Shared equality operators");
    {
        TSharedPtr<FDerived> Derived = MakeSharedPtr<FDerived>();
        TSharedPtr<FBase>    Base    = Derived;

        TWeakPtr<FBase>    WeakBase    = Base;
        TWeakPtr<FDerived> WeakDerived = Derived;

        TEST_EXPECT((WeakBase == WeakDerived));
        TEST_EXPECT((WeakBase == WeakDerived.Get()));
        TEST_EXPECT((WeakBase.Get() == WeakDerived));
        TEST_EXPECT((Base == Base.Get()));
        TEST_EXPECT((WeakBase == Base));
        TEST_EXPECT((Base == WeakBase));
    }

    TEST_SECTION("TSharedPtr / TUniquePtr lifetime stress (FInstanced, seeded sweep)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom Random(Seed);

            TArray<TSharedPtr<FInstanced>> Shared;
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                Shared.Add(MakeSharedPtr<FInstanced>(Step));
            }

            TEST_EXPECT(FInstanced::LiveCount() == TargetSize);

            // Random aliasing must share ownership rather than create new instances.
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                if (Random.RandBool() && !Shared.IsEmpty())
                {
                    const int32 At = static_cast<int32>(Random.RandInt(0, Shared.Size() - 1));
                    TSharedPtr<FInstanced> Alias = Shared[At];
                    TEST_EXPECT(Alias.GetStrongReferenceCount() >= 2);
                }
            }

            TEST_EXPECT(FInstanced::LiveCount() == TargetSize);

            // Release everything; every managed object must be destroyed exactly once.
            while (!Shared.IsEmpty())
            {
                Shared.RemoveAt(Shared.Size() - 1);
            }

            TEST_EXPECT(FInstanced::LiveCount() == 0);

            // High-volume unique-pointer create/reset cycles.
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                TUniquePtr<FInstanced> Unique = MakeUniquePtr<FInstanced>(Step);
                TEST_EXPECT(Unique.IsValid());
                TEST_EXPECT(Unique->GetId() == Step);
            }

            TEST_EXPECT(FInstanced::LiveCount() == 0);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
