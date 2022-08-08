#pragma once
#include "EnableIf.h"
#include "IsSigned.h"

#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions for BitArray

namespace NBits
{
    template<typename T, typename IndexType = uint32>
    inline bool LeastSignificant(T Mask, IndexType& OutIndex) noexcept
    {
        OutIndex = ~0u;

        while (Mask)
        {
            OutIndex += 1;
            if ((Mask & 1) == 1)
            {
                return true;
            }

            Mask >>= 1;
        }

        return false;
    }

    template<typename T, typename IndexType = uint32>
    inline bool MostSignificant(T Mask, IndexType& OutIndex) noexcept
    {
        if (Mask == 0)
        {
            return false;
        }

        OutIndex = 0;
        while (Mask >>= 1)
        {
            ++(OutIndex);
        }

        return true;
    }

    template<typename StorageType>
    inline typename TEnableIf<TIsUnsigned<StorageType>::Value, StorageType>::Type ReverseBits(StorageType Bits) noexcept
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
}