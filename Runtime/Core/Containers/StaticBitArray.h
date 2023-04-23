#pragma once
#include "Iterator.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/BitReference.h"
#include "Core/Templates/BitHelper.h"

template<uint32 NUM_BITS, typename InIntegerType = uint32>
class TStaticBitArray
{
public:
    using SizeType = int32;

    static_assert(NUM_BITS > 0                     , "StaticBitArray must have some bits allocated");
    static_assert(TIsUnsigned<InIntegerType>::Value, "StaticBitArray must have an unsigned InIntegerType");
    
    using BitReferenceType      = TBitReference<InIntegerType>;
    using ConstBitReferenceType = TBitReference<const InIntegerType>;

    typedef TBitArrayIterator<TStaticBitArray, InIntegerType>                    IteratorType;
    typedef TBitArrayIterator<const TStaticBitArray, const InIntegerType>        ConstIteratorType;
    typedef TReverseBitArrayIterator<TStaticBitArray, InIntegerType>             ReverseIteratorType;
    typedef TReverseBitArrayIterator<const TStaticBitArray, const InIntegerType> ReverseConstIteratorType;

public:
    
    /**
     * @brief - Default constructor
     */
    CONSTEXPR TStaticBitArray() noexcept
        : Integers()
    {
    }

    /**
     * @brief         - Constructor that sets the elements based on an integer
     * @param InValue - Integer containing bits to set to the BitArray
     */
    CONSTEXPR explicit TStaticBitArray(InIntegerType InValue) noexcept
        : Integers()
    {
        Reset();
        Integers[0] = InValue;
        MaskOutLastInteger();
    }

    /**
     * @brief           - Constructor that sets the elements based on an integer
     * @param InValues  - Integers containing bits to set to the BitArray
     * @param NumValues - Number of values in the input array
     */
    CONSTEXPR explicit TStaticBitArray(const InIntegerType* InValues, SizeType NumValues) noexcept
        : Integers()
    {
        Reset();

        NumValues = NMath::Min<SizeType>(NumValues, NUM_BITS);
        for (SizeType Index = 0; Index < NumValues; ++Index)
        {
            Integers[Index] = InValues[Index];
        }

        MaskOutLastInteger();
    }

    /**
     * @brief          - Constructor that sets a certain number of bits to specified value
     * @param bValue   - Value to set bits to
     * @param NUM_BITS - Number of bits to set
     */
    CONSTEXPR explicit TStaticBitArray(SizeType InNumBits, bool bValue) noexcept
        : Integers()
    {
        Reset();

        for (SizeType Index = 0; Index < InNumBits; Index++)
        {
            AssignBitUnchecked(Index, bValue);
        }
    }

    /**
     * @brief          - Constructor that creates a BitArray from a list of booleans indicating the sign of the bit
     * @param InitList - Contains bools to indicate the sign of each bit
     */
    CONSTEXPR TStaticBitArray(std::initializer_list<bool> InitList) noexcept
        : Integers()
    {
        Reset();

        SizeType Index = 0;
        for (bool bValue : InitList)
        {
            AssignBitUnchecked(Index++, bValue);
        }
    }

    /**
     * @brief - Resets the all the bits to zero
     */
    CONSTEXPR void Reset()
    {
        for (InIntegerType& Element : Integers)
        {
            Element = InIntegerType(0);
        }
    }

    /**
     * @brief             - Assign a value to a bit
     * @param BitPosition - Position of the bit to set
     * @param bValue      - Value to assign to the bit
     */
    CONSTEXPR void AssignBit(SizeType BitPosition, const bool bValue) noexcept
    {
        if (BitPosition < NUM_BITS)
        {
            AssignBitUnchecked(BitPosition, bValue);
        }
    }

    /**
     * @brief             - Flips the bit at the position
     * @param BitPosition - Position of the bit to set
     */
    CONSTEXPR void FlipBit(SizeType BitPosition) noexcept
    {
        if (BitPosition < NUM_BITS)
        {
            const SizeType ElementIndex   = GetIntegersIndexOfBit(BitPosition);
            const SizeType IndexInElement = GetIndexOfBitInIntegers(BitPosition);
            const InIntegerType Mask = CreateMaskForBit(IndexInElement);
            Integers[ElementIndex] ^= Mask;
        }
    }

