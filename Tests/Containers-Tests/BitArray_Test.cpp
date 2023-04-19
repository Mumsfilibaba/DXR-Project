#include "BitArray_Test.h"

#if RUN_TBITARRAY_TEST || RUN_TSTATICBITARRAY_TEST
#include "TestUtils.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

template<typename BitArrayType>
std::string MakeStringFromBitArray(const BitArrayType& BitArray)
{
    std::string NewString;
    NewString.reserve(BitArray.Size());

    for (BitArrayType::SizeType Index = 0; Index < BitArray.Size(); ++Index)
    {
        const bool bValue = (BitArray[Index] == true);
        NewString.push_back(bValue ? '1' : '0');
    }

    std::reverse(NewString.begin(), NewString.end());
    return NewString;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// BitArray test

#if RUN_TBITARRAY_TEST
#include <Core/Containers/BitArray.h>

bool TBitArray_Test()
{
    std::cout << '\n' << "----------TBitArray----------" << '\n' << '\n';

    {
        TBitArray<uint8> BitArray;
        TEST_CHECK(BitArray.Size()         == 0);
        TEST_CHECK(BitArray.Capacity()     == 0);
        TEST_CHECK(BitArray.StorageSize()     == 0);
        TEST_CHECK(BitArray.CapacityInBytes() == 0);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "");
    }

    {
        TBitArray<uint32> BitArray;
        TEST_CHECK(BitArray.Size()         == 0);
        TEST_CHECK(BitArray.Capacity()     == 0);
        TEST_CHECK(BitArray.StorageSize()     == 0);
        TEST_CHECK(BitArray.CapacityInBytes() == 0);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "");
    }

    {
        TBitArray BitArray(8, true);
        TEST_CHECK(BitArray.Size()         == 8);
        TEST_CHECK(BitArray.Capacity()     == 32);
        TEST_CHECK(BitArray.StorageSize()     == 1);
        TEST_CHECK(BitArray.CapacityInBytes() == 4);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "11111111");
    }

    {
        TBitArray<uint8> BitArray(uint8(0b01010101));
        TEST_CHECK(BitArray.Size()         == 8);
        TEST_CHECK(BitArray.Capacity()     == 8);
        TEST_CHECK(BitArray.StorageSize()     == 1);
        TEST_CHECK(BitArray.CapacityInBytes() == 1);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "01010101");
    }

    {
        const uint8 Bits[] =
        {
            0b01010101,
            0b01010101,
        };

        TBitArray<uint8> BitArray(Bits, ARRAY_COUNT(Bits));
        TEST_CHECK(BitArray.Size() == 16);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "0101010101010101");
    }

    {
        TBitArray BitArray = { false, true, false, true };
        TEST_CHECK(BitArray.Size() == 4);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "1010");
    }

    {
        TBitArray<uint8> BitArray0(9, true);
        TEST_CHECK(BitArray0.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        TBitArray<uint8> BitArray1(BitArray0);
        TEST_CHECK(BitArray1.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray1) == "111111111");

        TEST_CHECK((BitArray0 == BitArray1) == true);
    }

    {
        TBitArray<uint8> BitArray0(9, true);
        TEST_CHECK(BitArray0.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        TBitArray<uint8> BitArray1(Move(BitArray0));
        TEST_CHECK(BitArray0.Size() == 0);
        TEST_CHECK(BitArray1.Size() == 9);
        
        TEST_CHECK(MakeStringFromBitArray(BitArray0) == "");
        TEST_CHECK(MakeStringFromBitArray(BitArray1) == "111111111");

        TEST_CHECK((BitArray0 != BitArray1) == true);
    }

    {
        TBitArray<uint8> BitArray(9, true);
        TEST_CHECK(BitArray.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "111111111");

        BitArray.ResetWithZeros();

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "000000000");

        BitArray.ResetWithOnes();

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "111111111");
    }

    {
        TBitArray<uint8> BitArray;
        TEST_CHECK(BitArray.Size() == 0);
        TEST_CHECK(BitArray.IsEmpty() == true);

        BitArray.Add(false);
        BitArray.Add(true);
        BitArray.Add(false);
        BitArray.Add(true);
        BitArray.Add(true);
        BitArray.Add(false);
        BitArray.Add(true);
        BitArray.Add(true);
        BitArray.Add(false);
        BitArray.Add(true);
        BitArray.Add(false);

        TEST_CHECK(BitArray.Size() == 11);
        TEST_CHECK(BitArray.IsEmpty() == false);

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "01011011010");
    }

    {
        TBitArray<uint8> BitArray(0b00000000);
        TEST_CHECK(BitArray.Size() == 8);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00000000");

        BitArray.AssignBit(3, true);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00001000");

        BitArray.FlipBit(4);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00011000");

        TEST_CHECK(BitArray.CountAssignedBits() == 2);
        TEST_CHECK(BitArray.HasAnyBitSet()      == true);
        TEST_CHECK(BitArray.HasNoBitSet()       == false);

        const uint32 MostSignificantBit = BitArray.MostSignificant();
        TEST_CHECK(MostSignificantBit == 4);

        const uint32 LeastSignificantBit = BitArray.LeastSignificant();
        TEST_CHECK(LeastSignificantBit == 3);
    }

    {
        TBitArray BitArray = { false, true, false, true };
        TEST_CHECK(BitArray.Size() == 4);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "1010");

        BitArray = ~BitArray;

        TEST_CHECK(BitArray.Size() == 4);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "0101");
    }

    CONSTEXPR int32 BitCount = 18;
    {
        TBitArray<uint8> BitArray(BitCount, false);
        for (int32 Bit = 0; Bit < BitCount; ++Bit)
        {
            BitArray.Insert(3, true);

            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "000000000000000111111111111111111000");
    }

    {
        TBitArray<uint8> BitArray(BitCount, false);
        for (int32 Index = 0; Index < BitCount; ++Index)
        {
            if (Index % 2 == 0)
            {
                BitArray.FlipBit(Index);
            }

            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "010101010101010101");

        for (int32 Bit = 0; Bit < BitCount; ++Bit)
        {
            if (BitArray.Size() > 3)
            {
                BitArray.Remove(3);
            }
            else
            {
                break;
            }

            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "101");
    }

    std::cout << "Testing BitShift Left\n";
    {
        for (int32 Bit = 0; Bit <= BitCount; ++Bit)
        {
            TBitArray<uint8> BitArray;
            for (int32 Index = 0; Index < (BitCount / 2); ++Index)
            {
                if (Index % 2)
                {
                    BitArray.Add(false);
                    BitArray.Add(false);
                }
                else
                {
                    BitArray.Add(true);
                    BitArray.Add(true);
                }
            }

            BitArray.BitshiftLeft(Bit);
            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }
    }

    std::cout << "Testing BitShift Right\n";
    {
        for (int32 Bit = 0; Bit <= BitCount; ++Bit)
        {
            TBitArray<uint8> BitArray;
            for (int32 Index = 0; Index < (BitCount / 2); ++Index)
            {
                if (Index % 2)
                {
                    BitArray.Add(false);
                    BitArray.Add(false);
                }
                else
                {
                    BitArray.Add(true);
                    BitArray.Add(true);
                }
            }

            BitArray.BitshiftRight(Bit);
            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }
    }

    std::cout << "Testing Flip\n";
    {
        TBitArray<uint8> BitArray(BitCount, false);
        for (int32 Bit = 0; Bit < BitCount; ++Bit)
        {
            BitArray.FlipBit(Bit);

            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }
    }

    SUCCESS();
}
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// StaticBitArray Test

