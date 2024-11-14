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

    template<typename T>
    static constexpr typename TEnableIf<TIsUnsigned<T>::Value, T>::Type ReverseBits(T Bits) noexcept
    {
        constexpr T NumBits = sizeof(T) * 8;

        T BitCount = NumBits - 1;
        T NewBits  = Bits;
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
    template<typename T>
    static constexpr T CountAssignedBits(T Element)
    {
        T NumBits = 0;
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