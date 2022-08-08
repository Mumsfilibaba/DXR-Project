#pragma once
#include "Iterator.h"

#include "Core/CoreTypes.h"
#include "Core/Templates/BitReference.h"
#include "Core/Templates/BitUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticBitArray - Static array of packed bits

template<uint32 NumBits, typename InStorageType = uint32>
class TStaticBitArray
{
public:

    using SizeType    = uint32;
    using StorageType = InStorageType;

    static_assert(NumBits > 0                    , "StaticBitArray must have some bits allocated");
    static_assert(TIsUnsigned<StorageType>::Value, "StaticBitArray must have an unsigned StorageType");
    
    using BitReferenceType      = TBitReference<StorageType>;
    using ConstBitReferenceType = TBitReference<const StorageType>;

    typedef TBitArrayIterator<TStaticBitArray, StorageType>                    IteratorType;
    typedef TBitArrayIterator<const TStaticBitArray, const StorageType>        ConstIteratorType;
    typedef TReverseBitArrayIterator<TStaticBitArray, StorageType>             ReverseIteratorType;
    typedef TReverseBitArrayIterator<const TStaticBitArray, const StorageType> ReverseConstIteratorType;

    /**
     * Default constructor
     */
    FORCEINLINE TStaticBitArray() noexcept
        : Storage()
    {
    }

    /**
     * Constructor that sets the elements based on an integer
     *
     * @param InValue: Integer containing bits to set to the BitArray
     */
    FORCEINLINE explicit TStaticBitArray(StorageType InValue) noexcept
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
    FORCEINLINE explicit TStaticBitArray(const StorageType* InValues, SizeType NumValues) noexcept
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
    FORCEINLINE explicit TStaticBitArray(SizeType InNumBits, bool bValue) noexcept
        : Storage()
    {
        ResetWithZeros();

        for (SizeType Index = 0; Index < InNumBits; Index++)
        {
            AssignBit_Internal(Index, bValue);
        }
    }

    /**
     * Constructor that creates a BitArray from a list of booleans indicating the sign of the bit
     *
     * @param InitList: Contains bools to indicate the sign of each bit
     */
    FORCEINLINE TStaticBitArray(std::initializer_list<bool> InitList) noexcept
        : Storage()
    {
        ResetWithZeros();

        SizeType Index = 0;
        for (bool bValue : InitList)
        {
            AssignBit_Internal(Index++, bValue);
        }
    }

    /**
     * Resets the all the bits to zero
     */
    FORCEINLINE void ResetWithZeros()
    {
        FMemory::Memset(Storage, 0x00, CapacityInBytes());
    }

    /**
     * Resets the all the bits to ones
     */
    FORCEINLINE void ResetWithOnes()
    {
        FMemory::Memset(Storage, 0xff, CapacityInBytes());
        
        MaskOutLastStorageElement();
    }

    /**
     * Assign a value to a bit
     *
     * @param BitPosition: Position of the bit to set
     * @param bValue: Value to assign to the bit
     */
    inline void AssignBit(SizeType BitPosition, const bool bValue) noexcept
    {
        Check(BitPosition < NumBits);
        AssignBit_Internal(BitPosition, bValue);
    }

    /**
     * Flips the bit at the position
     *
     * @param BitPosition: Position of the bit to set
     */
    FORCEINLINE void FlipBit(SizeType BitPosition) noexcept
    {
        Check(BitPosition < NumBits);

        const SizeType ElementIndex   = GetStorageIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInStorage(BitPosition);

        const StorageType Mask = CreateMaskForBit(IndexInElement);
        Storage[ElementIndex] ^= Mask;
    }

    /**
     * Count the number of bits that are assigned
     *
     * @return: Returns the number of bits that are true
     */
    FORCEINLINE SizeType CountAssignedBits() const noexcept
    {
        SizeType CurrentBit = 0;

        for (SizeType Bit = 0; Bit < GetBitsPerStorage(); ++Bit)
        {
            const SizeType Index = GetStorageIndexOfBit(Bit);
            if (Storage[Index] & CreateMaskForBit(Bit))
            {
                CurrentBit++;
            }
        }

        return CurrentBit;
    }

    /**
     * Check if any bit is set
     *
     * @return: Returns true if any bit is set
     */
    FORCEINLINE bool HasAnyBitSet() const noexcept
    {
        return CountAssignedBits() != 0;
    }