#if RUN_TSTATICBITARRAY_TEST
#include <Core/Containers/StaticBitArray.h>

bool TStaticBitArray_Test()
{
    std::cout << '\n' << "----------TStaticBitArray----------" << '\n' << '\n';

    {
        CONSTEXPR TStaticBitArray<11, uint8> BitArray;
        static_assert(BitArray.Size()         == 11);
        static_assert(BitArray.Capacity()     == 16);
        static_assert(BitArray.StorageSize()     == 2);
        static_assert(BitArray.CapacityInBytes() == 2);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00000000000");
    }

    {
        CONSTEXPR TStaticBitArray<18, uint32> BitArray;
        static_assert(BitArray.Size()         == 18);
        static_assert(BitArray.Capacity()     == 32);
        static_assert(BitArray.StorageSize()     == 1);
        static_assert(BitArray.CapacityInBytes() == 4);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "000000000000000000");
    }

    {
        CONSTEXPR TStaticBitArray<9, uint8> BitArray(8, true);
        static_assert(BitArray.Size()         == 9);
        static_assert(BitArray.Capacity()     == 16);
        static_assert(BitArray.StorageSize()     == 2);
        static_assert(BitArray.CapacityInBytes() == 2);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "011111111");
    }

    {
        CONSTEXPR TStaticBitArray<8, uint8> BitArray(uint8(0b01010101));
        static_assert(BitArray.Size()         == 8);
        static_assert(BitArray.Capacity()     == 8);
        static_assert(BitArray.StorageSize()     == 1);
        static_assert(BitArray.CapacityInBytes() == 1);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "01010101");
    }

    {
        CONSTEXPR const uint8 Bits[] =
        {
            0b01010101,
            0b01010101,
        };

        CONSTEXPR TStaticBitArray<14, uint8> BitArray(Bits, ARRAY_COUNT(Bits));
        static_assert(BitArray.Size() == 14);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "01010101010101");
    }

    {
        CONSTEXPR TStaticBitArray<8> BitArray = { false, true, false, true };
        static_assert(BitArray.Size() == 8);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00001010");
    }

    {
        CONSTEXPR TStaticBitArray<9, uint8> BitArray0(9, true);
        static_assert(BitArray0.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        CONSTEXPR TStaticBitArray<9, uint8> BitArray1(BitArray0);
        static_assert(BitArray1.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray1) == "111111111");
        
        static_assert((BitArray0 == BitArray1) == true);
    }

    {
        CONSTEXPR TStaticBitArray<9, uint8> BitArray0(9, true);
        static_assert(BitArray0.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        CONSTEXPR TStaticBitArray<9, uint8> BitArray1(Move(BitArray0));
        static_assert(BitArray1.Size() == 9);
        
        TEST_CHECK(MakeStringFromBitArray(BitArray0) == "111111111");
        TEST_CHECK(MakeStringFromBitArray(BitArray1) == "111111111");
        
        static_assert((BitArray0 == BitArray1) == true);
    }

    {
        TStaticBitArray<9, uint8> BitArray(9, true);
        static_assert(BitArray.Size() == 9);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "111111111");

        BitArray.ResetWithZeros();

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "000000000");

        BitArray.ResetWithOnes();

        TEST_CHECK(MakeStringFromBitArray(BitArray) == "111111111");
    }

    {
        TStaticBitArray<8, uint8> BitArray(0b00000000);
        TEST_CHECK(BitArray.Size() == 8);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00000000");

        BitArray.AssignBit(3, true);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00001000");

        BitArray.FlipBit(4);
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "00011000");

        TEST_CHECK(BitArray.CountAssignedBits() == 2);
        TEST_CHECK(BitArray.HasAnyBitSet()      == true);
        TEST_CHECK(BitArray.HasNoBitSet()       == false);

        const uint32 MostSignificantBit = BitArray.MostSignificant();
        TEST_CHECK(MostSignificantBit == 4);

        const uint32 LeastSignificantBit = BitArray.LeastSignificant();
        TEST_CHECK(LeastSignificantBit == 3);
    }

    CONSTEXPR int32 BitCount = 18;
    {
        TStaticBitArray<BitCount, uint8> BitArray(BitCount, false);
        for (int32 Index = 0; Index < BitCount; ++Index)
        {
            if (Index % 2 == 0)
            {
                BitArray.FlipBit(Index);
            }
        }

        std::cout << MakeStringFromBitArray(BitArray) << '\n';
        TEST_CHECK(MakeStringFromBitArray(BitArray) == "010101010101010101");
    }

    std::cout << "Testing BitShift Left\n";
    {
        for (int32 Bit = 0; Bit <= BitCount; ++Bit)
        {
            TStaticBitArray<BitCount, uint8> BitArray;

            bool Value = true;
            for (int32 Index = 0; Index < BitCount; Index += 2)
            {
                BitArray.AssignBit(Index, Value);
                BitArray.AssignBit(Index + 1, Value);
                Value = !Value;
            }

            BitArray.BitshiftLeft(Bit);
            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }
    }

    std::cout << "Testing BitShift Right\n";
    {
        for (int32 Bit = 0; Bit <= BitCount; ++Bit)
        {
            TStaticBitArray<BitCount, uint8> BitArray;

            bool Value = true;
            for (int32 Index = 0; Index < BitCount; Index += 2)
            {
                BitArray.AssignBit(Index    , Value);
                BitArray.AssignBit(Index + 1, Value);
                Value = !Value;
            }

            BitArray.BitshiftRight(Bit);
            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }
    }

    std::cout << "Testing Flip\n";
    {
        TStaticBitArray<BitCount, uint8> BitArray(BitCount, false);
        for (int32 Bit = 0; Bit < BitCount; ++Bit)
        {
            BitArray.FlipBit(Bit);
            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }
    }

    SUCCESS();
}
#endif

#endif