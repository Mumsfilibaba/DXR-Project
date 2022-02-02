#include "Variant_Test.h"

#if RUN_TVARIANT_TEST

#include <Core/Containers/Variant.h>
#include <Core/Memory/Memory.h>
#include <Core/Templates/ClassUtilities.h>

#include <iostream>
#include <variant>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// STest

void TVariant_Test()
{
    TVariant<std::string, int32> Variant0;
    std::cout << "Variant0.IsType<int32>()="       << std::boolalpha << Variant0.IsType<int32>()       << std::endl;
    std::cout << "Variant0.IsType<std::string>()=" << std::boolalpha << Variant0.IsType<std::string>() << std::endl;

    TVariant<std::string, int32> Variant1(TInPlaceType<std::string>(), "Test");
    std::cout << "Variant1.IsType<int32>()="       << std::boolalpha << Variant1.IsType<int32>()       << std::endl;
    std::cout << "Variant1.IsType<std::string>()=" << std::boolalpha << Variant1.IsType<std::string>() << std::endl;

    std::cout << "Variant1.GetValue<std::string>()=" << Variant1.GetValue<std::string>() << std::endl;

    std::cout << "Variant1.TryGetValue<int32>()="       << Variant1.TryGetValue<int32>()       << std::endl;
    std::cout << "Variant1.TryGetValue<std::string>()=" << Variant1.TryGetValue<std::string>() << std::endl;

    TVariant<std::string, int32> Variant2{TInPlaceIndex<0>()};
    TVariant<std::string, int32> Variant3{TInPlaceIndex<1>()};
    std::cout << "Variant2.IsType<int32>()="       << std::boolalpha << Variant2.IsType<int32>()       << std::endl;
    std::cout << "Variant2.IsType<std::string>()=" << std::boolalpha << Variant2.IsType<std::string>() << std::endl;
    std::cout << "Variant3.IsType<int32>()="       << std::boolalpha << Variant3.IsType<int32>()       << std::endl;
    std::cout << "Variant3.IsType<std::string>()=" << std::boolalpha << Variant3.IsType<std::string>() << std::endl;

    std::cout << "Variant2.GetValue<std::string>()=" << Variant2.GetValue<std::string>() << std::endl;
    std::cout << "Variant3.GetValue<int32>()="       << Variant3.GetValue<int32>()       << std::endl;

    std::cout << "Variant2.TryGetValue<int32>()="       << Variant2.TryGetValue<int32>()       << std::endl;
    std::cout << "Variant2.TryGetValue<std::string>()=" << Variant2.TryGetValue<std::string>() << std::endl;

    std::cout << "Variant3.TryGetValue<int32>()="       << Variant3.TryGetValue<int32>()       << std::endl;
    std::cout << "Variant3.TryGetValue<std::string>()=" << Variant3.TryGetValue<std::string>() << std::endl;
    return;
}

#endif