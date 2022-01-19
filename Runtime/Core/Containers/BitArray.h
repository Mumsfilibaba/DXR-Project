#pragma once
#include "Core/Core.h"
#include "Iterator.h"

#include "Core/Templates/IsInteger.h"
#include "Core/Memory/Memory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions for BitArray

namespace NBitUtils
{
    template<typename T>
    inline bool LeastSignificantBit(T Mask, uint32& OutIndex)
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

    template<typename T>
    inline bool MostSignificantBit(T Mask, uint32& OutIndex)
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
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Packed bits

template<uint32 NumBits, typename StorageType>
class TBitArray
{
    typedef TBitArray<NumBits, StorageType> ThisType;

public:

    template<uint32, typename>
    friend class TBitArray;

    FORCEINLINE TBitArray()
        : Data()
    {
        ZeroAllBits();
    }

    explicit TBitArray(bool InitAllBits)
    {
        if (InitAllBits)
        {
            SetAll();
        }
        else
        {
            ZeroAllBits();
        }
    }

    template<typename T>
    explicit TBitArray(T Value)
    {
        static_assert(TIsInteger<T>::Value, "Value must be an integral type");

        ZeroAllBits();

        const uint32 Size = (sizeof(Value) < sizeof(StorageType) * Elements()) ? sizeof(Value) : sizeof(StorageType) * Elements();
        CMemory::Memcpy(Data, &Value, Size);
    }

    template<uint32 OtherNumBits, typename OtherStorageType>
    TBitArray(const TBitArray<OtherNumBits, OtherStorageType>& Other)
    {
        static_assert(NumBits <= OtherNumBits, "Cannot copy from a bitfield with more bits");

        ZeroAllBits();

        const uint32 size = NumBits <= OtherNumBits ? NumBits : OtherNumBits;
        CMemory::Memcpy(Data, Other.Data, size / 8);
    }

    FORCEINLINE void ZeroAllBits()
    {
        CMemory::Memset(Data, 0x00000000, sizeof(StorageType) * Elements());
    }

    FORCEINLINE void SetAll()
    {
        CMemory::Memset(Data, 0xffffffff, sizeof(StorageType) * Elements());
    }

    inline void SetBit(uint32 BitIndex)
    {
        Assert(BitIndex < Size());
        Data[StorageIndexOfBit(BitIndex)] |= MakeBitmaskForStorage(BitIndex);
    }

    inline void ZeroBit(uint32 BitIndex)
    {
        Assert(BitIndex < Size());
        Data[StorageIndexOfBit(BitIndex)] &= ~MakeBitmaskForStorage(BitIndex);
    }

    inline bool GetBit(uint32 BitIndex) const
    {
        Assert(BitIndex < Size());
        return (Data[StorageIndexOfBit(BitIndex)] & MakeBitmaskForStorage(BitIndex)) != 0;
    }

    FORCEINLINE void AssignBit(uint32 BitIndex, bool Value)
    {
        Value ? SetBit(BitIndex) : ZeroBit(BitIndex);
    }

    inline void SetRange(uint32 From, uint32 To, bool Value = true)
    {
        Assert(From < Size());
        Assert(To <= Size());
        Assert(From <= To);

        while (From < To)
        {
            uint32 FromInStorage = From % BitsPerStorage();
            uint32 StorageIndex = StorageIndexOfBit(From);
            uint32 MaxBitInStorage = (StorageIndex + 1) * BitsPerStorage();

            StorageType Mask = static_cast<StorageType>(~0) << FromInStorage;
            if (To < MaxBitInStorage)
            {
                StorageType Mask2 = (static_cast<StorageType>(1) << (To % BitsPerStorage())) - static_cast<StorageType>(1);
                Mask &= Mask2;
            }

            if (Value)
            {
                Data[StorageIndex] |= Mask;
            }
            else
            {
                Data[StorageIndex] &= ~Mask;
            }

            From = MaxBitInStorage;
        }
    }

    FORCEINLINE void SetBitAndUp(uint32 BitIndex, uint32 Count = ~0)
    {
        Assert(BitIndex < Size());

        Count = Count < Size() - BitIndex ? Count : Size() - BitIndex;
        SetRange(BitIndex, BitIndex + Count);
    }

    FORCEINLINE void SetBitAndDown(uint32 BitIndex, uint32 Count = ~0)
    {
        Assert(BitIndex < Size());

        Count = BitIndex < Count ? BitIndex : Count;
        SetRange(BitIndex - Count, BitIndex);
    }

    FORCEINLINE TBitArrayIterator<ThisType> StartIterator() const
    {
        return TBitArrayIterator<ThisType>(*this, 0);
    }

