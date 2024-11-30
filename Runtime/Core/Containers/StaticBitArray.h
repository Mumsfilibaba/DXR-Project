#pragma once
#include "Core/Containers/Iterator.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/BitReference.h"
#include "Core/Templates/BitHelper.h"

template<uint32 NUM_BITS, typename InIntegerType = uint32>
class TStaticBitArray
{
public:
    typedef int32 SIZETYPE;

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
     * @brief Default constructor
     */
    constexpr TStaticBitArray()
        : Integers()
    {
    }

    /**
     * @brief Constructor that sets the elements based on an integer
     * @param InValue Integer containing bits to set to the BitArray
     */
    constexpr explicit TStaticBitArray(InIntegerType InValue)
        : Integers()
    {
        Reset();
        Integers[0] = InValue;
        MaskLastInteger();
    }

    /**
     * @brief Constructor that sets the elements based on an integer array
     * @param InValues Integers containing bits to set to the BitArray
     * @param NumValues Number of values in the input array
     */
    constexpr explicit TStaticBitArray(const InIntegerType* InValues, SIZETYPE NumValues)
        : Integers()
    {
        Reset();

        NumValues = FMath::Min<SIZETYPE>(NumValues, NUM_BITS);
        for (SIZETYPE Index = 0; Index < NumValues; ++Index)
        {
            Integers[Index] = InValues[Index];
        }

        MaskLastInteger();
    }

    /**
     * @brief Constructor that sets a certain number of bits to a specified value
     * @param InNumBits Number of bits to set
     * @param bValue Value to set bits to
     */
    constexpr explicit TStaticBitArray(SIZETYPE InNumBits, bool bValue)
        : Integers()
    {
        Reset();

        for (SIZETYPE Index = 0; Index < InNumBits; Index++)
        {
            AssignBitUnchecked(Index, bValue);
        }
    }

    /**
     * @brief Constructor that creates a BitArray from a list of booleans indicating the state of the bit
     * @param InitList Contains bools to indicate the state of each bit
     */
    constexpr TStaticBitArray(std::initializer_list<bool> InitList)
        : Integers()
    {
        Reset();

        SIZETYPE Index = 0;
        for (bool bValue : InitList)
        {
            AssignBitUnchecked(Index++, bValue);
        }
    }

    /**
     * @brief Resets all the bits to zero
     */
    constexpr void Reset()
    {
        for (InIntegerType& Element : Integers)
        {
            Element = InIntegerType(0);
        }
    }

    /**
     * @brief Assign a value to a bit
     * @param BitPosition Position of the bit to set
     * @param bValue      - Value to assign to the bit
     */
    constexpr void AssignBit(SIZETYPE BitPosition, const bool bValue)
    {
        if (BitPosition < NUM_BITS)
        {
            AssignBitUnchecked(BitPosition, bValue);
        }
    }

    /**
     * @brief Flips the bit at the position
     * @param BitPosition Position of the bit to flip
     */
    constexpr void FlipBit(SIZETYPE BitPosition)
    {
        if (BitPosition < NUM_BITS)
        {
            const SIZETYPE ElementIndex   = GetIntegersIndexOfBit(BitPosition);
            const SIZETYPE IndexInElement = GetIndexOfBitInIntegers(BitPosition);
            const InIntegerType Mask = CreateMaskForBit(IndexInElement);
            Integers[ElementIndex] ^= Mask;
        }
    }

    /**
     * @brief Count the number of bits that are assigned
     * @return Returns the number of bits that are true
     */
    NODISCARD constexpr SIZETYPE CountAssignedBits() const
    {
        SIZETYPE BitCount = 0;
        for (SIZETYPE Index = 0; Index < NumIntegers(); ++Index)
        {
            const InIntegerType Element = Integers[Index];
            BitCount += FBitHelper::CountAssignedBits(Element);
        }

        return BitCount;
    }

    /**
     * @brief Check if any bit is set
     * @return Returns true if any bit is set
     */
    NODISCARD constexpr bool HasAnyBitSet() const
    {
        return CountAssignedBits() != 0;
    }

    /**
     * @brief Check if no bit is set
     * @return Returns true if no bit is set
     */
    NODISCARD constexpr bool HasNoBitSet() const
    {
        return CountAssignedBits() == 0;
    }

    /**
     * @brief Retrieve the most significant bit. Will return zero if no bits are set; check HasAnyBitSet.
     * @return Returns the index of the most significant bit
     */
    NODISCARD constexpr SIZETYPE MostSignificant() const
    {
        SIZETYPE Result = 0;
        for (int32 Index = int32(NumIntegers()) - 1; Index >= 0; --Index)
        {
            const InIntegerType Element = Integers[Index];
            if (Element)
            {
                const SIZETYPE BitIndex = FBitHelper::MostSignificant<SIZETYPE>(Element);
                Result = BitIndex + (Index * NumBitsPerInteger());
                break;
            }
        }

        return Result;
    }

