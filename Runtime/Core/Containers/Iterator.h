#pragma once
#include "Core/CoreTypes.h"
#include "Core/CoreDefines.h"

#include "Core/Templates/IsSigned.h"
#include "Core/Templates/AddressOf.h"
#include "Core/Templates/ReferenceWrapper.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Iterator for array types

template<typename ArrayType, typename ElementType>
class TArrayIterator
{
public:

    using SizeType = typename ArrayType::SizeType;

    TArrayIterator(const TArrayIterator&) = default;
    TArrayIterator(TArrayIterator&&) = default;
    ~TArrayIterator() = default;
    TArrayIterator& operator=(const TArrayIterator&) = default;
    TArrayIterator& operator=(TArrayIterator&&) = default;

    static_assert(TIsSigned<SizeType>::Value, "TArrayIterator wants a signed SizeType");
    static_assert(TIsConst<ArrayType>::Value == TIsConst<ElementType>::Value, "TArrayIterator require ArrayType and ElementType to have the same constness");

    /**
     * Create a new iterator
     * 
     * @param InArray: Array to iterate
     * @param StartIndex: Index in the array to start
     */
    FORCEINLINE TArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        Assert(IsValid());
    }

    /**
     * Check if the iterator belongs to a certain array
     * 
     * @param FromArray: Array to check
     * @return: Returns true if the iterator is from the array, otherwise false 
     */
    FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /**
     * Check if the iterator is valid
     * 
     * @return: Returns true if the iterator is valid
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().Size());
    }

    /**
     * Check if the iterator is equal to the end iterator
     * 
     * @return: Returns true if the iterator is the end-iterator
     */
    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == Array.Get().Size());
    }

    /**
     * Retrieve a raw pointer to the data
     * 
     * @return: Returns a raw pointer to the data 
     */
    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert(IsValid());
        return Array.Get().Data() + GetIndex();
    }

    /**
     * Retrieve the index to for the iterator in the array
     * 
     * @return: Returns the index to the element that the iterator represent within the array   
     */
    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index;
    }

    /**
     * Retrieve a raw pointer to the data
     * 
     * @return: Returns a raw pointer to the data 
     */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    /**
     * Retrieve the data
     * 
     * @return: Returns a reference to the data 
     */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    /**
     * Increment the index for the iterator  
     * 
     * @return: Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator++() noexcept
    {
        Index++;

        Assert(IsValid());
        return *this;
    }

    /**
     * Increment the index for the iterator  
     * 
     * @return: Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator++(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index++;

        Assert(IsValid());
        return NewIterator;
    }

    /**
     * Decrement the index for the iterator  
     * 
     * @return: Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator--() noexcept
    {
        Index--;

        Assert(IsValid());
        return *this;
    }

    /**
     * Decrement the index for the iterator  
     * 
     * @return: Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator--(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index--;

        Assert(IsValid());
        return NewIterator;
    }

    /**
     * Add a new value to the iterator
     * 
     * @param RHS: Value to add
     * @return: Returns a new iterator with the result from adding RHS to this value 
     */
    FORCEINLINE TArrayIterator operator+(SizeType RHS) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator += RHS;
    }

    /**
     * Subtract a new value to the iterator
     * 
     * @param RHS: Value to subtract
     * @return: Returns a new iterator with the result from subtracting RHS to this value 
     */
    FORCEINLINE TArrayIterator operator-(SizeType RHS) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator -= RHS;
    }

    /**
     * Add a value to the iterator and store it in this instance
     * 
     * @param RHS: Value to add
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator+=(SizeType RHS) noexcept
    {
        Index += RHS;

        Assert(IsValid());
        return *this;
    }

    /**
     * Subtract a value to the iterator and store it in this instance
     * 
     * @param RHS: Value to subtract
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator-=(SizeType RHS) noexcept
    {
        Index -= RHS;

        Assert(IsValid());
        return *this;
    }

    /**
     * Compare this and another instance
     * 
     * @param RHS: Value to compare with
     * @return: Returns true if the iterators are equal
     */
    FORCEINLINE bool operator==(const TArrayIterator& RHS) const noexcept
    {
        return (Index == RHS.Index) && RHS.IsFrom(Array);
    }

    /**
     * Compare this and another instance
     * 
     * @param RHS: Value to compare with
     * @return: Returns false if the iterators are equal
     */
    FORCEINLINE bool operator!=(const TArrayIterator& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    /**
     * Create a constant iterator from this
     * 
     * @return: Returns a new iterator based on the index from this instance
     */
    FORCEINLINE operator TArrayIterator<const ArrayType, const ElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TArrayIterator<const ArrayType, const ElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add offset to iterator and return a new

template<typename ArrayType, typename ElementType>
FORCEINLINE TArrayIterator<ArrayType, ElementType> operator+(typename TArrayIterator<ArrayType, ElementType>::SizeType LHS, const TArrayIterator<ArrayType, ElementType>& RHS) noexcept
{
    TArrayIterator NewIterator(RHS);
    return NewIterator += LHS;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Reverse array iterator

template<typename ArrayType, typename ElementType>
class TReverseArrayIterator
{
public:

    using SizeType = typename ArrayType::SizeType;

    TReverseArrayIterator(const TReverseArrayIterator&) = default;
    TReverseArrayIterator(TReverseArrayIterator&&) = default;
    ~TReverseArrayIterator() = default;
    TReverseArrayIterator& operator=(const TReverseArrayIterator&) = default;
    TReverseArrayIterator& operator=(TReverseArrayIterator&&) = default;

    static_assert(TIsSigned<SizeType>::Value, "TReverseArrayIterator wants a signed SizeType");
    static_assert(TIsConst<ArrayType>::Value == TIsConst<ElementType>::Value, "TReverseArrayIterator require ArrayType and ElementType to have the same constness");

    /**
     * Create a new iterator
     *
     * @param InArray: Array to iterate
     * @param StartIndex: Index in the array to start
     */
    FORCEINLINE TReverseArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        Assert(IsValid());
    }

    /**
     * Check if the iterator belongs to a certain array
     *
     * @param FromArray: Array to check
     * @return: Returns true if the iterator is from the array, otherwise false
     */
    FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /**
     * Check if the iterator is valid
     *
     * @return: Returns true if the iterator is valid
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().Size());
    }

    /**
     * Check if the iterator is equal to the end iterator
     *
     * @return: Returns true if the iterator is the end-iterator
     */
    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == 0);
    }

    /**
     * Retrieve a raw pointer to the data
     *
     * @return: Returns a raw pointer to the data
     */
    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert(IsValid());
        return Array.Get().Data() + GetIndex();
    }

    /**
     * Retrieve the index to for the iterator in the array
     *
     * @return: Returns the index to the element that the iterator represent within the array
     */
    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index - 1;
    }

    /**
     * Retrieve a raw pointer to the data
     *
     * @return: Returns a raw pointer to the data
     */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    /**
     * Retrieve the data
     *
     * @return: Returns a reference to the data
     */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    /**
     * Increment the index for the iterator
     *
     * @return: Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator++() noexcept
    {
        Index--;

        Assert(IsValid());
        return *this;
    }

    /**
     * Increment the index for the iterator
     *
     * @return: Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator++(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        Index--;

        Assert(IsValid());
        return NewIterator;
    }

    /**
     * Decrement the index for the iterator
     *
     * @return: Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator--() noexcept
    {
        Index++;

        Assert(IsValid());
        return *this;
    }

    /**
     * Decrement the index for the iterator
     *
     * @return: Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator--(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        NewIterator++;

        Assert(IsValid());
        return NewIterator;
    }

    /**
     * Add a new value to the iterator
     *
     * @param RHS: Value to add
     * @return: Returns a new iterator with the result from adding RHS to this value
     */
    FORCEINLINE TReverseArrayIterator operator+(SizeType RHS) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator += RHS; // Uses operator, therefore +=
    }

    /**
     * Subtract a new value to the iterator
     *
     * @param RHS: Value to subtract
     * @return: Returns a new iterator with the result from subtracting RHS to this value
     */
    FORCEINLINE TReverseArrayIterator operator-(SizeType RHS) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator -= RHS; // Uses operator, therefore -=
    }

    /**
     * Add a value to the iterator and store it in this instance
     *
     * @param RHS: Value to add
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator+=(SizeType RHS) noexcept
    {
        Index -= RHS;

        Assert(IsValid());
        return *this;
    }

    /**
     * Subtract a value to the iterator and store it in this instance
     *
     * @param RHS: Value to subtract
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator-=(SizeType RHS) noexcept
    {
        Index += RHS;

        Assert(IsValid());
        return *this;
    }

    /**
     * Compare this and another instance
     *
     * @param RHS: Value to compare with
     * @return: Returns true if the iterators are equal
     */
    FORCEINLINE bool operator==(const TReverseArrayIterator& RHS) const noexcept
    {
        return (Index == RHS.Index) && RHS.IsFrom(Array);
    }

    /**
     * Compare this and another instance
     *
     * @param RHS: Value to compare with
     * @return: Returns false if the iterators are equal
     */
    FORCEINLINE bool operator!=(const TReverseArrayIterator& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    /**
     * Create a constant iterator from this
     *
     * @return: Returns a new iterator based on the index from this instance
     */
    FORCEINLINE operator TReverseArrayIterator<const ArrayType, const ElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TReverseArrayIterator<const ArrayType, const ElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add offset to iterator and return a new

template<typename ArrayType, typename ElementType>
FORCEINLINE TReverseArrayIterator<ArrayType, ElementType> operator+(typename TReverseArrayIterator<ArrayType, ElementType>::SizeType LHS, const TReverseArrayIterator<ArrayType, ElementType>& RHS) noexcept
{
    TReverseArrayIterator NewIterator(RHS);
    return NewIterator += LHS;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Iterator for tree-structures such as TSet

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
        Assert(IsValid());
    }

    FORCEINLINE bool IsValid() const noexcept
    {
        return (Node != nullptr) && (Node->GetPointer() != nullptr);
    }

    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert(IsValid());
        return Node->GetPointer();
    }

    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    FORCEINLINE TTreeIterator operator++() noexcept
    {
        Assert(IsValid());

        Node = Node->GetNext();
        return *this;
    }

    FORCEINLINE TTreeIterator operator++(int) noexcept
    {
        TTreeIterator NewIterator(*this);
        Node = Node->GetNext();

        Assert(IsValid());
        return NewIterator;
    }

    FORCEINLINE TTreeIterator operator--() noexcept
    {
        Assert(IsValid());

        Node = Node->GetPrevious();
        return *this;
    }

    FORCEINLINE TTreeIterator operator--(int) noexcept
    {
        TTreeIterator NewIterator(*this);
        Node = Node->GetPrevious();

        Assert(IsValid());
        return NewIterator;
    }

    FORCEINLINE bool operator==(const TTreeIterator& RHS) const noexcept
    {
        return (Node == RHS.Node);
    }

    FORCEINLINE bool operator!=(const TTreeIterator& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    FORCEINLINE operator TTreeIterator<const NodeType, const ElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TTreeIterator<const NodeType, const ElementType>(Node);
    }