    /**
     * Check if no bit is set
     *
     * @return: Returns true if no bit is set
     */
    FORCEINLINE bool HasNoBitSet() const noexcept
    {
        return CountAssignedBits() == 0;
    }

    /**
     * Retrieve the most significant bit
     * 
     * @param OutIndex: Variable to store the index of the most significant bit
     * @return: Returns false if no bit is set
     */
    FORCEINLINE bool MostSignificantBit(SizeType& OutIndex) const
    {
        for (int32 Index = static_cast<int32>(Capacity()) - 1; Index >= 0; --Index)
        {
            if (FBitHelper::MostSignificant(Storage[Index], OutIndex))
            {
                OutIndex += Index * GetBitsPerStorage();
                return true;
            }
        }

        return false;
    }

    /**
     * Retrieve the least significant bit
     *
     * @param OutIndex: Variable to store the index of the least significant bit
     * @return: Returns false if no bit is set
     */
    FORCEINLINE bool LeastSignificantBit(SizeType& OutIndex) const
    {
        for (SizeType Index = 0; Index < Capacity(); ++Index)
        {
            if (FBitHelper::LeastSignificant(Storage[Index], OutIndex))
            {
                OutIndex += Index * GetBitsPerStorage();
                return true;
            }
        }

        return false;
    }

    /**
     * Perform a bitwise AND between this and another BitArray
     *
     * @param Other: BitArray to perform bitwise AND with
     */
    FORCEINLINE void BitwiseAnd(const TStaticBitArray& Other) noexcept
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
    FORCEINLINE void BitwiseOr(const TStaticBitArray& Other) noexcept
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
    FORCEINLINE void BitwiseXor(const TStaticBitArray& Other) noexcept
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
    FORCEINLINE void BitwiseNot() noexcept
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
    inline void BitshiftRight(SizeType Steps) noexcept
    {
        if (Steps)
        {
            BitshiftRight_Internal(Steps, 0);
        }
    }

    /**
     * Perform a left BitShift
     *
     * @param Steps: Number of steps to shift
     */
    inline void BitshiftLeft(SizeType Steps) noexcept
    {
        if (Steps)
        {
            BitshiftLeft_Internal(Steps, 0);
        }
    }

    /**
     * Retrieve a reference to the bit with the index
     *
     * @param Index: Index of the bit
     * @return: Returns a reference to the bit with the index
     */
    FORCEINLINE BitReferenceType GetBitReference(SizeType BitIndex) noexcept
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
    FORCEINLINE const ConstBitReferenceType GetBitReference(SizeType Index) const noexcept
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
    FORCEINLINE BitReferenceType operator[](SizeType Index) noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * Retrieve a bit with a certain index
     *
     * @param Index: Index to the bit
     * @return: Returns a BitReference to the specified bit
     */
    FORCEINLINE const ConstBitReferenceType operator[](SizeType Index) const noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * Compare operator
     *
     * @param Rhs: Right-hand side to compare
     * @return: Returns true if the BitArrays are equal
     */
    FORCEINLINE bool operator==(const TStaticBitArray& Rhs) const noexcept
    {
        for (SizeType Index = 0; Index < StorageSize(); ++Index)
        {
            if (Storage[Index] != Rhs.Storage[Index])
            {
                return false;
            }
        }

        return true;
    }

    /**
     * Compare operator
     *
     * @param Rhs: Right-hand side to compare
     * @return: Returns false if the BitArrays are equal
     */
    FORCEINLINE bool operator!=(const TStaticBitArray& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }

    /**
     * Bitwise AND operator, perform a bitwise AND between this and another BitArray
     *
     * @param Rhs: BitArray to perform bitwise AND with
     * @return: Returns a reference to this BitArray
     */
    FORCEINLINE TStaticBitArray& operator&=(const TStaticBitArray& Rhs) noexcept
    {
        BitwiseAnd(Rhs);
        return *this;
    }

    /**
     * Bitwise OR operator, perform a bitwise OR between this and another BitArray
     *
     * @param Rhs: BitArray to perform bitwise OR with
     * @return: Returns a reference to this BitArray
     */
    FORCEINLINE TStaticBitArray& operator|=(const TStaticBitArray& Rhs) noexcept
    {
        BitwiseOr(Rhs);
        return *this;
    }

