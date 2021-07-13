#pragma once
#include "Core/Types.h"

template<typename TIteratorType>
class TReverseIterator
{
public:
    TReverseIterator() noexcept
        : Iterator( nullptr )
    {
    }

    TReverseIterator( TIteratorType* InIterator ) noexcept
        : Iterator( InIterator )
    {
    }

    TReverseIterator( const TReverseIterator& Other ) noexcept
        : Iterator( Other.Raw() )
    {
    }

    template<typename U>
    TReverseIterator( const TReverseIterator<U>& Other ) noexcept
        : Iterator( Other.Raw() )
    {
    }

    ~TReverseIterator() = default;

    TIteratorType* Raw() const
    {
        return Iterator;
    }

    TIteratorType* operator->() const noexcept
    {
        return (Iterator - 1);
    }
    TIteratorType& operator*()  const noexcept
    {
        return *(Iterator - 1);
    }

    TReverseIterator operator++() noexcept
    {
        Iterator--;
        return *this;
    }

    TReverseIterator operator++( int32 ) noexcept
    {
        TReverseIterator Temp = *this;
        Iterator--;
        return Temp;
    }

    TReverseIterator operator--() noexcept
    {
        Iterator++;
        return *this;
    }

    TReverseIterator operator--( int32 ) noexcept
    {
        TReverseIterator Temp = *this;
        Iterator++;
        return Temp;
    }

    TReverseIterator operator+( int32 Offset ) const noexcept
    {
        TReverseIterator Temp = *this;
        return Temp += Offset;
    }

    TReverseIterator operator-( int32 Offset ) const noexcept
    {
        TReverseIterator Temp = *this;
        return Temp -= Offset;
    }

    TReverseIterator& operator+=( int32 Offset ) noexcept
    {
        Iterator -= Offset;
        return *this;
    }

    TReverseIterator& operator-=( int32 Offset ) noexcept
    {
        Iterator += Offset;
        return *this;
    }

    bool operator==( const TReverseIterator& Other ) const noexcept
    {
        return (Iterator == Other.Iterator);
    }
    bool operator!=( const TReverseIterator& Other ) const noexcept
    {
        return (Iterator != Other.Iterator);
    }

    operator TReverseIterator<const TIteratorType>() const noexcept
    {
        return TReverseIterator<const TIteratorType>( Iterator );
    }

private:
    TIteratorType* Iterator;
};