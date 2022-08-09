#pragma once
#include "Iterator.h"
#include "Allocators.h"

#include "Core/Core.h"
#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"
#include "Core/Templates/IsInteger.h"
#include "Core/Templates/BitHelper.h"
#include "Core/Templates/BitReference.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TBitArray - Array of packed bits

template<
    typename InStorageType   = uint32,
    typename InAllocatorType = TDefaultArrayAllocator<InStorageType>>
class TBitArray
{
public:
    using SizeType      = uint32;
    using StorageType   = InStorageType;
    using AllocatorType = InAllocatorType;

    static_assert(
        TIsUnsigned<StorageType>::Value,
        "BitArray must have an unsigned StorageType");

    using BitReferenceType      = TBitReference<StorageType>;
    using ConstBitReferenceType = TBitReference<const StorageType>;

    typedef TBitArrayIterator<TBitArray, StorageType>                    IteratorType;
    typedef TBitArrayIterator<const TBitArray, const StorageType>        ConstIteratorType;
    typedef TReverseBitArrayIterator<TBitArray, StorageType>             ReverseIteratorType;
    typedef TReverseBitArrayIterator<const TBitArray, const StorageType> ReverseConstIteratorType;

public:

    /**
     * Default constructor
     */
    FORCEINLINE TBitArray() noexcept
        : Storage()
        , NumBits(0)
        , NumElements(0)
    { }

    /**
     * Constructor that sets the elements based on an integer
     * 
     * @param InValue: Integer containing bits to set to the BitArray
     */
    FORCEINLINE explicit TBitArray(StorageType InValue) noexcept
        : Storage()
        , NumBits(GetBitsPerStorage())
        , NumElements(0)
    {
        AllocateAndZeroStorage(NumBits);

        StorageType& Element = GetStorage(0);
        Element = InValue;
    }

    /**
     * Constructor that sets the elements based on an integer
     *
     * @param InValues: Integers containing bits to set to the BitArray
     * @param NumValues: Number of values in the input array
     */
    NOINLINE explicit TBitArray(const StorageType* InValues, SizeType NumValues) noexcept
        : Storage()
        , NumBits(0)
        , NumElements(0)
    {
        NumBits = NumValues * GetBitsPerStorage();
        AllocateAndZeroStorage(NumBits);

        for (SizeType Index = 0; Index < NumValues; ++Index)
        {
            StorageType& Element = GetStorage(Index);
            Element = InValues[Index];
        }
    }