    /**
     * @brief Retrieve the least significant bit. Will return zero if no bits are set; check HasAnyBitSet.
     * @return Returns the index of the least significant bit
     */
    NODISCARD constexpr SIZETYPE LeastSignificant() const
    {
        SIZETYPE Result = 0;
        for (SIZETYPE Index = 0; Index < NumIntegers(); ++Index)
        {
            const InIntegerType Element = Integers[Index];
            if (Element)
            {
                const InIntegerType BitIndex = FBitHelper::LeastSignificant<SIZETYPE>(Element);
                Result = BitIndex + (Index * NumBitsPerInteger());
                break;
            }
        }

        return Result;
    }

    /**
     * @brief Perform a bitwise AND between this and another BitArray
     * @param Other BitArray to perform bitwise AND with
     */
    constexpr void BitwiseAnd(const TStaticBitArray& Other)
    {
        for (SIZETYPE Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] &= Other.Integers[Index];
        }
    }

    /**
     * @brief Perform a bitwise OR between this and another BitArray
     * @param Other BitArray to perform bitwise OR with
     */
    constexpr void BitwiseOr(const TStaticBitArray& Other)
    {
        for (SIZETYPE Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] |= Other.Integers[Index];
        }
    }

    /**
     * @brief Perform a bitwise XOR between this and another BitArray
     * @param Other BitArray to perform bitwise XOR with
     */
    constexpr void BitwiseXor(const TStaticBitArray& Other)
    {
        for (SIZETYPE Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] ^= Other.Integers[Index];
        }
    }

    /**
     * @brief Perform a bitwise NOT on each bit in this BitArray
     */
    constexpr void BitwiseNot()
    {
        for (SIZETYPE Index = 0; Index < Capacity(); Index++)
        {
            Integers[Index] = ~Integers[Index];
        }
    }

    /**
     * @brief Perform a right BitShift
     * @param Steps Number of steps to shift
     */
    constexpr void BitshiftRight(SIZETYPE Steps)
    {
        if (Steps)
        {
            BitshiftRightUnchecked(Steps);
        }
    }

    /**
     * @brief Perform a left BitShift
     * @param Steps Number of steps to shift
     */
    constexpr void BitshiftLeft(SIZETYPE Steps)
    {
        if (Steps)
        {
            BitshiftLeftUnchecked(Steps);
        }
    }

public:

    /**
     * @brief Retrieve a bit with a certain index
     * @param BitIndex Index to the bit
     * @return Returns a BitReference to the specified bit
     */
    NODISCARD constexpr BitReferenceType operator[](SIZETYPE BitIndex)
    {
        CHECK(BitIndex < NUM_BITS);
        const SIZETYPE ElementIndex = GetIntegersIndexOfBit(BitIndex);
        CHECK(ElementIndex < Capacity());
        return BitReferenceType(Integers[ElementIndex], CreateMaskForBit(BitIndex));
    }

    /**
     * @brief Retrieve a bit with a certain index
     * @param BitIndex Index to the bit
     * @return Returns a BitReference to the specified bit
     */
    NODISCARD constexpr ConstBitReferenceType operator[](SIZETYPE BitIndex) const
    {
        CHECK(BitIndex < NUM_BITS);
        const SIZETYPE ElementIndex = GetIntegersIndexOfBit(BitIndex);
        CHECK(ElementIndex < Capacity());
        return ConstBitReferenceType(Integers[ElementIndex], CreateMaskForBit(BitIndex));
    }

    /**
     * @brief Compare operator
     * @param Other Right-hand side to compare
     * @return Returns true if the BitArrays are equal
     */
    NODISCARD constexpr bool operator==(const TStaticBitArray& Other) const
    {
        constexpr SIZETYPE ThisCapacity = NumIntegers();
        if (ThisCapacity != Other.NumIntegers())
        {
            return false;
        }

        for (SIZETYPE Index = 0; Index < ThisCapacity; Index++)
        {
            if (Integers[Index] != Other.Integers[Index])
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Compare operator
     * @param Other Right-hand side to compare
     * @return Returns false if the BitArrays are equal
     */
    NODISCARD constexpr bool operator!=(const TStaticBitArray& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Bitwise AND operator, perform a bitwise AND between this and another BitArray
     * @param Other BitArray to perform bitwise AND with
     * @return Returns a reference to this BitArray
     */
    constexpr TStaticBitArray& operator&=(const TStaticBitArray& Other)
    {
        BitwiseAnd(Other);
        return *this;
    }

    /**
     * @brief Bitwise OR operator, perform a bitwise OR between this and another BitArray
     * @param Other BitArray to perform bitwise OR with
     * @return Returns a reference to this BitArray
     */
    constexpr TStaticBitArray& operator|=(const TStaticBitArray& Other)
    {
        BitwiseOr(Other);
        return *this;
    }

    /**
     * @brief Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     * @param Other BitArray to perform bitwise XOR with
     * @return Returns a reference to this BitArray
     */
    constexpr TStaticBitArray& operator^=(const TStaticBitArray& Other)
    {
        BitwiseXor(Other);
        return *this;
    }

    /**
     * @brief Perform a bitwise NOT on each bit in this BitArray
     */
    constexpr TStaticBitArray operator~() const
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitwiseNot();
        return NewArray;
    }

    /**
     * @brief Perform a bitshift right
     * @param Steps Number of steps to bitshift
     * @return Returns a copy that is bitshifted to the right
     */
    constexpr TStaticBitArray operator>>(SIZETYPE Steps) const
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftRight(Steps);
        return NewArray;
    }

    /**
     * @brief Perform a bitshift right
     * @param Steps Number of steps to bitshift
     * @return Returns a reference to this object
     */
    constexpr TStaticBitArray& operator>>=(SIZETYPE Steps)
    {
        BitshiftRight(Steps);
        return *this;
    }

    /**
     * @brief Perform a bitshift left
     * @param Steps Number of steps to bitshift
     * @return Returns a copy that is bitshifted to the left
     */
    constexpr TStaticBitArray operator<<(SIZETYPE Steps) const
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftLeft(Steps);
        return NewArray;
    }

    /**
     * @brief Perform a bitshift left
     * @param Steps Number of steps to bitshift
     * @return Returns a reference to this object
     */
    constexpr TStaticBitArray& operator<<=(SIZETYPE Steps)
    {
        BitshiftLeft(Steps);
        return *this;
    }