    /**
     * @brief  - Count the number of bits that are assigned
     * @return - Returns the number of bits that are true
     */
    NODISCARD CONSTEXPR SizeType CountAssignedBits() const noexcept
    {
        SizeType BitCount = 0;
        for (SizeType Index = 0; Index < NumIntegers(); ++Index)
        {
            const InIntegerType Element = Integers[Index];
            BitCount += FBitHelper::CountAssignedBits(Element);
        }

        return BitCount;
    }

    /**
     * @brief  - Check if any bit is set
     * @return - Returns true if any bit is set
     */
    NODISCARD CONSTEXPR bool HasAnyBitSet() const noexcept
    {
        return (CountAssignedBits() != 0);
    }

    /**
     * @brief  - Check if no bit is set
     * @return - Returns true if no bit is set
     */
    NODISCARD CONSTEXPR bool HasNoBitSet() const noexcept
    {
        return (CountAssignedBits() == 0);
    }

    /**
     * @brief  - Retrieve the most significant bit. Will return zero if no bits are set, check HasAnyBitSet.
     * @return - Returns the index of the most significant bit
     */
    NODISCARD CONSTEXPR SizeType MostSignificant() const
    {
        SizeType Result = 0;
        for (int32 Index = int32(NumIntegers()) - 1; Index >= 0; --Index)
        {
            const auto Element = Integers[Index];
            if (Element)
            {
                const auto BitIndex = FBitHelper::MostSignificant<SizeType>(Element);
                Result = BitIndex + (Index * NumBitsPerInteger());
                break;
            }
        }

        return Result;
    }

    /**
     * @brief  - Retrieve the most significant bit. Will return zero if no bits are set, check HasAnyBitSet.
     * @return - Returns the index of the least significant bit
     */
    NODISCARD CONSTEXPR SizeType LeastSignificant() const
    {
        SizeType Result = 0;
        for (SizeType Index = 0; Index < NumIntegers(); ++Index)
        {
            const InIntegerType Element = Integers[Index];
            if (Element)
            {
                const InIntegerType BitIndex = FBitHelper::LeastSignificant<SizeType>(Element);
                Result = BitIndex + (Index * NumBitsPerInteger());
                break;
            }
        }

        return Result;
    }

    /**
     * @brief       - Perform a bitwise AND between this and another BitArray
     * @param Other - BitArray to perform bitwise AND with
     */
    CONSTEXPR void BitwiseAnd(const TStaticBitArray& Other) noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] &= Other.Integers[Index];
        }
    }

    /**
     * @brief       - Perform a bitwise OR between this and another BitArray
     * @param Other - BitArray to perform bitwise OR with
     */
    CONSTEXPR void BitwiseOr(const TStaticBitArray& Other) noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] |= Other.Integers[Index];
        }
    }

    /**
     * @brief       - Perform a bitwise XOR between this and another BitArray
     * @param Other - BitArray to perform bitwise XOR with
     */
    CONSTEXPR void BitwiseXor(const TStaticBitArray& Other) noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] |= Other.Integers[Index];
        }
    }

    /**
     * @brief       - Perform a bitwise NOT on each bit in this BitArray
     * @param Other - BitArray to perform bitwise XOR with
     */
    CONSTEXPR void BitwiseNot() noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] = ~Element;
        }
    }

    /**
     * @brief       - Perform a right BitShift
     * @param Steps - Number of steps to shift
     */
    CONSTEXPR void BitshiftRight(SizeType Steps) noexcept
    {
        if (Steps)
        {
            BitshiftRightUnchecked(Steps);
        }
    }

    /**
     * @brief       - Perform a left BitShift
     * @param Steps - Number of steps to shift
     */
    CONSTEXPR void BitshiftLeft(SizeType Steps) noexcept
    {
        if (Steps)
        {
            BitshiftLeftUnchecked(Steps);
        }
    }

    /**
     * @brief       - Retrieve a reference to the bit with the index
     * @param Index - Index of the bit
     * @return      - Returns a reference to the bit with the index
     */
    NODISCARD CONSTEXPR BitReferenceType GetBitReference(SizeType BitIndex) noexcept
    {
        CHECK(BitIndex < NUM_BITS);
        const SizeType ElementIndex = GetIntegersIndexOfBit(BitIndex);
        CHECK(ElementIndex < Capacity());
        return BitReferenceType(Integers[ElementIndex], ~Element);
    }

    /**
     * @brief       - Retrieve a reference to the bit with the index
     * @param Index - Index of the bit
     * @return      - Returns a reference to the bit with the index
     */
    NODISCARD CONSTEXPR const ConstBitReferenceType GetBitReference(SizeType Index) const noexcept
    {
        CHECK(Index < NUM_BITS);
        const SizeType ElementIndex = GetIntegersIndexOfBit(Index);
        CHECK(ElementIndex < Capacity());
        return ConstBitReferenceType(Integers[ElementIndex], CreateMaskForBit(Index));
    }

