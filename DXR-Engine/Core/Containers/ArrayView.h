#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"

/* TArrayView - View of an array similar to std::span */
template<typename T>
class TArrayView
{
public:
    using ElementType = T;
    using SizeType = int32;

    /* Iterators */
    typedef TArrayIterator<TArrayView, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArrayView, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArrayView, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArrayView, const ElementType> ReverseConstIteratorType;

    /* Default construct an empty view */
    FORCEINLINE TArrayView() noexcept
        : View( nullptr )
        , ViewSize( 0 )
    {
    }

    /* Create a view from a templated array type */
    template<typename ArrayType>
    FORCEINLINE explicit TArrayView( ArrayType& InArray ) noexcept
        : View( InArray.Data() )
        , ViewSize( InArray.Size() )
    {
    }

    /* Create a view from a bounded array */
    template<const SizeType N>
    FORCEINLINE explicit TArrayView( ElementType( &InArray )[N] ) noexcept
        : View( InArray )
        , ViewSize( N )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE explicit TArrayView( ElementType* InArray, SizeType Count ) noexcept
        : View( InArray )
        , ViewSize( Count )
    {
    }

    /* Create a view from another view */
    FORCEINLINE TArrayView( const TArrayView& Other ) noexcept
        : View( Other.View )
        , ViewSize( Other.ViewSize )
    {
    }

    /* Move anther view into this one */
    FORCEINLINE TArrayView( TArrayView&& Other ) noexcept
        : View( Other.View )
        , ViewSize( Other.ViewSize )
    {
        Other.View = nullptr;
        Other.ViewSize = 0;
    }

    /* Check if the size is zero or not */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ViewSize == 0);
    }

    /* Retrive the first element */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert( IsEmpty() );
        return Data()[0];
    }

    /* Retrive the first element */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[0];
    }

    /* Retrive the last element */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert( IsEmpty() );
        return Data()[ViewSize - 1];
    }

    /* Retrive the last element */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[ViewSize - 1];
    }

    /* Retrive an element at a certain position */
    FORCEINLINE ElementType& At( SizeType Index ) noexcept
    {
        Assert( Index < ViewSize );
        return Data()[Index];
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const ElementType& At( SizeType Index ) const noexcept
    {
        Assert( Index < ViewSize );
        return Data()[Index];
    }

    /* Swap two views */
    FORCEINLINE void Swap( TArrayView& Other ) noexcept
    {
        TArrayView Temp( Move( *this ) );
        *this = Move( Other );
        Other = Move( Temp );
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType( *this, 0 );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType( *this, Size() );
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType( *this, 0 );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType( *this, 0 );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, 0 );
    }

    /* Fills the container with the specified value */
    FORCEINLINE void Fill( const ElementType& InputElement ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = InputElement;
        }
    }

    /* Retrive the last valid index for the view */
    FORCEINLINE SizeType LastIndex() const noexcept
    {
        return ViewSize > 0 ? ViewSize - 1 : 0;
    }

    /* Retrive the size of the view */
    FORCEINLINE SizeType Size() const noexcept
    {
        return ViewSize;
    }

    /* Retrive the size of the view in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof( ElementType );
    }

    /* Retrive the data of the view */
    FORCEINLINE ElementType* Data() noexcept
    {
        return View;
    }

    /* Retrive the data of the view */
    FORCEINLINE const ElementType* Data() const noexcept
    {
        return View;
    }

    /* Create a subview */
    FORCEINLINE const ElementType* SubView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert((Count < ViewSize) && (Offset + Count < ViewSize) );
        return TArrayView(View + Offset, Count);
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename ArrayType>
    FORCEINLINE bool operator==( const ArrayType& Other ) const noexcept
    {
        if ( Size() != Other.Size() )
        {
            return false;
        }

        return CompareRange<ElementType>( Data(), Other.Data(), Size() );
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename ArrayType>
    FORCEINLINE bool operator!=( const ArrayType& Other ) const noexcept
    {
        return !(*this == Other);
    }

    /* Retrive an element at a certain position */
    FORCEINLINE ElementType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const ElementType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    /* Assign from another view */
    FORCEINLINE TArrayView& operator=( const TArrayView& Other ) noexcept
    {
        View = Other.View;
        ViewSize = Other.ViewSize;
        return *this;
    }

    /* Move-assign from another view */
    FORCEINLINE TArrayView& operator=( TArrayView&& Other ) noexcept
    {
        if ( this != &Other )
        {
            View = Other.View;
            ViewSize = Other.ViewSize;
            Other.View = nullptr;
            Other.ViewSize = 0;
        }

        return *this;
    }

public:

    /* STL iterator functions - Enables Range-based for-loops */

    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:
    ElementType* View;
    SizeType ViewSize;
};
