#pragma once
#include "Utilities.h"
#include "Iterator.h"

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

    T& Front() noexcept
    {
        return Elements[0];
    }
    const T& Front() const noexcept
    {
        return Elements[0];
    }

    T& Back() noexcept
    {
        return Elements[N - 1];
    }
    const T& Back() const noexcept
    {
        return Elements[N - 1];
    }

    T& At( SizeType Index ) noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    const T& At( SizeType Index ) const noexcept
    {
        Assert( Index < N );
        return Elements[Index];
    }

    void Fill( const T& Value ) noexcept
    {
        for ( uint32 i = 0; i < N; i++ )
        {
            Elements[i] = Value;
        }
    }

    void Swap( TStaticArray& Other ) noexcept
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

    T* Data() noexcept
    {
        return Elements;
    }
    const T* Data() const noexcept
    {
        return Elements;
    }

    T& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }
    const T& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    // STL iterator functions - Enables Range-based for-loops
public:
    Iterator begin() noexcept
    {
        return Elements;
    }
    Iterator end() noexcept
    {
        return Elements + N;
    }

    ConstIterator begin() const noexcept
    {
        return Elements;
    }
    ConstIterator end() const noexcept
    {
        return Elements + N;
    }

    ConstIterator cbegin() const noexcept
    {
        return Elements;
    }
    ConstIterator cend() const noexcept
    {
        return Elements + N;
    }

    ReverseIterator rbegin() noexcept
    {
        return ReverseIterator( end() );
    }
    ReverseIterator rend() noexcept
    {
        return ReverseIterator( begin() );
    }

    ConstReverseIterator rbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }
    ConstReverseIterator rend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

    ConstReverseIterator crbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }
    ConstReverseIterator crend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

public:
    T Elements[N];
};