public:

    /**
     * @brief Bitwise AND operator, perform a bitwise AND between two BitArrays
     * @param LHS Left-hand side to bitwise AND with
     * @param RHS Right-hand side to bitwise AND with
     * @return Returns a BitArray with the result
     */
    NODISCARD friend constexpr TStaticBitArray operator&(const TStaticBitArray& LHS, const TStaticBitArray& RHS)
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseAnd(RHS);
        return NewArray;
    }

    /**
     * @brief Bitwise OR operator, perform a bitwise OR between two BitArrays
     * @param LHS Left-hand side to bitwise OR with
     * @param RHS Right-hand side to bitwise OR with
     * @return Returns a BitArray with the result
     */
    NODISCARD friend constexpr TStaticBitArray operator|(const TStaticBitArray& LHS, const TStaticBitArray& RHS)
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseOr(RHS);
        return NewArray;
    }

    /**
     * @brief Bitwise XOR operator, perform a bitwise XOR between two BitArrays
     * @param LHS Left-hand side to bitwise XOR with
     * @param RHS Right-hand side to bitwise XOR with
     * @return Returns a BitArray with the result
     */
    NODISCARD friend constexpr TStaticBitArray operator^(const TStaticBitArray& LHS, const TStaticBitArray& RHS)
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseXor(RHS);
        return NewArray;
    }

public:

    /**
     * @brief Retrieve the number of bits
     * @return Returns the number of bits in the array
     */
    NODISCARD constexpr SIZETYPE Size() const
    {
        return NUM_BITS;
    }

    /**
     * @brief Retrieve the maximum number of bits
     * @return Returns the maximum number of bits in the array
     */
    NODISCARD constexpr SIZETYPE Capacity() const
    {
        return IntegerSize() * NumBitsPerInteger();
    }

    /**
     * @brief Retrieve the number of integers used to store the bits
     * @return Returns the number of integers used to store the bits
     */
    NODISCARD constexpr SIZETYPE IntegerSize() const
    {
        return NumIntegers();
    }

    /**
     * @brief Retrieve the capacity of the array in bytes
     * @return Returns the capacity of the array in bytes
     */
    NODISCARD constexpr SIZETYPE CapacityInBytes() const
    {
        return IntegerSize() * sizeof(InIntegerType);
    }

    /**
     * @brief Retrieve the data of the array
     * @return Returns a pointer to the stored data
     */
    NODISCARD constexpr InIntegerType* Data()
    {
        return Integers;
    }

    /**
     * @brief Retrieve the data of the array
     * @return Returns a pointer to the stored data
     */
    NODISCARD constexpr const InIntegerType* Data() const
    {
        return Integers;
    }

public:

    // Iterators
    NODISCARD FORCEINLINE IteratorType Iterator()
    {
        return IteratorType(*this, 0);
    }

    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const
    {
        return ConstIteratorType(*this, 0);
    }

    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator()
    {
        return ReverseIteratorType(*this, NUM_BITS);
    }

    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseIterator() const
    {
        return ReverseConstIteratorType(*this, NUM_BITS);
    }

