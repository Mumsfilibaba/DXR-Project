#include "SharedPtr_Test.h"

#if RUN_TSHAREDPTR_TEST
#include "TestUtils.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Containers/SharedRef.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Containers/Array.h>

#include <iostream>

/* Helper classes */

struct FBase
{
    virtual ~FBase() = default;

    uint32 X = 0;
};

struct FDerived : public FBase
{
    uint32 Y = 0;
};

class FVirtualBase
{
public:
    virtual ~FVirtualBase() = default;
    virtual void Func() = 0;

private:
    int64 Value64;
};

class FVirtualDerived : public FVirtualBase
{
public:
    virtual void Func() override { }
};

#define TEST_REF_COUNT(Pointer, StrongReferenceCount, WeakReferenceCount)        \
    TEST_CHECK(((Pointer).GetStrongReferenceCount() == (StrongReferenceCount))); \
    TEST_CHECK(((Pointer).GetWeakReferenceCount()   == (WeakReferenceCount)))    \

/* Test */

struct FRefCountedTest : public IRefCounted
{
public:
    virtual int32 AddRef() override
    {
        CHECK(StrongReferences.Load() > 0);
        ++StrongReferences;
        return StrongReferences.Load();
    }

    virtual int32 Release() override
    {
        const int32 RefCount = --StrongReferences;
        CHECK(RefCount >= 0);

        if (RefCount < 1)
        {
            delete this;
        }

        return RefCount;
    }

    virtual int32 GetRefCount() const override
    {
        return StrongReferences.Load();
    }

private:
    FAtomicInt32 StrongReferences = 1;
};


