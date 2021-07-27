#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Templates/IsSigned.h"
#include "Core/Templates/AddressOf.h"

/* Iterator for array types */
template<typename ArrayType, typename ElementType>
class TArrayIterator
{
public:
    typedef ArrayType::SizeType SizeType;

    TArrayIterator(const TArrayIterator&) = default;
    TArrayIterator(TArrayIterator&&) = default;
    ~TArrayIterator() = default;
    TArrayIterator& operator=(const TArrayIterator&) = default;
    TArrayIterator& operator=(TArrayIterator&&) = default;

    static_assert(TIsSigned<SizeType>::Value, "TArrayIterator wants a signed SizeType");

    /* Constructor creating a new iterator by taking in the array and pointer */
    FORCEINLINE TArrayIterator(const ArrayType& InArray, SizeType StartIndex) noexcept
        : Array( InArray )
        , Index( StartIndex )
    {
        Assert(IsValid());
    }

    /* Ensure that the pointer is in the range of the array */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Size()); 
    }

    /* Ensure that the pointer is not the endpointer */
    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == Array.Size()); 
    }

    /* Retrive the raw pointer */
    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert(IsValid());
        return Array.Data() + GetIndex();
    }

    /* Retrive the useable index */
    FORCEINLINE GetIndex() const noexcept
    {
        return Index;
    }

    /* Retrive the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    /* Dereference the pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    /* Increment the iterator */
    FORCEINLINE TArrayIterator operator++() noexcept
    {
        Index++;
        Assert( IsValid() );
        return *this;
    }

    /* Increment the iterator */
    FORCEINLINE TArrayIterator operator++( int ) noexcept
    {
        TArrayIterator Temp = *this;
        Index++;
        Assert( IsValid() );
        return Temp;
    }

    /* Decrement the iterator */
    FORCEINLINE TArrayIterator operator--() noexcept
    {
        Index--;
        Assert( IsValid() );
        return *this;
    }

    /* Decrement the iterator */
    FORCEINLINE TArrayIterator operator--( int ) noexcept
    {
        TArrayIterator Temp = *this;
        Index--;
        Assert( IsValid() );
        return Temp;
    }

    /* Add offset to iterator and return a new */
    FORCEINLINE TArrayIterator operator+( SizeType RHS ) const noexcept
    {
        TArrayIterator Temp = *this;
        return Temp += RHS;
    }

    /* Add offset to iterator and return a new */
    friend FORCEINLINE TArrayIterator operator+( SizeType LHS, const TArrayIterator& RHS) const noexcept
    {
        TArrayIterator Temp = RHS;
        return Temp += LHS;
    }

    /* Subtract offset from iterator and return a new */
    FORCEINLINE TArrayIterator operator-( SizeType RHS ) const noexcept
    {
        TArrayIterator Temp = *this;
        return Temp -= RHS;
    }

    /* Subtract offset to iterator and return a new */
    friend FORCEINLINE TArrayIterator operator-( SizeType LHS, const TArrayIterator& RHS) const noexcept
    {
        TArrayIterator Temp = RHS;
        return Temp -= LHS;
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
        return (Index == RHS.Index) && (AddressOf(Array) == AddressOf(RHS.Array));
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator!=( const TArrayIterator& RHS ) const noexcept
    {
        return (*this == RHS);
    }

    /* Convert into a const iterator */
    FORCEINLINE operator TArrayIterator<const ElementType>() const noexcept
    {
        /* The arraytype must be const here in order to make the dereference work properly */
        return TArrayIterator<const ArrayType, const ElementType>( Array, Index );
    }

private:
    ArrayType& Array;
    SizeType Index;
};

/* Reverse array iterator */
template<typename ArrayType, typename ElementType>
class TReverseArrayIterator
{
public:
    typedef ArrayType::SizeType SizeType;

