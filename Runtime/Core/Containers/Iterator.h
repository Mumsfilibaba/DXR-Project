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

    FORCEINLINE TArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        Assert(IsValid());
    }

    FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().Size());
    }

    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == Array.Get().Size());
    }

    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert(IsValid());
        return Array.Get().Data() + GetIndex();
    }

    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index;
    }

    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    FORCEINLINE TArrayIterator operator++() noexcept
    {
        Index++;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE TArrayIterator operator++(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index++;

        Assert(IsValid());
        return NewIterator;
    }

    FORCEINLINE TArrayIterator operator--() noexcept
    {
        Index--;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE TArrayIterator operator--(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index--;

        Assert(IsValid());
        return NewIterator;
    }

    FORCEINLINE TArrayIterator operator+(SizeType RHS) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator += RHS;
    }

    FORCEINLINE TArrayIterator operator-(SizeType RHS) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator -= RHS;
    }

    FORCEINLINE TArrayIterator& operator+=(SizeType RHS) noexcept
    {
        Index += RHS;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE TArrayIterator& operator-=(SizeType RHS) noexcept
    {
        Index -= RHS;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE bool operator==(const TArrayIterator& RHS) const noexcept
    {
        return (Index == RHS.Index) && RHS.IsFrom(Array);
    }

    FORCEINLINE bool operator!=(const TArrayIterator& RHS) const noexcept
    {
        return !(*this == RHS);
    }

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

    FORCEINLINE TReverseArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        Assert(IsValid());
    }

    FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().Size());
    }

    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == 0);
    }

    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert(IsValid());
        return Array.Get().Data() + GetIndex();
    }

    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index - 1;
    }

    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    FORCEINLINE TReverseArrayIterator operator++() noexcept
    {
        Index--;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE TReverseArrayIterator operator++(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        Index--;

        Assert(IsValid());
        return NewIterator;
    }

    FORCEINLINE TReverseArrayIterator operator--() noexcept
    {
        Index++;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE TReverseArrayIterator operator--(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        NewIterator++;

        Assert(IsValid());
        return NewIterator;
    }

    FORCEINLINE TReverseArrayIterator operator+(SizeType RHS) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator += RHS; // Uses operator, therefore +=
    }

    FORCEINLINE TReverseArrayIterator operator-(SizeType RHS) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator -= RHS; // Uses operator, therefore -=
    }

    FORCEINLINE TReverseArrayIterator& operator+=(SizeType RHS) noexcept
    {
        Index -= RHS;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE TReverseArrayIterator& operator-=(SizeType RHS) noexcept
    {
        Index += RHS;

        Assert(IsValid());
        return *this;
    }

    FORCEINLINE bool operator==(const TReverseArrayIterator& RHS) const noexcept
    {
        return (Index == RHS.Index) && RHS.IsFrom(Array);
    }

    FORCEINLINE bool operator!=(const TReverseArrayIterator& RHS) const noexcept
    {
        return !(*this == RHS);
    }

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