    /**
     * Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     *
     * @param Rhs: BitArray to perform bitwise XOR with
     * @return: Returns a reference to this BitArray
     */
    FORCEINLINE TStaticBitArray& operator^=(const TStaticBitArray& Rhs) noexcept
    {
        BitwiseXor(Rhs);
        return *this;
    }

    /**
     * Perform a bitwise NOT on each bit in this BitArray
     *
     * @param Other: BitArray to perform bitwise XOR with
     */
    FORCEINLINE TStaticBitArray operator~() const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitwiseNot();
        return NewArray;
    }

    /**
     * Perform a bitshift right
     *
     * @param Rhs: Number of steps to bitshift
     * @return: Returns a copy that is bitshifted to the right
     */
    FORCEINLINE TStaticBitArray operator>>(SizeType Rhs) const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftRight(Rhs);
        return NewArray;
    }

    /**
     * Perform a bitshift right
     *
     * @param Rhs: Number of steps to bitshift
     * @return: Returns a reference to this object
     */
    FORCEINLINE TStaticBitArray& operator>>=(SizeType Rhs) const noexcept
    {
        BitshiftRight(Rhs);
        return *this;
    }

    /**
     * Perform a bitshift left
     *
     * @param Rhs: Number of steps to bitshift
     * @return: Returns a copy that is bitshifted to the left
     */
    FORCEINLINE TStaticBitArray operator<<(SizeType Rhs) const noexcept
    {
        TStaticBitArray NewArray(*this);
        NewArray.BitshiftLeft(Rhs);
        return NewArray;
    }

    /**
     * Perform a bitshift left
     *
     * @param Rhs: Number of steps to bitshift
     * @return: Returns a reference to this object
     */
    FORCEINLINE TStaticBitArray& operator<<=(SizeType Rhs) const noexcept
    {
        BitshiftLeft(Rhs);
        return *this;
    }

public:

    /**
     * Bitwise AND operator, perform a bitwise AND between this and another BitArray
     *
     * @param Lhs: Left-hand side to bitwise AND with
     * @param Rhs: Right-hand side to bitwise AND with
     * @return: Returns a BitArray with the result
     */
    friend FORCEINLINE TStaticBitArray operator&(const TStaticBitArray& Lhs, const TStaticBitArray& Rhs) noexcept
    {
        TStaticBitArray NewArray(Lhs);
        NewArray.BitwiseAnd(Rhs);
        return NewArray;
    }

    /**
     * Bitwise OR operator, perform a bitwise OR between this and another BitArray
     *
     * @param Lhs: Left-hand side to bitwise OR with
     * @param Rhs: Right-hand side to bitwise OR with
     * @return: Returns a BitArray with the result
     */
    friend FORCEINLINE TStaticBitArray operator|(const TStaticBitArray& Lhs, const TStaticBitArray& Rhs) noexcept
    {
        TStaticBitArray NewArray(Lhs);
        NewArray.BitwiseOr(Rhs);
        return NewArray;
    }

    /**
     * Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     *
     * @param Lhs: Left-hand side to bitwise XOR with
     * @param Rhs: Right-hand side to bitwise XOR with
     * @return: Returns a BitArray with the result
     */
    friend FORCEINLINE TStaticBitArray operator^(const TStaticBitArray& Lhs, const TStaticBitArray& Rhs) noexcept
    {
        TStaticBitArray NewArray(Lhs);
        NewArray.BitwiseXor(Rhs);
        return NewArray;
    }

public:

    /**
     * Retrieve the number of bits
     *
     * @return: Returns the number of bits in the array
     */
    constexpr SizeType Size() const noexcept
    {
        return NumBits;
    }

    /**
     * Retrieve the maximum number of bits
     *
     * @return: Returns the maximum number of bits in the array
     */
    constexpr SizeType Capacity() const noexcept
    {
        return (NumBits + GetBitsPerStorage() - 1) / GetBitsPerStorage();
    }

    /**
     * Retrieve the number of integers used to store the bits
     *
     * @return: Returns the number of integers used to store the bits
     */
    constexpr SizeType StorageSize() const noexcept
    {
        return Capacity();
    }

    /**
     * Retrieve the capacity of the array in bytes
     *
     * @return: Returns the capacity of the array in bytes
     */
    constexpr SizeType CapacityInBytes() const noexcept
    {
        return Capacity() * sizeof(StorageType);
    }

    /**
     * Retrieve the data of the Array
     *
     * @return: Returns a pointer to the stored data
     */
    constexpr StorageType* Data() noexcept
    {
        return Storage;
    }

    /**
     * Retrieve the data of the Array
     *
     * @return: Returns a pointer to the stored data
     */
    constexpr const StorageType* Data() const noexcept
    {
        return Storage;
    }

