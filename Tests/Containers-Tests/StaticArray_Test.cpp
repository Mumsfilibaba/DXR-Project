#include "StaticArray_Test.h"

#if RUN_TSTATICARRAY_TEST
#include "TestUtils.h"
#include <Core/Containers/StaticArray.h>

#include <iostream>
#include <array>

bool TStaticArray_Test()
{
    std::cout << std::endl << "----------TStaticArray----------" << std::endl << std::endl;

    CONSTEXPR uint32 Num = 16;
    TStaticArray<int32, Num> Numbers;
    const TStaticArray<int32, Num>& ConstNumbers = Numbers;

    std::cout << "Testing operator[]:" << std::endl;
    for (uint32 i = 0; i < Num; i++)
    {
        Numbers[i] = i;
    }

    for (uint32 i = 0; i < Num; i++)
    {
        std::cout << ConstNumbers[i] << std::endl;
    }

    std::cout << "Testing FirstElement:" << std::endl;
    TEST_CHECK(Numbers.FirstElement()      == 0);
    TEST_CHECK(ConstNumbers.FirstElement() == 0);

    std::cout << "Testing LastElement" << std::endl;
    TEST_CHECK(Numbers.LastElement()      == 15);
    TEST_CHECK(ConstNumbers.LastElement() == 15);

    std::cout << "Testing Size" << std::endl;
    TEST_CHECK(Numbers.Size() == Num);

    std::cout << "Testing Fill" << std::endl;
    Numbers.Fill(5);

    for (uint32 i = 0; i < Num; i++)
    {
        TEST_CHECK(Numbers[i] == 5);
    }

    std::cout << "Testing Range Based For-Loops" << std::endl;

    CONSTEXPR uint32 Num2 = 6;
    TStaticArray<uint32, Num2> Numbers1 = { 5, 6, 7 };
    TEST_CHECK(Numbers1[0] == 5);
    TEST_CHECK(Numbers1[1] == 6);
    TEST_CHECK(Numbers1[2] == 7);

    TStaticArray<uint32, Num2> Numbers2 = { 15, 16, 17 };
    TEST_CHECK(Numbers2[0] == 15);
    TEST_CHECK(Numbers2[1] == 16);
    TEST_CHECK(Numbers2[2] == 17);

    for (const uint32 Number : Numbers1)
    {
        std::cout << Number << std::endl;
    }

    std::cout << "Testing Swap" << std::endl;

    for (const uint32 Number : Numbers2)
    {
        std::cout << Number << std::endl;
    }

    Numbers1.Swap(Numbers2);

    TEST_CHECK(Numbers1.LastElementIndex() == ((Num2 > 0) ? (Num2 - 1) : 0));
    TEST_CHECK(Numbers1.Size()             == Num2);
    TEST_CHECK(Numbers1.SizeInBytes()      == Num2 * sizeof(uint32));

    TEST_CHECK(Numbers1 != Numbers2);

    SUCCESS();
}

#endif
