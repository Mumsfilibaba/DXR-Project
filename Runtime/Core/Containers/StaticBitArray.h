#pragma once
#include "Iterator.h"

#include "Core/CoreTypes.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/BitReference.h"
#include "Core/Templates/BitUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticBitArray - Static array of packed bits

template<
    uint32 NumBits,
    typename InStorageType = uint32>
class TStaticBitArray
{
public:
    using SizeType    = uint32;
    using StorageType = InStorageType;

    static_assert(
        NumBits > 0,
        "StaticBitArray must have some bits allocated");
    static_assert(
        TIsUnsigned<StorageType>::Value,
        "StaticBitArray must have an unsigned StorageType");
    
    using BitReferenceType      = TBitReference<StorageType>;
    using ConstBitReferenceType = TBitReference<const StorageType>;

    typedef TBitArrayIterator<TStaticBitArray, StorageType>                    IteratorType;
    typedef TBitArrayIterator<const TStaticBitArray, const StorageType>        ConstIteratorType;
    typedef TReverseBitArrayIterator<TStaticBitArray, StorageType>             ReverseIteratorType;
    typedef TReverseBitArrayIterator<const TStaticBitArray, const StorageType> ReverseConstIteratorType;

public:
    
    /**
     * Default constructor
     */
    CONSTEXPR TStaticBitArray() noexcept
        : Storage()
    { }

    /**
     * Constructor that sets the elements based on an integer
     *
     * @param InValue: Integer containing bits to set to the BitArray
     */
    CONSTEXPR explicit TStaticBitArray(StorageType InValue) noexcept
        : Storage()
    {
        ResetWithZeros();
        Storage[0] = InValue;
        MaskOutLastStorageElement();
    }

    /**
     * Constructor that sets the elements based on an integer
     *
     * @param InValues: Integers containing bits to set to the BitArray
     * @param NumValues: Number of values in the input array
     */
    CONSTEXPR explicit TStaticBitArray(const StorageType* InValues, SizeType NumValues) noexcept
        : Storage()
    {
        ResetWithZeros();

        NumValues = NMath::Min(NumValues, NumBits);
        for (SizeType Index = 0; Index < NumValues; ++Index)
        {
            Storage[Index] = InValues[Index];
        }

        MaskOutLastStorageElement();
    }

    /**
     * Constructor that sets a certain number of bits to specified value
     *
     * @param bValue: Value to set bits to
     * @param NumBits: Number of bits to set
     */
    CONSTEXPR explicit TStaticBitArray(SizeType InNumBits, bool bValue) noexcept
        : Storage()
    {
        ResetWithZeros();

        for (SizeType Index = 0; Index < InNumBits; Index++)
        {
            AssignBitUnchecked(Index, bValue);
        }
    }

    /**
     * Constructor that creates a BitArray from a list of booleans indicating the sign of the bit
     *
     * @param InitList: Contains bools to indicate the sign of each bit
     */
    CONSTEXPR TStaticBitArray(std::initializer_list<bool> InitList) noexcept
        : Storage()
    {
        ResetWithZeros();

        SizeType Index = 0;
        for (bool bValue : InitList)
        {
            AssignBitUnchecked(Index++, bValue);
        }
    }

    /**
     * Resets the all the bits to zero
     */
    CONSTEXPR void ResetWithZeros()
    {
        for (StorageType& Element : Storage)
        {
            Element = StorageType(0);
        }
    }

    /**
     * Resets the all the bits to ones
     */
    CONSTEXPR void ResetWithOnes()
    {
        for (StorageType& Element : Storage)
        {
            Element = StorageType(~0);
        }

        MaskOutLastStorageElement();
    }

    /**
     * Assign a value to a bit
     *
     * @param BitPosition: Position of the bit to set
     * @param bValue: Value to assign to the bit
     */
    CONSTEXPR void AssignBit(SizeType BitPosition, const bool bValue) noexcept
    {
        if (BitPosition < NumBits)
        {
            AssignBitUnchecked(BitPosition, bValue);
        }
    }

    /**
     * Flips the bit at the position
     *
     * @param BitPosition: Position of the bit to set
     */
    CONSTEXPR void FlipBit(SizeType BitPosition) noexcept
    {
        if (BitPosition < NumBits)
        {
            const SizeType ElementIndex   = GetStorageIndexOfBit(BitPosition);
            const SizeType IndexInElement = GetIndexOfBitInStorage(BitPosition);

            const StorageType Mask = CreateMaskForBit(IndexInElement);
            Storage[ElementIndex] ^= Mask;
        }
    }

