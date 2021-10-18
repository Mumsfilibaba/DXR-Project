#include "SharedPtr_Test.h"

#if RUN_TSHAREDPTR_TEST

#include <Core/Containers/SharedPtr.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Containers/Array.h>

#include <iostream>

/* Helper classes */

struct Base
{
    uint32 X = 0;
};

struct Derived : public Base
{
    uint32 Y = 0;
};

class VirtualBase
{
public:
    virtual ~VirtualBase() = default;
    virtual void Func() = 0;
};

class VirtualDerived : public VirtualBase
{
public:
    virtual void Func() override
    {
    }
};

template<typename RefPointerType>
void PrintRefCountFunc( const RefPointerType& Pointer, const char* Name )
{
    std::cout << Name << " StrongRefCount=" << Pointer.GetStrongRefCount() << ", WeakRefCount=" << Pointer.GetWeakRefCount() << std::endl;
}

#define PrintRefCount( Pointer ) PrintRefCountFunc( Pointer, #Pointer )

/* Test */

void TSharedPtr_Test()
{
    // TSharedPtr
    std::cout << std::endl << "----------TSharedPtr----------" << std::endl << std::endl;

    uint32* Ptr0 = new uint32( 9 );
    uint32* Ptr1 = new uint32( 10 );

    // Test nullptr
    std::cout << std::endl << "----Testing Constructors----" << std::endl << std::endl;
    TSharedPtr<uint32> Null;
    Null = Ptr0; // Takes ownership of Ptr0

    TSharedPtr<uint32> SharedNullPtr = nullptr;

    TSharedPtr<uint32> UintPtr0 = MakeShared<uint32>( 5 );
    PrintRefCount( UintPtr0 );

    TSharedPtr<uint32> UintPtr1 = Null;
    TSharedPtr<uint32> UintPtr2 = TSharedPtr<uint32>( Ptr1 ); // Takes ownership of Ptr1
    PrintRefCount( UintPtr2 );

    std::cout << std::endl << "----Testing StaticCast (Scalar)----" << std::endl << std::endl;
    TSharedPtr<Derived> DerivedPtr0 = MakeShared<Derived>();
    PrintRefCount( DerivedPtr0 );
    TSharedPtr<Base> BasePtr0 = DerivedPtr0;
    PrintRefCount( BasePtr0 );
    TSharedPtr<Base> BasePtr = TSharedPtr<Base>( new Derived() );
    PrintRefCount( BasePtr );
    TSharedPtr<Derived> DerivedPtr1 = StaticCast<Derived>( BasePtr0 );
    PrintRefCount( DerivedPtr1 );
    TSharedPtr<Derived> DerivedPtr3 = StaticCast<Derived>( Move( BasePtr0 ) );
    PrintRefCount( DerivedPtr3 );

    std::cout << std::endl << "----Testing StaticCast (Array)----" << std::endl << std::endl;
    TSharedPtr<Derived[]> DerivedPtr2 = MakeShared<Derived[]>( 5 );
    PrintRefCount( DerivedPtr2 );
    TSharedPtr<Base[]> BasePtr1 = DerivedPtr2;
    PrintRefCount( BasePtr1 );
    TSharedPtr<Base[]> BasePtrArray = TSharedPtr<Base[]>( new Derived[5] );
    PrintRefCount( BasePtrArray );
    TSharedPtr<Derived[]> BasePtr2 = StaticCast<Derived[]>( DerivedPtr2 );
    PrintRefCount( BasePtr2 );
    TSharedPtr<Derived[]> BasePtr3 = StaticCast<Derived[]>( Move( DerivedPtr2 ) );
    PrintRefCount( BasePtr3 );

    std::cout << std::endl << "----Testing ConstCast----" << std::endl << std::endl;
    TSharedPtr<const uint32> ConstPtr0 = MakeShared<const uint32>( 5 );
    std::cout << "ConstPtr0=" << *ConstPtr0 << std::endl;
    PrintRefCount( ConstPtr0 );

    TSharedPtr<uint32> ConstPtr1 = ConstCast<uint32>( ConstPtr0 );
    std::cout << "ConstPtr1=" << *ConstPtr1 << std::endl;
    PrintRefCount( ConstPtr1 );

    std::cout << std::endl << "----Testing ReinterpretCast----" << std::endl << std::endl;
    TSharedPtr<int32> ReintPtr0 = MakeShared<int32>( 5 );
    std::cout << "ReintPtr0=" << *ReintPtr0 << std::endl;
    PrintRefCount( ReintPtr0 );

    TSharedPtr<float> ReintPtr1 = ReinterpretCast<float>( ReintPtr0 );
    std::cout << "ReintPtr1=" << *ReintPtr1 << std::endl;
    PrintRefCount( ReintPtr1 );

    std::cout << std::endl << "----Testing DynamicCast----" << std::endl << std::endl;
    TSharedPtr<VirtualBase> VirtualPtr0 = MakeShared<VirtualDerived>();
    PrintRefCount( VirtualPtr0 );

    TSharedPtr<VirtualDerived> VirtualPtr1 = DynamicCast<VirtualDerived>( VirtualPtr0 );
    PrintRefCount( VirtualPtr1 );

    std::cout << std::endl << "----Testing Operator[]----" << std::endl << std::endl;
    constexpr uint32 Num = 5;
    TSharedPtr<uint32[]> ConstPtr3 = MakeShared<uint32[]>( 5 );
    PrintRefCount( ConstPtr3 );

    std::cout << "ConstPtr3=" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        ConstPtr3[i] = i;
        std::cout << ConstPtr3[i] << std::endl;
    }

    TSharedPtr<const uint32[]> ConstPtr4 = ConstCast<const uint32[]>( ConstPtr3 );
    PrintRefCount( ConstPtr4 );

    std::cout << "ConstPtr4=" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        std::cout << ConstPtr4[i] << std::endl;
    }

    std::cout << std::endl << "----Testing WeakPtr----" << std::endl << std::endl;
    TWeakPtr<Base> WeakBase0 = BasePtr;
    PrintRefCount( WeakBase0 );

    TWeakPtr<Derived> WeakBase1 = DerivedPtr0;
    PrintRefCount( WeakBase1 );

    std::cout << std::endl << "----Testing Equallity----" << std::endl << std::endl;
    std::cout << "operator==(Weak, WeaK): " << std::boolalpha << (WeakBase0 == WeakBase1) << std::endl;
    std::cout << "operator==(Weak, Raw): " << std::boolalpha << (WeakBase0 == WeakBase0.Get()) << std::endl;
    std::cout << "operator==(Raw, Weak): " << std::boolalpha << (WeakBase0.Get() == WeakBase0) << std::endl;

    std::cout << "operator==(Shared, Shared): " << std::boolalpha << (BasePtr == BasePtr) << std::endl;
    std::cout << "operator==(Shared, Raw): " << std::boolalpha << (BasePtr == BasePtr.Get()) << std::endl;
    std::cout << "operator==(Raw, Shared): " << std::boolalpha << (BasePtr.Get() == BasePtr) << std::endl;

    std::cout << "operator==(Weak, Shared): " << std::boolalpha << (WeakBase0 == BasePtr) << std::endl;
    std::cout << "operator==(Shared, Weak): " << std::boolalpha << (BasePtr == WeakBase0) << std::endl;

    std::cout << std::endl << "----Testing Move----" << std::endl << std::endl;
    {
        TSharedPtr<uint32> MovePtr0 = MakeShared<uint32>( 32 );
        PrintRefCount( MovePtr0 );
        std::cout << "MovePtr=" << *MovePtr0 << std::endl;

        TSharedPtr<uint32> MovePtr1 = Move( MovePtr0 );
        PrintRefCount( MovePtr1 );
        std::cout << "MovePtr1=" << *MovePtr1 << std::endl;

        TSharedPtr<uint32[]> MovePtr3 = MakeShared<uint32[]>( Num );
        PrintRefCount( MovePtr3 );

        std::cout << "MovePtr3=" << std::endl;
        for ( uint32 i = 0; i < Num; i++ )
        {
            MovePtr3[i] = i;
            std::cout << MovePtr3[i] << std::endl;
        }

        TSharedPtr<uint32[]> MovePtr4 = Move( MovePtr3 );
        PrintRefCount( MovePtr4 );

        std::cout << "MovePtr4=" << std::endl;
        for ( uint32 i = 0; i < Num; i++ )
        {
            std::cout << MovePtr4[i] << std::endl;
        }
    }

    std::cout << std::endl << "----Testing Array types----" << std::endl << std::endl;
    {
        TSharedPtr<uint32[]> UintArr0 = MakeShared<uint32[]>( 5 );
        TWeakPtr<uint32[]>   WeakArr = UintArr0;
        TSharedPtr<uint32[]> UintArr1 = WeakArr.MakeShared();

        TUniquePtr<uint32[]> UniqueUintArr = MakeUnique<uint32[]>( 5 );

        std::cout << "----Testing Index operator----" << std::endl;
        WeakArr[0] = 5;
        WeakArr[1] = 6;
        std::cout << WeakArr[0] << std::endl;
        std::cout << WeakArr[1] << std::endl;

        std::cout << "----Testing bool operators----" << std::endl;
        std::cout << std::boolalpha << (WeakBase0 == WeakBase1) << std::endl;
        std::cout << std::boolalpha << (UintPtr0 == UintPtr1) << std::endl;
    }

    std::cout << "----Testing Unique to Shared----" << std::endl;
    {
        TUniquePtr<uint32> UniqueInt = MakeUnique<uint32>( 5 );
        TSharedPtr<uint32> UintPtr3 = TSharedPtr<uint32>( Move( UniqueInt ) );
    }

    std::cout << "----Testing UniquePtr (Scalar)----" << std::endl;
    {
        TUniquePtr<uint32> Unique0 = MakeUnique<uint32>( 5 );
        TUniquePtr<uint32> Unique1 = nullptr;
        TUniquePtr<uint32> Unique2 = TUniquePtr( new uint32( 5 ) );
        Unique2.Reset( new uint32( 15 ) );

        std::cout << std::boolalpha << Unique0.IsValid() << std::endl;
        std::cout << "Unique0=" << *Unique0 << std::endl;

        uint32* Raw = Unique0.Release();
        delete Raw;


    }

    std::cout << "----Testing UniquePtr (Array)----" << std::endl;
    {
        TUniquePtr<uint32[]> Unique0 = MakeUnique<uint32[]>( 5 );
        TUniquePtr<uint32[]> Unique1 = nullptr;
        TUniquePtr<uint32[]> Unique2 = TUniquePtr<uint32[]>( new uint32[5] );
        Unique2.Reset( new uint32[15] );

        std::cout << std::boolalpha << Unique0.IsValid() << std::endl;

        std::cout << "Unique0=" << std::endl;
        for ( uint32 i = 0; i < 5; i++ )
        {
            std::cout << Unique0[i] << std::endl;
        }

        uint32* Raw = Unique0.Release();
        delete[] Raw;
    }

    std::cout << "----Testing UniquePtr with TArray----" << std::endl;

    TArray<TUniquePtr<int32>> UniqueArray;
    for ( uint32 i = 0; i < 200;i++)
    {
        UniqueArray.Emplace( MakeUnique<int32>(i) );
    }

    TArray<TUniquePtr<int32>> UniqueArray2 = Move(UniqueArray);
}

#endif