public:

    // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return Iterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       { return IteratorType(*this, NUM_BITS); }
    NODISCARD FORCEINLINE ConstIteratorType end() const { return ConstIteratorType(*this, NUM_BITS); }

private:
    NODISCARD static constexpr SIZETYPE GetIntegersIndexOfBit(SIZETYPE BitIndex)
    {
        return BitIndex / NumBitsPerInteger();
    }

    NODISCARD static constexpr SIZETYPE GetIndexOfBitInIntegers(SIZETYPE BitIndex)
    {
        return BitIndex % NumBitsPerInteger();
    }

    NODISCARD static constexpr SIZETYPE NumBitsPerInteger()
    {
        return sizeof(InIntegerType) * 8;
    }

    NODISCARD static constexpr SIZETYPE NumIntegers()
    {
        return (NUM_BITS + (NumBitsPerInteger() - 1)) / NumBitsPerInteger();
    }

    NODISCARD static constexpr InIntegerType CreateMaskForBit(SIZETYPE BitIndex)
    {
        return InIntegerType(1) << GetIndexOfBitInIntegers(BitIndex);
    }

    NODISCARD static constexpr InIntegerType CreateMaskUpToBit(SIZETYPE BitIndex)
    {
        return CreateMaskForBit(BitIndex) - 1;
    }

private:
    constexpr void AssignBitUnchecked(SIZETYPE BitPosition, const bool bValue)
    {
        const SIZETYPE ElementIndex   = GetIntegersIndexOfBit(BitPosition);
        const SIZETYPE IndexInElement = GetIndexOfBitInIntegers(BitPosition);

        const InIntegerType Mask  = CreateMaskForBit(IndexInElement);
        const InIntegerType Value = bValue ? Mask : InIntegerType(0);
        Integers[ElementIndex] |= Value;
    }

    constexpr void BitshiftRightUnchecked(SIZETYPE Steps, SIZETYPE StartBit = 0)
    {
        const SIZETYPE StartElementIndex = GetIntegersIndexOfBit(StartBit);
        InIntegerType* Pointer = Integers + StartElementIndex;

        const SIZETYPE RemainingElements = IntegerSize() - StartElementIndex;
        const SIZETYPE RemainingBits     = NUM_BITS - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const InIntegerType StartValue  = *Pointer;
            const InIntegerType Mask        = CreateMaskUpToBit(StartBit);
            const InIntegerType InverseMask = ~Mask;
            *Pointer = (StartValue & InverseMask);

            const SIZETYPE DiscardCount = Steps / NumBitsPerInteger();
            const SIZETYPE RangeSize    = RemainingElements - DiscardCount;

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

    constexpr void BitshiftRight_Simple(SIZETYPE Steps, SIZETYPE StartElementIndex, SIZETYPE ElementsToShift)
    {
        InIntegerType* Pointer = Integers + StartElementIndex + ElementsToShift;
        const SIZETYPE CurrShift = Steps % NumBitsPerInteger();
        const SIZETYPE PrevShift = NumBitsPerInteger() - CurrShift;

        InIntegerType Previous = 0;
        while (ElementsToShift)
        {
            const InIntegerType Current = *(--Pointer);
            *(Pointer) = (Current >> CurrShift) | (Previous << PrevShift);
            Previous = Current;

            ElementsToShift--;
        }
    }

    constexpr void BitshiftLeftUnchecked(SIZETYPE Steps, SIZETYPE StartBit = 0)
    {
        const SIZETYPE StartElementIndex = GetIntegersIndexOfBit(StartBit);
        InIntegerType* Pointer = Integers + StartElementIndex;

        const SIZETYPE RemainingElements = IntegerSize() - StartElementIndex;
        const SIZETYPE RemainingBits     = NUM_BITS - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const InIntegerType StartValue  = *Pointer;
            const InIntegerType Mask        = CreateMaskUpToBit(StartBit);
            const InIntegerType InverseMask = ~Mask;
            *Pointer = (StartValue & InverseMask);

            const SIZETYPE DiscardCount = Steps / NumBitsPerInteger();
            const SIZETYPE RangeSize    = RemainingElements - DiscardCount;

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

    constexpr void BitshiftLeft_Simple(SIZETYPE Steps, SIZETYPE StartElementIndex, SIZETYPE ElementsToShift)
    {
        InIntegerType* Pointer = Integers + StartElementIndex;

        const SIZETYPE CurrShift = Steps % NumBitsPerInteger();
        const SIZETYPE PrevShift = NumBitsPerInteger() - CurrShift;

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
        const SIZETYPE LastValidBit     = NUM_BITS - 1;
        const SIZETYPE LastElementIndex = GetIntegersIndexOfBit(LastValidBit);
        const SIZETYPE LastBitIndex     = GetIndexOfBitInIntegers(LastValidBit);

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
