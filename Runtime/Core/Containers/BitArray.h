#pragma once
#include "Iterator.h"
#include "Allocators.h"
#include "Core/Core.h"
#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/BitHelper.h"
#include "Core/Templates/BitReference.h"
#include "Core/Templates/ArrayContainerHelper.h"

template<typename InIntegerType = uint32, typename InAllocatorType = TDefaultArrayAllocator<InIntegerType>>
class TBitArray
{
public:
    using SizeType = int32;
    static_assert(TIsUnsigned<InIntegerType>::Value, "BitArray must have an unsigned InIntegerType");

    using BitReferenceType      = TBitReference<InIntegerType>;
    using ConstBitReferenceType = TBitReference<const InIntegerType>;

    typedef TBitArrayIterator<TBitArray, InIntegerType>                    IteratorType;
    typedef TBitArrayIterator<const TBitArray, const InIntegerType>        ConstIteratorType;
    typedef TReverseBitArrayIterator<TBitArray, InIntegerType>             ReverseIteratorType;
    typedef TReverseBitArrayIterator<const TBitArray, const InIntegerType> ReverseConstIteratorType;

public:

    /** @brief - Default constructor */
    TBitArray() noexcept = default;

    /**
     * @brief         - Constructor that sets the elements based on an integer
     * @param InValue - Integer containing bits to set to the BitArray
     */
    FORCEINLINE explicit TBitArray(InIntegerType InValue) noexcept
        : Allocator()
        , NumBits(NumBitsPerInteger())
        , NumElements(0)
    {
        InitializeZeroed(NumBits);
        InIntegerType& Element = GetInteger(0);
        Element = InValue;
    }

    /**
     * @brief           - Constructor that sets the elements based on an integer
     * @param InValues  - Integers containing bits to set to the BitArray
     * @param NumValues - Number of values in the input array
     */
    NOINLINE explicit TBitArray(const InIntegerType* InValues, SizeType NumValues) noexcept
        : Allocator()
        , NumBits(NumValues * NumBitsPerInteger())
        , NumElements(0)
    {
        InitializeZeroed(NumBits);

        for (SizeType Index = 0; Index < NumValues; ++Index)
        {
            InIntegerType& Element = GetInteger(Index);
            Element = InValues[Index];
        }
    }

    /**
     * @brief         - Constructor that sets a certain number of bits to specified value
     * @param bValue  - Value to set bits to
     * @param NumBits - Number of bits to set
     */
    FORCEINLINE explicit TBitArray(SizeType InNumBits, bool bValue) noexcept
        : Allocator()
        , NumBits(InNumBits)
        , NumElements(0)
    {
        InitializeZeroed(NumBits);
        for (SizeType Index = 0; Index < InNumBits; Index++)
        {
            AssignBitUnchecked(Index, bValue);
        }
    }

    /**
     * @brief          - Constructor that creates a BitArray from a list of booleans indicating the sign of the bit
     * @param InitList - Contains bools to indicate the sign of each bit
     */
    FORCEINLINE TBitArray(std::initializer_list<bool> InitList) noexcept
        : Allocator()
        , NumBits(FArrayContainerHelper::Size(InitList))
        , NumElements(0)
    {
        InitializeZeroed(NumBits);
        
        SizeType Index = 0;
        for (bool bValue : InitList)
        {
            AssignBitUnchecked(Index++, bValue);
        }
    }

    /**
     * @brief       - Copy constructor
     * @param Other - BitArray to copy from
     */
    FORCEINLINE TBitArray(const TBitArray& Other) noexcept
        : Allocator()
        , NumBits(Other.NumBits)
        , NumElements(0)
    {
        InitializeZeroed(NumBits);
        CopyFrom(Other);
    }

