#pragma once
#include "Core/CoreTypes.h"
#include "Core/CoreDefines.h"

#include "Core/Templates/IsSigned.h"
#include "Core/Templates/AddressOf.h"
#include "Core/Templates/ReferenceWrapper.h"
#include "Core/Templates/RemoveCV.h"
#include "Core/Templates/BitReference.h"

// TODO: Put some functionality into a base-class
template<
    typename ArrayType,
    typename ElementType>
class TArrayIterator
{
    using ConstElementType = const typename TRemoveCV<ElementType>::Type;
    using ConstArrayType   = const typename TRemoveCV<ArrayType>::Type;

public:
    using SizeType = typename ArrayType::SizeType;

    TArrayIterator(const TArrayIterator&) = default;
    TArrayIterator(TArrayIterator&&) = default;
    ~TArrayIterator() = default;

    TArrayIterator& operator=(const TArrayIterator&) = default;
    TArrayIterator& operator=(TArrayIterator&&) = default;

    static_assert(
        TIsSigned<SizeType>::Value,
        "TArrayIterator wants a signed SizeType");

    /**
     * @brief            - Create a new iterator
     * @param InArray    - Array to iterate
     * @param StartIndex - Index in the array to start
     */
    FORCEINLINE explicit TArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        CHECK(IsValid());
    }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false 
     */
    NODISCARD FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /**
     * @brief  - Check if the iterator is valid
     * @return - Returns true if the iterator is valid
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().GetSize());
    }

    /**
     * @brief  - Check if the iterator is equal to the end iterator
     * @return - Returns true if the iterator is the end-iterator
     */
    NODISCARD FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == Array.Get().GetSize());
    }

    /**
     * @brief  - Retrieve a raw pointer to the data
     * @return - Returns a raw pointer to the data 
     */
    NODISCARD FORCEINLINE ElementType* GetPointer() const noexcept
    {
        CHECK(IsValid());
        return Array.Get().GetData() + GetIndex();
    }

    /**
     * @brief  - Retrieve the index to for the iterator in the array
     * @return - Returns the index to the element that the iterator represent within the array   
     */
    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index;
    }