    /**
     * Count the number of bits that are assigned
     *
     * @return: Returns the number of bits that are true
     */
    NODISCARD CONSTEXPR SizeType CountAssignedBits() const noexcept
    {
        SizeType BitCount = 0;
        for (SizeType Index = 0; Index < GetNumElements(); ++Index)
        {
            const StorageType Element = Storage[Index];
            BitCount += FBitHelper::CountAssignedBits(Element);
        }

        return BitCount;
    }

    /**
     * Check if any bit is set
     *
     * @return: Returns true if any bit is set
     */
    NODISCARD CONSTEXPR bool HasAnyBitSet() const noexcept
    {
        return (CountAssignedBits() != 0);
    }

    /**
     * Check if no bit is set
     *
     * @return: Returns true if no bit is set
     */
    NODISCARD CONSTEXPR bool HasNoBitSet() const noexcept
    {
        return (CountAssignedBits() == 0);
    }

    /**
     * Retrieve the most significant bit. Will return zero if no bits are set, check HasAnyBitSet.
     *
     * @return: Returns the index of the most significant bit
     */
    NODISCARD CONSTEXPR SizeType MostSignificant() const
    {
        SizeType Result = 0;
        for (int32 Index = int32(GetNumElements()) - 1; Index >= 0; --Index)
        {
            const auto Element = Storage[Index];
            if (Element)
            {
                const auto BitIndex = FBitHelper::MostSignificant<SizeType>(Element);
                Result = BitIndex + (Index * GetBitsPerStorage());
                break;
            }
        }

        return Result;
    }

    /**
     * Retrieve the most significant bit. Will return zero if no bits are set, check HasAnyBitSet.
     *
     * @return: Returns the index of the least significant bit
     */
    NODISCARD CONSTEXPR SizeType LeastSignificant() const
    {
        SizeType Result = 0;
        for (SizeType Index = 0; Index < GetNumElements(); ++Index)
        {
            const auto Element = Storage[Index];
            if (Element)
            {
                const auto BitIndex = FBitHelper::LeastSignificant<SizeType>(Element);
                Result = BitIndex + (Index * GetBitsPerStorage());
                break;
            }
        }

        return Result;
    }

    /**
     * Perform a bitwise AND between this and another BitArray
     *
     * @param Other: BitArray to perform bitwise AND with
     */
    CONSTEXPR void BitwiseAnd(const TStaticBitArray& Other) noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Storage[Index] &= Other.Storage[Index];
        }
    }

    /**
     * Perform a bitwise OR between this and another BitArray
     *
     * @param Other: BitArray to perform bitwise OR with
     */
    CONSTEXPR void BitwiseOr(const TStaticBitArray& Other) noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Storage[Index] |= Other.Storage[Index];
        }
    }

    /**
     * Perform a bitwise XOR between this and another BitArray
     *
     * @param Other: BitArray to perform bitwise XOR with
     */
    CONSTEXPR void BitwiseXor(const TStaticBitArray& Other) noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Storage[Index] |= Other.Storage[Index];
        }
    }

    /**
     * Perform a bitwise NOT on each bit in this BitArray
     *
     * @param Other: BitArray to perform bitwise XOR with
     */
    CONSTEXPR void BitwiseNot() noexcept
    {
        for (SizeType Index = 0; Index < Capacity(); Index++)
        {
            Storage[Index] = ~Element;
        }
    }

    /**
     * Perform a right BitShift
     *
     * @param Steps: Number of steps to shift
     */
    CONSTEXPR void BitshiftRight(SizeType Steps) noexcept
    {
        if (Steps)
        {
            BitshiftRightUnchecked(Steps);
        }
    }

    /**
     * Perform a left BitShift
     *
     * @param Steps: Number of steps to shift
     */
    CONSTEXPR void BitshiftLeft(SizeType Steps) noexcept
    {
        if (Steps)
        {
            BitshiftLeftUnchecked(Steps);
        }
    }

    /**
     * Retrieve a reference to the bit with the index
     *
     * @param Index: Index of the bit
     * @return: Returns a reference to the bit with the index
     */
    NODISCARD CONSTEXPR BitReferenceType GetBitReference(SizeType BitIndex) noexcept
    {
        Check(BitIndex < NumBits);

        const SizeType ElementIndex = GetStorageIndexOfBit(BitIndex);
        Check(ElementIndex < Capacity());

        return BitReferenceType(Storage[ElementIndex], ~Element);
    }

    /**
     * Retrieve a reference to the bit with the index
     *
     * @param Index: Index of the bit
     * @return: Returns a reference to the bit with the index
     */
    NODISCARD CONSTEXPR const ConstBitReferenceType GetBitReference(SizeType Index) const noexcept
    {
        Check(Index < NumBits);

        const SizeType ElementIndex = GetStorageIndexOfBit(Index);
        Check(ElementIndex < Capacity());

        return ConstBitReferenceType(Storage[ElementIndex], CreateMaskForBit(Index));
    }