    /**
     * @brief       - Move constructor
     * @param Other - BitArray to move from
     */ 
    FORCEINLINE TBitArray(TBitArray&& Other) noexcept
        : Allocator()
        , NumBits(Other.NumBits)
        , NumElements(Other.NumElements)
    {
        MoveFrom(::Move(Other));
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TBitArray()
    {
        Allocator.Free();
        NumBits     = 0;
        NumElements = 0;
    }

    /**
     * @brief - Resets the all the bits to zero
     */
    FORCEINLINE void Reset()
    {
        FMemory::Memset(Allocator.GetAllocation(), 0x0, CapacityInBytes());
    }

    /**
     * @brief  - Checks if an index is a valid index
     * @return - Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SizeType Index) const noexcept
    {
        return (Index >= 0) && (Index < NumBits);
    }

    /**
     * @brief  - Check if the array is empty
     * @return - Returns true if the array is empty
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return NumBits == 0;
    }

    /**
     * @brief        - Add a new bit with the specified value
     * @param bValue - Value of the new bit
     */
    void Add(const bool bValue) noexcept
    {
        Reserve(NumBits + 1);
        AssignBitUnchecked(NumBits, bValue);
        NumBits++;
    }

    /**
     * @brief             - Assign a value to a bit
     * @param BitPosition - Position of the bit to set
     * @param bValue      - Value to assign to the bit
     */
    void AssignBit(SizeType BitPosition, const bool bValue) noexcept
    {
        CHECK(BitPosition < NumBits);
        AssignBitUnchecked(BitPosition, bValue);
    }

    /**
     * @brief             - Flips the bit at the position
     * @param BitPosition - Position of the bit to set
     */
    FORCEINLINE void FlipBit(SizeType BitPosition) noexcept
    {
        CHECK(BitPosition < NumBits);
        const SizeType ElementIndex   = GetArrayIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInArray(BitPosition);

        const InIntegerType Mask = CreateMaskForBit(IndexInElement);
        InIntegerType& Element = GetInteger(ElementIndex);
        Element ^= Mask;
    }

    /**
     * @brief  - Count the number of bits that are assigned
     * @return - Returns the number of bits that are true
     */
    NODISCARD FORCEINLINE SizeType CountAssignedBits() const noexcept
    {
        SizeType BitCount = 0;
        for (SizeType Index = 0; Index < NumElements; ++Index)
        {
            const InIntegerType Element = GetInteger(Index);
            BitCount += FBitHelper::CountAssignedBits(Element);
        }

        return BitCount;
    }

    /**
     * @brief  - Check if any bit is set
     * @return - Returns true if any bit is set
     */
    NODISCARD FORCEINLINE bool HasAnyBitSet() const noexcept
    {
        return CountAssignedBits() != 0;
    }

    /**
     * @brief  - Check if no bit is set
     * @return - Returns true if no bit is set
     */
    NODISCARD FORCEINLINE bool HasNoBitSet() const noexcept
    {
        return CountAssignedBits() == 0;
    }

    /**
     * @brief  - Retrieve the most significant bit. Will return zero if no bits are set, check HasAnyBitSet.
     * @return - Returns the index of the most significant bit
     */
    NODISCARD FORCEINLINE SizeType MostSignificant() const
    {
        SizeType Result = 0;
        for (int32 Index = int32(NumElements) - 1; Index >= 0; --Index)
        {
            const auto Element = GetInteger(Index);
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
    NODISCARD FORCEINLINE SizeType LeastSignificant() const
    {
        SizeType Result = 0;
        for (SizeType Index = 0; Index < NumElements; ++Index)
        {
            const auto Element = GetInteger(Index);
            if (Element)
            {
                const auto BitIndex = FBitHelper::LeastSignificant<SizeType>(Element);
                Result = BitIndex + (Index * NumBitsPerInteger());
                break;
            }
        }

        return Result;
    }

    /**
     * @brief             - Insert a new bit with a certain value
     * @param BitPosition - Position of the bit to set
     * @param bValue      - Value to assign to the bit
     */
    void Insert(SizeType BitPosition, const bool bValue) noexcept
    {
        CHECK(BitPosition <= NumBits);
        Reserve(NumBits + 1);
        BitshiftLeft_SimpleWithBitOffset(1, BitPosition);

        const SizeType ElementIndex = GetArrayIndexOfBit(BitPosition);
        InIntegerType& Element = GetInteger(ElementIndex);
        Element |= (InIntegerType(bValue) << BitPosition);
        NumBits++;
    }

    /**
     * @brief             - Remove a bit from the array
     * @param BitPosition - Position of the bit to set
     */
    inline void Remove(SizeType BitPosition) noexcept
    {
        CHECK(BitPosition <= NumBits);
        BitshiftRight_SimpleWithBitOffset(1, BitPosition);
        NumBits--;
    }

    /**
     * @brief         - Reserve a certain number of bits to be stored
     * @param NumBits - Number of bits to be able to store
     */
    FORCEINLINE void Reserve(SizeType InNumBits) noexcept
    {
        const SizeType MaxBits = Capacity();
        if (InNumBits >= MaxBits)
        {
            const SizeType NewNumElements = GetNumIntegersRequiredForBits(InNumBits);
            Allocator.Realloc(NumElements, NewNumElements);

            InIntegerType* Array = Allocator.GetAllocation();
            for (SizeType Index = NumElements; Index < NewNumElements; ++Index)
            {
                Array[Index] = 0;
            }

            NumElements = NewNumElements;
        }
    }

    /**
     * @brief           - Resize the array to a certain number of bits
     * @param InNumBits - New number of bits in the array
     */
    inline void Resize(SizeType InNumBits) noexcept
    {
        Reserve(InNumBits);
        NumBits = InNumBits;
    }

    /**
     * @brief - Shrink the allocated array to fit the number of bits and remove unnecessary space
     */
    FORCEINLINE void Shrink() noexcept
    {
        const SizeType RequiredElements = GetNumIntegersRequiredForBits(NumBits);
        if (RequiredElements >= NumElements)
        {
            Allocator.Realloc(NumElements, RequiredElements);
            NumElements = RequiredElements;
        }
    }

    /**
     * @brief       - Perform a bitwise AND between this and another BitArray
     * @param Other - BitArray to perform bitwise AND with
     */
    FORCEINLINE void BitwiseAnd(const TBitArray& Other) noexcept
    {
        const SizeType Count = FMath::Min<SizeType>(NumElements, Other.NumElements);
        for (SizeType Index = 0; Index < Count; Index++)
        {
            InIntegerType& Element = GetInteger(Index);
            Element &= Other.GetInteger(Index);
        }
    }

    /**
     * @brief       - Perform a bitwise OR between this and another BitArray
     * @param Other - BitArray to perform bitwise OR with
     */
    FORCEINLINE void BitwiseOr(const TBitArray& Other) noexcept
    {
        const SizeType Count = FMath::Min<SizeType>(NumElements, Other.NumElements);
        for (SizeType Index = 0; Index < Count; Index++)
        {
            InIntegerType& Element = GetInteger(Index);
            Element |= Other.GetInteger(Index);
        }
    }

    /**
     * @brief       - Perform a bitwise XOR between this and another BitArray
     * @param Other - BitArray to perform bitwise XOR with
     */
    FORCEINLINE void BitwiseXor(const TBitArray& Other) noexcept
    {
        const SizeType Count = FMath::Min<SizeType>(NumElements, Other.NumElements);
        for (SizeType Index = 0; Index < Count; Index++)
        {
            InIntegerType& Element = GetInteger(Index);
            Element |= Other.GetInteger(Index);
        }
    }

    /**
     * @brief       - Perform a bitwise NOT on each bit in this BitArray
     * @param Other - BitArray to perform bitwise XOR with
     */
    FORCEINLINE void BitwiseNot() noexcept
    {
        for (SizeType Index = 0; Index < NumElements; Index++)
        {
            InIntegerType& Element = GetInteger(Index);
            Element = ~Element;
        }
    }

    /**
     * @brief       - Perform a right BitShift
     * @param Steps - Number of steps to shift
     */
    FORCEINLINE void BitshiftRight(SizeType Steps) noexcept
    {
        if (Steps && NumBits)
        {
            BitshiftRightUnchecked(Steps, 0);
        }
    }

    /**
     * @brief       - Perform a left BitShift
     * @param Steps - Number of steps to shift
     */
    FORCEINLINE void BitshiftLeft(SizeType Steps) noexcept
    {
        if (Steps && NumBits)
        {
            BitshiftLeftUnchecked(Steps, 0);
        }
    }

    /**
     * @brief       - Retrieve a reference to the bit with the index
     * @param Index - Index of the bit
     * @return      - Returns a reference to the bit with the index
     */
    NODISCARD FORCEINLINE BitReferenceType GetBitReference(SizeType BitIndex) noexcept
    {
        CHECK(BitIndex < NumBits);
        const SizeType ElementIndex = GetArrayIndexOfBit(BitIndex);
        CHECK(ElementIndex < NumElements);

        InIntegerType& Element = GetInteger(ElementIndex);
        return BitReferenceType(Element, ~Element);
    }

    /**
     * @brief       - Retrieve a reference to the bit with the index
     * @param Index - Index of the bit
     * @return      - Returns a reference to the bit with the index
     */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitReference(SizeType BitIndex) const noexcept
    {
        CHECK(BitIndex < NumBits);
        const SizeType ElementIndex = GetArrayIndexOfBit(BitIndex);
        CHECK(ElementIndex < NumElements);

        const InIntegerType& Element = GetInteger(ElementIndex);
        return ConstBitReferenceType(Element, CreateMaskForBit(BitIndex));
    }

    /**
     * @brief  - Retrieve the number of bits
     * @return - Returns the number of bits in the array
     */
    NODISCARD FORCEINLINE SizeType Size() const noexcept
    {
        return NumBits;
    }

    /**
     * @brief  - Retrieve the number of integers used to store the bits
     * @return - Returns the number of integers used to store the bits
     */
    NODISCARD FORCEINLINE SizeType IntegerSize() const noexcept
    {
        return NumElements;
    }

    /**
     * @brief  - Retrieve the maximum number of bits
     * @return - Returns the maximum number of bits in the array
     */
    NODISCARD FORCEINLINE SizeType Capacity() const noexcept
    {
        return NumElements * NumBitsPerInteger();
    }

    /**
     * @brief  - Retrieve the capacity of the array in bytes
     * @return - Returns the capacity of the array in bytes
     */
    NODISCARD FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return NumElements * sizeof(InIntegerType);
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD FORCEINLINE InIntegerType* Data() noexcept
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD FORCEINLINE const InIntegerType* Data() const noexcept
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD FORCEINLINE InAllocatorType& GetAllocator() noexcept
    {
        return Allocator;
    }

    /**
     * @brief  - Retrieve the data of the Array
     * @return - Returns a pointer to the stored data
     */
    NODISCARD FORCEINLINE const InAllocatorType& GetAllocator() const noexcept
    {
        return Allocator;
    }

public:

    /**
     * @brief     - Bitwise AND operator, perform a bitwise AND between this and another BitArray
     * @param RHS - BitArray to perform bitwise AND with
     * @return    - Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator&=(const TBitArray& RHS) noexcept
    {
        BitwiseAnd(RHS);
        return *this;
    }

    /**
     * @brief     - Bitwise OR operator, perform a bitwise OR between this and another BitArray
     * @param RHS - BitArray to perform bitwise OR with
     * @return    - Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator|=(const TBitArray& RHS) noexcept
    {
        BitwiseOr(RHS);
        return *this;
    }

    /**
     * @brief     - Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     * @param RHS - BitArray to perform bitwise XOR with
     * @return    - Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator^=(const TBitArray& RHS) noexcept
    {
        BitwiseXor(RHS);
        return *this;
    }

    /**
     * @brief       - Perform a bitwise NOT on each bit in this BitArray
     * @param Other - BitArray to perform bitwise XOR with
     */
    FORCEINLINE TBitArray operator~() const noexcept
    {
        TBitArray NewArray(*this);
        NewArray.BitwiseNot();
        return NewArray;
    }

    /**
     * @brief     - Perform a bitshift right
     * @param RHS - Number of steps to bitshift
     * @return    - Returns a copy that is bitshifted to the right
     */
    FORCEINLINE TBitArray operator>>(SizeType RHS) const noexcept
    {
        TBitArray NewArray(*this);
        NewArray.BitshiftRight(RHS);
        return NewArray;
    }

    /**
     * @brief     - Perform a bitshift right
     * @param RHS - Number of steps to bitshift
     * @return    - Returns a reference to this object
     */
    FORCEINLINE TBitArray& operator>>=(SizeType RHS) const noexcept
    {
        BitshiftRight(RHS);
        return *this;
    }

    /**
     * @brief     - Perform a bitshift left
     * @param RHS - Number of steps to bitshift
     * @return    - Returns a copy that is bitshifted to the left
     */
    FORCEINLINE TBitArray operator<<(SizeType RHS) const noexcept
    {
        TBitArray NewArray(*this);
        NewArray.BitshiftLeft(RHS);
        return NewArray;
    }

    /**
     * @brief     - Perform a bitshift left
     * @param RHS - Number of steps to bitshift
     * @return    - Returns a reference to this object
     */
    FORCEINLINE TBitArray& operator<<=(SizeType RHS) const noexcept
    {
        BitshiftLeft(RHS);
        return *this;
    }

    /**
     * @brief       - Retrieve a bit with a certain index
     * @param Index - Index to the bit
     * @return      - Returns a BitReference to the specified bit
     */
    NODISCARD FORCEINLINE BitReferenceType operator[](SizeType Index) noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * @brief       - Retrieve a bit with a certain index
     * @param Index - Index to the bit
     * @return      - Returns a BitReference to the specified bit
     */
    NODISCARD FORCEINLINE const ConstBitReferenceType operator[](SizeType Index) const noexcept
    {
        return GetBitReference(Index);
    }

    /**
     * @brief     - Copy assignment operator
     * @param RHS - BitArray to copy from
     * @return    - Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator=(const TBitArray& RHS) noexcept
    {
        CopyFrom(RHS);
        return *this;
    }

    /**
     * @brief     - Move assignment operator
     * @param RHS - BitArray to move from
     * @return    - Returns a reference to this BitArray
     */
    FORCEINLINE TBitArray& operator=(TBitArray&& RHS) noexcept
    {
        MoveFrom(::Move(RHS));
        return *this;
    }

    /**
     * @brief     - Compare operator
     * @param RHS - Right-hand side to compare
     * @return    - Returns true if the BitArrays are equal
     */
    template<typename OtherIntegerType, typename OtherAllocatorType>
    NODISCARD FORCEINLINE bool operator==(const TBitArray<OtherIntegerType, OtherAllocatorType>& RHS) const noexcept
    {
        if (NumBits != RHS.NumBits)
        {
            return false;
        }

        for (SizeType Index = 0; Index < NumElements; ++Index)
        {
            if (GetInteger(Index) != RHS.GetInteger(Index))
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief     - Compare operator
     * @param RHS - Right-hand side to compare
     * @return    - Returns false if the BitArrays are equal
     */
    template<typename OtherIntegerType, typename OtherAllocatorType>
    NODISCARD FORCEINLINE bool operator!=(const TBitArray<OtherIntegerType, OtherAllocatorType>& RHS) const noexcept
    {
        return !(*this == RHS);
    }

public:

    /**
     * @brief     - Bitwise AND operator, perform a bitwise AND between this and another BitArray
     * @param LHS - Left-hand side to bitwise AND with
     * @param RHS - Right-hand side to bitwise AND with
     * @return    - Returns a BitArray with the result
     */
    NODISCARD friend FORCEINLINE TBitArray operator&(const TBitArray& LHS, const TBitArray& RHS) noexcept
    {
        TBitArray NewArray(LHS);
        NewArray.BitwiseAnd(RHS);
        return NewArray;
    }

    /**
     * @brief     - Bitwise OR operator, perform a bitwise OR between this and another BitArray
     * @param LHS - Left-hand side to bitwise OR with
     * @param RHS - Right-hand side to bitwise OR with
     * @return    - Returns a BitArray with the result
     */
    NODISCARD friend FORCEINLINE TBitArray operator|(const TBitArray& LHS, const TBitArray& RHS) noexcept
    {
        TBitArray NewArray(LHS);
        NewArray.BitwiseOr(RHS);
        return NewArray;
    }

    /**
     * @brief     - Bitwise XOR operator, perform a bitwise XOR between this and another BitArray
     * @param LHS - Left-hand side to bitwise XOR with
     * @param RHS - Right-hand side to bitwise XOR with
     * @return    - Returns a BitArray with the result
     */
    NODISCARD friend FORCEINLINE TBitArray operator^(const TBitArray& LHS, const TBitArray& RHS) noexcept
    {
        TBitArray NewArray(LHS);
        NewArray.BitwiseXor(RHS);
        return NewArray;
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
        return ReverseIteratorType(*this, NumBits);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, NumBits);
    }

public: // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return ConstIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return IteratorType(*this, NumBits); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return ConstIteratorType(*this, NumBits); }

public: 
    NODISCARD static constexpr SizeType NumBitsPerInteger() noexcept
    {
        return sizeof(InIntegerType) * 8;
    }

private:
    NODISCARD static constexpr SizeType GetArrayIndexOfBit(SizeType BitIndex) noexcept
    {
        return (BitIndex / NumBitsPerInteger());
    }

    NODISCARD static constexpr SizeType GetIndexOfBitInArray(SizeType BitIndex) noexcept
    {
        return (BitIndex % NumBitsPerInteger());
    }

    NODISCARD static constexpr InIntegerType CreateMaskForBit(SizeType BitIndex) noexcept
    {
        return InIntegerType(1) << GetIndexOfBitInArray(BitIndex);
    }

    NODISCARD static constexpr InIntegerType CreateMaskUpToBit(SizeType BitIndex) noexcept
    {
        return CreateMaskForBit(BitIndex) - 1;
    }

    NODISCARD static constexpr SizeType GetNumIntegersRequiredForBits(SizeType InNumBits) noexcept
    {
        return (InNumBits + (NumBitsPerInteger() - 1)) / NumBitsPerInteger();
    }

private:
    FORCEINLINE void InitializeZeroed(SizeType InNumBits) noexcept
    {
        const SizeType NewNumElements = GetNumIntegersRequiredForBits(InNumBits);
        Allocator.Realloc(NumElements, NewNumElements);
        NumElements = NewNumElements;
        FMemory::Memzero(Allocator.GetAllocation(), CapacityInBytes());
    }

    FORCEINLINE void CopyFrom(const TBitArray& Other) noexcept
    {
        FMemory::Memcpy(Allocator.GetAllocation(), Other.Allocator.GetAllocation(), Other.NumElements * sizeof(InIntegerType));
    }

    FORCEINLINE void MoveFrom(TBitArray&& Other) noexcept
    {
        Allocator.MoveFrom(::Move(Other.Allocator));
        Other.NumBits     = 0;
        Other.NumElements = 0;
    }

    FORCEINLINE void AssignBitUnchecked(SizeType BitPosition, const bool bValue) noexcept
    {
        const SizeType ElementIndex   = GetArrayIndexOfBit(BitPosition);
        const SizeType IndexInElement = GetIndexOfBitInArray(BitPosition);

        const InIntegerType Mask  = CreateMaskForBit(IndexInElement);
        const InIntegerType Value = bValue ? Mask : InIntegerType(0);

        InIntegerType& Element = GetInteger(ElementIndex);
        Element |= Value;
    }

    FORCEINLINE void BitshiftRightUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
    {
        const SizeType StartElementIndex = GetArrayIndexOfBit(StartBit);
        InIntegerType* Array = Allocator.GetAllocation() + StartElementIndex;

        const SizeType RemainingElements = IntegerSize() - StartElementIndex;
        const SizeType RemainingBits     = NumBits - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const InIntegerType StartValue  = *Array;
            const InIntegerType Mask        = CreateMaskUpToBit(StartBit);
            const InIntegerType InverseMask = ~Mask;
            *Array = (StartValue & InverseMask);

            const SizeType DiscardCount = Steps / NumBitsPerInteger();
            const SizeType RangeSize    = RemainingElements - DiscardCount;

            FMemory::Memmove(Array, Array + DiscardCount, sizeof(InIntegerType) * RangeSize);
            FMemory::Memzero(Array + RangeSize, sizeof(InIntegerType) * DiscardCount);

            BitshiftRight_Simple(Steps, StartElementIndex, RangeSize);

            const InIntegerType CurrentValue = *Array;
            *Array = (CurrentValue & InverseMask) | (StartValue & Mask);
        }
        else
        {
            FMemory::Memzero(Array, RemainingElements * sizeof(InIntegerType));
        }
    }

    FORCEINLINE void BitshiftRight_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        InIntegerType* Array = Allocator.GetAllocation() + StartElementIndex + ElementsToShift;
        const SizeType CurrShift = Steps % NumBitsPerInteger();
        const SizeType PrevShift = NumBitsPerInteger() - CurrShift;

        InIntegerType Previous = 0;
        while (ElementsToShift)
        {
            const InIntegerType Current = *(--Array);
            *(Array) = (Current >> CurrShift) | (Previous << PrevShift);
            Previous = Current;
            ElementsToShift--;
        }
    }

    FORCEINLINE void BitshiftRight_SimpleWithBitOffset(SizeType Steps, SizeType BitPosition)
    {
        const SizeType StartElementIndex = GetArrayIndexOfBit(BitPosition);
        const SizeType ElementsToShift = NumElements - StartElementIndex;
        InIntegerType* Array = Allocator.GetAllocation() + StartElementIndex;

        // Mask value to ensure that we get zeros shifted in
        const InIntegerType StartValue  = *Array;
        const InIntegerType Mask        = CreateMaskUpToBit(BitPosition);
        const InIntegerType InverseMask = ~Mask;
        *Array = (StartValue & InverseMask);

        BitshiftRight_Simple(Steps, StartElementIndex, ElementsToShift);

        const InIntegerType CurrentValue = *Array;
        *Array = (CurrentValue & InverseMask) | (StartValue & Mask);
    }

    FORCEINLINE void BitshiftLeftUnchecked(SizeType Steps, SizeType StartBit = 0) noexcept
    {
        const SizeType StartElementIndex = GetArrayIndexOfBit(StartBit);
        InIntegerType* Array = Allocator.GetAllocation() + StartElementIndex;

        const SizeType RemainingElements = IntegerSize() - StartElementIndex;
        const SizeType RemainingBits     = NumBits - StartBit;
        if (Steps < RemainingBits)
        {
            // Mask value to ensure that we get zeros shifted in
            const InIntegerType StartValue  = *Array;
            const InIntegerType Mask        = CreateMaskUpToBit(StartBit);
            const InIntegerType InverseMask = ~Mask;
            *Array = (StartValue & InverseMask);

            const SizeType DiscardCount = Steps / NumBitsPerInteger();
            const SizeType RangeSize    = RemainingElements - DiscardCount;

            FMemory::Memmove(Array + DiscardCount, Array, sizeof(InIntegerType) * RangeSize);
            FMemory::Memzero(Array, sizeof(InIntegerType) * DiscardCount);

            BitshiftLeft_Simple(Steps, StartElementIndex + DiscardCount, RangeSize);

            const InIntegerType CurrentValue = *Array;
            *Array = (CurrentValue & InverseMask) | (StartValue & Mask);
        }
        else
        {
            FMemory::Memzero(Array, RemainingElements * sizeof(InIntegerType));
        }
    }

    FORCEINLINE void BitshiftLeft_Simple(SizeType Steps, SizeType StartElementIndex, SizeType ElementsToShift)
    {
        InIntegerType* Array = Allocator.GetAllocation() + StartElementIndex;
        const SizeType CurrShift = Steps % NumBitsPerInteger();
        const SizeType PrevShift = NumBitsPerInteger() - CurrShift;

        InIntegerType Previous = 0;
        while (ElementsToShift)
        {
            const InIntegerType Current = *Array;
            *(Array++) = (Current << CurrShift) | (Previous >> PrevShift);
            Previous = Current;
            ElementsToShift--;
        }
    }

    FORCEINLINE void BitshiftLeft_SimpleWithBitOffset(SizeType Steps, SizeType BitPosition)
    {
        const SizeType StartElementIndex = GetArrayIndexOfBit(BitPosition);
        const SizeType ElementsToShift   = NumElements - StartElementIndex;
        InIntegerType* Array = Allocator.GetAllocation() + StartElementIndex;

        // Mask value to ensure that we get zeros shifted in
        const InIntegerType StartValue  = *Array;
        const InIntegerType Mask        = CreateMaskUpToBit(BitPosition);
        const InIntegerType InverseMask = ~Mask;
        *Array = (StartValue & InverseMask);

        BitshiftLeft_Simple(Steps, StartElementIndex, ElementsToShift);

        const InIntegerType CurrentValue = *Array;
        *Array = (CurrentValue & InverseMask) | (StartValue & Mask);
    }

    FORCEINLINE void MaskOutLastInteger()
    {
        const SizeType LastValidBit     = NumBits ? (NumBits - 1) : 0;
        const SizeType LastElementIndex = GetArrayIndexOfBit(LastValidBit);
        const SizeType LastBitIndex     = GetIndexOfBitInArray(LastValidBit);

        const InIntegerType Mask = CreateMaskUpToBit(LastBitIndex) | CreateMaskForBit(LastBitIndex);
        InIntegerType& Element = GetInteger(LastElementIndex);
        Element &= Mask;
    }

    NODISCARD FORCEINLINE InIntegerType& GetIntegerForBit(SizeType BitIndex) noexcept
    {
        const SizeType ElementIndex = GetArrayIndexOfBit(BitIndex);
        return GetInteger(ElementIndex);
    }

    NODISCARD FORCEINLINE const InIntegerType& GetIntegerForBit(SizeType BitIndex) const noexcept
    {
        const SizeType ElementIndex = GetArrayIndexOfBit(BitIndex);
        return GetInteger(ElementIndex);
    }

    NODISCARD FORCEINLINE InIntegerType& GetInteger(SizeType Index) noexcept
    {
        InIntegerType* Array = Allocator.GetAllocation();
        return Array[Index];
    }

    NODISCARD FORCEINLINE const InIntegerType& GetInteger(SizeType Index) const noexcept
    {
        const InIntegerType* Array = Allocator.GetAllocation();
        return Array[Index];
    }

private:
    InAllocatorType Allocator;
    SizeType NumBits{0};
    SizeType NumElements{0};
};

typedef TBitArray<uint8>  FBitArray8;
typedef TBitArray<uint16> FBitArray16;
typedef TBitArray<uint32> FBitArray32;
typedef TBitArray<uint64> FBitArray64;