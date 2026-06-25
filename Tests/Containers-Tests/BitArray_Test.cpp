#include "BitArray_Test.h"

#if RUN_TBITARRAY_TEST || RUN_TSTATICBITARRAY_TEST
#include <Core/Containers/String.h>

#include "TestUtils.h"

// Renders a bit array as a string with the most-significant (highest index) bit first.
template<typename BitArrayType>
static String MakeStringFromBitArray(const BitArrayType& BitArray)
{
    String NewString;
    NewString.Reserve(BitArray.Size());

    for (int32 Index = BitArray.Size() - 1; Index >= 0; --Index)
    {
        NewString.Append((BitArray[Index] == true) ? '1' : '0');
    }

    return NewString;
}
#endif

#if RUN_TBITARRAY_TEST
#include <Core/Containers/BitArray.h>

#include <vector>

bool TBitArray_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Empty / IsEmpty / Size");
    {
        TBitArray<uint8> Bits;
        TEST_EXPECT(Bits.IsEmpty());
        TEST_EXPECT_EQ(Bits.Size(), 0);
        TEST_EXPECT(Bits.HasNoBitSet());
        TEST_EXPECT(!Bits.HasAnyBitSet());
    }

    TEST_SECTION("Add / AssignBit / operator[] read");
    {
        TBitArray<uint8> Bits;
        Bits.Add(true);
        Bits.Add(false);
        Bits.Add(true);
        
        TEST_EXPECT_EQ(Bits.Size(), 3);
        TEST_EXPECT(Bits[0] == true);
        TEST_EXPECT(Bits[1] == false);
        TEST_EXPECT(Bits[2] == true);

        Bits.AssignBit(1, true);
        TEST_EXPECT(Bits[1] == true);
    }

    TEST_SECTION("FlipBit");
    {
        TBitArray<uint8> Bits(4, false);
        Bits.FlipBit(2);
        TEST_EXPECT(Bits[2] == true);
        Bits.FlipBit(2);
        TEST_EXPECT(Bits[2] == false);
    }

    TEST_SECTION("CountAssignedBits / HasAnyBitSet / HasNoBitSet");
    {
        TBitArray<uint8> Bits(8, false);
        TEST_EXPECT_EQ(Bits.CountAssignedBits(), 0);
        TEST_EXPECT(Bits.HasNoBitSet());

        Bits.AssignBit(0, true);
        Bits.AssignBit(3, true);
        Bits.AssignBit(7, true);

        TEST_EXPECT_EQ(Bits.CountAssignedBits(), 3);
        TEST_EXPECT(Bits.HasAnyBitSet());
    }

    TEST_SECTION("MostSignificant / LeastSignificant");
    {
        TBitArray<uint8> Bits(8, false);
        Bits.AssignBit(2, true);
        Bits.AssignBit(5, true);
        TEST_EXPECT_EQ(Bits.LeastSignificant(), 2);
        TEST_EXPECT_EQ(Bits.MostSignificant(), 5);
    }

    TEST_SECTION("Reset clears all bits");
    {
        TBitArray<uint8> Bits(8, true);
        TEST_EXPECT(Bits.HasAnyBitSet());
        Bits.Reset();
        TEST_EXPECT(Bits.HasNoBitSet());
    }

    TEST_SECTION("Copy construction / operator== / operator!=");
    {
        TBitArray<uint8> First(9, true);
        TBitArray<uint8> Second(First);
        TEST_EXPECT(First == Second);

        Second.AssignBit(0, false);
        TEST_EXPECT(First != Second);
    }

    TEST_SECTION("Bitwise And / Or / Xor / Not");
    {
        TBitArray<uint8> First  = { true, true, false, false };
        TBitArray<uint8> Second = { true, false, true, false };

        TBitArray<uint8> AndResult(First);
        AndResult.BitwiseAnd(Second);
        TEST_EXPECT(AndResult[0] == true);
        TEST_EXPECT(AndResult[1] == false);
        TEST_EXPECT(AndResult[2] == false);

        TBitArray<uint8> OrResult(First);
        OrResult.BitwiseOr(Second);
        TEST_EXPECT(OrResult[0] == true);
        TEST_EXPECT(OrResult[1] == true);
        TEST_EXPECT(OrResult[2] == true);
        TEST_EXPECT(OrResult[3] == false);

        TBitArray<uint8> XorResult(First);
        XorResult.BitwiseXor(Second);
        TEST_EXPECT(XorResult[0] == false);
        TEST_EXPECT(XorResult[1] == true);
        TEST_EXPECT(XorResult[2] == true);
        TEST_EXPECT(XorResult[3] == false);
    }

    TEST_SECTION("Default int type metadata (Capacity/IntegerSize/CapacityInBytes)");
    {
        TBitArray<uint8> Empty8;
        TEST_EXPECT_EQ(Empty8.Capacity(), 0);
        TEST_EXPECT_EQ(Empty8.IntegerSize(), 0);
        TEST_EXPECT_EQ(Empty8.CapacityInBytes(), 0);

        TBitArray Bits(8, true);
        TEST_EXPECT_EQ(Bits.Size(), 8);
        TEST_EXPECT_EQ(Bits.Capacity(), 32);
        TEST_EXPECT_EQ(Bits.IntegerSize(), 1);
        TEST_EXPECT_EQ(Bits.CapacityInBytes(), 4);
        TEST_EXPECT(MakeStringFromBitArray(Bits) == "11111111");
    }

    TEST_SECTION("Construction (single integer / ptr+count / initializer-list)");
    {
        TBitArray<uint8> FromInt(uint8(0b01010101));
        TEST_EXPECT_EQ(FromInt.Size(), 8);
        TEST_EXPECT(MakeStringFromBitArray(FromInt) == "01010101");

        const uint8 Bytes[] = { 0b01010101, 0b01010101 };
        TBitArray<uint8> FromPtr(Bytes, ARRAY_COUNT(Bytes));
        TEST_EXPECT_EQ(FromPtr.Size(), 16);
        TEST_EXPECT(MakeStringFromBitArray(FromPtr) == "0101010101010101");

        TBitArray FromList = { false, true, false, true };
        TEST_EXPECT_EQ(FromList.Size(), 4);
        TEST_EXPECT(MakeStringFromBitArray(FromList) == "1010");
    }

    TEST_SECTION("Move construction empties the source");
    {
        TBitArray<uint8> Source(9, true);
        TBitArray<uint8> Moved(::Move(Source));
        TEST_EXPECT_EQ(Source.Size(), 0);
        TEST_EXPECT_EQ(Moved.Size(), 9);
        TEST_EXPECT(MakeStringFromBitArray(Moved) == "111111111");
        TEST_EXPECT(Source != Moved);
    }

    TEST_SECTION("operator~ (bitwise not)");
    {
        TBitArray<uint8> Bits = { false, true, false, true };
        TEST_EXPECT(MakeStringFromBitArray(Bits) == "1010");

        Bits = ~Bits;
        TEST_EXPECT(MakeStringFromBitArray(Bits) == "0101");
    }

    TEST_SECTION("Insert shifts bits up");
    {
        constexpr int32 BitCount = 18;
        TBitArray<uint8> Bits(BitCount, false);
        for (int32 Index = 0; Index < BitCount; ++Index)
        {
            Bits.Insert(3, true);
        }

        TEST_EXPECT(MakeStringFromBitArray(Bits) == "000000000000000111111111111111111000");
    }

    TEST_SECTION("Remove shifts bits down");
    {
        constexpr int32 BitCount = 18;
        TBitArray<uint8> Bits(BitCount, false);
        for (int32 Index = 0; Index < BitCount; ++Index)
        {
            if (Index % 2 == 0)
            {
                Bits.FlipBit(Index);
            }
        }

        TEST_EXPECT(MakeStringFromBitArray(Bits) == "010101010101010101");

        while (Bits.Size() > 3)
        {
            Bits.Remove(3);
        }

        TEST_EXPECT(MakeStringFromBitArray(Bits) == "101");
    }

    TEST_SECTION("BitshiftLeft / BitshiftRight (zero shift is a no-op, size preserved)");
    {
        TBitArray<uint8> Bits = { true, false, true, true };
        TBitArray<uint8> Copy(Bits);

        Bits.BitshiftLeft(0);
        TEST_EXPECT(Bits == Copy);
        
        Bits.BitshiftRight(0);
        TEST_EXPECT(Bits == Copy);

        Bits.BitshiftLeft(2);
        TEST_EXPECT_EQ(Bits.Size(), 4);

        Bits.BitshiftRight(2);
        TEST_EXPECT_EQ(Bits.Size(), 4);
    }

    TEST_SECTION("TBitArray reallocation stress (seeded size sweep vs std::vector<bool>)");
    {
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom           Random(Seed);
            TBitArray<uint8>  Bits;
            std::vector<bool> Oracle;

            // Grow bit-by-bit so storage reallocates repeatedly across the boundary sizes.
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const bool bValue = Random.RandBool();
                Bits.Add(bValue);
                Oracle.push_back(bValue);
            }

            // Random flips to stress the bit addressing inside the (possibly reallocated) storage.
            for (int32 Step = 0; (Step < TargetSize) && !Oracle.empty(); ++Step)
            {
                if (Random.RandBool())
                {
                    const int32 At = static_cast<int32>(Random.RandInt(0, static_cast<int64>(Oracle.size()) - 1));
                    Bits.FlipBit(At);
                    Oracle[At] = !Oracle[At];
                }
            }

            bool  bMatches = (Bits.Size() == static_cast<int32>(Oracle.size()));
            int32 SetCount = 0;
            for (int32 Index = 0; bMatches && (Index < Bits.Size()); ++Index)
            {
                const bool bLhs = (Bits[Index] == true);
                const bool bRhs = Oracle[Index];
                bMatches = (bLhs == bRhs);
                if (bRhs)
                {
                    ++SetCount;
                }
            }

            bMatches = bMatches && (Bits.CountAssignedBits() == SetCount);
            if (!bMatches)
            {
                LOG_ERROR("[STRESS FAIL] TBitArray seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bMatches);
        }
    }

    TEST_END();
}
#endif

