#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"

// TStaticArray - Static Array similar to std::array

template<typename T, int32 N>
struct TStaticArray
{
public:
    typedef T* Iterator;
    typedef const T* ConstIterator;
    typedef TReverseIterator<T>       ReverseIterator;
    typedef TReverseIterator<const T> ConstReverseIterator;
    typedef uint32                    SizeType;

    FORCEINLINE T& Front() noexcept
    {
        return Elements[0];
    }
    
    FORCEINLINE const T& Front() const noexcept
    {
        return Elements[0];
    }

    FORCEINLINE T& Back() noexcept
    {
        return Elements[N - 1];
    }

    FORCEINLINE const T& Back() const noexcept
    {
        return Elements[N - 1];
    }

    FORCEINLINE T& At( SizeType Index ) noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    FORCEINLINE const T& At( SizeType Index ) const noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    FORCEINLINE void Fill( const T& Value ) noexcept
    {
        for ( uint32 i = 0; i < N; i++ )
        {
            Elements[i] = Value;
        }
    }

    FORCEINLINE void Swap( TStaticArray& Other ) noexcept
    {
        TStaticArray TempArray( ::Move( *this ) );
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
        return N * sizeof( T );
    }

    FORCEINLINE T* Data() noexcept
    {
        return Elements;
    }

    FORCEINLINE const T* Data() const noexcept
    {
        return Elements;
    }

    FORCEINLINE T& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    FORCEINLINE const T& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    // STL iterator functions - Enables Range-based for-loops
public:
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
    T Elements[N];
};