public:

    /**
     * Retrieve a bit with a certain index
     *
     * @param Index: Index to the bit
     * @return: Returns a BitReference to the specified bit
     */
    NODISCARD CONSTEXPR BitReferenceType operator[](SizeType Index) noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * Retrieve a bit with a certain index
     *
     * @param Index: Index to the bit
     * @return: Returns a BitReference to the specified bit
     */
    NODISCARD CONSTEXPR ConstBitReferenceType operator[](SizeType Index) const noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * Compare operator
     *
     * @param RHS: Right-hand side to compare
     * @return: Returns true if the BitArrays are equal
     */
    NODISCARD CONSTEXPR bool operator==(const TStaticBitArray& RHS) const noexcept
    {
        for (SizeType Index = 0; Index < StorageSize(); ++Index)
        {
            if (Storage[Index] != RHS.Storage[Index])
            {
                return false;
            }
        }

        return true;
    }

    /**
     * Compare operator
     *
     * @param RHS: Right-hand side to compare
     * @return: Returns false if the BitArrays are equal
     */
    NODISCARD CONSTEXPR bool operator!=(const TStaticBitArray& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    /**
     * Bitwise AND operator, perform a bitwise AND between this and another BitArray
     *
     * @param RHS: BitArray to perform bitwise AND with
     * @return: Returns a reference to this BitArray
     */
    CONSTEXPR TStaticBitArray& operator&=(const TStaticBitArray& RHS) noexcept
    {
        BitwiseAnd(RHS);
        return *this;
    }

    /**
     * Bitwise OR operator, perform a bitwise OR between this and another BitArray
     *
     * @param RHS: BitArray to perform bitwise OR with
     * @return: Returns a reference to this BitArray
     */
    CONSTEXPR TStaticBitArray& operator|=(const TStaticBitArray& RHS) noexcept
    {
        BitwiseOr(RHS);
        return *this;
    }

    /**
     * Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     *
     * @param RHS: BitArray to perform bitwise XOR with
     * @return: Returns a reference to this BitArray
     */
    CONSTEXPR TStaticBitArray& operator^=(const TStaticBitArray& RHS) noexcept
    {
        BitwiseXor(RHS);
        return *this;
    }

    /**
     * Perform a bitwise NOT on each bit in this BitArray
     *
     * @param Other: BitArray to perform bitwise XOR with
     */
    CONSTEXPR TStaticBitArray operator~() const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitwiseNot();
        return NewArray;
    }

    /**
     * Perform a bitshift right
     *
     * @param RHS: Number of steps to bitshift
     * @return: Returns a copy that is bitshifted to the right
     */
    CONSTEXPR TStaticBitArray operator>>(SizeType RHS) const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftRight(RHS);
        return NewArray;
    }

    /**
     * Perform a bitshift right
     *
     * @param RHS: Number of steps to bitshift
     * @return: Returns a reference to this object
     */
    CONSTEXPR TStaticBitArray& operator>>=(SizeType RHS) const noexcept
    {
        BitshiftRight(RHS);
        return *this;
    }

    /**
     * Perform a bitshift left
     *
     * @param RHS: Number of steps to bitshift
     * @return: Returns a copy that is bitshifted to the left
     */
    CONSTEXPR TStaticBitArray operator<<(SizeType RHS) const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftLeft(RHS);
        return NewArray;
    }

    /**
     * Perform a bitshift left
     *
     * @param RHS: Number of steps to bitshift
     * @return: Returns a reference to this object
     */
    CONSTEXPR TStaticBitArray& operator<<=(SizeType RHS) const noexcept
    {
        BitshiftLeft(RHS);
        return *this;
    }

