#include "Variant_Test.h"

#if RUN_TVARIANT_TEST

#include <Core/Containers/Variant.h>
#include <Core/Memory/Memory.h>

#include <iostream>
#include <variant>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// STest

void TVariant_Test()
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Basic

    {
        TVariant<std::string, int32> Variant;
        std::cout << "Variant.IsType<int32>()="       << std::boolalpha << Variant.IsType<int32>()       << '\n';
        std::cout << "Variant.IsType<std::string>()=" << std::boolalpha << Variant.IsType<std::string>() << '\n';
    }
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TInPlaceType
    
    {
        TVariant<std::string, int32> Variant(TInPlaceType<std::string>(), "Test");
        std::cout << "Variant.IsType<int32>()="       << std::boolalpha << Variant.IsType<int32>()       << '\n';
        std::cout << "Variant.IsType<std::string>()=" << std::boolalpha << Variant.IsType<std::string>() << '\n';

        std::cout << "Variant.GetValue<std::string>()=" << Variant.GetValue<std::string>() << '\n';
        
        const auto& VariantRef = Variant;
        std::cout << "Variant.GetValue<std::string>()=" << VariantRef.GetValue<std::string>() << '\n';

        std::cout << "Variant.TryGetValue<int32>()="       << Variant.TryGetValue<int32>()       << '\n';
        std::cout << "Variant.TryGetValue<std::string>()=" << Variant.TryGetValue<std::string>() << '\n';

		std::cout << "Variant.TryGetValue<int32>()="       << VariantRef.TryGetValue<int32>()       << '\n';
		std::cout << "Variant.TryGetValue<std::string>()=" << VariantRef.TryGetValue<std::string>() << '\n';
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TInPlaceIndex

    {
        TVariant<std::string, int32> Variant0{TInPlaceIndex<0>()};
        TVariant<std::string, int32> Variant1{TInPlaceIndex<1>()};
        std::cout << "Variant0.IsType<int32>()="       << std::boolalpha << Variant0.IsType<int32>()       << '\n';
        std::cout << "Variant0.IsType<std::string>()=" << std::boolalpha << Variant0.IsType<std::string>() << '\n';
        std::cout << "Variant1.IsType<int32>()="       << std::boolalpha << Variant1.IsType<int32>()       << '\n';
        std::cout << "Variant1.IsType<std::string>()=" << std::boolalpha << Variant1.IsType<std::string>() << '\n';

        std::cout << "Variant0.GetValue<std::string>()=" << Variant0.GetValue<std::string>() << '\n';
        std::cout << "Variant1.GetValue<int32>()="       << Variant1.GetValue<int32>()       << '\n';

        std::cout << "Variant0.TryGetValue<int32>()="       << Variant0.TryGetValue<int32>()       << '\n';
        std::cout << "Variant0.TryGetValue<std::string>()=" << Variant0.TryGetValue<std::string>() << '\n';

        std::cout << "Variant1.TryGetValue<int32>()="       << Variant1.TryGetValue<int32>()       << '\n';
        std::cout << "Variant1.TryGetValue<std::string>()=" << Variant1.TryGetValue<std::string>() << '\n';
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Swap

    {
        TVariant<std::string, int32> Variant0(TInPlaceType<std::string>(), "String Number One which equals 11111111111111111111111111111111111111111111111111111111");
        TVariant<std::string, int32> Variant1(TInPlaceType<std::string>(), "String Number Two which equals 22222222222222222222222222222222222222222222222222222222");

        std::cout << "Variant0.GetValue<std::string>()=" << Variant0.GetValue<std::string>() << '\n';
        std::cout << "Variant1.GetValue<std::string>()=" << Variant1.GetValue<std::string>() << '\n';

        Variant0.Swap(Variant1);

        std::cout << "Variant0.GetValue<std::string>()=" << Variant0.GetValue<std::string>() << '\n';
        std::cout << "Variant1.GetValue<std::string>()=" << Variant1.GetValue<std::string>() << '\n';

        Variant0.Swap(Variant1);

        std::cout << "Variant0.GetValue<std::string>()=" << Variant0.GetValue<std::string>() << '\n';
        std::cout << "Variant1.GetValue<std::string>()=" << Variant1.GetValue<std::string>() << '\n';

        Variant0.Swap(Variant1);

        std::cout << "Variant0.GetValue<std::string>()=" << Variant0.GetValue<std::string>() << '\n';
        std::cout << "Variant1.GetValue<std::string>()=" << Variant1.GetValue<std::string>() << '\n';
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Emplace

    {
        TVariant<std::string, int32> Variant;
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");
        Variant.Emplace<std::string>("Long string long string long string long string long string long string");

        std::cout << "Variant.GetValue<std::string>()=" << Variant.GetValue<std::string>() << '\n';
    }
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Comparison
    
    {
        TVariant<std::string, int32> Variant0(TInPlaceType<int32>(), 10);
        TVariant<std::string, int32> Variant1(TInPlaceType<int32>(), 20);

        std::cout << "Variant0 == Variant1 -> " << std::boolalpha << (Variant0 == Variant1) << '\n';
        std::cout << "Variant0 != Variant1 -> " << std::boolalpha << (Variant0 != Variant1) << '\n';

        std::cout << "Variant0  < Variant1 -> " << std::boolalpha << (Variant0  < Variant1) << '\n';
        std::cout << "Variant0 <= Variant1 -> " << std::boolalpha << (Variant0 <= Variant1) << '\n';

        std::cout << "Variant0 >  Variant1 -> " << std::boolalpha << (Variant0 >  Variant1) << '\n';
        std::cout << "Variant0 >= Variant1 -> " << std::boolalpha << (Variant0 >= Variant1) << '\n';
    }

    return;
}

#endif