    inline bool HasAnyBitSet() const
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            if (Data[Index] > 0)
            {
                return true;
            }
        }

        return false;
    }

    inline bool HasNoBitSet() const
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            if (Data[Index] > 0)
            {
                return false;
            }
        }

        return true;
    }

    inline bool MostSignificantBit(uint32& OutIndex) const
    {
        for (int32 Index = static_cast<int32>(Elements()) - 1; Index >= 0; --Index)
        {
            if (NBitUtils::MostSignificantBit(Data[Index], OutIndex))
            {
                OutIndex += Index * BitsPerStorage();
                return true;
            }
        }

        return false;
    }

    inline bool LeastSignificantBit(uint32* OutIndex) const
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            if (NBitUtils::LeastSignificantBit(Data[Index], OutIndex))
            {
                OutIndex += Index * BitsPerStorage();
                return true;
            }
        }

        return false;
    }

    FORCEINLINE bool operator[](uint32 BitIndex) const
    {
        return GetBit(BitIndex);
    }

    FORCEINLINE bool operator==(const TBitArray& Other) const
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            if (Data[Index] != Other.Data[Index])
            {
                return false;
            }
        }
        return true;
    }

    FORCEINLINE bool operator!=(const TBitArray& Other) const
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            if (Data[Index] != Other.Data[Index])
            {
                return true;
            }
        }

        return false;
    }

    FORCEINLINE TBitArray& operator&=(const TBitArray& Other)
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            Data[Index] &= Other.Data[Index];
        }

        return *this;
    }

    FORCEINLINE TBitArray& operator|=(const TBitArray& Other)
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            Data[Index] |= Other.Data[Index];
        }

        return *this;
    }

    FORCEINLINE TBitArray& operator^=(const TBitArray& Other)
    {
        for (uint32 Index = 0; Index < Elements(); ++Index)
        {
            Data[Index] ^= Other.Data[Index];
        }

        return *this;
    }

    FORCEINLINE TBitArray operator&(const TBitArray& Other) const
    {
        TBitArray NewArray;
        for (uint32 i = 0; i < Elements(); ++i)
        {
            NewArray.Data[i] = Data[i] & Other.Data[i];
        }

        return NewArray;
    }

    FORCEINLINE TBitArray operator|(const TBitArray& Other) const
    {
        TBitArray NewArray;
        for (uint32 i = 0; i < Elements(); ++i)
        {
            NewArray.Data[i] = Data[i] | Other.Data[i];
        }

        return NewArray;
    }

    FORCEINLINE TBitArray operator^(const TBitArray& Other) const
    {
        TBitArray NewArray;
        for (uint32 i = 0; i < Elements(); ++i)
        {
            NewArray.Data[i] = Data[i] ^ Other.Data[i];
        }

        return NewArray;
    }

    FORCEINLINE TBitArray operator~() const
    {
        TBitArray NewArray;
        for (uint32 i = 0; i < Elements(); ++i)
        {
            NewArray.Data[i] = ~Data[i];
        }

        return NewArray;
    }

    static constexpr uint32 Size()
    {
        return NumBits;
    }

    static constexpr uint32 Capacity()
    {
        return NumBits;
    }

public:

    FORCEINLINE TBitArrayIterator<ThisType> begin() const
    {
        return TBitArrayIterator<ThisType>(*this, 0);
    }

    FORCEINLINE TBitArrayIterator<ThisType> end() const
    {
        uint32 Index;
        LeastSignificantBit(&Index);

        return TBitArrayIterator<ThisType>(*this, Index);
    }

private:

    static constexpr uint32 StorageIndexOfBit(uint32 BitIndex)
    {
        return BitIndex / BitsPerStorage();
    }

    static constexpr uint32 IndexOfBitInStorage(uint32 BitIndex)
    {
        return BitIndex % BitsPerStorage();
    }

    static constexpr uint32 BitsPerStorage()
    {
        return sizeof(StorageType) * 8;
    }

    static constexpr StorageType MakeBitmaskForStorage(uint32 BitIndex)
    {
        return StorageType(1) << IndexOfBitInStorage(BitIndex);
    }

    static constexpr uint32 Elements()
    {
        return (NumBits + BitsPerStorage() - 1) / BitsPerStorage();
    }

    StorageType Data[Elements()];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Pre-defined types

template<uint32 NumBits, typename StorageType = uint32>
class TBitArray;

using BitField16 = TBitArray<16, uint16>;
using BitField32 = TBitArray<32, uint32>;
using BitField64 = TBitArray<64, uint32>;