private:
    NodeType* Node;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Iterator for BitArrays

template<typename BitFieldType>
class TBitArrayIterator
{
public:

    enum
    {
        Invalid = ~0
    };

    TBitArrayIterator(const TBitArrayIterator&) = default;
    TBitArrayIterator(TBitArrayIterator&&) = default;
    ~TBitArrayIterator() = default;
    TBitArrayIterator& operator=(const TBitArrayIterator&) = default;
    TBitArrayIterator& operator=(TBitArrayIterator&&) = default;

    explicit TBitArrayIterator(const BitFieldType& InBitField, uint32 InIndex)
        : Index(InIndex)
        , BitField(InBitField)
    {
    }

    FORCEINLINE void operator++()
    {
        while (++Index < BitFieldType::Capacity())
        {
            if (BitField.Get().GetBit(Index))
            {
                return;
            }
        }

        Index = Invalid;
    }

    FORCEINLINE bool operator!=(const TBitArrayIterator& other)
    {
        return (Index != other.Index);
    }

    FORCEINLINE bool Valid() const
    {
        return Index < BitFieldType::Capacity();
    }

    FORCEINLINE uint32 Value() const
    {
        return Index;
    }

    FORCEINLINE uint32 operator*() const
    {
        return Index;
    }

private:
    TReferenceWrapper<BitFieldType> BitField;
    uint32 Index;
};