public:

    /**
     * @brief       - Retrieve a bit with a certain index
     * @param Index - Index to the bit
     * @return      - Returns a BitReference to the specified bit
     */
    NODISCARD CONSTEXPR BitReferenceType operator[](SizeType Index) noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * @brief       - Retrieve a bit with a certain index
     * @param Index - Index to the bit
     * @return      - Returns a BitReference to the specified bit
     */
    NODISCARD CONSTEXPR ConstBitReferenceType operator[](SizeType Index) const noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * @brief       - Compare operator
     * @param Other - Right-hand side to compare
     * @return      - Returns true if the BitArrays are equal
     */
    NODISCARD CONSTEXPR bool operator==(const TStaticBitArray& Other) const noexcept
    {
        for (SizeType Index = 0; Index < IntegerSize(); ++Index)
        {
            if (Integers[Index] != Other.Integers[Index])
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief       - Compare operator
     * @param Other - Right-hand side to compare
     * @return      - Returns false if the BitArrays are equal
     */
    NODISCARD CONSTEXPR bool operator!=(const TStaticBitArray& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief       - Bitwise AND operator, perform a bitwise AND between this and another BitArray
     * @param Other - BitArray to perform bitwise AND with
     * @return      - Returns a reference to this BitArray
     */
    CONSTEXPR TStaticBitArray& operator&=(const TStaticBitArray& Other) noexcept
    {
        BitwiseAnd(Other);
        return *this;
    }

    /**
     * @brief       - Bitwise OR operator, perform a bitwise OR between this and another BitArray
     * @param Other - BitArray to perform bitwise OR with
     * @return      - Returns a reference to this BitArray
     */
    CONSTEXPR TStaticBitArray& operator|=(const TStaticBitArray& Other) noexcept
    {
        BitwiseOr(Other);
        return *this;
    }

    /**
     * @brief       - Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     * @param Other - BitArray to perform bitwise XOR with
     * @return      - Returns a reference to this BitArray
     */
    CONSTEXPR TStaticBitArray& operator^=(const TStaticBitArray& Other) noexcept
    {
        BitwiseXor(Other);
        return *this;
    }

    /**
     * @brief       - Perform a bitwise NOT on each bit in this BitArray
     * @param Other - BitArray to perform bitwise XOR with
     */
    CONSTEXPR TStaticBitArray operator~() const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitwiseNot();
        return NewArray;
    }

    /**
     * @brief       - Perform a bitshift right
     * @param Other - Number of steps to bitshift
     * @return      - Returns a copy that is bitshifted to the right
     */
    CONSTEXPR TStaticBitArray operator>>(SizeType Other) const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftRight(Other);
        return NewArray;
    }

    /**
     * @brief       - Perform a bitshift right
     * @param Other - Number of steps to bitshift
     * @return      - Returns a reference to this object
     */
    CONSTEXPR TStaticBitArray& operator>>=(SizeType Other) const noexcept
    {
        BitshiftRight(Other);
        return *this;
    }

    /**
     * @brief       - Perform a bitshift left
     * @param Other - Number of steps to bitshift
     * @return      - Returns a copy that is bitshifted to the left
     */
    CONSTEXPR TStaticBitArray operator<<(SizeType Other) const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftLeft(Other);
        return NewArray;
    }

    /**
     * @brief       - Perform a bitshift left
     * @param Other - Number of steps to bitshift
     * @return      - Returns a reference to this object
     */
    CONSTEXPR TStaticBitArray& operator<<=(SizeType Other) const noexcept
    {
        BitshiftLeft(Other);
        return *this;
    }

