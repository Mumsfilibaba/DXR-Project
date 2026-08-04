#include "StaticArray_Test.h"

#if RUN_TSTATICARRAY_TEST
#include "TestUtils.h"

#include <Core/Containers/StaticArray.h>
#include <Core/Containers/Array.h>

bool TStaticArray_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Size / Capacity / SizeInBytes / indexing");
    {
        TStaticArray<int32, 5> Array = { 10, 20, 30, 40, 50 };
        TEST_EXPECT_EQ(Array.Size(), 5);
        TEST_EXPECT_EQ(Array.Capacity(), 5);
        TEST_EXPECT_EQ(Array.SizeInBytes(), static_cast<int32>(5 * sizeof(int32)));
        TEST_EXPECT_EQ(Array.LastIndex(), 4);
        TEST_EXPECT_EQ(Array[0], 10);
        TEST_EXPECT_EQ(Array[4], 50);
        TEST_EXPECT(Array.IsValidIndex(0));
        TEST_EXPECT(Array.IsValidIndex(4));
        TEST_EXPECT(!Array.IsValidIndex(5));
        TEST_EXPECT(!Array.IsValidIndex(-1));
    }

    TEST_SECTION("First / Last / Data");
    {
        TStaticArray<int32, 5> Array = { 1, 2, 3, 4, 5 };
        TEST_EXPECT_EQ(Array.First(), 1);
        TEST_EXPECT_EQ(Array.Last(), 5);
        TEST_EXPECT(Array.Data() != nullptr);
        TEST_EXPECT_EQ(Array.Data()[2], 3);

        const TStaticArray<int32, 5>& ConstArray = Array;
        TEST_EXPECT_EQ(ConstArray.First(), 1);
        TEST_EXPECT_EQ(ConstArray.Last(), 5);
    }

    TEST_SECTION("Fill / Memzero");
    {
        TStaticArray<int32, 5> Array = { 1, 2, 3, 4, 5 };
        Array.Fill(7);

        for (int32 Index = 0; Index < Array.Size(); ++Index)
        {
            TEST_EXPECT_EQ(Array[Index], 7);
        }

        Array.Memzero();
        for (int32 Index = 0; Index < Array.Size(); ++Index)
        {
            TEST_EXPECT_EQ(Array[Index], 0);
        }
    }

    TEST_SECTION("Find / FindLast / Contains / predicates");
    {
        TStaticArray<int32, 5> Array = { 5, 10, 15, 10, 25 };
        TEST_EXPECT_EQ(Array.Find(15), 2);
        TEST_EXPECT_EQ(Array.Find(10), 1);
        TEST_EXPECT_EQ(Array.FindLast(10), 3);
        TEST_EXPECT_EQ(Array.Find(99), (TStaticArray<int32, 5>::InvalidIndex));
        TEST_EXPECT(Array.Contains(25));
        TEST_EXPECT(!Array.Contains(99));

        const auto IsBig = [](int32 Value)
        {
            return Value > 20;
        };

        TEST_EXPECT_EQ(Array.FindWithPredicate(IsBig), 4);
        TEST_EXPECT(Array.ContainsWithPredicate(IsBig));
        
        TEST_EXPECT_EQ(Array.FindLastWithPredicate([](int32 Value)
        {
            return Value == 10;
        }), 3);
    }

    TEST_SECTION("Swap(index, index) / Foreach");
    {
        TStaticArray<int32, 5> Array = { 1, 2, 3, 4, 5 };
        Array.Swap(0, 4);

        TEST_EXPECT_EQ(Array[0], 5);
        TEST_EXPECT_EQ(Array[4], 1);

        int32 Sum = 0;
        Array.Foreach([&Sum](int32 Value)
        {
            Sum += Value;
        });

        TEST_EXPECT_EQ(Sum, 15);
    }

    TEST_SECTION("Range-based iteration");
    {
        TStaticArray<int32, 5> Array = { 2, 4, 6, 8, 10 };
        
        int32 Sum = 0;
        for (int32 Value : Array)
        {
            Sum += Value;
        }

        TEST_EXPECT_EQ(Sum, 30);
    }

    TEST_SECTION("operator== / operator!= against TArray");
    {
        TStaticArray<int32, 5> Array = { 1, 2, 3, 4, 5 };
        
        TArray<int32> Same      = { 1, 2, 3, 4, 5 };
        TArray<int32> Different = { 1, 2, 3, 4, 6 };
        
        TEST_EXPECT(Array == Same);
        TEST_EXPECT(Array != Different);
    }

    TEST_SECTION("Whole-array ::Swap / partial initializer list");
    {
        TStaticArray<uint32, 6> First = { 5, 6, 7 };
        TStaticArray<uint32, 6> Second = { 15, 16, 17 };
        TEST_EXPECT_EQ(First[0], 5u);
        TEST_EXPECT_EQ(Second[0], 15u);

        ::Swap(First, Second);
        
        TEST_EXPECT_EQ(First[0], 15u);
        TEST_EXPECT_EQ(Second[0], 5u);
        TEST_EXPECT(First != Second);
    }

    TEST_END();
}
#endif
