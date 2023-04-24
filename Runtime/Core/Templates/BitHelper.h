#pragma once
#include "TypeTraits.h"

struct FBitHelper
{
    template<typename IndexType, typename MaskType>
    static constexpr IndexType LeastSignificant(MaskType Mask) noexcept
    {
        IndexType Result = 0;
        while (Mask)
        {
            if ((Mask & 1) == 1)
            {
                break;
            }

            ++Result;
            Mask >>= 1;
        }

        return Result;
    }

    template<typename IndexType, typename MaskType>
    static constexpr IndexType MostSignificant(MaskType Mask) noexcept
    {
        IndexType Result = 0;
        while (Mask >>= 1)
        {
            ++(Result);
        }

        return Result;
    }

    template<typename StorageType>
    static typename TEnableIf<TIsUnsigned<StorageType>::Value, StorageType>::Type ReverseBits(StorageType Bits) noexcept
    {
        constexpr StorageType NumBits = sizeof(StorageType) * 8;

        StorageType BitCount = NumBits - 1;
        StorageType NewBits  = Bits;
        Bits >>= 1;

        while (Bits)
        {
            NewBits <<= 1;
            NewBits |= Bits & 1;
            Bits >>= 1;
            BitCount--;
        }

        NewBits <<= BitCount;
        return NewBits;
    }

    // TODO: Check if intrinsics are viable
    template<typename IntegerType>
    static constexpr IntegerType CountAssignedBits(IntegerType Element)
    {
        IntegerType NumBits = 0;
        while (Element)
        {
            if ((Element & 1) == 1)
            {
                NumBits++;
            }

            Element >>= 1;
        }

        return NumBits;
    }
};