bool TSharedPtr_Test()
{
    {
        TSharedRef<FRefCountedTest> Test;
        TEST_CHECK(Test.Get() == nullptr);
        
        TSharedRef<FRefCountedTest> Test0 = new FRefCountedTest();
        TEST_CHECK(Test0 != nullptr);
        TEST_CHECK(Test0->GetRefCount() == 1);

        TSharedRef<FRefCountedTest> Test1 = Test0;
        TEST_CHECK(Test1 != nullptr);
        TEST_CHECK(Test1->GetRefCount() == 2);

        TSharedRef<FRefCountedTest> Test2 = Test0;
        TEST_CHECK(Test2 != nullptr);
        TEST_CHECK(Test2->GetRefCount() == 3);

        TSharedRef<FRefCountedTest> Test3 = ::Move(Test2);
        TEST_CHECK(Test2 == nullptr);
        TEST_CHECK(Test3 != nullptr);
        TEST_CHECK(Test3->GetRefCount() == 3);
    }

    // TSharedPtr
    std::cout << std::endl << "----------TSharedPtr----------" << std::endl << std::endl;
    uint32* Ptr0 = new uint32(9);
    uint32* Ptr1 = new uint32(10);

    // Test nullptr
    std::cout << std::endl << "----Testing Constructors----" << std::endl << std::endl;
    TSharedPtr<uint32> Null;
    TEST_CHECK(Null == nullptr);

    TWeakPtr<uint32> NullWeak;
    TEST_CHECK(NullWeak == nullptr);

    Null = Ptr0; // Takes ownership of Ptr0
    TEST_CHECK(Null == Ptr0);

    {
        TSharedPtr<uint32> SharedNullPtr = nullptr;
        TEST_CHECK(SharedNullPtr == nullptr);

        TSharedPtr<uint32> UintPtr0 = MakeShared<uint32>(5);
        TEST_REF_COUNT(UintPtr0, 1, 1);

        TSharedPtr<uint32> UintPtr1 = Null;
        TEST_REF_COUNT(UintPtr1, 2, 1);

        TSharedPtr<uint32> UintPtr2 = TSharedPtr<uint32>(Ptr1); // Takes ownership of Ptr1
        TEST_REF_COUNT(UintPtr2, 1, 1);
    }

    std::cout << std::endl << "----Testing StaticCast (Scalar)----" << std::endl << std::endl;
    {
        TSharedPtr<FDerived> DerivedPtr0 = MakeShared<FDerived>();
        TEST_REF_COUNT(DerivedPtr0, 1, 1);
        TSharedPtr<FBase> BasePtr0 = DerivedPtr0;
        TEST_REF_COUNT(BasePtr0, 2, 1);
        TSharedPtr<FBase> BasePtr = TSharedPtr<FBase>(new FDerived());
        TEST_REF_COUNT(BasePtr, 1, 1);
        TSharedPtr<FDerived> DerivedPtr1 = StaticCastSharedPtr<FDerived>(BasePtr0);
        TEST_REF_COUNT(DerivedPtr1, 3, 1);
        TSharedPtr<FDerived> DerivedPtr3 = StaticCastSharedPtr<FDerived>(::Move(BasePtr0));
        TEST_REF_COUNT(DerivedPtr3, 3, 1);
    }

    std::cout << std::endl << "----Testing StaticCast (Array)----" << std::endl << std::endl;
    {
        TSharedPtr<uint32[]> Integers = MakeShared<uint32[32]>();
        TEST_REF_COUNT(Integers, 1, 1);
        TSharedPtr<FDerived[]> DerivedPtr0 = MakeShared<FDerived[]>(5);
        TEST_REF_COUNT(DerivedPtr0, 1, 1);
        TSharedPtr<FBase[]> BasePtr1 = DerivedPtr0;
        TEST_REF_COUNT(BasePtr1, 2, 1);
        TSharedPtr<FBase[]> BasePtrArray = TSharedPtr<FBase[]>(new FDerived[5]);
        TEST_REF_COUNT(BasePtrArray, 1, 1);
        TSharedPtr<FDerived[]> DerivedPtr1 = StaticCastSharedPtr<FDerived[]>(BasePtr1);
        TEST_REF_COUNT(DerivedPtr1, 3, 1);
        TSharedPtr<FDerived[]> DerivedPtr2 = StaticCastSharedPtr<FDerived[]>(::Move(BasePtr1));
        TEST_REF_COUNT(DerivedPtr2, 3, 1);
    }

    {
        std::cout << std::endl << "----Testing ConstCast----" << std::endl << std::endl;
        TSharedPtr<const uint32> ConstPtr0 = MakeShared<const uint32>(5);
        TEST_CHECK(*ConstPtr0 == 5);
        TEST_REF_COUNT(ConstPtr0, 1, 1);

        TSharedPtr<uint32> ConstPtr1 = ConstCastSharedPtr<uint32>(ConstPtr0);
        TEST_CHECK(*ConstPtr1 == 5);
        TEST_REF_COUNT(ConstPtr1, 2, 1);

        std::cout << std::endl << "----Testing ReinterpretCast----" << std::endl << std::endl;
        TSharedPtr<int32> ReintPtr0 = MakeShared<int32>(1065353216);
        TEST_CHECK(*ReintPtr0 == 1065353216);
        TEST_REF_COUNT(ReintPtr0, 1, 1);

        TSharedPtr<float> ReintPtr1 = ReinterpretCastSharedPtr<float>(ReintPtr0);
        TEST_CHECK(*ReintPtr1 == 1.0f);
        TEST_REF_COUNT(ReintPtr1, 2, 1);
    }

    std::cout << std::endl << "----Testing Deleter----" << std::endl << std::endl;
    {
        struct FMyDeleter
        {
            FMyDeleter()                  = default;
            FMyDeleter(const FMyDeleter&) = default;
            FMyDeleter(FMyDeleter&&)      = default;
            ~FMyDeleter()                 = default;

            FMyDeleter& operator=(const FMyDeleter&) = default;
            FMyDeleter& operator=(FMyDeleter&&)      = default;

            FORCEINLINE void Call(uint32* Pointer) noexcept
            {
                delete Pointer;
            }
        };

        TSharedPtr<uint32> UintPtr = TSharedPtr<uint32>(new uint32(5), FMyDeleter());

        TUniquePtr<uint32, FMyDeleter> UniquePtr = TUniquePtr(new uint32(5), FMyDeleter());
        UintPtr = TSharedPtr<uint32>(::Move(UniquePtr));
    }

    std::cout << std::endl << "----Testing DynamicCast----" << std::endl << std::endl;
    {
        TSharedPtr<FVirtualBase> VirtualPtr0 = MakeShared<FVirtualDerived>();
        TEST_REF_COUNT(VirtualPtr0, 1, 1);

        TSharedPtr<FVirtualDerived> VirtualPtr1 = DynamicCastSharedPtr<FVirtualDerived>(VirtualPtr0);
        TEST_REF_COUNT(VirtualPtr0, 2, 1);
    }

    constexpr uint32 Num = 5;
    std::cout << std::endl << "----Testing Operator[]----" << std::endl << std::endl;
    {
        TSharedPtr<uint32[]> ConstPtr3 = MakeShared<uint32[]>(5);
        TEST_REF_COUNT(ConstPtr3, 1, 1);

        std::cout << "ConstPtr3=" << std::endl;
        auto TempPtr = ConstPtr3.Get();
        for (uint32 i = 0; i < Num; i++)
        {
            ConstPtr3[i] = i;
            TEST_CHECK(TempPtr[i] == ConstPtr3[i]);
        }

        TSharedPtr<const uint32[]> ConstPtr4 = ConstCastSharedPtr<const uint32[]>(ConstPtr3);
        TEST_REF_COUNT(ConstPtr4, 2, 1);

        std::cout << "ConstPtr4=" << std::endl;
        for (uint32 i = 0; i < Num; i++)
        {
            TEST_CHECK(TempPtr[i] == ConstPtr4[i]);
        }
    }

    std::cout << std::endl << "----Testing WeakPtr----" << std::endl << std::endl;
    {
        TSharedPtr<FDerived> DerivedPtr = MakeShared<FDerived>();
        TEST_REF_COUNT(DerivedPtr, 1, 1);
        TSharedPtr<FBase> BasePtr = DerivedPtr;
        TEST_REF_COUNT(BasePtr, 2, 1);

        TSharedPtr<uint32> UintPtr0 = MakeShared<uint32>(5);
        TEST_REF_COUNT(UintPtr0, 1, 1);
        TSharedPtr<uint32> UintPtr1 = Null;
        TEST_REF_COUNT(UintPtr1, 2, 1);

        TWeakPtr<uint32> WeakUintPtr0 = UintPtr0;
        TEST_REF_COUNT(WeakUintPtr0, 1, 2);
        TWeakPtr<uint32> WeakUintPtr1 = UintPtr1;
        TEST_REF_COUNT(WeakUintPtr1, 2, 2);

        TWeakPtr<FBase> WeakBase = BasePtr;
        TEST_REF_COUNT(WeakBase, 2, 2);

        TWeakPtr<FDerived> WeakDerived = DerivedPtr;
        TEST_REF_COUNT(WeakDerived, 2, 3);

        std::cout << std::endl << "----Testing Equality----" << std::endl << std::endl;
        TEST_CHECK((WeakBase       == WeakDerived)       == true);
        TEST_CHECK((WeakBase       == WeakDerived.Get()) == true);
        TEST_CHECK((WeakBase.Get() == WeakDerived)       == true);

        TEST_CHECK((BasePtr       == BasePtr)       == true);
        TEST_CHECK((BasePtr       == BasePtr.Get()) == true);
        TEST_CHECK((BasePtr.Get() == BasePtr)       == true);

        TEST_CHECK((WeakBase == BasePtr)  == true);
        TEST_CHECK((BasePtr  == WeakBase) == true);

        std::cout << std::endl << "----Testing Array types----" << std::endl << std::endl;
        TSharedPtr<uint32[]> UintArr0 = MakeShared<uint32[]>(5);
        TEST_REF_COUNT(UintArr0, 1, 1);

        TWeakPtr<uint32[]> WeakArr = UintArr0;
        TEST_REF_COUNT(UintArr0, 1, 2);

        TSharedPtr<uint32[]> UintArr1 = WeakArr.ToSharedPtr();
        TEST_REF_COUNT(UintArr0, 2, 2);

        TUniquePtr<uint32[]> UniqueUintArr = MakeUnique<uint32[]>(5);

        std::cout << "----Testing Index operator----" << std::endl;
        WeakArr[0] = 5;
        TEST_CHECK(WeakArr[0] == 5);

        WeakArr[1] = 6;
        TEST_CHECK(WeakArr[1] == 6);
    }

    std::cout << std::endl << "----Testing ::Move----" << std::endl << std::endl;
    {
        TSharedPtr<uint32> MovePtr0 = MakeShared<uint32>(32);
        TEST_REF_COUNT(MovePtr0, 1, 1);

        auto TempPtr = MovePtr0.Get();
        TSharedPtr<uint32> MovePtr1 = ::Move(MovePtr0);
        TEST_REF_COUNT(MovePtr1, 1, 1);
        TEST_CHECK(MovePtr1.Get() == TempPtr);

        TSharedPtr<uint32[]> MovePtr3 = MakeShared<uint32[]>(Num);
        TEST_REF_COUNT(MovePtr3, 1, 1);

        for (uint32 i = 0; i < Num; i++)
        {
            MovePtr3[i] = i;
        }

        TempPtr = MovePtr3.Get();
        TSharedPtr<uint32[]> MovePtr4 = ::Move(MovePtr3);
        TEST_REF_COUNT(MovePtr4, 1, 1);

        for (uint32 i = 0; i < Num; i++)
        {
            TEST_CHECK(MovePtr4[i] == TempPtr[i]);
        }
    }

    std::cout << "----Testing Unique to Shared----" << std::endl;
    {
        TUniquePtr<uint32> UniqueInt = MakeUnique<uint32>(5);
        TEST_CHECK(UniqueInt != nullptr);

        TSharedPtr<uint32> UintPtr3 = TSharedPtr<uint32>(::Move(UniqueInt));
        TEST_REF_COUNT(UintPtr3, 1, 1);
        TEST_CHECK(UniqueInt == nullptr);
    }

    std::cout << "----Testing UniquePtr (Scalar)----" << std::endl;
    {
        TUniquePtr<uint32> Unique0 = MakeUnique<uint32>(5);
        TEST_CHECK(*Unique0 == 5);
        
        TUniquePtr<uint32> Unique1 = nullptr;
        TEST_CHECK(Unique1.IsValid() == false);

        TUniquePtr<uint32> Unique2 = TUniquePtr(new uint32(5));
        TEST_CHECK(*Unique2 == 5);
        Unique2.Reset(new uint32(15));
        TEST_CHECK(*Unique2 == 15);

        TEST_CHECK(Unique0.IsValid() == true);

        uint32* Raw = Unique0.Release();
        delete Raw;
    }

    std::cout << "----Testing UniquePtr (Array)----" << std::endl;
    {
        TUniquePtr<uint32[]> Unique0 = MakeUnique<uint32[]>(5);
        TUniquePtr<uint32[]> Unique1 = nullptr;
        TUniquePtr<uint32[]> Unique2 = TUniquePtr<uint32[]>(new uint32[5]);
        Unique2.Reset(new uint32[15]);

        TEST_CHECK(Unique0.IsValid() == true);

        std::cout << "Unique0=" << std::endl;
        for (uint32 i = 0; i < 5; i++)
        {
            std::cout << Unique0[i] << std::endl;
        }

        uint32* Raw = Unique0.Release();
        delete[] Raw;
    }

    std::cout << "----Testing UniquePtr with TArray----" << std::endl;
    {
        TArray<TUniquePtr<int32>> UniqueArray;
        for (uint32 i = 0; i < 200; i++)
        {
            UniqueArray.Emplace(MakeUnique<int32>(i));
        }

        TArray<TUniquePtr<int32>> UniqueArray2 = ::Move(UniqueArray);
    }

    std::cout << "----Testing TSharedFromThis----" << std::endl;
    {
        class FSharedClass : public TSharedFromThis<FSharedClass>
        {
        public:
            FSharedClass(int32 InValue)
                : Value(InValue)
            {
            }

        private:
            int32 Value;
        };

        static_assert(TIsBaseOf<TSharedFromThis<FSharedClass>, FSharedClass>::Value == true, "TSharedFromThis is not working correctly");

        TSharedPtr<FSharedClass> SharedInstance1;
        {
            TSharedPtr<FSharedClass> SharedInstance0 = MakeShared<FSharedClass>(500);
            TEST_REF_COUNT(SharedInstance0, 1, 2);

            SharedInstance1 = SharedInstance0->AsSharedPtr();
            TEST_REF_COUNT(SharedInstance1, 2, 2);
        }

        TSharedPtr<FSharedClass> SharedInstance2 = SharedInstance1;
        TEST_REF_COUNT(SharedInstance1, 2, 2);
    }

    SUCCESS();
}

#endif
