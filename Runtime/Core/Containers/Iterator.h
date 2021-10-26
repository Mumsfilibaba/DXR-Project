#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Templates/IsSigned.h"
#include "Core/Templates/AddressOf.h"
#include "Core/Templates/ReferenceWrapper.h"

/* Iterator for array types */
template<typename ArrayType, typename ElementType>
class TArrayIterator
{
public:

    using SizeType = typename ArrayType::SizeType;

    TArrayIterator( const TArrayIterator& ) = default;
    TArrayIterator( TArrayIterator&& ) = default;
    ~TArrayIterator() = default;
    TArrayIterator& operator=( const TArrayIterator& ) = default;
    TArrayIterator& operator=( TArrayIterator&& ) = default;

    static_assert(TIsSigned<SizeType>::Value, "TArrayIterator wants a signed SizeType");
    static_assert(TIsConst<ArrayType>::Value == TIsConst<ElementType>::Value, "TArrayIterator require ArrayType and ElementType to have the same constness");

    /* Constructor creating a new iterator by taking in the array and pointer */
    FORCEINLINE TArrayIterator( ArrayType& InArray, SizeType StartIndex ) noexcept
        : Array( InArray )
        , Index( StartIndex )
    {
        Assert( IsValid() );
    }

    /* Checks if the iterator comes from the specified array */
    FORCEINLINE bool IsFrom( const ArrayType& FromArray ) const noexcept
    {
        const ArrayType* FromPointer = AddressOf( FromArray );
        return Array.AddressOf() == FromPointer;
    }