    /**
     * Constructor that sets a certain number of bits to specified value
     *
     * @param bValue: Value to set bits to
     * @param NumBits: Number of bits to set
     */
    FORCEINLINE explicit TBitArray(SizeType InNumBits, bool bValue) noexcept
        : Storage()
        , NumBits(InNumBits)
        , NumElements(0)
    {
        AllocateAndZeroStorage(NumBits);

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
    FORCEINLINE TBitArray(std::initializer_list<bool> InitList) noexcept
        : Storage()
        , NumBits(static_cast<SizeType>(InitList.size()))
        , NumElements(0)
    {
        AllocateAndZeroStorage(NumBits);
        
        SizeType Index = 0;
        for (bool bValue :  InitList)
        {
            AssignBitUnchecked(Index++, bValue);
        }
    }

    /**
     * Copy constructor
     * 
     * @param Other: BitArray to copy from
     */
    FORCEINLINE TBitArray(const TBitArray& Other) noexcept
        : Storage()
        , NumBits(Other.NumBits)
        , NumElements(0)
    {
        AllocateAndZeroStorage(NumBits);
        CopyFrom(Other);
    }

    /**
     * Move constructor
     *
     * @param Other: BitArray to move from
     */ 
    FORCEINLINE TBitArray(TBitArray&& Other) noexcept
        : Storage()
        , NumBits(Other.NumBits)
        , NumElements(Other.NumElements)
    {
        MoveFrom(Move(Other));
    }

    /**
     * Destructor
     */
    FORCEINLINE ~TBitArray()
    {
        NumBits     = 0;
        NumElements = 0;
    }

    /**
     * Resets the all the bits to zero
     */
    FORCEINLINE void ResetWithZeros()
    {
        FMemory::Memset(GetData(), 0x00, CapacityInBytes());
    }

    /**
     * Resets the all the bits to ones
     */
    FORCEINLINE void ResetWithOnes()
    {
        FMemory::Memset(GetData(), 0xff, CapacityInBytes());
        MaskOutLastStorageElement();
    }

    /**
     * Check if the array is empty
     * 
     * @return: Returns true if the array is empty
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return (NumBits == 0);
    }

    /**
     * Push a new bit with the specified value
     * 
     * @param bValue: Value of the new bit
     */
    inline void Push(const bool bValue) noexcept
    {
        Reserve(NumBits + 1);
        AssignBitUnchecked(NumBits, bValue);
        NumBits++;
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
        AssignBitUnchecked(BitPosition, bValue);
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

        StorageType& Element = GetStorage(ElementIndex);
        Element ^= Mask;
    }

    /**
     * Count the number of bits that are assigned
     * 
     * @return: Returns the number of bits that are true
     */
    NODISCARD FORCEINLINE SizeType CountAssignedBits() const noexcept
    {
        SizeType BitCount = 0;
        for (SizeType Index = 0; Index < NumElements; ++Index)
        {
            const StorageType Element = GetStorage(Index);
            BitCount += FBitHelper::CountAssignedBits(Element);
        }

        return BitCount;
    }

    /**
     * Check if any bit is set
     *
     * @return: Returns true if any bit is set
     */
    NODISCARD FORCEINLINE bool HasAnyBitSet() const noexcept
    {
        return (CountAssignedBits() != 0);
    }

    /**
     * Check if no bit is set
     *
     * @return: Returns true if no bit is set
     */
    NODISCARD FORCEINLINE bool HasNoBitSet() const noexcept
    {
        return (CountAssignedBits() == 0);
    }

    /**
     * Retrieve the most significant bit. Will return zero if no bits are set, check HasAnyBitSet.
     *
     * @return: Returns the index of the most significant bit
     */
    NODISCARD FORCEINLINE SizeType MostSignificant() const
    {
        SizeType Result = 0;
        for (int32 Index = int32(NumElements) - 1; Index >= 0; --Index)
        {
            const auto Element = GetStorage(Index);
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
    NODISCARD FORCEINLINE SizeType LeastSignificant() const
    {
        SizeType Result = 0;
        for (SizeType Index = 0; Index < NumElements; ++Index)
        {
            const auto Element = GetStorage(Index);
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
     * Insert a new bit with a certain value
     *
     * @param BitPosition: Position of the bit to set
     * @param bValue: Value to assign to the bit
     */
    inline void Insert(SizeType BitPosition, const bool bValue) noexcept
    {
        Check(BitPosition <= NumBits);

        Reserve(NumBits + 1);

        BitshiftLeft_SimpleWithBitOffset(1, BitPosition);

        const SizeType ElementIndex = GetStorageIndexOfBit(BitPosition);

        StorageType& Element = GetStorage(ElementIndex);
        Element |= (StorageType(bValue) << BitPosition);

        NumBits++;
    }

    /**
     * Remove a bit from the array
     * 
     * @param BitPosition: Position of the bit to set
     */
    inline void Remove(SizeType BitPosition) noexcept
    {
        Check(BitPosition <= NumBits);

        BitshiftRight_SimpleWithBitOffset(1, BitPosition);
        NumBits--;
    }

    /**
     * Reserve a certain number of bits to be stored
     * 
     * @param NumBits: Number of bits to be able to store
     */
    FORCEINLINE void Reserve(SizeType InNumBits) noexcept
    {
        const SizeType MaxBits = Capacity();
        if (InNumBits >= MaxBits)
        {
            const SizeType NewNumElements = GetRequiredStorageForBits(InNumBits);
            Storage.Realloc(NumElements, NewNumElements);

            StorageType* Pointer = Storage.GetAllocation();
            for (SizeType Index = NumElements; Index < NewNumElements; ++Index)
            {
                Pointer[Index] = 0;
            }

            NumElements = NewNumElements;
        }
    }

    /**
     * Resize the array to a certain number of bits
     * 
     * @param InNumBits: New number of bits in the array
     */
    inline void Resize(SizeType InNumBits) noexcept
    {
        Reserve(InNumBits);
        NumBits = InNumBits;
    }

    /**
     * Shrink the allocated array to fit the number of bits and remove unnecessary space
     */
    FORCEINLINE void ShrinkToFit() noexcept
    {
        const SizeType RequiredElements = GetRequiredStorageForBits(NumBits);
        if (RequiredElements >= NumElements)
        {
            Storage.Realloc(NumElements, RequiredElements);
            NumElements = RequiredElements;
        }
    }

    /**
     * Perform a bitwise AND between this and another BitArray
     * 
     * @param Other: BitArray to perform bitwise AND with
     */
    FORCEINLINE void BitwiseAnd(const TBitArray& Other) noexcept
    {
        const SizeType Count = NMath::Min<SizeType>(NumElements, Other.NumElements);
        for (SizeType Index = 0; Index < Count; Index++)
        {
            StorageType& Element = GetStorage(Index);
            Element &= Other.GetStorage(Index);
        }
    }

    /**
     * Perform a bitwise OR between this and another BitArray
     *
     * @param Other: BitArray to perform bitwise OR with
     */
    FORCEINLINE void BitwiseOr(const TBitArray& Other) noexcept
    {
        const SizeType Count = NMath::Min<SizeType>(NumElements, Other.NumElements);
        for (SizeType Index = 0; Index < Count; Index++)
        {
            StorageType& Element = GetStorage(Index);
            Element |= Other.GetStorage(Index);
        }
    }

    /**
     * Perform a bitwise XOR between this and another BitArray
     *
     * @param Other: BitArray to perform bitwise XOR with
     */
    FORCEINLINE void BitwiseXor(const TBitArray& Other) noexcept
    {
        const SizeType Count = NMath::Min<SizeType>(NumElements, Other.NumElements);
        for (SizeType Index = 0; Index < Count; Index++)
        {
            StorageType& Element = GetStorage(Index);
            Element |= Other.GetStorage(Index);
        }
    }

    /**
     * Perform a bitwise NOT on each bit in this BitArray
     *
     * @param Other: BitArray to perform bitwise XOR with
     */
    FORCEINLINE void BitwiseNot() noexcept
    {
        for (SizeType Index = 0; Index < NumElements; Index++)
        {
            StorageType& Element = GetStorage(Index);
            Element = ~Element;
        }
    }

    /**
     * Perform a right BitShift
     * 
     * @param Steps: Number of steps to shift
     */
    inline void BitshiftRight(SizeType Steps) noexcept
    {
        if (Steps && NumBits)
        {
            BitshiftRightUnchecked(Steps, 0);
        }
    }

    /**
     * Perform a left BitShift
     *
     * @param Steps: Number of steps to shift
     */
    inline void BitshiftLeft(SizeType Steps) noexcept
    {
        if (Steps && NumBits)
        {
            BitshiftLeftUnchecked(Steps, 0);
        }
    }

    /**
     * Retrieve a reference to the bit with the index
     * 
     * @param Index: Index of the bit
     * @return: Returns a reference to the bit with the index
     */
    NODISCARD FORCEINLINE BitReferenceType GetBitReference(SizeType BitIndex) noexcept
    {
        Check(BitIndex < NumBits);

        const SizeType ElementIndex = GetStorageIndexOfBit(BitIndex);
        Check(ElementIndex < NumElements);

        StorageType& Element = GetStorage(ElementIndex);
        return BitReferenceType(Element, ~Element);
    }

    /**
     * Retrieve a reference to the bit with the index
     *
     * @param Index: Index of the bit
     * @return: Returns a reference to the bit with the index
     */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitReference(SizeType BitIndex) const noexcept
    {
        Check(BitIndex < NumBits);

        const SizeType ElementIndex = GetStorageIndexOfBit(BitIndex);
        Check(ElementIndex < NumElements);

        const StorageType& Element = GetStorage(ElementIndex);
        return ConstBitReferenceType(Element, CreateMaskForBit(BitIndex));
    }

    /**
     * Retrieve the number of bits
     *
     * @return: Returns the number of bits in the array
     */
    NODISCARD FORCEINLINE SizeType GetSize() const noexcept
    {
        return NumBits;
    }

    /**
     * Retrieve the number of integers used to store the bits
     * 
     * @return: Returns the number of integers used to store the bits
     */
    NODISCARD FORCEINLINE SizeType StorageSize() const noexcept
    {
        return NumElements;
    }

    /**
     * Retrieve the maximum number of bits
     *
     * @return: Returns the maximum number of bits in the array
     */
    NODISCARD FORCEINLINE SizeType Capacity() const noexcept
    {
        return NumElements * GetBitsPerStorage();
    }

    /**
     * Retrieve the capacity of the array in bytes
     *
     * @return: Returns the capacity of the array in bytes
     */
    NODISCARD FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return NumElements * sizeof(StorageType);
    }

    /**
     * Retrieve the data of the Array
     * 
     * @return: Returns a pointer to the stored data
     */
    NODISCARD FORCEINLINE StorageType* GetData() noexcept
    {
        return Storage.GetAllocation();
    }

    /**
     * Retrieve the data of the Array
     *
     * @return: Returns a pointer to the stored data
     */
    NODISCARD FORCEINLINE const StorageType* GetData() const noexcept
    {
        return Storage.GetAllocation();
    }

public:

    /**
     * Bitwise AND operator, perform a bitwise AND between this and another BitArray
     *
     * @param RHS: BitArray to perform bitwise AND with
     * @return: Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator&=(const TBitArray& RHS) noexcept
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
    FORCEINLINE TBitArray& operator|=(const TBitArray& RHS) noexcept
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
    FORCEINLINE TBitArray& operator^=(const TBitArray& RHS) noexcept
    {
        BitwiseXor(RHS);
        return *this;
    }

    /**
     * Perform a bitwise NOT on each bit in this BitArray
     *
     * @param Other: BitArray to perform bitwise XOR with
     */
    FORCEINLINE TBitArray operator~() const noexcept
    {
        TBitArray NewArray(*this);
        NewArray.BitwiseNot();
        return NewArray;
    }

    /**
     * Perform a bitshift right
     * 
     * @param RHS: Number of steps to bitshift
     * @return: Returns a copy that is bitshifted to the right
     */
    FORCEINLINE TBitArray operator>>(SizeType RHS) const noexcept
    {
        TBitArray NewArray(*this);
        NewArray.BitshiftRight(RHS);
        return NewArray;
    }

    /**
     * Perform a bitshift right
     *
     * @param RHS: Number of steps to bitshift
     * @return: Returns a reference to this object
     */
    FORCEINLINE TBitArray& operator>>=(SizeType RHS) const noexcept
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
    FORCEINLINE TBitArray operator<<(SizeType RHS) const noexcept
    {
        TBitArray NewArray(*this);
        NewArray.BitshiftLeft(RHS);
        return NewArray;
    }

    /**
     * Perform a bitshift left
     *
     * @param RHS: Number of steps to bitshift
     * @return: Returns a reference to this object
     */
    FORCEINLINE TBitArray& operator<<=(SizeType RHS) const noexcept
    {
        BitshiftLeft(RHS);
        return *this;
    }

    /**
     * Retrieve a bit with a certain index
     *
     * @param Index: Index to the bit
     * @return: Returns a BitReference to the specified bit
     */
    NODISCARD FORCEINLINE BitReferenceType operator[](SizeType Index) noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * Retrieve a bit with a certain index
     *
     * @param Index: Index to the bit
     * @return: Returns a BitReference to the specified bit
     */
    NODISCARD FORCEINLINE const ConstBitReferenceType operator[](SizeType Index) const noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * Copy assignment operator
     * 
     * @param RHS: BitArray to copy from
     * @return: Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator=(const TBitArray& RHS) noexcept
    {
        CopyFrom(RHS);
        return *this;
    }

    /**
     * Move assignment operator
     *
     * @param RHS: BitArray to move from
     * @return: Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator=(TBitArray&& RHS) noexcept
    {
        MoveFrom(Move(RHS));
        return *this;
    }

    /**
     * Compare operator
     * 
     * @param RHS: Right-hand side to compare
     * @return: Returns true if the BitArrays are equal
     */
    template<typename OtherStorageType, typename OtherAllocatorType>
    NODISCARD FORCEINLINE bool operator==(const TBitArray<OtherStorageType, OtherAllocatorType>& RHS) const noexcept
    {
        if (NumBits != RHS.NumBits)
        {
            return false;
        }

        for (SizeType Index = 0; Index < NumElements; ++Index)
        {
            if (GetStorage(Index) != RHS.GetStorage(Index))
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
    template<typename OtherStorageType, typename OtherAllocatorType>
    NODISCARD FORCEINLINE bool operator!=(const TBitArray<OtherStorageType, OtherAllocatorType>& RHS) const noexcept
    {
        return !(*this == RHS);
    }

public:

    /**
     * Bitwise AND operator, perform a bitwise AND between this and another BitArray
     *
     * @param LHS: Left-hand side to bitwise AND with
     * @param RHS: Right-hand side to bitwise AND with
     * @return: Returns a BitArray with the result
     */
    friend NODISCARD FORCEINLINE TBitArray operator&(const TBitArray& LHS, const TBitArray& RHS) noexcept
    {
        TBitArray NewArray(LHS);
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
    friend NODISCARD FORCEINLINE TBitArray operator|(const TBitArray& LHS, const TBitArray& RHS) noexcept
    {
        TBitArray NewArray(LHS);
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
    friend NODISCARD FORCEINLINE TBitArray operator^(const TBitArray& LHS, const TBitArray& RHS) noexcept
    {
        TBitArray NewArray(LHS);
        NewArray.BitwiseXor(RHS);
        return NewArray;
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
     * STL start iterator, same as TBitArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TBitArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    NODISCARD FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * STL start iterator, same as TBitArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TBitArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

public: 
    static NODISCARD CONSTEXPR SizeType GetBitsPerStorage() noexcept
    {
        return sizeof(StorageType) * 8;
    }

private:
    static NODISCARD CONSTEXPR SizeType GetStorageIndexOfBit(SizeType BitIndex) noexcept
    {
        return (BitIndex / GetBitsPerStorage());
    }

    static NODISCARD CONSTEXPR SizeType GetIndexOfBitInStorage(SizeType BitIndex) noexcept
    {
        return (BitIndex % GetBitsPerStorage());
    }

    static NODISCARD CONSTEXPR StorageType CreateMaskForBit(SizeType BitIndex) noexcept
    {
        return StorageType(1) << GetIndexOfBitInStorage(BitIndex);
    }

    static NODISCARD CONSTEXPR StorageType CreateMaskUpToBit(SizeType BitIndex) noexcept
    {
        return CreateMaskForBit(BitIndex) - 1;
    }

    static NODISCARD CONSTEXPR SizeType GetRequiredStorageForBits(SizeType InNumBits) noexcept
    {
        return (InNumBits + (GetBitsPerStorage() - 1)) / GetBitsPerStorage();
    }

private:
    FORCEINLINE void AllocateAndZeroStorage(SizeType InNumBits) noexcept
    {
        const SizeType NewNumElements = GetRequiredStorageForBits(InNumBits);
        Storage.Realloc(NumElements, NewNumElements);

        NumElements = NewNumElements;
         
        FMemory::Memzero(Storage.GetAllocation(), CapacityInBytes());
    }

    FORCEINLINE void CopyFrom(const TBitArray& Other) noexcept
    {
        FMemory::Memcpy(Storage.GetAllocation(), Other.Storage.GetAllocation(), Other.NumElements * sizeof(StorageType));
    }

    FORCEINLINE void MoveFrom(TBitArray&& Other) noexcept
    {
        Storage.MoveFrom(Move(Other.Storage));
        Other.NumBits     = 0;
        Other.NumElements = 0;
    }

    FORCEINLINE void AssignBitUnchecked(SizeType BitPosition, const bool bValue) noexcept
    {
        const SizeType ElementIndex   = GetStorageIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInStorage(BitPosition);

        const StorageType Mask  = CreateMaskForBit(IndexInElement);
        const StorageType Value = bValue ? Mask : StorageType(0);

        StorageType& Element = GetStorage(ElementIndex);
        Element |= Value;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Right shift

    FORCEINLINE void BitshiftRightUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
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

    FORCEINLINE void BitshiftRight_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
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

    FORCEINLINE void BitshiftRight_SimpleWithBitOffset(SizeType Steps, SizeType BitPosition)
    {
        const SizeType StartElementIndex = GetStorageIndexOfBit(BitPosition);
        const SizeType ElementsToShift = NumElements - StartElementIndex;

        StorageType* Pointer = GetData() + StartElementIndex;

        // Mask value to ensure that we get zeros shifted in
        const StorageType StartValue  = *Pointer;
        const StorageType Mask        = CreateMaskUpToBit(BitPosition);
        const StorageType InverseMask = ~Mask;
        *Pointer = (StartValue & InverseMask);

        BitshiftRight_Simple(Steps, StartElementIndex, ElementsToShift);

        const StorageType CurrentValue = *Pointer;
        *Pointer = (CurrentValue & InverseMask) | (StartValue & Mask);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Left shift

    FORCEINLINE void BitshiftLeftUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
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

    FORCEINLINE void BitshiftLeft_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
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

    FORCEINLINE void BitshiftLeft_SimpleWithBitOffset(SizeType Steps, SizeType BitPosition)
    {
        const SizeType StartElementIndex = GetStorageIndexOfBit(BitPosition);
        const SizeType ElementsToShift   = NumElements - StartElementIndex;

        StorageType* Pointer = GetData() + StartElementIndex;

        // Mask value to ensure that we get zeros shifted in
        const StorageType StartValue  = *Pointer;
        const StorageType Mask        = CreateMaskUpToBit(BitPosition);
        const StorageType InverseMask = ~Mask;
        *Pointer = (StartValue & InverseMask);

        BitshiftLeft_Simple(Steps, StartElementIndex, ElementsToShift);

        const StorageType CurrentValue = *Pointer;
        *Pointer = (CurrentValue & InverseMask) | (StartValue & Mask);
    }

    FORCEINLINE void MaskOutLastStorageElement()
    {
        const SizeType LastValidBit     = NumBits ? (NumBits - 1) : 0;
        const SizeType LastElementIndex = GetStorageIndexOfBit(LastValidBit);
        const SizeType LastBitIndex     = GetIndexOfBitInStorage(LastValidBit);

        const StorageType Mask = CreateMaskUpToBit(LastBitIndex) | CreateMaskForBit(LastBitIndex);

        StorageType& Element = GetStorage(LastElementIndex);
        Element &= Mask;
    }

    NODISCARD FORCEINLINE StorageType& GetStorageForBit(SizeType BitIndex) noexcept
    {
        const SizeType ElementIndex = GetStorageIndexOfBit(BitIndex);
        return GetStorage(ElementIndex);
    }

    NODISCARD FORCEINLINE const StorageType& GetStorageForBit(SizeType BitIndex) const noexcept
    {
        const SizeType ElementIndex = GetStorageIndexOfBit(BitIndex);
        return GetStorage(ElementIndex);
    }

    NODISCARD FORCEINLINE StorageType& GetStorage(SizeType Index) noexcept
    {
        StorageType* Pointer = Storage.GetAllocation();
        return Pointer[Index];
    }

    NODISCARD FORCEINLINE const StorageType& GetStorage(SizeType Index) const noexcept
    {
        const StorageType* Pointer = Storage.GetAllocation();
        return Pointer[Index];
    }

    AllocatorType Storage;
    SizeType      NumBits     = 0;
    SizeType      NumElements = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

typedef TBitArray<uint8>  FBitArray8;
typedef TBitArray<uint16> FBitArray16;
typedef TBitArray<uint32> FBitArray32;
typedef TBitArray<uint64> FBitArray64;