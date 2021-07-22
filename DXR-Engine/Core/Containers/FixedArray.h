#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"

/* A fixed size array similar to std::array */
template<typename T, int32 N>
struct TFixedArray
{
public:
    typedef T                                   ElementType;
    typedef ElementType*                        Iterator;
    typedef const ElementType*                  ConstIterator;
    typedef TReverseIterator<ElementType>       ReverseIterator;
    typedef TReverseIterator<const ElementType> ConstReverseIterator;
    typedef uint32                              SizeType;

    FORCEINLINE ElementType& Front() noexcept
    {
        return Elements[0];
    }

    FORCEINLINE const ElementType& Front() const noexcept
    {
        return Elements[0];
    }

    FORCEINLINE ElementType& Back() noexcept
    {
        return Elements[N - 1];
    }

    FORCEINLINE const ElementType& Back() const noexcept
    {
        return Elements[N - 1];
    }

    FORCEINLINE ElementType& At( SizeType Index ) noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    FORCEINLINE const ElementType& At( SizeType Index ) const noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    FORCEINLINE void Fill( const ElementType& Value ) noexcept
    {
        for ( uint32 i = 0; i < N; i++ )
        {
            Elements[i] = Value;
        }
    }

    FORCEINLINE void Fill( ElementType&& Value ) noexcept
    {
        for ( uint32 i = 0; i < N; i++ )
        {
            Elements[i] = ::Move(Value);
        }
    }

    FORCEINLINE void Swap( TFixedArray& Other ) noexcept
    {
        TFixedArray TempArray( ::Move( *this ) );
        *this = ::Move( Other );
        Other = ::Move( TempArray );
    }

    constexpr SizeType LastIndex() const noexcept
    {
        return N > 0 ? N - 1 : 0;
    }

    constexpr SizeType Size() const noexcept
    {
        return N;
    }

    constexpr SizeType SizeInBytes() const noexcept
    {
        return N * sizeof( ElementType );
    }

    FORCEINLINE ElementType* Data() noexcept
    {
        return Elements;
    }

    FORCEINLINE const ElementType* Data() const noexcept
    {
        return Elements;
    }

    FORCEINLINE ElementType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

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

    FORCEINLINE ConstIterator cbegin() const noexcept
    {
        return Elements;
    }

    FORCEINLINE ConstIterator cend() const noexcept
    {
        return Elements + N;
    }

    FORCEINLINE ReverseIterator rbegin() noexcept
    {
        return ReverseIterator( end() );
    }

    FORCEINLINE ReverseIterator rend() noexcept
    {
        return ReverseIterator( begin() );
    }

    FORCEINLINE ConstReverseIterator rbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }

    FORCEINLINE ConstReverseIterator rend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

    FORCEINLINE ConstReverseIterator crbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }

    FORCEINLINE ConstReverseIterator crend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

public:
    ElementType Elements[N];
};