    /* Ensure that the pointer is in the range of the array */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().Size());
    }

    /* Ensure that the pointer is not the endpointer */
    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == Array.Get().Size());
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert( IsValid() );
        return Array.Get().Data() + GetIndex();
    }

    /* Retrieve the usable index */
    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index;
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    /* Dereference the pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    /* Pre-increment the iterator */
    FORCEINLINE TArrayIterator operator++() noexcept
    {
        Index++;

        Assert( IsValid() );
        return *this;
    }

    /* Post-increment the iterator */
    FORCEINLINE TArrayIterator operator++( int ) noexcept
    {
        TArrayIterator NewIterator( *this );
        Index++;

        Assert( IsValid() );
        return NewIterator;
    }

    /* Pre-decrement the iterator */
    FORCEINLINE TArrayIterator operator--() noexcept
    {
        Index--;

        Assert( IsValid() );
        return *this;
    }

    /* Post-decrement the iterator */
    FORCEINLINE TArrayIterator operator--( int ) noexcept
    {
        TArrayIterator NewIterator( *this );
        Index--;

        Assert( IsValid() );
        return NewIterator;
    }

    /* Add offset to iterator and return a new */
    FORCEINLINE TArrayIterator operator+( SizeType RHS ) const noexcept
    {
        TArrayIterator NewIterator( *this );
        return NewIterator += RHS;
    }

    /* Subtract offset from iterator and return a new */
    FORCEINLINE TArrayIterator operator-( SizeType RHS ) const noexcept
    {
        TArrayIterator NewIterator( *this );
        return NewIterator -= RHS;
    }

    /* Add offset to iterator */
    FORCEINLINE TArrayIterator& operator+=( SizeType RHS ) noexcept
    {
        Index += RHS;

        Assert( IsValid() );
        return *this;
    }

    /* Subtract offset from iterator */
    FORCEINLINE TArrayIterator& operator-=( SizeType RHS ) noexcept
    {
        Index -= RHS;

        Assert( IsValid() );
        return *this;
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator==( const TArrayIterator& RHS ) const noexcept
    {
        return (Index == RHS.Index) && RHS.IsFrom( Array );
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator!=( const TArrayIterator& RHS ) const noexcept
    {
        return !(*this == RHS);
    }

    /* Convert into a const iterator */
    FORCEINLINE operator TArrayIterator<const ArrayType, const ElementType>() const noexcept
    {
        /* The array type must be const here in order to make the dereference work properly */
        return TArrayIterator<const ArrayType, const ElementType>( Array, Index );
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType Index;
};

/* Add offset to iterator and return a new */
template<typename ArrayType, typename ElementType>
FORCEINLINE TArrayIterator<ArrayType, ElementType> operator+( typename TArrayIterator<ArrayType, ElementType>::SizeType LHS, const TArrayIterator<ArrayType, ElementType>& RHS ) noexcept
{
    TArrayIterator NewIterator( RHS );
    return NewIterator += LHS;
}

/* Reverse array iterator */
template<typename ArrayType, typename ElementType>
class TReverseArrayIterator
{
public:

    using SizeType = typename ArrayType::SizeType;

    TReverseArrayIterator( const TReverseArrayIterator& ) = default;
    TReverseArrayIterator( TReverseArrayIterator&& ) = default;
    ~TReverseArrayIterator() = default;
    TReverseArrayIterator& operator=( const TReverseArrayIterator& ) = default;
    TReverseArrayIterator& operator=( TReverseArrayIterator&& ) = default;

    static_assert(TIsSigned<SizeType>::Value, "TReverseArrayIterator wants a signed SizeType");
    static_assert(TIsConst<ArrayType>::Value == TIsConst<ElementType>::Value, "TReverseArrayIterator require ArrayType and ElementType to have the same constness");

    /* Constructor creating a new iterator by taking in the array and pointer */
    FORCEINLINE TReverseArrayIterator( ArrayType& InArray, SizeType StartIndex ) noexcept
        : Array( InArray )
        , Index( StartIndex )
    {
        Assert( IsValid() );
    }

    /* Checks if the iterator comes from the specified array */
    FORCEINLINE bool IsFrom( const ArrayType& FromArray ) const noexcept
    {
        const ArrayType* FromPointer = AddressOf( FromArray );
        return Array.AddressOf() == FromPointer;
    }

    /* Ensure that the pointer is in the range of the array */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Get().Size());
    }

    /* Ensure that the pointer is not the end pointer */
    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == 0);
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert( IsValid() );
        return Array.Get().Data() + GetIndex();
    }

    /* Retrieve the usable index */
    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index - 1;
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    /* Dereference the raw pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    /* Pre-increment the iterator */
    FORCEINLINE TReverseArrayIterator operator++() noexcept
    {
        Index--;

        Assert( IsValid() );
        return *this;
    }

    /* Post-increment the iterator  */
    FORCEINLINE TReverseArrayIterator operator++( int ) noexcept
    {
        TReverseArrayIterator NewIterator( *this );
        Index--;

        Assert( IsValid() );
        return NewIterator;
    }

    /* Pre-decrement the iterator */
    FORCEINLINE TReverseArrayIterator operator--() noexcept
    {
        Index++;

        Assert( IsValid() );
        return *this;
    }

    /* Post-decrement the iterator */
    FORCEINLINE TReverseArrayIterator operator--( int ) noexcept
    {
        TReverseArrayIterator NewIterator( *this );
        NewIterator++;

        Assert( IsValid() );
        return NewIterator;
    }

    /* Add offset to iterator and return a new */
    FORCEINLINE TReverseArrayIterator operator+( SizeType RHS ) const noexcept
    {
        TReverseArrayIterator NewIterator( *this );
        return NewIterator += RHS; // Uses operator, therefore +=
    }

    /* Subtract offset from iterator and return a new */
    FORCEINLINE TReverseArrayIterator operator-( SizeType RHS ) const noexcept
    {
        TReverseArrayIterator NewIterator( *this );
        return NewIterator -= RHS; // Uses operator, therefore -=
    }

    /* Add offset to iterator */
    FORCEINLINE TReverseArrayIterator& operator+=( SizeType RHS ) noexcept
    {
        Index -= RHS;

        Assert( IsValid() );
        return *this;
    }

    /* Subtract offset from iterator */
    FORCEINLINE TReverseArrayIterator& operator-=( SizeType RHS ) noexcept
    {
        Index += RHS;

        Assert( IsValid() );
        return *this;
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator==( const TReverseArrayIterator& RHS ) const noexcept
    {
        return (Index == RHS.Index) && RHS.IsFrom( Array );
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator!=( const TReverseArrayIterator& RHS ) const noexcept
    {
        return !(*this == RHS);
    }

    /* Convert into a const iterator */
    FORCEINLINE operator TReverseArrayIterator<const ArrayType, const ElementType>() const noexcept
    {
        /* The array type must be const here in order to make the dereference work properly */
        return TReverseArrayIterator<const ArrayType, const ElementType>( Array, Index );
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType Index;
};

/* Add offset to iterator and return a new */
template<typename ArrayType, typename ElementType>
FORCEINLINE TReverseArrayIterator<ArrayType, ElementType> operator+( typename TReverseArrayIterator<ArrayType, ElementType>::SizeType LHS, const TReverseArrayIterator<ArrayType, ElementType>& RHS ) noexcept
{
    TReverseArrayIterator NewIterator( RHS );
    return NewIterator += LHS;
}

/* Iterator for tree-structures such as TSet */
template<typename NodeType, typename ElementType>
class TTreeIterator
{
public:

    using SizeType = int32;

    TTreeIterator( const TTreeIterator& ) = default;
    TTreeIterator( TTreeIterator&& ) = default;
    ~TTreeIterator() = default;
    TTreeIterator& operator=( const TTreeIterator& ) = default;
    TTreeIterator& operator=( TTreeIterator&& ) = default;

    /* Constructor creating a new iterator by taking in the array and pointer */
    FORCEINLINE TTreeIterator( NodeType* InNode ) noexcept
        : Node( InNode )
    {
        Assert( IsValid() );
    }

    /* Ensure that the pointer is in the range of the array */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Node != nullptr) && (Node->GetPointer() != nullptr);
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert( IsValid() );
        return Node->GetPointer();
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    /* Dereference the raw pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    /* Pre-increment the iterator */
    FORCEINLINE TTreeIterator operator++() noexcept
    {
        Assert( IsValid() );

        Node = Node->GetNext();
        return *this;
    }

    /* Post-increment the iterator */
    FORCEINLINE TTreeIterator operator++( int ) noexcept
    {
        TTreeIterator NewIterator( *this );
        Node = Node->GetNext();

        Assert( IsValid() );
        return NewIterator;
    }

    /* Pre-decrement the iterator */
    FORCEINLINE TTreeIterator operator--() noexcept
    {
        Assert( IsValid() );

        Node = Node->GetPrevious();
        return *this;
    }

    /* Post-decrement the iterator */
    FORCEINLINE TTreeIterator operator--( int ) noexcept
    {
        TTreeIterator NewIterator( *this );
        Node = Node->GetPrevious();

        Assert( IsValid() );
        return NewIterator;
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator==( const TTreeIterator& RHS ) const noexcept
    {
        return (Node == RHS.Node);
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator!=( const TTreeIterator& RHS ) const noexcept
    {
        return !(*this == RHS);
    }

    /* Convert into a const iterator */
    FORCEINLINE operator TTreeIterator<const NodeType, const ElementType>() const noexcept
    {
        /* The array type must be const here in order to make the dereference work properly */
        return TTreeIterator<const NodeType, const ElementType>( Node );
    }

private:
    NodeType* Node;
};