public:

    /**
     * @brief  - Retrieve a raw pointer to the data
     * @return - Returns a raw pointer to the data 
     */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return GetPointer();
    }

    /**
     * @brief  - Retrieve the data
     * @return - Returns a reference to the data 
     */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *GetPointer();
    }

    /**
     * @brief  - Increment the index for the iterator  
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator++() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Increment the index for the iterator  
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator++(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Decrement the index for the iterator
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator--() noexcept
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Decrement the index for the iterator  
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator--(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Add a new value to the iterator
     * @param Other - Value to add
     * @return      - Returns a new iterator with the result from adding Other to this value 
     */
    NODISCARD FORCEINLINE TArrayIterator operator+(SizeType Other) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator += Other;
    }

    /**
     * @brief       - Subtract a new value to the iterator
     * @param Other - Value to subtract
     * @return      - Returns a new iterator with the result from subtracting Other to this value 
     */
    NODISCARD FORCEINLINE TArrayIterator operator-(SizeType Other) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator -= Other;
    }

    /**
     * @brief       - Add a value to the iterator and store it in this instance
     * @param Other - Value to add
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator+=(SizeType Other) noexcept
    {
        Index += Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Subtract a value to the iterator and store it in this instance
     * @param Other - Value to subtract
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator-=(SizeType Other) noexcept
    {
        Index -= Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TArrayIterator& Other) const noexcept
    {
        return (Index == Other.Index) && Other.IsFrom(Array);
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns false if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TArrayIterator<ConstArrayType, ConstElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TArrayIterator<ConstArrayType, ConstElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType                     Index;
};


template<
    typename ArrayType,
    typename ElementType>
FORCEINLINE TArrayIterator<ArrayType, ElementType> operator+(typename TArrayIterator<ArrayType, ElementType>::SizeType LHS, TArrayIterator<ArrayType, ElementType>& Other) noexcept
{
    TArrayIterator NewIterator(Other);
    return NewIterator += LHS;
}


template<
    typename ArrayType,
    typename ElementType>
class TReverseArrayIterator
{
    using ConstElementType = const typename TRemoveCV<ElementType>::Type;
    using ConstArrayType   = const typename TRemoveCV<ArrayType>::Type;

public:
    using SizeType = typename ArrayType::SizeType;

    TReverseArrayIterator(const TReverseArrayIterator&) = default;
    TReverseArrayIterator(TReverseArrayIterator&&) = default;
    ~TReverseArrayIterator() = default;

    TReverseArrayIterator& operator=(const TReverseArrayIterator&) = default;
    TReverseArrayIterator& operator=(TReverseArrayIterator&&) = default;

    static_assert(
        TIsSigned<SizeType>::Value,
        "TReverseArrayIterator wants a signed SizeType");
    static_assert(
        TIsConst<ArrayType>::Value == TIsConst<ElementType>::Value,
        "TReverseArrayIterator require ArrayType and ElementType to have the same constness");

    /**
     * @brief            - Create a new iterator
     * @param InArray    - Array to iterate
     * @param StartIndex - Index in the array to start
     */
    FORCEINLINE explicit TReverseArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        CHECK(IsValid());
    }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /**
     * @brief  - Check if the iterator is valid
     * @return - Returns true if the iterator is valid
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().GetSize());
    }

    /**
     * @brief  - Check if the iterator is equal to the end iterator
     * @return - Returns true if the iterator is the end-iterator
     */
    NODISCARD FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == 0);
    }

    /**
     * @brief  - Retrieve a raw pointer to the data
     * @return - Returns a raw pointer to the data
     */
    NODISCARD FORCEINLINE ElementType* GetPointer() const noexcept
    {
        CHECK(IsValid());
        return Array.Get().GetData() + GetIndex();
    }

    /**
     * @brief  - Retrieve the index to for the iterator in the array
     * @return - Returns the index to the element that the iterator represent within the array
     */
    NODISCARD FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index - 1;
    }

