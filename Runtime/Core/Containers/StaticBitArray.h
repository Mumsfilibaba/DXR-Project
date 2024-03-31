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
    typedef int32 SizeType;

    static_assert(NUM_BITS > 0, "StaticBitArray must have some bits allocated");
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
    constexpr TStaticBitArray() noexcept
        : Integers()
    {
    }

    /**
     * @brief         - Constructor that sets the elements based on an integer
     * @param InValue - Integer containing bits to set to the BitArray
     */
    constexpr explicit TStaticBitArray(InIntegerType InValue) noexcept
        : Integers()
    {
        Reset();
        Integers[0] = InValue;
        MaskLastInteger();
    }

    /**
     * @brief           - Constructor that sets the elements based on an integer
     * @param InValues  - Integers containing bits to set to the BitArray
     * @param NumValues - Number of values in the input array
     */
    constexpr explicit TStaticBitArray(const InIntegerType* InValues, SizeType NumValues) noexcept
        : Integers()
    {
        Reset();

        NumValues = FMath::Min<SizeType>(NumValues, NUM_BITS);
        for (SizeType Index = 0; Index < NumValues; ++Index)
        {
            Integers[Index] = InValues[Index];
        }

        MaskLastInteger();
    }

    /**
     * @brief          - Constructor that sets a certain number of bits to specified value
     * @param bValue   - Value to set bits to
     * @param NUM_BITS - Number of bits to set
     */
    constexpr explicit TStaticBitArray(SizeType InNumBits, bool bValue) noexcept
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
    constexpr TStaticBitArray(std::initializer_list<bool> InitList) noexcept
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
    constexpr void Reset()
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
    constexpr void AssignBit(SizeType BitPosition, const bool bValue) noexcept
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
    constexpr void FlipBit(SizeType BitPosition) noexcept
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
    NODISCARD constexpr SizeType CountAssignedBits() const noexcept
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
    NODISCARD constexpr bool HasAnyBitSet() const noexcept
    {
        return CountAssignedBits() != 0;
    }

    /**
     * @brief  - Check if no bit is set
     * @return - Returns true if no bit is set
     */
    NODISCARD constexpr bool HasNoBitSet() const noexcept
    {
        return CountAssignedBits() == 0;
    }

    /**
     * @brief  - Retrieve the most significant bit. Will return zero if no bits are set, check HasAnyBitSet.
     * @return - Returns the index of the most significant bit
     */
    NODISCARD constexpr SizeType MostSignificant() const
    {
        SizeType Result = 0;
        for (int32 Index = int32(NumIntegers()) - 1; Index >= 0; --Index)
        {
            const InIntegerType Element = Integers[Index];
            if (Element)
            {
                const SizeType BitIndex = FBitHelper::MostSignificant<SizeType>(Element);
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
    NODISCARD constexpr SizeType LeastSignificant() const
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
    constexpr void BitwiseAnd(const TStaticBitArray& Other) noexcept
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
    constexpr void BitwiseOr(const TStaticBitArray& Other) noexcept
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
    constexpr void BitwiseXor(const TStaticBitArray& Other) noexcept
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
    constexpr void BitwiseNot() noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] = ~Integers[Index];
        }
    }

    /**
     * @brief       - Perform a right BitShift
     * @param Steps - Number of steps to shift
     */
    constexpr void BitshiftRight(SizeType Steps) noexcept
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
    constexpr void BitshiftLeft(SizeType Steps) noexcept
    {
        if (Steps)
        {
            BitshiftLeftUnchecked(Steps);
        }
    }

