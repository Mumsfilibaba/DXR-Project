#include "Variant_Test.h"

#if RUN_TVARIANT_TEST
#include "TestUtils.h"

#include <Core/Containers/Variant.h>
#include <Core/Memory/Memory.h>

#include <iostream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// STest

bool TVariant_Test()
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Basic

    {
        TVariant<std::string, int32> Variant;
        TEST_CHECK(Variant.IsType<int32>()       == false);
        TEST_CHECK(Variant.IsType<std::string>() == false);
    }
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TInPlaceType
    
    {
        TVariant<std::string, int32> Variant(TInPlaceType<std::string>(), "Test");
        TEST_CHECK(Variant.IsType<int32>()       == false);
        TEST_CHECK(Variant.IsType<std::string>() == true);

        TEST_CHECK(Variant.GetValue<std::string>() == "Test");

        const TVariant<std::string, int32>& VariantRef = Variant;
        TEST_CHECK(VariantRef.GetValue<std::string>() == "Test");

        TEST_CHECK(Variant.TryGetValue<int32>()       == nullptr);
        TEST_CHECK(Variant.TryGetValue<std::string>() != nullptr);

        TEST_CHECK(VariantRef.TryGetValue<int32>()       == nullptr);
        TEST_CHECK(VariantRef.TryGetValue<std::string>() != nullptr);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TInPlaceIndex

    {
        TVariant<std::string, int32> Variant0{ TInPlaceIndex<0>(), "Test" };
        TVariant<std::string, int32> Variant1{ TInPlaceIndex<1>(), 0 };
        TEST_CHECK(Variant0.IsType<int32>()       == false);
        TEST_CHECK(Variant0.IsType<std::string>() == true);
        TEST_CHECK(Variant1.IsType<int32>()       == true);
        TEST_CHECK(Variant1.IsType<std::string>() == false);

        TEST_CHECK(Variant0.GetValue<std::string>() == "Test");
        TEST_CHECK(Variant1.GetValue<int32>()       == 0);

        TEST_CHECK(Variant0.TryGetValue<int32>()       == nullptr);
        TEST_CHECK(Variant0.TryGetValue<std::string>() != nullptr);

        TEST_CHECK(Variant1.TryGetValue<int32>()       != nullptr);
        TEST_CHECK(Variant1.TryGetValue<std::string>() == nullptr);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Swap

    {
        TVariant<std::string, int32> Variant0(TInPlaceType<std::string>(), "String Number One which equals 11111111111111111111111111111111111111111111111111111111");
        TVariant<std::string, int32> Variant1(TInPlaceType<std::string>(), "String Number Two which equals 22222222222222222222222222222222222222222222222222222222");

        TEST_CHECK(Variant0.GetValue<std::string>() == "String Number One which equals 11111111111111111111111111111111111111111111111111111111");
        TEST_CHECK(Variant1.GetValue<std::string>() == "String Number Two which equals 22222222222222222222222222222222222222222222222222222222");

        Variant0.Swap(Variant1);

        TEST_CHECK(Variant0.GetValue<std::string>() == "String Number Two which equals 22222222222222222222222222222222222222222222222222222222");
        TEST_CHECK(Variant1.GetValue<std::string>() == "String Number One which equals 11111111111111111111111111111111111111111111111111111111");

        Variant0.Swap(Variant1);

        TEST_CHECK(Variant0.GetValue<std::string>() == "String Number One which equals 11111111111111111111111111111111111111111111111111111111");
        TEST_CHECK(Variant1.GetValue<std::string>() == "String Number Two which equals 22222222222222222222222222222222222222222222222222222222");

        Variant0.Swap(Variant1);

        TEST_CHECK(Variant0.GetValue<std::string>() == "String Number Two which equals 22222222222222222222222222222222222222222222222222222222");
        TEST_CHECK(Variant1.GetValue<std::string>() == "String Number One which equals 11111111111111111111111111111111111111111111111111111111");
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Emplace

    {
        TVariant<std::string, int32> Variant;

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 1111111111111111111");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 1111111111111111111");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 2222222222222222222");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 2222222222222222222");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 3333333333333333333");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 3333333333333333333");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 4444444444444444444");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 4444444444444444444");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 5555555555555555555");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 5555555555555555555");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 6666666666666666666");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 6666666666666666666");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 7777777777777777777");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 7777777777777777777");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 8888888888888888888");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 8888888888888888888");

        Variant.Emplace<std::string>("Long string long string long string long string long string long string 9999999999999999999");
        TEST_CHECK(Variant.GetValue<std::string>() == "Long string long string long string long string long string long string 9999999999999999999");
    }
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Comparison
    
    {
        TVariant<std::string, int32> Variant0(TInPlaceType<int32>(), 10);
        TVariant<std::string, int32> Variant1(TInPlaceType<int32>(), 20);

        TEST_CHECK((Variant0 == Variant1) == false);
        TEST_CHECK((Variant0 != Variant1) == true);

        TEST_CHECK((Variant0 <  Variant1) == true);
        TEST_CHECK((Variant0 <= Variant1) == true);

        TEST_CHECK((Variant0 >  Variant1) == false);
        TEST_CHECK((Variant0 >= Variant1) == false);
    }

    SUCCESS();
}

#endif