public:

    /**
     * @brief     - Bitwise AND operator, perform a bitwise AND between this and another BitArray
     * @param LHS - Left-hand side to bitwise AND with
     * @param RHS - Right-hand side to bitwise AND with
     * @return    - Returns a BitArray with the result
     */
    NODISCARD friend CONSTEXPR TStaticBitArray operator&(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseAnd(RHS);
        return NewArray;
    }

    /**
     * @brief     - Bitwise OR operator, perform a bitwise OR between this and another BitArray
     * @param LHS - Left-hand side to bitwise OR with
     * @param RHS - Right-hand side to bitwise OR with
     * @return    - Returns a BitArray with the result
     */
    NODISCARD friend CONSTEXPR TStaticBitArray operator|(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseOr(RHS);
        return NewArray;
    }

    /**
     * @brief     - Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     * @param LHS - Left-hand side to bitwise XOR with
     * @param RHS - Right-hand side to bitwise XOR with
     * @return    - Returns a BitArray with the result
     */
    NODISCARD friend CONSTEXPR TStaticBitArray operator^(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseXor(RHS);
        return NewArray;
    }

public:

    /**
     * @brief  - Retrieve the number of bits
     * @return - Returns the number of bits in the array
     */
    NODISCARD CONSTEXPR SizeType Size() const noexcept
    {
        return NUM_BITS;
    }

    /**
     * @brief  - Retrieve the maximum number of bits
     * @return - Returns the maximum number of bits in the array
     */
    NODISCARD CONSTEXPR SizeType Capacity() const noexcept
    {
        return IntegerSize() * NumBitsPerInteger();
    }

    /**
     * @brief  - Retrieve the number of integers used to store the bits
     * @return - Returns the number of integers used to store the bits
     */
    NODISCARD CONSTEXPR SizeType IntegerSize() const noexcept
    {
        return NumIntegers();
    }

    /**
     * @brief  - Retrieve the capacity of the array in bytes
     * @return - Returns the capacity of the array in bytes
     */
    NODISCARD CONSTEXPR SizeType CapacityInBytes() const noexcept
    {
        return IntegerSize() * sizeof(InIntegerType);
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD CONSTEXPR InIntegerType* Data() noexcept
    {
        return Integers;
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD CONSTEXPR const InIntegerType* Data() const noexcept
    {
        return Integers;
    }

public:

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType Iterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator() noexcept
    {
        return ReverseIteratorType(*this, NUM_BITS);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, NUM_BITS);
    }

public: // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return Iterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return IteratorType(*this, NUM_BITS); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return ConstIteratorType(*this, NUM_BITS); }

private:
    NODISCARD static CONSTEXPR SizeType GetIntegersIndexOfBit(SizeType BitIndex) noexcept
    {
        return BitIndex / NumBitsPerInteger();
    }

    NODISCARD static CONSTEXPR SizeType GetIndexOfBitInIntegers(SizeType BitIndex) noexcept
    {
        return BitIndex % NumBitsPerInteger();
    }

    NODISCARD static CONSTEXPR SizeType NumBitsPerInteger() noexcept
    {
        return sizeof(InIntegerType) * 8;
    }

    NODISCARD static CONSTEXPR SizeType NumIntegers() noexcept
    {
        return (NUM_BITS + NumBitsPerInteger() - 1) / NumBitsPerInteger();
    }

    NODISCARD static CONSTEXPR InIntegerType CreateMaskForBit(SizeType BitIndex) noexcept
    {
        return InIntegerType(1) << GetIndexOfBitInIntegers(BitIndex);
    }

    NODISCARD static CONSTEXPR InIntegerType CreateMaskUpToBit(SizeType BitIndex) noexcept
    {
        return CreateMaskForBit(BitIndex) - 1;
    }

private:
    CONSTEXPR void AssignBitUnchecked(SizeType BitPosition, const bool bValue) noexcept
    {
        const SizeType ElementIndex   = GetIntegersIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInIntegers(BitPosition);

        const InIntegerType Mask  = CreateMaskForBit(IndexInElement);
        const InIntegerType Value = bValue ? Mask : InIntegerType(0);
        Integers[ElementIndex] |= Value;
    }

    CONSTEXPR void BitshiftRightUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
    {
        const SizeType StartElementIndex = GetIntegersIndexOfBit(StartBit);
        InIntegerType* Pointer = Integers + StartElementIndex;

        const SizeType RemainingElements = IntegerSize() - StartElementIndex;
        const SizeType RemainingBits     = NUM_BITS - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const InIntegerType StartValue  = *Pointer;
            const InIntegerType Mask        = CreateMaskUpToBit(StartBit);
            const InIntegerType InverseMask = ~Mask;
            *Pointer = (StartValue & InverseMask);

            const SizeType DiscardCount = Steps / NumBitsPerInteger();
            const SizeType RangeSize    = RemainingElements - DiscardCount;

            FMemory::Memmove(Pointer, Pointer + DiscardCount, sizeof(InIntegerType) * RangeSize);
            FMemory::Memzero(Pointer + RangeSize, sizeof(InIntegerType) * DiscardCount);

            BitshiftRight_Simple(Steps, StartElementIndex, RangeSize);

            const InIntegerType CurrentValue = *Pointer;
            *Pointer = (CurrentValue & InverseMask) | (StartValue & Mask);
        }
        else
        {
            FMemory::Memzero(Pointer, RemainingElements * sizeof(InIntegerType));
        }
    }

    CONSTEXPR void BitshiftRight_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        InIntegerType* Pointer = Integers + StartElementIndex + ElementsToShift;
        const SizeType CurrShift = Steps % NumBitsPerInteger();
        const SizeType PrevShift = NumBitsPerInteger() - CurrShift;

        InIntegerType Previous = 0;
        while (ElementsToShift)
        {
            const InIntegerType Current = *(--Pointer);
            *(Pointer) = (Current >> CurrShift) | (Previous << PrevShift);
            Previous = Current;

            ElementsToShift--;
        }
    }

    CONSTEXPR void BitshiftLeftUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
    {
        const SizeType StartElementIndex = GetIntegersIndexOfBit(StartBit);
        InIntegerType* Pointer = Integers + StartElementIndex;

        const SizeType RemainingElements = IntegerSize() - StartElementIndex;
        const SizeType RemainingBits     = NUM_BITS - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const InIntegerType StartValue  = *Pointer;
            const InIntegerType Mask        = CreateMaskUpToBit(StartBit);
            const InIntegerType InverseMask = ~Mask;
            *Pointer = (StartValue & InverseMask);

            const SizeType DiscardCount = Steps / NumBitsPerInteger();
            const SizeType RangeSize    = RemainingElements - DiscardCount;

            FMemory::Memmove(Pointer + DiscardCount, Pointer, sizeof(InIntegerType) * RangeSize);
            FMemory::Memzero(Pointer, sizeof(InIntegerType) * DiscardCount);

            BitshiftLeft_Simple(Steps, StartElementIndex + DiscardCount, RangeSize);

            const InIntegerType CurrentValue = *Pointer;
            *Pointer = (CurrentValue & InverseMask) | (StartValue & Mask);
        }
        else
        {
            FMemory::Memzero(Pointer, RemainingElements * sizeof(InIntegerType));
        }
    }

    CONSTEXPR void BitshiftLeft_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        InIntegerType* Pointer = Integers + StartElementIndex;

        const SizeType CurrShift = Steps % NumBitsPerInteger();
        const SizeType PrevShift = NumBitsPerInteger() - CurrShift;

        InIntegerType Previous = 0;
        while (ElementsToShift)
        {
            const InIntegerType Current = *Pointer;
            *(Pointer++) = (Current << CurrShift) | (Previous >> PrevShift);
            Previous = Current;

            ElementsToShift--;
        }
    }

    CONSTEXPR void MaskOutLastInteger()
    {
        const SizeType LastValidBit     = NUM_BITS - 1;
        const SizeType LastElementIndex = GetIntegersIndexOfBit(LastValidBit);
        const SizeType LastBitIndex     = GetIndexOfBitInIntegers(LastValidBit);

        const InIntegerType Mask = CreateMaskUpToBit(LastBitIndex) | CreateMaskForBit(LastBitIndex);
        Integers[LastElementIndex] &= Mask;
    }

    InIntegerType Integers[NumIntegers()];
};

using FStaticBitArray8  = TStaticBitArray<8, uint8>;
using FStaticBitArray16 = TStaticBitArray<16, uint16>;
using FStaticBitArray32 = TStaticBitArray<32, uint32>;
using FStaticBitArray64 = TStaticBitArray<64, uint64>;