public:

    /**
     * Bitwise AND operator, perform a bitwise AND between this and another BitArray
     *
     * @param LHS: Left-hand side to bitwise AND with
     * @param RHS: Right-hand side to bitwise AND with
     * @return: Returns a BitArray with the result
     */
    friend NODISCARD CONSTEXPR TStaticBitArray operator&(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseAnd(RHS);
        return NewArray;
    }

    /**
     * Bitwise OR operator, perform a bitwise OR between this and another BitArray
     *
     * @param LHS: Left-hand side to bitwise OR with
     * @param RHS: Right-hand side to bitwise OR with
     * @return: Returns a BitArray with the result
     */
    friend NODISCARD CONSTEXPR TStaticBitArray operator|(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseOr(RHS);
        return NewArray;
    }

    /**
     * Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     *
     * @param LHS: Left-hand side to bitwise XOR with
     * @param RHS: Right-hand side to bitwise XOR with
     * @return: Returns a BitArray with the result
     */
    friend NODISCARD CONSTEXPR TStaticBitArray operator^(const TStaticBitArray& LHS, const TStaticBitArray& RHS) noexcept
    {
        TStaticBitArray NewArray(LHS);
        NewArray.BitwiseXor(RHS);
        return NewArray;
    }

public:

    /**
     * Retrieve the number of bits
     *
     * @return: Returns the number of bits in the array
     */
    NODISCARD CONSTEXPR SizeType GetSize() const noexcept
    {
        return NumBits;
    }

    /**
     * Retrieve the maximum number of bits
     *
     * @return: Returns the maximum number of bits in the array
     */
    NODISCARD CONSTEXPR SizeType Capacity() const noexcept
    {
        return GetNumElements() * GetBitsPerStorage();
    }

    /**
     * Retrieve the number of integers used to store the bits
     *
     * @return: Returns the number of integers used to store the bits
     */
    NODISCARD CONSTEXPR SizeType StorageSize() const noexcept
    {
        return GetNumElements();
    }

    /**
     * Retrieve the capacity of the array in bytes
     *
     * @return: Returns the capacity of the array in bytes
     */
    NODISCARD CONSTEXPR SizeType CapacityInBytes() const noexcept
    {
        return StorageSize() * sizeof(StorageType);
    }

    /**
     * Retrieve the data of the Array
     *
     * @return: Returns a pointer to the stored data
     */
    NODISCARD CONSTEXPR StorageType* GetData() noexcept
    {
        return Storage;
    }

    /**
     * Retrieve the data of the Array
     *
     * @return: Returns a pointer to the stored data
     */
    NODISCARD CONSTEXPR const StorageType* GetData() const noexcept
    {
        return Storage;
    }