public:

    /**
     * Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, Size());
    }

    /**
     * Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /**
     * STL start iterator, same as TStaticBitArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TStaticBitArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * STL start iterator, same as TStaticBitArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TStaticBitArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:

    static constexpr SizeType GetStorageIndexOfBit(SizeType BitIndex) noexcept
    {
        return BitIndex / GetBitsPerStorage();
    }

    static constexpr SizeType GetIndexOfBitInStorage(SizeType BitIndex) noexcept
    {
        return BitIndex % GetBitsPerStorage();
    }

    static constexpr SizeType GetBitsPerStorage() noexcept
    {
        return sizeof(StorageType) * 8;
    }

    static constexpr SizeType GetNumElements() noexcept
    {
        return (NumBits + GetBitsPerStorage() - 1) / GetBitsPerStorage();
    }

    static constexpr StorageType CreateMaskForBit(SizeType BitIndex) noexcept
    {
        return StorageType(1) << GetIndexOfBitInStorage(BitIndex);
    }

    static constexpr StorageType CreateMaskUpToBit(SizeType BitIndex) noexcept
    {
        return CreateMaskForBit(BitIndex) - 1;
    }

private:

    FORCEINLINE void AssignBit_Internal(SizeType BitPosition, const bool bValue) noexcept
    {
        const SizeType ElementIndex   = GetStorageIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInStorage(BitPosition);

        const StorageType Mask  = CreateMaskForBit(IndexInElement);
        const StorageType Value = bValue ? (StorageType(~0) & Mask) : StorageType(0);
        Storage[ElementIndex] |= Value;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Right shift

    FORCEINLINE void BitshiftRight_Internal(SizeType Steps) noexcept
    {
        StorageType* Pointer = Data();
        if (Steps < Size())
        {
            const SizeType DiscardCount = Steps / GetBitsPerStorage();
            const SizeType RangeSize    = StorageSize() - DiscardCount;

            Memory::Memmove(Pointer, Pointer + DiscardCount, sizeof(StorageType) * RangeSize);
            Memory::Memzero(Pointer + RangeSize, sizeof(StorageType) * DiscardCount);

            BitshiftRight_Simple(Steps, 0, RangeSize);
        }
        else
        {
            Memory::Memzero(Pointer, CapacityInBytes());
        }
    }

    FORCEINLINE void BitshiftRight_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        StorageType* Pointer = Data() + StartElementIndex + ElementsToShift;

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

    FORCEINLINE void BitshiftLeft_Internal(SizeType Steps) noexcept
    {
        StorageType* Pointer = Data() + StartElementIndex;
        if (Steps < Size())
        {
            const SizeType DiscardCount = Steps / GetBitsPerStorage();
            const SizeType RangeSize    = StorageSize() - DiscardCount;

            Memory::Memmove(Pointer + DiscardCount, Pointer, sizeof(StorageType) * RangeSize);
            Memory::Memzero(Pointer, sizeof(StorageType) * DiscardCount);

            BitshiftLeft_Simple(Steps, DiscardCount, RangeSize);
        }
        else
        {
            Memory::Memzero(Pointer, CapacityInBytes());
        }
    }

    FORCEINLINE void BitshiftLeft_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        StorageType* Pointer = Data() + StartElementIndex;

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

    FORCEINLINE void MaskOutLastStorageElement()
    {
        const SizeType LastElementIndex = GetStorageIndexOfBit(NumBits);
        const SizeType LastBitIndex     = GetIndexOfBitInStorage(NumBits);

        const StorageType Mask = CreateMaskUpToBit(LastBitIndex);
        Storage[LastElementIndex] &= Mask;
    }

    StorageType Storage[GetNumElements()];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Pre-defined types

using StaticBitArray8  = TStaticBitArray<8, uint8>;
using StaticBitArray16 = TStaticBitArray<16, uint16>;
using StaticBitArray32 = TStaticBitArray<32, uint32>;
using StaticBitArray64 = TStaticBitArray<64, uint64>;