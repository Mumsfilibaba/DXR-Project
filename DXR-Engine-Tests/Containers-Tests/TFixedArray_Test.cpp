#include "TFixedArray_Test.h"

#include <Core/Containers/FixedArray.h>

#include <iostream>
#include <array>

void TStaticArray_Test()
{
    std::cout << std::endl << "----------TFixedArray----------" << std::endl << std::endl;

    constexpr uint32 Num = 16;
    TFixedArray<int32, Num> Numbers;
    const TFixedArray<int32, Num>& ConstNumbers = Numbers;

    std::cout << "Testing At()" << std::endl;
    for (uint32 i = 0; i < Num; i++)
    {
        Numbers.At(i) = i;
    }
    
    for (uint32 i = 0; i < Num; i++)
    {
        std::cout << ConstNumbers.At(i) << std::endl;
    }
    
    std::cout << "Testing operator[]" << std::endl;
    for (uint32 i = 0; i < Num; i++)
    {
        Numbers[i] = Num + i;
    }
    
    for (uint32 i = 0; i < Num; i++)
    {
        std::cout << ConstNumbers[i] << std::endl;
    }
    
    std::cout << "Testing FirstElement" << std::endl;
    std::cout << "[0]:" << Numbers.FirstElement() << std::endl;
    std::cout << "[0]:" << Numbers.FirstElement() << std::endl;
    
    std::cout << "Testing LastElement" << std::endl;
    std::cout << "[" << Num-1 << "]:" << Numbers.LastElement() << std::endl;
    std::cout << "[" << Num-1 << "]:" << Numbers.LastElement() << std::endl;
    
    std::cout << "Testing Size" << std::endl;
    std::cout << "Size:" << Numbers.Size() << std::endl;
    
    std::cout << "Testing Fill" << std::endl;
    Numbers.Fill(5);
    
    for (uint32 i = 0; i < Num; i++)
    {
        std::cout << Numbers[i] << std::endl;
    }
    
    std::cout << "Testing Range Based For-Loops" << std::endl;
    
    constexpr uint32 Num2 = 6;
    TFixedArray<uint32, Num2> Numbers1 = { 5, 6, 7 };
    
    for (const uint32 Number : Numbers1)
    {
        std::cout << Number << std::endl;
    }
}