public:

    /**
     * Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, GetSize());
    }

    /**
     * Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, GetSize());
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, GetSize());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, GetSize());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /**
     * STL start iterator, same as TStaticBitArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TStaticBitArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    NODISCARD FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * STL start iterator, same as TStaticBitArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TStaticBitArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:
    static NODISCARD CONSTEXPR SizeType GetStorageIndexOfBit(SizeType BitIndex) noexcept
    {
        return BitIndex / GetBitsPerStorage();
    }

    static NODISCARD CONSTEXPR SizeType GetIndexOfBitInStorage(SizeType BitIndex) noexcept
    {
        return BitIndex % GetBitsPerStorage();
    }

    static NODISCARD CONSTEXPR SizeType GetBitsPerStorage() noexcept
    {
        return sizeof(StorageType) * 8;
    }

    static NODISCARD CONSTEXPR SizeType GetNumElements() noexcept
    {
        return (NumBits + GetBitsPerStorage() - 1) / GetBitsPerStorage();
    }

    static NODISCARD CONSTEXPR StorageType CreateMaskForBit(SizeType BitIndex) noexcept
    {
        return StorageType(1) << GetIndexOfBitInStorage(BitIndex);
    }

    static NODISCARD CONSTEXPR StorageType CreateMaskUpToBit(SizeType BitIndex) noexcept
    {
        return CreateMaskForBit(BitIndex) - 1;
    }

private:
    CONSTEXPR void AssignBitUnchecked(SizeType BitPosition, const bool bValue) noexcept
    {
        const SizeType ElementIndex   = GetStorageIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInStorage(BitPosition);

        const StorageType Mask  = CreateMaskForBit(IndexInElement);
        const StorageType Value = bValue ? Mask : StorageType(0);
        Storage[ElementIndex] |= Value;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Right shift

    CONSTEXPR void BitshiftRightUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
    {
        const SizeType StartElementIndex = GetStorageIndexOfBit(StartBit);

        StorageType* Pointer = GetData() + StartElementIndex;

        const SizeType RemainingElements = StorageSize() - StartElementIndex;
        const SizeType RemainingBits     = GetSize() - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const StorageType StartValue  = *Pointer;
            const StorageType Mask        = CreateMaskUpToBit(StartBit);
            const StorageType InverseMask = ~Mask;
            *Pointer = (StartValue & InverseMask);

            const SizeType DiscardCount = Steps / GetBitsPerStorage();
            const SizeType RangeSize    = RemainingElements - DiscardCount;

            FMemory::Memmove(Pointer, Pointer + DiscardCount, sizeof(StorageType) * RangeSize);
            FMemory::Memzero(Pointer + RangeSize, sizeof(StorageType) * DiscardCount);

            BitshiftRight_Simple(Steps, StartElementIndex, RangeSize);

            const StorageType CurrentValue = *Pointer;
            *Pointer = (CurrentValue & InverseMask) | (StartValue & Mask);
        }
        else
        {
            FMemory::Memzero(Pointer, RemainingElements * sizeof(StorageType));
        }
    }

    CONSTEXPR void BitshiftRight_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        StorageType* Pointer = GetData() + StartElementIndex + ElementsToShift;

        const SizeType CurrShift = Steps % GetBitsPerStorage();
        const SizeType PrevShift = GetBitsPerStorage() - CurrShift;

        StorageType Previous = 0;
        while (ElementsToShift)
        {
            const StorageType Current = *(--Pointer);
            *(Pointer) = (Current >> CurrShift) | (Previous << PrevShift);
            Previous = Current;

            ElementsToShift--;
        }
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Left shift

    CONSTEXPR void BitshiftLeftUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
    {
        const SizeType StartElementIndex = GetStorageIndexOfBit(StartBit);

        StorageType* Pointer = GetData() + StartElementIndex;

        const SizeType RemainingElements = StorageSize() - StartElementIndex;
        const SizeType RemainingBits     = GetSize() - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const StorageType StartValue  = *Pointer;
            const StorageType Mask        = CreateMaskUpToBit(StartBit);
            const StorageType InverseMask = ~Mask;
            *Pointer = (StartValue & InverseMask);

            const SizeType DiscardCount = Steps / GetBitsPerStorage();
            const SizeType RangeSize    = RemainingElements - DiscardCount;

            FMemory::Memmove(Pointer + DiscardCount, Pointer, sizeof(StorageType) * RangeSize);
            FMemory::Memzero(Pointer, sizeof(StorageType) * DiscardCount);

            BitshiftLeft_Simple(Steps, StartElementIndex + DiscardCount, RangeSize);

            const StorageType CurrentValue = *Pointer;
            *Pointer = (CurrentValue & InverseMask) | (StartValue & Mask);
        }
        else
        {
            FMemory::Memzero(Pointer, RemainingElements * sizeof(StorageType));
        }
    }

    CONSTEXPR void BitshiftLeft_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        StorageType* Pointer = GetData() + StartElementIndex;

        const SizeType CurrShift = Steps % GetBitsPerStorage();
        const SizeType PrevShift = GetBitsPerStorage() - CurrShift;

        StorageType Previous = 0;
        while (ElementsToShift)
        {
            const StorageType Current = *Pointer;
            *(Pointer++) = (Current << CurrShift) | (Previous >> PrevShift);
            Previous = Current;

            ElementsToShift--;
        }
    }

    CONSTEXPR void MaskOutLastStorageElement()
    {
        const SizeType LastValidBit     = NumBits - 1;
        const SizeType LastElementIndex = GetStorageIndexOfBit(LastValidBit);
        const SizeType LastBitIndex     = GetIndexOfBitInStorage(LastValidBit);

        const StorageType Mask = CreateMaskUpToBit(LastBitIndex) | CreateMaskForBit(LastBitIndex);
        Storage[LastElementIndex] &= Mask;
    }

    StorageType Storage[GetNumElements()];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Pre-defined types

using FStaticBitArray8  = TStaticBitArray<8, uint8>;
using FStaticBitArray16 = TStaticBitArray<16, uint16>;
using FStaticBitArray32 = TStaticBitArray<32, uint32>;
using FStaticBitArray64 = TStaticBitArray<64, uint64>;