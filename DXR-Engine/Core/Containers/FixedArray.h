#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsCopyable.h"
#include "Core/Templates/IsMovable.h"
#include "Core/Templates/AddReference.h"
#include "Core/Templates/ObjectHandling.h"

/* A fixed size array similar to std::array */
template<typename T, const int32 ArraySize>
struct TFixedArray
{
public:
    using ElementType = T;
    using SizeType = int32;

    /* Iterators */
    typedef TArrayIterator<TFixedArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TFixedArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TFixedArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TFixedArray, const ElementType> ReverseConstIteratorType;

    static_assert(ArraySize > 0, "The number of elements has to be more than zero");

    /* Retrive the first element */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        return Elements[0];
    }

    /* Retrive the first element */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        return Elements[0];
    }

    /* Retrive the last element */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        return Elements[ArraySize - 1];
    }

    /* Retrive the last element */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        return Elements[ArraySize - 1];
    }

    /* Retrive the element at a certain position */
    FORCEINLINE ElementType& At( SizeType Index ) noexcept
    {
        Assert( Index < ArraySize );
        return Elements[Index];
    }

    /* Retrive the element at a certain position */
    FORCEINLINE const ElementType& At( SizeType Index ) const noexcept
    {
        Assert( Index < ArraySize );
        return Elements[Index];
    }

    /* Fills the container with the specified value */
    FORCEINLINE void Fill( const ElementType& InputElement ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = InputElement;
        }
    }

    /* Swaps this array with another */
    FORCEINLINE void Swap( TFixedArray& Other ) noexcept
    {
        TFixedArray Temp( Move( *this ) );
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

    /* Retrive the last valid index */
    constexpr SizeType LastIndex() const noexcept
    {
        return ArraySize - 1;
    }

    /* Retrive the size of the array */
    constexpr SizeType Size() const noexcept
    {
        return ArraySize;
    }

    /* Retrive the size of the array in bytes */
    constexpr SizeType SizeInBytes() const noexcept
    {
        return ArraySize * sizeof( ElementType );
    }

    /* Retrive the data of the array */
    FORCEINLINE ElementType* Data() noexcept
    {
        return Elements;
    }

    /* Retrive the data of the array */
    FORCEINLINE const ElementType* Data() const noexcept
    {
        return Elements;
    }

    /* Retrive the element at a certain position */
    FORCEINLINE ElementType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    /* Retrive the element at a certain position */
    FORCEINLINE const ElementType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
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

public:
    ElementType Elements[ArraySize];
};
