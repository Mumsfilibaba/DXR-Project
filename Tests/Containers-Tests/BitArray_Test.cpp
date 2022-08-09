#include "BitArray_Test.h"

#if RUN_TBITARRAY_TEST || RUN_TSTATICBITARRAY_TEST
#include "TestUtils.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

template<typename BitArrayType>
std::string MakeStringFromBitArray(const BitArrayType& BitArray)
{
    std::string NewString;
    NewString.reserve(BitArray.GetSize());

    for (BitArrayType::SizeType Index = 0; Index < BitArray.GetSize(); ++Index)
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
        CHECK(BitArray.GetSize()            == 0);
        CHECK(BitArray.Capacity()        == 0);
        CHECK(BitArray.StorageSize()     == 0);
        CHECK(BitArray.CapacityInBytes() == 0);
        CHECK(MakeStringFromBitArray(BitArray) == "");
    }

    {
        TBitArray<uint32> BitArray;
        CHECK(BitArray.GetSize()            == 0);
        CHECK(BitArray.Capacity()        == 0);
        CHECK(BitArray.StorageSize()     == 0);
        CHECK(BitArray.CapacityInBytes() == 0);
        CHECK(MakeStringFromBitArray(BitArray) == "");
    }

    {
        TBitArray BitArray(8, true);
        CHECK(BitArray.GetSize()            == 8);
        CHECK(BitArray.Capacity()        == 32);
        CHECK(BitArray.StorageSize()     == 1);
        CHECK(BitArray.CapacityInBytes() == 4);
        CHECK(MakeStringFromBitArray(BitArray) == "11111111");
    }

    {
        TBitArray<uint8> BitArray(uint8(0b01010101));
        CHECK(BitArray.GetSize()            == 8);
        CHECK(BitArray.Capacity()        == 8);
        CHECK(BitArray.StorageSize()     == 1);
        CHECK(BitArray.CapacityInBytes() == 1);
        CHECK(MakeStringFromBitArray(BitArray) == "01010101");
    }

    {
        const uint8 Bits[] =
        {
            0b01010101,
            0b01010101,
        };

        TBitArray<uint8> BitArray(Bits, ARRAY_COUNT(Bits));
        CHECK(BitArray.GetSize() == 16);
        CHECK(MakeStringFromBitArray(BitArray) == "0101010101010101");
    }

    {
        TBitArray BitArray = { false, true, false, true };
        CHECK(BitArray.GetSize() == 4);
        CHECK(MakeStringFromBitArray(BitArray) == "1010");
    }

    {
        TBitArray<uint8> BitArray0(9, true);
        CHECK(BitArray0.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        TBitArray<uint8> BitArray1(BitArray0);
        CHECK(BitArray1.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray1) == "111111111");

        CHECK((BitArray0 == BitArray1) == true);
    }

    {
        TBitArray<uint8> BitArray0(9, true);
        CHECK(BitArray0.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        TBitArray<uint8> BitArray1(Move(BitArray0));
        CHECK(BitArray0.GetSize() == 0);
        CHECK(BitArray1.GetSize() == 9);
        
        CHECK(MakeStringFromBitArray(BitArray0) == "");
        CHECK(MakeStringFromBitArray(BitArray1) == "111111111");

        CHECK((BitArray0 != BitArray1) == true);
    }

    {
        TBitArray<uint8> BitArray(9, true);
        CHECK(BitArray.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray) == "111111111");

        BitArray.ResetWithZeros();

        CHECK(MakeStringFromBitArray(BitArray) == "000000000");

        BitArray.ResetWithOnes();

        CHECK(MakeStringFromBitArray(BitArray) == "111111111");
    }

    {
        TBitArray<uint8> BitArray;
        CHECK(BitArray.GetSize()    == 0);
        CHECK(BitArray.IsEmpty() == true);

        BitArray.Push(false);
        BitArray.Push(true);
        BitArray.Push(false);
        BitArray.Push(true);
        BitArray.Push(true);
        BitArray.Push(false);
        BitArray.Push(true);
        BitArray.Push(true);
        BitArray.Push(false);
        BitArray.Push(true);
        BitArray.Push(false);

        CHECK(BitArray.GetSize()    == 11);
        CHECK(BitArray.IsEmpty() == false);

        CHECK(MakeStringFromBitArray(BitArray) == "01011011010");
    }

    {
        TBitArray<uint8> BitArray(0b00000000);
        CHECK(BitArray.GetSize() == 8);
        CHECK(MakeStringFromBitArray(BitArray) == "00000000");

        BitArray.AssignBit(3, true);
        CHECK(MakeStringFromBitArray(BitArray) == "00001000");

        BitArray.FlipBit(4);
        CHECK(MakeStringFromBitArray(BitArray) == "00011000");

        CHECK(BitArray.CountAssignedBits() == 2);
        CHECK(BitArray.HasAnyBitSet()      == true);
        CHECK(BitArray.HasNoBitSet()       == false);

        const uint32 MostSignificantBit = BitArray.MostSignificant();
        CHECK(MostSignificantBit == 4);

        const uint32 LeastSignificantBit = BitArray.LeastSignificant();
        CHECK(LeastSignificantBit == 3);
    }

    {
        TBitArray BitArray = { false, true, false, true };
        CHECK(BitArray.GetSize() == 4);
        CHECK(MakeStringFromBitArray(BitArray) == "1010");

        BitArray = ~BitArray;

        CHECK(BitArray.GetSize() == 4);
        CHECK(MakeStringFromBitArray(BitArray) == "0101");
    }

    constexpr int32 BitCount = 18;
    {
        TBitArray<uint8> BitArray(BitCount, false);
        for (int32 Bit = 0; Bit < BitCount; ++Bit)
        {
            BitArray.Insert(3, true);

            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }

        CHECK(MakeStringFromBitArray(BitArray) == "000000000000000111111111111111111000");
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

        CHECK(MakeStringFromBitArray(BitArray) == "010101010101010101");

        for (int32 Bit = 0; Bit < BitCount; ++Bit)
        {
            if (BitArray.GetSize() > 3)
            {
                BitArray.Remove(3);
            }
            else
            {
                break;
            }

            std::cout << MakeStringFromBitArray(BitArray) << '\n';
        }

        CHECK(MakeStringFromBitArray(BitArray) == "101");
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
                    BitArray.Push(false);
                    BitArray.Push(false);
                }
                else
                {
                    BitArray.Push(true);
                    BitArray.Push(true);
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
                    BitArray.Push(false);
                    BitArray.Push(false);
                }
                else
                {
                    BitArray.Push(true);
                    BitArray.Push(true);
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
        static_assert(BitArray.GetSize()            == 11);
        static_assert(BitArray.Capacity()        == 16);
        static_assert(BitArray.StorageSize()     == 2);
        static_assert(BitArray.CapacityInBytes() == 2);
        CHECK(MakeStringFromBitArray(BitArray) == "00000000000");
    }

    {
        CONSTEXPR TStaticBitArray<18, uint32> BitArray;
        static_assert(BitArray.GetSize()            == 18);
        static_assert(BitArray.Capacity()        == 32);
        static_assert(BitArray.StorageSize()     == 1);
        static_assert(BitArray.CapacityInBytes() == 4);
        CHECK(MakeStringFromBitArray(BitArray) == "000000000000000000");
    }

    {
        CONSTEXPR TStaticBitArray<9, uint8> BitArray(8, true);
        static_assert(BitArray.GetSize()            == 9);
        static_assert(BitArray.Capacity()        == 16);
        static_assert(BitArray.StorageSize()     == 2);
        static_assert(BitArray.CapacityInBytes() == 2);
        CHECK(MakeStringFromBitArray(BitArray) == "011111111");
    }

    {
        CONSTEXPR TStaticBitArray<8, uint8> BitArray(uint8(0b01010101));
        static_assert(BitArray.GetSize()            == 8);
        static_assert(BitArray.Capacity()        == 8);
        static_assert(BitArray.StorageSize()     == 1);
        static_assert(BitArray.CapacityInBytes() == 1);
        CHECK(MakeStringFromBitArray(BitArray) == "01010101");
    }

    {
        CONSTEXPR const uint8 Bits[] =
        {
            0b01010101,
            0b01010101,
        };

        CONSTEXPR TStaticBitArray<14, uint8> BitArray(Bits, ARRAY_COUNT(Bits));
        static_assert(BitArray.GetSize() == 14);
        CHECK(MakeStringFromBitArray(BitArray) == "01010101010101");
    }

    {
        CONSTEXPR TStaticBitArray<8> BitArray = { false, true, false, true };
        static_assert(BitArray.GetSize() == 8);
        CHECK(MakeStringFromBitArray(BitArray) == "00001010");
    }

    {
        CONSTEXPR TStaticBitArray<9, uint8> BitArray0(9, true);
        static_assert(BitArray0.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        CONSTEXPR TStaticBitArray<9, uint8> BitArray1(BitArray0);
        static_assert(BitArray1.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray1) == "111111111");
        
        static_assert((BitArray0 == BitArray1) == true);
    }

    {
        CONSTEXPR TStaticBitArray<9, uint8> BitArray0(9, true);
        static_assert(BitArray0.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray0) == "111111111");

        CONSTEXPR TStaticBitArray<9, uint8> BitArray1(Move(BitArray0));
        static_assert(BitArray1.GetSize() == 9);
        
        CHECK(MakeStringFromBitArray(BitArray0) == "111111111");
        CHECK(MakeStringFromBitArray(BitArray1) == "111111111");
        
        static_assert((BitArray0 == BitArray1) == true);
    }

    {
        TStaticBitArray<9, uint8> BitArray(9, true);
        static_assert(BitArray.GetSize() == 9);
        CHECK(MakeStringFromBitArray(BitArray) == "111111111");

        BitArray.ResetWithZeros();

        CHECK(MakeStringFromBitArray(BitArray) == "000000000");

        BitArray.ResetWithOnes();

        CHECK(MakeStringFromBitArray(BitArray) == "111111111");
    }

    {
        TStaticBitArray<8, uint8> BitArray(0b00000000);
        CHECK(BitArray.GetSize() == 8);
        CHECK(MakeStringFromBitArray(BitArray) == "00000000");

        BitArray.AssignBit(3, true);
        CHECK(MakeStringFromBitArray(BitArray) == "00001000");

        BitArray.FlipBit(4);
        CHECK(MakeStringFromBitArray(BitArray) == "00011000");

        CHECK(BitArray.CountAssignedBits() == 2);
        CHECK(BitArray.HasAnyBitSet()      == true);
        CHECK(BitArray.HasNoBitSet()       == false);

        const uint32 MostSignificantBit = BitArray.MostSignificant();
        CHECK(MostSignificantBit == 4);

        const uint32 LeastSignificantBit = BitArray.LeastSignificant();
        CHECK(LeastSignificantBit == 3);
    }

    constexpr int32 BitCount = 18;
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
        CHECK(MakeStringFromBitArray(BitArray) == "010101010101010101");
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