public:

    /**
     * @brief       - Retrieve a bit with a certain index
     * @param Index - Index to the bit
     * @return      - Returns a BitReference to the specified bit
     */
    NODISCARD constexpr BitReferenceType operator[](SizeType BitIndex) noexcept
    {
        CHECK(BitIndex < NUM_BITS);
        const SizeType ElementIndex = GetIntegersIndexOfBit(BitIndex);
        CHECK(ElementIndex < Capacity());
        return BitReferenceType(Integers[ElementIndex], CreateMaskForBit(BitIndex));
    }

    /**
     * @brief       - Retrieve a bit with a certain index
     * @param Index - Index to the bit
     * @return      - Returns a BitReference to the specified bit
     */
    NODISCARD constexpr ConstBitReferenceType operator[](SizeType BitIndex) const noexcept
    {
        CHECK(BitIndex < NUM_BITS);
        const SizeType ElementIndex = GetIntegersIndexOfBit(BitIndex);
        CHECK(ElementIndex < Capacity());
        return ConstBitReferenceType(Integers[ElementIndex], CreateMaskForBit(BitIndex));
    }

    /**
     * @brief       - Compare operator
     * @param Other - Right-hand side to compare
     * @return      - Returns true if the BitArrays are equal
     */
    NODISCARD constexpr bool operator==(const TStaticBitArray& Other) const noexcept
    {
        return Capacity() == Other.Capacity() ? FMemory::Memcmp(Integers, Other.Integers, CapacityInBytes()) : false;
    }

    /**
     * @brief       - Compare operator
     * @param Other - Right-hand side to compare
     * @return      - Returns false if the BitArrays are equal
     */
    NODISCARD constexpr bool operator!=(const TStaticBitArray& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief       - Bitwise AND operator, perform a bitwise AND between this and another BitArray
     * @param Other - BitArray to perform bitwise AND with
     * @return      - Returns a reference to this BitArray
     */
    constexpr TStaticBitArray& operator&=(const TStaticBitArray& Other) noexcept
    {
        BitwiseAnd(Other);
        return *this;
    }

    /**
     * @brief       - Bitwise OR operator, perform a bitwise OR between this and another BitArray
     * @param Other - BitArray to perform bitwise OR with
     * @return      - Returns a reference to this BitArray
     */
    constexpr TStaticBitArray& operator|=(const TStaticBitArray& Other) noexcept
    {
        BitwiseOr(Other);
        return *this;
    }

    /**
     * @brief       - Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     * @param Other - BitArray to perform bitwise XOR with
     * @return      - Returns a reference to this BitArray
     */
    constexpr TStaticBitArray& operator^=(const TStaticBitArray& Other) noexcept
    {
        BitwiseXor(Other);
        return *this;
    }

    /**
     * @brief       - Perform a bitwise NOT on each bit in this BitArray
     * @param Other - BitArray to perform bitwise XOR with
     */
    constexpr TStaticBitArray operator~() const noexcept
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
    constexpr TStaticBitArray operator>>(SizeType Other) const noexcept
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
    constexpr TStaticBitArray& operator>>=(SizeType Other) const noexcept
    {
        BitshiftRight(Other);
        return *this;
    }

    /**
     * @brief       - Perform a bitshift left
     * @param Other - Number of steps to bitshift
     * @return      - Returns a copy that is bitshifted to the left
     */
    constexpr TStaticBitArray operator<<(SizeType Other) const noexcept
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
    constexpr TStaticBitArray& operator<<=(SizeType Other) const noexcept
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
    NODISCARD friend constexpr TStaticBitArray operator&(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
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
    NODISCARD friend constexpr TStaticBitArray operator|(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
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
    NODISCARD friend constexpr TStaticBitArray operator^(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
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
    NODISCARD constexpr SizeType Size() const noexcept
    {
        return NUM_BITS;
    }

    /**
     * @brief  - Retrieve the maximum number of bits
     * @return - Returns the maximum number of bits in the array
     */
    NODISCARD constexpr SizeType Capacity() const noexcept
    {
        return IntegerSize() * NumBitsPerInteger();
    }

    /**
     * @brief  - Retrieve the number of integers used to store the bits
     * @return - Returns the number of integers used to store the bits
     */
    NODISCARD constexpr SizeType IntegerSize() const noexcept
    {
        return NumIntegers();
    }

    /**
     * @brief  - Retrieve the capacity of the array in bytes
     * @return - Returns the capacity of the array in bytes
     */
    NODISCARD constexpr SizeType CapacityInBytes() const noexcept
    {
        return IntegerSize() * sizeof(InIntegerType);
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD constexpr InIntegerType* Data() noexcept
    {
        return Integers;
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD constexpr const InIntegerType* Data() const noexcept
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
    NODISCARD static constexpr SizeType GetIntegersIndexOfBit(SizeType BitIndex) noexcept
    {
        return BitIndex / NumBitsPerInteger();
    }

    NODISCARD static constexpr SizeType GetIndexOfBitInIntegers(SizeType BitIndex) noexcept
    {
        return BitIndex % NumBitsPerInteger();
    }

    NODISCARD static constexpr SizeType NumBitsPerInteger() noexcept
    {
        return sizeof(InIntegerType) * 8;
    }

    NODISCARD static constexpr SizeType NumIntegers() noexcept
    {
        return (NUM_BITS + (NumBitsPerInteger() - 1)) / NumBitsPerInteger();
    }

    NODISCARD static constexpr InIntegerType CreateMaskForBit(SizeType BitIndex) noexcept
    {
        return InIntegerType(1) << GetIndexOfBitInIntegers(BitIndex);
    }

    NODISCARD static constexpr InIntegerType CreateMaskUpToBit(SizeType BitIndex) noexcept
    {
        return CreateMaskForBit(BitIndex) - 1;
    }

private:
    constexpr void AssignBitUnchecked(SizeType BitPosition, const bool bValue) noexcept
    {
        const SizeType ElementIndex   = GetIntegersIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInIntegers(BitPosition);

        const InIntegerType Mask  = CreateMaskForBit(IndexInElement);
        const InIntegerType Value = bValue ? Mask : InIntegerType(0);
        Integers[ElementIndex] |= Value;
    }

    constexpr void BitshiftRightUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
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

    constexpr void BitshiftRight_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
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

    constexpr void BitshiftLeftUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
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

    constexpr void BitshiftLeft_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
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

    constexpr void MaskLastInteger()
    {
        const SizeType LastValidBit     = NUM_BITS - 1;
        const SizeType LastElementIndex = GetIntegersIndexOfBit(LastValidBit);
        const SizeType LastBitIndex     = GetIndexOfBitInIntegers(LastValidBit);

        const InIntegerType Mask = CreateMaskUpToBit(LastBitIndex) | CreateMaskForBit(LastBitIndex);
        Integers[LastElementIndex] &= Mask;
    }

    InIntegerType Integers[NumIntegers()];
};

typedef TStaticBitArray<8, uint8>   FStaticBitArray8 ;
typedef TStaticBitArray<16, uint16> FStaticBitArray16;
typedef TStaticBitArray<32, uint32> FStaticBitArray32;
typedef TStaticBitArray<64, uint64> FStaticBitArray64;
typedef FStaticBitArray32           FStaticBitArray;