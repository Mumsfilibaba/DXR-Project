#include "FixedArray_Test.h"

#if RUN_TFIXEDARRAY_TEST
#include <Core/Containers/StaticArray.h>

#include <iostream>
#include <array>

void TFixedArray_Test()
{
    std::cout << std::endl << "----------TStaticArray----------" << std::endl << std::endl;

    constexpr uint32 Num = 16;
    TStaticArray<int32, Num> Numbers;
    const TStaticArray<int32, Num>& ConstNumbers = Numbers;

    std::cout << "Testing At()" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        Numbers.At( i ) = i;
    }

    for ( uint32 i = 0; i < Num; i++ )
    {
        std::cout << ConstNumbers.At( i ) << std::endl;
    }

    std::cout << "Testing operator[]" << std::endl;
    for ( uint32 i = 0; i < Num; i++ )
    {
        Numbers[i] = Num + i;
    }

    for ( uint32 i = 0; i < Num; i++ )
    {
        std::cout << ConstNumbers[i] << std::endl;
    }

    std::cout << "Testing FirstElement" << std::endl;
    std::cout << "[0]:" << Numbers.FirstElement() << std::endl;
    std::cout << "[0]:" << Numbers.FirstElement() << std::endl;

    std::cout << "Testing LastElement" << std::endl;
    std::cout << "[" << Num - 1 << "]:" << Numbers.LastElement() << std::endl;
    std::cout << "[" << Num - 1 << "]:" << Numbers.LastElement() << std::endl;

    std::cout << "Testing Size" << std::endl;
    std::cout << "Size:" << Numbers.Size() << std::endl;

    std::cout << "Testing Fill" << std::endl;
    Numbers.Fill( 5 );

    for ( uint32 i = 0; i < Num; i++ )
    {
        std::cout << Numbers[i] << std::endl;
    }

    std::cout << "Testing Range Based For-Loops" << std::endl;

    constexpr uint32 Num2 = 6;
    TStaticArray<uint32, Num2> Numbers1 = { 5, 6, 7 };
    TStaticArray<uint32, Num2> Numbers2 = { 15, 16, 17 };

    for ( const uint32 Number : Numbers1 )
    {
        std::cout << Number << std::endl;
    }

    std::cout << "Testing Swap" << std::endl;

    for ( const uint32 Number : Numbers2 )
    {
        std::cout << Number << std::endl;
    }

    Numbers1.Swap( Numbers2 );

    std::cout << "LastIndex=" << Numbers1.LastElementIndex();
    std::cout << "Size=" << Numbers1.Size();
    std::cout << "SizeInBytes=" << Numbers1.SizeInBytes();

    std::cout << "operator== : " << std::boolalpha << (Numbers1 == Numbers2) << std::endl;
    std::cout << "operator!= : " << std::boolalpha << (Numbers1 != Numbers2) << std::endl;
}

#endif
