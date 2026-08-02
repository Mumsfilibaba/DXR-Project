#include "ArrayView_Test.h"

#if RUN_TARRAYVIEW_TEST
#include "TestUtils.h"

#include <Core/Containers/ArrayView.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/StaticArray.h>

bool TArrayView_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Construct from raw pointer + size / Size / IsEmpty / indexing");
    {
        int32 Values[5] = { 10, 20, 30, 40, 50 };
        TArrayView<int32> ArrayView(Values, 5);
        TEST_EXPECT_EQ(ArrayView.Size(), 5);
        TEST_EXPECT(!ArrayView.IsEmpty());
        TEST_EXPECT_EQ(ArrayView[0], 10);
        TEST_EXPECT_EQ(ArrayView[4], 50);
        TEST_EXPECT_EQ(ArrayView.LastElementIndex(), 4);
        TEST_EXPECT(ArrayView.IsValidIndex(4));
        TEST_EXPECT(!ArrayView.IsValidIndex(5));

        TArrayView<int32> Empty(Values, 0);
        TEST_EXPECT(Empty.IsEmpty());
    }

    TEST_SECTION("Construct from TArray / FirstElement / LastElement / Data");
    {
        TArray<int32> Source = { 1, 2, 3, 4 };
        TArrayView<int32> ArrayView(Source);
        TEST_EXPECT_EQ(ArrayView.Size(), 4);
        TEST_EXPECT_EQ(ArrayView.FirstElement(), 1);
        TEST_EXPECT_EQ(ArrayView.LastElement(), 4);
        TEST_EXPECT_EQ(ArrayView.Data(), Source.Data());
    }

    TEST_SECTION("Find / FindLast / Contains / predicates");
    {
        int32 Values[5] = { 5, 10, 15, 10, 25 };
        TArrayView<int32> ArrayView(Values, 5);
        TEST_EXPECT_EQ(ArrayView.Find(15), 2);
        TEST_EXPECT_EQ(ArrayView.Find(10), 1);
        TEST_EXPECT_EQ(ArrayView.FindLast(10), 3);
        TEST_EXPECT_EQ(ArrayView.Find(99), TArrayView<int32>::InvalidIndex);
        TEST_EXPECT(ArrayView.Contains(25));
        TEST_EXPECT(!ArrayView.Contains(99));
        
        TEST_EXPECT_EQ(ArrayView.FindWithPredicate([](int32 Value)
        {
            return Value > 20;
        }), 4);

        TEST_EXPECT(ArrayView.ContainsWithPredicate([](int32 Value)
        {
            return Value == 15;
        }));
    }

    TEST_SECTION("SubView");
    {
        int32 Values[6] = { 0, 1, 2, 3, 4, 5 };
        TArrayView<int32> ArrayView(Values, 6);
        TArrayView<int32> Sub = ArrayView.SubView(2, 3);

        TEST_EXPECT_EQ(Sub.Size(), 3);
        TEST_EXPECT_EQ(Sub[0], 2);
        TEST_EXPECT_EQ(Sub[2], 4);
    }

    TEST_SECTION("Fill / mutation writes through to underlying memory");
    {
        int32 Values[4] = { 1, 2, 3, 4 };
        
        TArrayView<int32> ArrayView(Values, 4);
        ArrayView.Fill(9);

        for (int32 Index = 0; Index < 4; ++Index)
        {
            TEST_EXPECT_EQ(Values[Index], 9);
        }

        ArrayView[1] = 100;
        TEST_EXPECT_EQ(Values[1], 100);
    }

    TEST_SECTION("Swap");
    {
        int32 ValuesA[3] = { 1, 2, 3 };
        int32 ValuesB[2] = { 9, 8 };

        TArrayView<int32> First(ValuesA, 3);
        TArrayView<int32> Second(ValuesB, 2);
        First.Swap(Second);

        TEST_EXPECT_EQ(First.Size(), 2);
        TEST_EXPECT_EQ(Second.Size(), 3);
        TEST_EXPECT_EQ(First[0], 9);
        TEST_EXPECT_EQ(Second[0], 1);
    }

    TEST_SECTION("Range-based iteration / Foreach");
    {
        int32 Values[5] = { 2, 4, 6, 8, 10 };
        TArrayView<int32> ArrayView(Values, 5);

        int32 Sum = 0;
        for (int32 Value : ArrayView)
        {
            Sum += Value;
        }

        TEST_EXPECT_EQ(Sum, 30);

        int32 ForeachSum = 0;
        ArrayView.Foreach([&ForeachSum](int32 Value)
        {
            ForeachSum += Value;
        });

        TEST_EXPECT_EQ(ForeachSum, 30);
    }

    TEST_SECTION("operator== / operator!= against TArray");
    {
        int32 Values[4] = { 1, 2, 3, 4 };
        TArrayView<int32> ArrayView(Values, 4);
        
        TArray<int32> Same      = { 1, 2, 3, 4 };
        TArray<int32> Different = { 1, 2, 3, 5 };
        
        TEST_EXPECT(ArrayView == Same);
        TEST_EXPECT(ArrayView != Different);
    }

    TEST_SECTION("Construct from TStaticArray / C-array / MakeArrayView helpers");
    {
        TStaticArray<int32, 4> Static = { 11, 12, 13, 14 };
        TArrayView<int32> FromStatic(Static);
        TEST_EXPECT_EQ(FromStatic.Size(), 4);
        TEST_EXPECT_EQ(FromStatic[0], 11);

        int32 CArray[4] = { 21, 22, 23, 24 };
        TArrayView<int32> FromCArray(CArray);
        TEST_EXPECT_EQ(FromCArray.Size(), 4);
        TEST_EXPECT_EQ(FromCArray[3], 24);

        TArray<int32> Source = { 1, 2, 3, 4 };
        TArrayView<int32> FromArrayHelper = MakeArrayView(Source);
        TEST_EXPECT_EQ(FromArrayHelper.Size(), 4);

        TArrayView<int32> FromStaticHelper = MakeArrayView(Static);
        TEST_EXPECT_EQ(FromStaticHelper.Size(), 4);

        TArrayView<int32> FromPtrHelper = MakeArrayView(CArray, 4);
        TEST_EXPECT_EQ(FromPtrHelper.Size(), 4);
    }

    TEST_SECTION("Const-element view / copy / move construction");
    {
        const TArray<int32> Source = { 1, 2, 3, 4 };
        TArrayView<const int32> ConstView(Source);
        TEST_EXPECT_EQ(ConstView.Size(), 4);
        TEST_EXPECT_EQ(ConstView[2], 3);

        int32 Values[3] = { 7, 8, 9 };
        TArrayView<int32> Original(Values, 3);
        TArrayView<int32> Copy = Original;
        TEST_EXPECT_EQ(Copy.Size(), 3);
        TEST_EXPECT_EQ(Copy[0], 7);

        TArrayView<int32> Moved = ::Move(Original);
        TEST_EXPECT_EQ(Moved.Size(), 3);
        TEST_EXPECT_EQ(Moved[2], 9);
    }

    TEST_END();
}
#endif
