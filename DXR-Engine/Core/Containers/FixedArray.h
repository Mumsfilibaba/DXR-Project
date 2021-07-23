#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"

/* A fixed size array similar to std::array */
template<typename T, const uint32 N>
struct TFixedArray
{
public:
    typedef T                                   ElementType;
    typedef ElementType*                        Iterator;
    typedef const ElementType*                  ConstIterator;
    typedef TReverseIterator<ElementType>       ReverseIterator;
    typedef TReverseIterator<const ElementType> ConstReverseIterator;
    typedef uint32                               SizeType;

    static_assert(N > 0, "The number of elements has to be more than zero");

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
        return Elements[N - 1];
    }

    /* Retrive the last element */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        return Elements[N - 1];
    }

    /* Retrive the element at a certain position */
    FORCEINLINE ElementType& At( SizeType Index ) noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    /* Retrive the element at a certain position */
    FORCEINLINE const ElementType& At( SizeType Index ) const noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    /* Fills the container with the specified value */
    template<typename FillType>
    FORCEINLINE typename TEnableIf<TIsAssignable<T, typename TAddLeftReference<const FillType>::Type>::Value>::Type Fill( const FillType& InputElement ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = InputElement;
        }
    }

    /* Fills the container with the specified value */
    template<typename FillType>
    FORCEINLINE typename TEnableIf<TIsAssignable<T, typename TAddRightReference<FillType>::Type>::Value>::Type Fill( FillType&& InputElement ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = ::Move( InputElement );
        }
    }

    /* Swaps this array with another */
    FORCEINLINE void Swap( TFixedArray& Other ) noexcept
    {
        TFixedArray Temp( ::Move( *this ) );
        *this = ::Move( Other );
        Other = ::Move( Temp );
    }

    /* Retrive the last valid index */
    constexpr SizeType LastIndex() const noexcept
    {
        return N - 1;
    }

    /* Retrive the size of the array */
    constexpr SizeType Size() const noexcept
    {
        return N;
    }

    /* Retrive the size of the array in bytes */
    constexpr SizeType SizeInBytes() const noexcept
    {
        return N * sizeof( ElementType );
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

public:

    /* STL iterator functions - Enables Range-based for-loops */
    FORCEINLINE Iterator begin() noexcept
    {
        return Elements;
    }

    FORCEINLINE Iterator end() noexcept
    {
        return Elements + N;
    }

    FORCEINLINE ConstIterator begin() const noexcept
    {
        return Elements;
    }

    FORCEINLINE ConstIterator end() const noexcept
    {
        return Elements + N;
    }

public:
    ElementType Elements[N];
};
