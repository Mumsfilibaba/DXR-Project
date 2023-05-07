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

template<typename RefPointerType>
void PrintRefCountFunc(const RefPointerType& Pointer, const CHAR* Name)
{
    std::cout << Name << " StrongRefCount=" << Pointer.GetStrongRefCount() << ", WeakRefCount=" << Pointer.GetWeakRefCount() << std::endl;
}

#define PrintRefCount(Pointer) PrintRefCountFunc(Pointer, #Pointer)

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
    TEST_CHECK(Null == nullptr);

    Null = Ptr0; // Takes ownership of Ptr0

    {
        TSharedPtr<uint32> SharedNullPtr = nullptr;

        TSharedPtr<uint32> UintPtr0 = MakeShared<uint32>(5);
        PrintRefCount(UintPtr0);

        TSharedPtr<uint32> UintPtr1 = Null;
        TSharedPtr<uint32> UintPtr2 = TSharedPtr<uint32>(Ptr1); // Takes ownership of Ptr1
        PrintRefCount(UintPtr2);
    }

    std::cout << std::endl << "----Testing StaticCast (Scalar)----" << std::endl << std::endl;
    {
        TSharedPtr<FDerived> DerivedPtr0 = MakeShared<FDerived>();
        PrintRefCount(DerivedPtr0);
        TSharedPtr<FBase> BasePtr0 = DerivedPtr0;
        PrintRefCount(BasePtr0);
        TSharedPtr<FBase> BasePtr = TSharedPtr<FBase>(new FDerived());
        PrintRefCount(BasePtr);
        TSharedPtr<FDerived> DerivedPtr1 = StaticCastSharedPtr<FDerived>(BasePtr0);
        PrintRefCount(DerivedPtr1);
        TSharedPtr<FDerived> DerivedPtr3 = StaticCastSharedPtr<FDerived>(Move(BasePtr0));
        PrintRefCount(DerivedPtr3);
    }

    std::cout << std::endl << "----Testing StaticCast (Array)----" << std::endl << std::endl;
    {
        TSharedPtr<FDerived[]> DerivedPtr2 = MakeShared<FDerived[]>(5);
        PrintRefCount(DerivedPtr2);
        TSharedPtr<FBase[]> BasePtr1 = DerivedPtr2;
        PrintRefCount(BasePtr1);
        TSharedPtr<FBase[]> BasePtrArray = TSharedPtr<FBase[]>(new FDerived[5]);
        PrintRefCount(BasePtrArray);
        TSharedPtr<FDerived[]> BasePtr2 = StaticCastSharedPtr<FDerived[]>(DerivedPtr2);
        PrintRefCount(BasePtr2);
        TSharedPtr<FDerived[]> BasePtr3 = StaticCastSharedPtr<FDerived[]>(Move(DerivedPtr2));
        PrintRefCount(BasePtr3);

        std::cout << std::endl << "----Testing ConstCast----" << std::endl << std::endl;
        TSharedPtr<const uint32> ConstPtr0 = MakeShared<const uint32>(5);
        std::cout << "ConstPtr0=" << *ConstPtr0 << std::endl;
        PrintRefCount(ConstPtr0);

        TSharedPtr<uint32> ConstPtr1 = ConstCastSharedPtr<uint32>(ConstPtr0);
        std::cout << "ConstPtr1=" << *ConstPtr1 << std::endl;
        PrintRefCount(ConstPtr1);

        std::cout << std::endl << "----Testing ReinterpretCast----" << std::endl << std::endl;
        TSharedPtr<int32> ReintPtr0 = MakeShared<int32>(5);
        std::cout << "ReintPtr0=" << *ReintPtr0 << std::endl;
        PrintRefCount(ReintPtr0);

        TSharedPtr<float> ReintPtr1 = ReinterpretCastSharedPtr<float>(ReintPtr0);
        std::cout << "ReintPtr1=" << *ReintPtr1 << std::endl;
        PrintRefCount(ReintPtr1);
    }

    std::cout << std::endl << "----Testing DynamicCast----" << std::endl << std::endl;
    {
        TSharedPtr<FVirtualBase> VirtualPtr0 = MakeShared<FVirtualDerived>();
        PrintRefCount(VirtualPtr0);

        TSharedPtr<FVirtualDerived> VirtualPtr1 = DynamicCastSharedPtr<FVirtualDerived>(VirtualPtr0);
        PrintRefCount(VirtualPtr1);
    }

    constexpr uint32 Num = 5;
    std::cout << std::endl << "----Testing Operator[]----" << std::endl << std::endl;
    {
        TSharedPtr<uint32[]> ConstPtr3 = MakeShared<uint32[]>(5);
        PrintRefCount(ConstPtr3);

        std::cout << "ConstPtr3=" << std::endl;
        for (uint32 i = 0; i < Num; i++)
        {
            ConstPtr3[i] = i;
            std::cout << ConstPtr3[i] << std::endl;
        }

        TSharedPtr<const uint32[]> ConstPtr4 = ConstCastSharedPtr<const uint32[]>(ConstPtr3);
        PrintRefCount(ConstPtr4);

        std::cout << "ConstPtr4=" << std::endl;
        for (uint32 i = 0; i < Num; i++)
        {
            std::cout << ConstPtr4[i] << std::endl;
        }
    }

    std::cout << std::endl << "----Testing WeakPtr----" << std::endl << std::endl;
    {
        TSharedPtr<FDerived> DerivedPtr = MakeShared<FDerived>();
        PrintRefCount(DerivedPtr);
        TSharedPtr<FBase> BasePtr = DerivedPtr;
        PrintRefCount(BasePtr);
        TSharedPtr<uint32> UintPtr0 = MakeShared<uint32>(5);
        PrintRefCount(UintPtr0);
        TSharedPtr<uint32> UintPtr1 = Null;

        TWeakPtr<FBase> WeakBase0 = BasePtr;
        PrintRefCount(WeakBase0);

        TWeakPtr<FDerived> WeakBase1 = DerivedPtr;
        PrintRefCount(WeakBase1);

        std::cout << std::endl << "----Testing Equality----" << std::endl << std::endl;
        std::cout << "operator==(Weak, WeaK): " << std::boolalpha << (WeakBase0 == WeakBase1) << std::endl;
        std::cout << "operator==(Weak, Raw): " << std::boolalpha << (WeakBase0 == WeakBase0.Get()) << std::endl;
        std::cout << "operator==(Raw, Weak): " << std::boolalpha << (WeakBase0.Get() == WeakBase0) << std::endl;

        std::cout << "operator==(Shared, Shared): " << std::boolalpha << (BasePtr == BasePtr) << std::endl;
        std::cout << "operator==(Shared, Raw): " << std::boolalpha << (BasePtr == BasePtr.Get()) << std::endl;
        std::cout << "operator==(Raw, Shared): " << std::boolalpha << (BasePtr.Get() == BasePtr) << std::endl;

        std::cout << "operator==(Weak, Shared): " << std::boolalpha << (WeakBase0 == BasePtr) << std::endl;
        std::cout << "operator==(Shared, Weak): " << std::boolalpha << (BasePtr == WeakBase0) << std::endl;

        std::cout << std::endl << "----Testing Array types----" << std::endl << std::endl;
        TSharedPtr<uint32[]> UintArr0 = MakeShared<uint32[]>(5);
        TWeakPtr<uint32[]>   WeakArr  = UintArr0;
        TSharedPtr<uint32[]> UintArr1 = WeakArr.ToSharedPtr();

        TUniquePtr<uint32[]> UniqueUintArr = MakeUnique<uint32[]>(5);

        std::cout << "----Testing Index operator----" << std::endl;
        WeakArr[0] = 5;
        WeakArr[1] = 6;
        std::cout << WeakArr[0] << std::endl;
        std::cout << WeakArr[1] << std::endl;

        std::cout << "----Testing bool operators----" << std::endl;
        std::cout << std::boolalpha << (WeakBase0 == WeakBase1) << std::endl;
        std::cout << std::boolalpha << (UintPtr0 == UintPtr1) << std::endl;
    }

    std::cout << std::endl << "----Testing Move----" << std::endl << std::endl;
    {
        TSharedPtr<uint32> MovePtr0 = MakeShared<uint32>(32);
        PrintRefCount(MovePtr0);
        std::cout << "MovePtr=" << *MovePtr0 << std::endl;

        TSharedPtr<uint32> MovePtr1 = Move(MovePtr0);
        PrintRefCount(MovePtr1);
        std::cout << "MovePtr1=" << *MovePtr1 << std::endl;

        TSharedPtr<uint32[]> MovePtr3 = MakeShared<uint32[]>(Num);
        PrintRefCount(MovePtr3);

        std::cout << "MovePtr3=" << std::endl;
        for (uint32 i = 0; i < Num; i++)
        {
            MovePtr3[i] = i;
            std::cout << MovePtr3[i] << std::endl;
        }

        TSharedPtr<uint32[]> MovePtr4 = Move(MovePtr3);
        PrintRefCount(MovePtr4);

        std::cout << "MovePtr4=" << std::endl;
        for (uint32 i = 0; i < Num; i++)
        {
            std::cout << MovePtr4[i] << std::endl;
        }
    }

    std::cout << "----Testing Unique to Shared----" << std::endl;
    {
        TUniquePtr<uint32> UniqueInt = MakeUnique<uint32>(5);
        TSharedPtr<uint32> UintPtr3 = TSharedPtr<uint32>(Move(UniqueInt));
    }

    std::cout << "----Testing UniquePtr (Scalar)----" << std::endl;
    {
        TUniquePtr<uint32> Unique0 = MakeUnique<uint32>(5);
        TUniquePtr<uint32> Unique1 = nullptr;
        TUniquePtr<uint32> Unique2 = TUniquePtr(new uint32(5));
        Unique2.Reset(new uint32(15));

        std::cout << std::boolalpha << Unique0.IsValid() << std::endl;
        std::cout << "Unique0=" << *Unique0 << std::endl;

        uint32* Raw = Unique0.Release();
        delete Raw;
    }

    std::cout << "----Testing UniquePtr (Array)----" << std::endl;
    {
        TUniquePtr<uint32[]> Unique0 = MakeUnique<uint32[]>(5);
        TUniquePtr<uint32[]> Unique1 = nullptr;
        TUniquePtr<uint32[]> Unique2 = TUniquePtr<uint32[]>(new uint32[5]);
        Unique2.Reset(new uint32[15]);

        std::cout << std::boolalpha << Unique0.IsValid() << std::endl;

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

        TArray<TUniquePtr<int32>> UniqueArray2 = Move(UniqueArray);
    }

    std::cout << "----Testing TEnableSharedFromThis----" << std::endl;
    {
        class FSharedClass : public TEnableSharedFromThis<FSharedClass>
        {
        public:
            FSharedClass(int32 InValue)
                : Value(InValue)
            {
            }

        private:
            int32 Value;
        };

        static_assert(TIsBaseOf<TEnableSharedFromThis<FSharedClass>, FSharedClass>::Value == true, "TEnableSharedFromThis is not working correctly");

        TSharedPtr<FSharedClass> SharedInstance1;
        {
            TSharedPtr<FSharedClass> SharedInstance0 = MakeShared<FSharedClass>(500);
            SharedInstance1 = SharedInstance0->AsSharedPtr();
        }

        TSharedPtr<FSharedClass> SharedInstance2 = SharedInstance1;
    }

    SUCCESS();
}

#endif
