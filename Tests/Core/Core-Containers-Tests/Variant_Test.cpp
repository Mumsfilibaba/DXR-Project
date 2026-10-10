#include "Variant_Test.h"

#if RUN_TVARIANT_TEST
#include "TestUtils.h"

#include <Core/Containers/Variant.h>
#include <Core/Containers/String.h>

bool TVariant_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Default / IsValid / IsType");
    {
        TVariant<int32, float, char> Var;
        TEST_EXPECT(!Var.IsValid());
        TEST_EXPECT(!Var.IsType<int32>());
        TEST_EXPECT(!Var.IsType<float>());
    }

    TEST_SECTION("TInPlaceType construction / GetValue / GetIndex");
    {
        TVariant<int32, float, char> Var(TInPlaceType<float>(), 3.5f);
        TEST_EXPECT(Var.IsValid());
        TEST_EXPECT(Var.IsType<float>());
        TEST_EXPECT(!Var.IsType<int32>());
        TEST_EXPECT_EQ(Var.GetValue<float>(), 3.5f);
        TEST_EXPECT_EQ(Var.GetIndex(), 1);
    }

    TEST_SECTION("TInPlaceIndex construction");
    {
        TVariant<int32, float, char> V0(TInPlaceIndex<0>(), 11);
        TVariant<int32, float, char> V2(TInPlaceIndex<2>(), 'z');
        TEST_EXPECT(V0.IsType<int32>());
        TEST_EXPECT_EQ(V0.GetValue<int32>(), 11);
        TEST_EXPECT(V2.IsType<char>());
        TEST_EXPECT_EQ(V2.GetValue<char>(), 'z');
    }

    TEST_SECTION("TryGetValue / GetValueOrDefault");
    {
        TVariant<int32, float, char> Var(TInPlaceType<int32>(), 77);
        TEST_EXPECT(Var.TryGetValue<int32>() != nullptr);
        TEST_EXPECT_EQ(*Var.TryGetValue<int32>(), 77);
        TEST_EXPECT(Var.TryGetValue<float>() == nullptr);
        TEST_EXPECT_EQ(Var.GetValueOrDefault<int32>(0), 77);
        TEST_EXPECT_EQ(Var.GetValueOrDefault<float>(1.5f), 1.5f);
    }

    TEST_SECTION("Emplace / Reset");
    {
        TVariant<int32, float, char> Var;
        Var.Emplace<int32>(5);
        TEST_EXPECT(Var.IsType<int32>());
        TEST_EXPECT_EQ(Var.GetValue<int32>(), 5);

        Var.Emplace<char>('a');
        TEST_EXPECT(Var.IsType<char>());
        TEST_EXPECT(!Var.IsType<int32>());
        TEST_EXPECT_EQ(Var.GetValue<char>(), 'a');

        Var.Reset();
        TEST_EXPECT(!Var.IsValid());
    }

    TEST_SECTION("Copy / move / value assignment");
    {
        TVariant<int32, float, char> Var(TInPlaceType<int32>(), 9);
        TVariant<int32, float, char> Copy = Var;
        TEST_EXPECT(Copy.IsType<int32>());
        TEST_EXPECT_EQ(Copy.GetValue<int32>(), 9);

        TVariant<int32, float, char> Moved = ::Move(Copy);
        TEST_EXPECT(Moved.IsType<int32>());
        TEST_EXPECT_EQ(Moved.GetValue<int32>(), 9);

        Moved = 2.5f;
        TEST_EXPECT(Moved.IsType<float>());
        TEST_EXPECT_EQ(Moved.GetValue<float>(), 2.5f);
    }

    TEST_SECTION("Swap");
    {
        TVariant<int32, float, char> First(TInPlaceType<int32>(), 1);
        TVariant<int32, float, char> Second(TInPlaceType<char>(), 'x');
        First.Swap(Second);
        TEST_EXPECT(First.IsType<char>());
        TEST_EXPECT_EQ(First.GetValue<char>(), 'x');
        TEST_EXPECT(Second.IsType<int32>());
        TEST_EXPECT_EQ(Second.GetValue<int32>(), 1);
    }

    TEST_SECTION("Comparison operators");
    {
        TVariant<int32, float, char> First(TInPlaceType<int32>(), 5);
        TVariant<int32, float, char> Second(TInPlaceType<int32>(), 5);
        TVariant<int32, float, char> Third(TInPlaceType<int32>(), 6);
        TEST_EXPECT(First == Second);
        TEST_EXPECT(First != Third);
        TEST_EXPECT(First < Third);
        TEST_EXPECT(Third > First);
        TEST_EXPECT(First <= Second);
        TEST_EXPECT(First >= Second);
    }

    TEST_SECTION("Non-trivial type (String): InPlace / const GetValue / TryGetValue");
    {
        TVariant<FString, int32> Var(TInPlaceType<FString>(), "Test");
        TEST_EXPECT(Var.IsType<FString>());
        TEST_EXPECT(!Var.IsType<int32>());
        TEST_EXPECT(Var.GetValue<FString>() == "Test");

        const TVariant<FString, int32>& Ref = Var;
        TEST_EXPECT(Ref.GetValue<FString>() == "Test");
        TEST_EXPECT(Ref.TryGetValue<int32>() == nullptr);
        TEST_EXPECT(Ref.TryGetValue<FString>() != nullptr);
    }

    TEST_SECTION("Non-trivial type (String): Swap with heap strings / Emplace reassign");
    {
        const FString One = "String Number One 11111111111111111111111111111111111111111111111111";
        const FString Two = "String Number Two 22222222222222222222222222222222222222222222222222";

        TVariant<FString, int32> First(TInPlaceType<FString>(), One);
        TVariant<FString, int32> Second(TInPlaceType<FString>(), Two);

        First.Swap(Second);
        TEST_EXPECT(First.GetValue<FString>() == Two);
        TEST_EXPECT(Second.GetValue<FString>() == One);

        TVariant<FString, int32> Var;
        
        Var.Emplace<FString>("Long string long string long string long string 1111111111111111");
        TEST_EXPECT(Var.GetValue<FString>() == "Long string long string long string long string 1111111111111111");

        Var.Emplace<FString>("Long string long string long string long string 2222222222222222");
        TEST_EXPECT(Var.GetValue<FString>() == "Long string long string long string long string 2222222222222222");
    }

    TEST_END();
}
#endif