public:

    /**
     * @brief  - Retrieve a raw pointer to the data
     * @return - Returns a raw pointer to the data
     */
    NODISCARD FORCEINLINE ElementType* operator->() const noexcept
    {
        return GetPointer();
    }

    /**
     * @brief  - Retrieve the data
     * @return - Returns a reference to the data
     */
    NODISCARD FORCEINLINE ElementType& operator*() const noexcept
    {
        return *GetPointer();
    }

    /**
     * @brief  - Increment the index for the iterator
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator++() noexcept
    {
        Index--;

        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Increment the index for the iterator
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator++(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Decrement the index for the iterator
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator--() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Decrement the index for the iterator
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator--(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        NewIterator++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Add a new value to the iterator
     * @param Other - Value to add
     * @return      - Returns a new iterator with the result from adding Other to this value
     */
    NODISCARD FORCEINLINE TReverseArrayIterator operator+(SizeType Other) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator += Other; // Uses operator, therefore +=
    }

    /**
     * @brief       - Subtract a new value to the iterator
     * @param Other - Value to subtract
     * @return      - Returns a new iterator with the result from subtracting Other to this value
     */
    NODISCARD FORCEINLINE TReverseArrayIterator operator-(SizeType Other) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator -= Other; // Uses operator, therefore -=
    }

    /**
     * @brief       - Add a value to the iterator and store it in this instance
     * @param Other - Value to add
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator+=(SizeType Other) noexcept
    {
        Index -= Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Subtract a value to the iterator and store it in this instance
     * @param Other - Value to subtract
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator-=(SizeType Other) noexcept
    {
        Index += Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TReverseArrayIterator& Other) const noexcept
    {
        return (Index == Other.Index) && Other.IsFrom(Array);
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns false if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TReverseArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TReverseArrayIterator<ConstArrayType, ConstElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TReverseArrayIterator<ConstArrayType, ConstElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType                     Index;
};


template<
    typename ArrayType,
    typename ElementType>
NODISCARD FORCEINLINE TReverseArrayIterator<ArrayType, ElementType> operator+(
    typename TReverseArrayIterator<ArrayType, ElementType>::SizeType LHS,
    TReverseArrayIterator<ArrayType, ElementType>& Other) noexcept
{
    TReverseArrayIterator NewIterator(Other);
    return NewIterator += LHS;
}


#if 0
template<typename NodeType, typename ElementType>
class TTreeIterator
{
public:
    using SizeType = int32;

    TTreeIterator(const TTreeIterator&) = default;
    TTreeIterator(TTreeIterator&&) = default;
    ~TTreeIterator() = default;

    TTreeIterator& operator=(const TTreeIterator&) = default;
    TTreeIterator& operator=(TTreeIterator&&) = default;

    FORCEINLINE TTreeIterator(NodeType* InNode) noexcept
        : Node(InNode)
    {
        CHECK(IsValid());
    }

    FORCEINLINE bool IsValid() const noexcept
    {
        return (Node != nullptr) && (Node->GetPointer() != nullptr);
    }

    FORCEINLINE ElementType* GetPointer() const noexcept
    {
        CHECK(IsValid());
        return Node->GetPointer();
    }

    FORCEINLINE ElementType* operator->() const noexcept
    {
        return GetPointer();
    }

    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *GetPointer();
    }

    FORCEINLINE TTreeIterator operator++() noexcept
    {
        CHECK(IsValid());

        Node = Node->GetNext();
        return *this;
    }

    FORCEINLINE TTreeIterator operator++(int) noexcept
    {
        TTreeIterator NewIterator(*this);
        Node = Node->GetNext();

        CHECK(IsValid());
        return NewIterator;
    }

    FORCEINLINE TTreeIterator operator--() noexcept
    {
        CHECK(IsValid());

        Node = Node->GetPrevious();
        return *this;
    }

    FORCEINLINE TTreeIterator operator--(int) noexcept
    {
        TTreeIterator NewIterator(*this);
        Node = Node->GetPrevious();

        CHECK(IsValid());
        return NewIterator;
    }

    FORCEINLINE bool operator==(const TTreeIterator& Other) const noexcept
    {
        return (Node == Other.Node);
    }

    FORCEINLINE bool operator!=(const TTreeIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    FORCEINLINE operator TTreeIterator<const NodeType, const ElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TTreeIterator<const NodeType, const ElementType>(Node);
    }

private:
    NodeType* Node;
};
#endif


template<typename BitArrayType, typename StorageType>
class TBitArrayIterator
{
    using ConstStorageType  = const typename TRemoveCV<StorageType>::Type;
    using ConstBitArrayType = const typename TRemoveCV<BitArrayType>::Type;

public:
    using BitReferenceType      = TBitReference<StorageType>;
    using ConstBitReferenceType = TBitReference<ConstStorageType>;

    TBitArrayIterator(const TBitArrayIterator&) = default;
    TBitArrayIterator(TBitArrayIterator&&) = default;
    ~TBitArrayIterator() = default;

    TBitArrayIterator& operator=(const TBitArrayIterator&) = default;
    TBitArrayIterator& operator=(TBitArrayIterator&&) = default;

    /**
     * @brief            - Constructor taking array and index of the iterator
     * @param InBitArray - Array that the iterator belongs to
     * @param InIndex    - Index of the bit inside the BitArray
     */
    FORCEINLINE explicit TBitArrayIterator(const BitArrayType& InBitArray, uint32 InIndex) noexcept
        : Index(InIndex)
        , BitArray(InBitArray)
    { }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const BitArrayType& FromArray) const noexcept
    {
        const BitArrayType* FromPointer = AddressOf(FromArray);
        return BitArray.AddressOf() == FromPointer;
    }

    /**
     * @brief  - Check if the iterator is valid
     * @return - Returns true if the iterator is valid
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        const auto Count = BitArray.Get().Count();
        return (Index >= 0) && (Index <= Count);
    }

    /**
     * @brief  - Retrieve the value of the bit
     * @return - Returns the value of the bit
     */
    NODISCARD FORCEINLINE BitReferenceType GetBitValue() noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

    /**
     * @brief  - Retrieve the value of the bit
     * @return - Returns the value of the bit
     */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitValue() const noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