    TReverseArrayIterator(const TReverseArrayIterator&) = default;
    TReverseArrayIterator(TReverseArrayIterator&&) = default;
    ~TReverseArrayIterator() = default;
    TReverseArrayIterator& operator=(const TReverseArrayIterator&) = default;
    TReverseArrayIterator& operator=(TReverseArrayIterator&&) = default;

    static_assert(TIsSigned<SizeType>::Value, "TReverseArrayIterator wants a signed SizeType");

    /* Constructor creating a new iterator by taking in the array and pointer */
    FORCEINLINE TReverseArrayIterator(const ArrayType& InArray, SizeType StartIndex) noexcept
        : Array( InArray )
        , Index( StartIndex )
    {
        Assert(IsValid());
    }

    /* Ensure that the pointer is in the range of the array */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Index >= 0) && (Index <= Array.Size()); 
    }

    /* Ensure that the pointer is not the endpointer */
    FORCEINLINE bool IsEnd() const noexcept
    {
        return (Index == 0); 
    }

    /* Retrive the raw pointer */
    FORCEINLINE ElementType* Raw() const noexcept
    {
        Assert(IsValid());
        return Array.Data() + GetIndex();
    }
    
    /* Retrive the useable index */
    FORCEINLINE GetIndex() const noexcept
    {
        return Index - 1;
    }

    /* Retrive the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Raw();
    }

    /* Dereference the raw pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Raw();
    }

    /* Add to the iterator */
    FORCEINLINE TReverseArrayIterator operator++() noexcept
    {
        Index--;
        Assert( IsValid() );
        return *this;
    }

    /* Add to the iterator */
    FORCEINLINE TReverseArrayIterator operator++( int ) noexcept
    {
        TReverseArrayIterator Temp = *this;
        Index--;
        Assert( IsValid() );
        return Temp;
    }

    /* Subtract the iterator */
    FORCEINLINE TReverseArrayIterator operator--() noexcept
    {
        Index++;
        Assert( IsValid() );
        return *this;
    }

    /* Subtract the iterator */
    FORCEINLINE TReverseArrayIterator operator--( int ) noexcept
    {
        TReverseArrayIterator Temp = *this;
        Iterator++;
        Assert( IsValid() );
        return Temp;
    }

    /* Add offset to iterator and return a new */
    FORCEINLINE TReverseArrayIterator operator+( SizeType RHS ) const noexcept
    {
        TReverseArrayIterator Temp = *this;
        return Temp += RHS; // Uses operator, therefore +=
    }

    /* Add offset to iterator and return a new */
    friend FORCEINLINE TReverseArrayIterator operator+( SizeType LHS, const TReverseArrayIterator& RHS) const noexcept
    {
        TReverseArrayIterator Temp = RHS;
        return Temp += LHS; // Uses operator, therefore +=
    }

    /* Subtract offset from iterator and return a new */
    FORCEINLINE TReverseArrayIterator operator-( SizeType RHS ) const noexcept
    {
        TReverseArrayIterator Temp = *this;
        return Temp -= RHS; // Uses operator, therefore -=
    }

    /* Subtract offset to iterator and return a new */
    friend FORCEINLINE TArrayIterator operator-( SizeType LHS, const TArrayIterator& RHS) const noexcept
    {
        TArrayIterator Temp = RHS;
        return Temp -= LHS; // Uses operator, therefore -=
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
        return (Index == RHS.Index) && (::AddressOf(Array) == ::AddressOf(RHS.Array));
    }

    /* Compare equality two iterators */
    FORCEINLINE bool operator!=( const TReverseArrayIterator& RHS ) const noexcept
    {
        return (*this == RHS);
    }

    /* Convert into a const iterator */
    FORCEINLINE operator TReverseArrayIterator<const ElementType>() const noexcept
    {
        /* The arraytype must be const here in order to make the dereference work properly */
        return TReverseArrayIterator<const ArrayType, const ElementType>( Array, Index );
    }

private:
    ArrayType& Array;
    SizeType Index;
};