#if RUN_TSTATICBITARRAY_TEST
#include <Core/Containers/StaticBitArray.h>

bool TStaticBitArray_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Default / Size / all clear");
    {
        TStaticBitArray<16, uint8> Bits;
        TEST_EXPECT_EQ(Bits.Size(), 16);
        TEST_EXPECT(Bits.HasNoBitSet());
        TEST_EXPECT_EQ(Bits.CountAssignedBits(), 0);
    }

    TEST_SECTION("constexpr metadata (Size/Capacity/IntegerSize/CapacityInBytes)");
    {
        constexpr TStaticBitArray<11, uint8> Bits8;
        static_assert(Bits8.Size() == 11);
        static_assert(Bits8.Capacity() == 16);
        static_assert(Bits8.IntegerSize() == 2);
        static_assert(Bits8.CapacityInBytes() == 2);

        constexpr TStaticBitArray<18, uint32> Bits32;
        static_assert(Bits32.Size() == 18);
        static_assert(Bits32.Capacity() == 32);
        static_assert(Bits32.IntegerSize() == 1);
        static_assert(Bits32.CapacityInBytes() == 4);
        TEST_EXPECT(MakeStringFromBitArray(Bits8) == "00000000000");
    }

    TEST_SECTION("Construction (count+value / single integer / ptr+count / initializer-list)");
    {
        constexpr TStaticBitArray<9, uint8> CountValue(8, true);
        static_assert(CountValue.Size() == 9);
        TEST_EXPECT(MakeStringFromBitArray(CountValue) == "011111111");

        constexpr TStaticBitArray<8, uint8> FromInt(uint8(0b01010101));
        TEST_EXPECT(MakeStringFromBitArray(FromInt) == "01010101");

        constexpr const uint8 Bytes[] = { 0b01010101, 0b01010101 };
        constexpr TStaticBitArray<14, uint8> FromPtr(Bytes, ARRAY_COUNT(Bytes));
        static_assert(FromPtr.Size() == 14);
        TEST_EXPECT(MakeStringFromBitArray(FromPtr) == "01010101010101");

        constexpr TStaticBitArray<8> FromList = { false, true, false, true };
        static_assert(FromList.Size() == 8);
        TEST_EXPECT(MakeStringFromBitArray(FromList) == "00001010");
    }

    TEST_SECTION("Copy / move construction (move keeps source intact)");
    {
        constexpr TStaticBitArray<9, uint8> Source(9, true);
        constexpr TStaticBitArray<9, uint8> Copy(Source);
        static_assert((Source == Copy) == true);

        constexpr TStaticBitArray<9, uint8> Moved(::Move(Source));
        static_assert(Moved.Size() == 9);

        TEST_EXPECT(MakeStringFromBitArray(Source) == "111111111");
        TEST_EXPECT(MakeStringFromBitArray(Moved) == "111111111");
    }

    TEST_SECTION("BitshiftLeft / BitshiftRight (zero shift no-op, size preserved)");
    {
        TStaticBitArray<8, uint8> Bits = { true, false, true, true, false, false, false, false };
        TStaticBitArray<8, uint8> Copy(Bits);

        Bits.BitshiftLeft(0);
        TEST_EXPECT(Bits == Copy);
        
        Bits.BitshiftRight(0);
        TEST_EXPECT(Bits == Copy);

        Bits.BitshiftLeft(2);
        TEST_EXPECT_EQ(Bits.Size(), 8);
        
        Bits.BitshiftRight(2);
        TEST_EXPECT_EQ(Bits.Size(), 8);
    }

    TEST_SECTION("AssignBit / operator[] / FlipBit");
    {
        TStaticBitArray<16, uint8> Bits;
        Bits.AssignBit(0, true);
        Bits.AssignBit(15, true);

        TEST_EXPECT(Bits[0] == true);
        TEST_EXPECT(Bits[15] == true);
        TEST_EXPECT(Bits[1] == false);
        TEST_EXPECT_EQ(Bits.CountAssignedBits(), 2);

        Bits.FlipBit(0);
        TEST_EXPECT(Bits[0] == false);
    }

    TEST_SECTION("MostSignificant / LeastSignificant / HasAnyBitSet");
    {
        TStaticBitArray<16, uint8> Bits;
        Bits.AssignBit(3, true);
        Bits.AssignBit(10, true);

        TEST_EXPECT(Bits.HasAnyBitSet());
        TEST_EXPECT_EQ(Bits.LeastSignificant(), 3);
        TEST_EXPECT_EQ(Bits.MostSignificant(), 10);
    }

    TEST_SECTION("Initializer-list construction / operator== / operator!=");
    {
        TStaticBitArray<4, uint8> First  = { true, false, true, false };
        TStaticBitArray<4, uint8> Second = { true, false, true, false };
        TStaticBitArray<4, uint8> Third  = { true, true, true, false };

        TEST_EXPECT(First == Second);
        TEST_EXPECT(First != Third);
    }

    TEST_SECTION("Bitwise operators");
    {
        TStaticBitArray<4, uint8> First     = { true, true, false, false };
        TStaticBitArray<4, uint8> Second    = { true, false, true, false };
        TStaticBitArray<4, uint8> AndResult = First & Second;

        TEST_EXPECT(AndResult[0] == true);
        TEST_EXPECT(AndResult[1] == false);

        TStaticBitArray<4, uint8> OrResult = First | Second;
        TEST_EXPECT(OrResult[2] == true);

        TStaticBitArray<4, uint8> XorResult = First ^ Second;
        TEST_EXPECT(XorResult[1] == true);
        TEST_EXPECT(XorResult[0] == false);
    }

    TEST_SECTION("Reset");
    {
        TStaticBitArray<8, uint8> Bits = { true, true, true, true, true, true, true, true };
        TEST_EXPECT(Bits.HasAnyBitSet());

        Bits.Reset();
        TEST_EXPECT(Bits.HasNoBitSet());
    }

    TEST_END();
}
#endif