public:

    /**
     * @brief  - Pre-increment operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TBitArrayIterator operator++() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Post-increment operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TBitArrayIterator operator++(int) noexcept
    {
        TBitArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TBitArrayIterator operator--() noexcept
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TBitArrayIterator operator--(int) noexcept
    {
        TBitArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns true if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator==(const TBitArrayIterator& Other) const noexcept
    {
        return (Index == Other.Index) && BitArray.IsFrom(Other.BitArray);
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns false if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator!=(const TBitArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Retrieve the data
     * @return - Returns a reference to the data
     */
    NODISCARD FORCEINLINE BitReferenceType& operator*() noexcept
    {
        return GetBitValue();
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TBitArrayIterator<ConstBitArrayType, ConstStorageType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TBitArrayIterator<ConstBitArrayType, ConstStorageType>(BitArray, Index);
    }

private:
    TReferenceWrapper<BitArrayType> BitArray;
    uint32                          Index;
};


template<typename BitArrayType, typename StorageType>
class TReverseBitArrayIterator
{
    using ConstStorageType  = const typename TRemoveCV<StorageType>::Type;
    using ConstBitArrayType = const typename TRemoveCV<BitArrayType>::Type;

public:
    using BitReferenceType      = TBitReference<StorageType>;
    using ConstBitReferenceType = TBitReference<ConstStorageType>;

    TReverseBitArrayIterator(const TReverseBitArrayIterator&) = default;
    TReverseBitArrayIterator(TReverseBitArrayIterator&&) = default;
    ~TReverseBitArrayIterator() = default;
    TReverseBitArrayIterator& operator=(const TReverseBitArrayIterator&) = default;
    TReverseBitArrayIterator& operator=(TReverseBitArrayIterator&&) = default;

    /**
     * @brief            - Constructor taking array and index of the iterator
     * @param InBitArray - Array that the iterator belongs to
     * @param InIndex    - Index of the bit inside the BitArray
     */
    FORCEINLINE explicit TReverseBitArrayIterator(const BitArrayType& InBitArray, uint32 InIndex) noexcept
        : Index(InIndex)
        , BitArray(InBitArray)
    { }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const BitArrayType& FromArray) const noexcept
    {
        const BitArrayType* FromPointer = AddressOf(FromArray);
        return BitArray.AddressOf() == FromPointer;
    }

    /**
     * @brief  - Check if the iterator is valid
     * @return - Returns true if the iterator is valid
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        const auto Count = BitArray.Get().Count();
        return (Index >= 0) && (Index <= Count);
    }

    /**
     * @brief  - Retrieve the value of the bit
     * @return - Returns the value of the bit
     */
    NODISCARD FORCEINLINE BitReferenceType GetBitValue() noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

    /**
     * @brief  - Retrieve the value of the bit
     * @return - Returns the value of the bit
     */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitValue() const noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

public:

    /**
     * @brief  - Pre-increment operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TReverseBitArrayIterator operator++() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Post-increment operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TReverseBitArrayIterator operator++(int) noexcept
    {
        TReverseBitArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TReverseBitArrayIterator operator--() noexcept
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TReverseBitArrayIterator operator--(int) noexcept
    {
        TReverseBitArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns true if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator==(const TReverseBitArrayIterator& Other) const noexcept
    {
        return (Index == Other.Index) && BitArray.IsFrom(Other.BitArray);
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns false if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator!=(const TReverseBitArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Retrieve the data
     * @return - Returns a reference to the data
     */
    NODISCARD FORCEINLINE BitReferenceType& operator*() noexcept
    {
        return GetBitValue();
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TReverseBitArrayIterator<ConstBitArrayType, ConstStorageType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TReverseBitArrayIterator<ConstBitArrayType, ConstStorageType>(BitArray, Index);
    }

private:
    TReferenceWrapper<BitArrayType> BitArray;
    uint32                          Index;
};
