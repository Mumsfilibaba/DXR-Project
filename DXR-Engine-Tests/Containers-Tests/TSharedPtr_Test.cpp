#include "TSharedPtr_Test.h"

#include <Core/Containers/SharedPtr.h>

#include <iostream>

void TSharedPtr_Test()
{
    // TSharedPtr
    std::cout << std::endl << "----------TSharedPtr----------" << std::endl << std::endl;

    struct Base
    {
        uint32 X;
    };

    struct Derived : public Base
    {
        uint32 Y;
    };

    uint32* Ptr0 = new uint32( 9 );
    uint32* Ptr1 = new uint32( 10 );

    // Test nullptr
    TSharedPtr<uint32> Null;
    Null = Ptr0; // Takes ownership of Ptr0

    TSharedPtr<uint32> UintPtr0 = MakeShared<uint32>( 5 );
    TSharedPtr<uint32> UintPtr1 = Null;
    TSharedPtr<uint32> UintPtr2 = TSharedPtr<uint32>( Ptr1 ); // Takes ownership of Ptr1

    std::cout << "Testing casting" << std::endl;
    TSharedPtr<Derived> DerivedPtr0 = MakeShared<Derived>();
    TSharedPtr<Base> BasePtr0 = DerivedPtr0;
    TSharedPtr<Derived> DerivedPtr1 = StaticCast<Derived>( BasePtr0 );
    TSharedPtr<Derived> DerivedPtr3 = StaticCast<Derived>( Move( BasePtr0 ) );

    TSharedPtr<Derived[]> DerivedPtr2 = MakeShared<Derived[]>( 5 );
    TSharedPtr<Base[]> BasePtr1 = DerivedPtr2;
    TSharedPtr<Derived[]> BasePtr2 = StaticCast<Derived[]>( DerivedPtr2 );
    TSharedPtr<Derived[]> BasePtr3 = StaticCast<Derived[]>( Move( DerivedPtr2 ) );

    TSharedPtr<const uint32> ConstPtr0 = MakeShared<const uint32>( 5 );
    std::cout << "ConstPtr0=" << *ConstPtr0 << std::endl;
    TSharedPtr<uint32> ConstPtr1 = ConstCast<uint32>( ConstPtr0 );
    std::cout << "ConstPtr1=" << *ConstPtr1 << std::endl;

    constexpr uint32 Num = 5;
    TSharedPtr<uint32[]> ConstPtr3 = MakeShared<uint32[]>( 5 );
    std::cout << "ConstPtr3=" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        ConstPtr3[i] = i;
        std::cout << ConstPtr3[i] << std::endl;
    }

    TSharedPtr<const uint32[]> ConstPtr4 = ConstCast<const uint32[]>( ConstPtr3 );
    std::cout << "ConstPtr4=" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        std::cout << ConstPtr4[i] << std::endl;
    }

    TWeakPtr<Base> WeakBase0 = BasePtr0;
    TWeakPtr<Derived> WeakBase1 = DerivedPtr0;

    std::cout << "Testing Move" << std::endl;
    TSharedPtr<uint32> MovePtr0 = MakeShared<uint32>( 32 );
    std::cout << "MovePtr=" << *MovePtr0 << std::endl;
    TSharedPtr<uint32> MovePtr1 = Move( MovePtr0 );
    std::cout << "MovePtr1=" << *MovePtr1 << std::endl;

    TSharedPtr<uint32[]> MovePtr3 = MakeShared<uint32[]>( Num );
    std::cout << "MovePtr3=" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        MovePtr3[i] = i;
        std::cout << MovePtr3[i] << std::endl;
    }

    TSharedPtr<uint32[]> MovePtr4 = Move( MovePtr3 );
    std::cout << "MovePtr4=" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        std::cout << MovePtr4[i] << std::endl;
    }

    std::cout << "Testing Array types" << std::endl;
    TSharedPtr<uint32[]> UintArr0 = MakeShared<uint32[]>( 5 );
    TWeakPtr<uint32[]> WeakArr = UintArr0;
    TSharedPtr<uint32[]> UintArr1 = WeakArr.MakeShared();

    TUniquePtr<uint32> UniqueInt = MakeUnique<uint32>( 5 );
    TSharedPtr<uint32> UintPtr3 = TSharedPtr<uint32>( Move( UniqueInt ) );

    TUniquePtr<uint32[]> UniqueUintArr = MakeUnique<uint32[]>( 5 );

    std::cout << "Testing Index operator" << std::endl;
    WeakArr[0] = 5;
    WeakArr[1] = 6;
    std::cout << WeakArr[0] << std::endl;
    std::cout << WeakArr[1] << std::endl;

    std::cout << "Testing bool operators" << std::endl;
    std::cout << std::boolalpha << (WeakBase0 == WeakBase1) << std::endl;
    std::cout << std::boolalpha << (UintPtr0 == UintPtr1) << std::endl;
}
