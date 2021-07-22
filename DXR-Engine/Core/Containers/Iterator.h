#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

template<typename IteratorType>
class TReverseIterator
{
public:
    FORCEINLINE TReverseIterator() noexcept
        : Iterator( nullptr )
    {
    }

    FORCEINLINE TReverseIterator( IteratorType* InIterator ) noexcept
        : Iterator( InIterator )
    {
    }

    FORCEINLINE TReverseIterator( const TReverseIterator& Other ) noexcept
        : Iterator( Other.Raw() )
    {
    }

    template<typename U>
    FORCEINLINE TReverseIterator( const TReverseIterator<U>& Other ) noexcept
        : Iterator( Other.Raw() )
    {
    }

    FORCEINLINE IteratorType* Raw() const
    {
        return Iterator;
    }

    FORCEINLINE IteratorType* operator->() const noexcept
    {
        return (Iterator - 1);
    }

    FORCEINLINE IteratorType& operator*()  const noexcept
    {
        return *(Iterator - 1);
    }

    FORCEINLINE TReverseIterator operator++() noexcept
    {
        Iterator--;
        return *this;
    }

    FORCEINLINE TReverseIterator operator++( int32 ) noexcept
    {
        TReverseIterator Temp = *this;
        Iterator--;
        return Temp;
    }

    FORCEINLINE TReverseIterator operator--() noexcept
    {
        Iterator++;
        return *this;
    }

    FORCEINLINE TReverseIterator operator--( int32 ) noexcept
    {
        TReverseIterator Temp = *this;
        Iterator++;
        return Temp;
    }

    FORCEINLINE TReverseIterator operator+( int32 Offset ) const noexcept
    {
        TReverseIterator Temp = *this;
        return Temp += Offset;
    }

    FORCEINLINE TReverseIterator operator-( int32 Offset ) const noexcept
    {
        TReverseIterator Temp = *this;
        return Temp -= Offset;
    }

    FORCEINLINE TReverseIterator& operator+=( int32 Offset ) noexcept
    {
        Iterator -= Offset;
        return *this;
    }

    FORCEINLINE TReverseIterator& operator-=( int32 Offset ) noexcept
    {
        Iterator += Offset;
        return *this;
    }

    FORCEINLINE bool operator==( const TReverseIterator& Other ) const noexcept
    {
        return (Iterator == Other.Iterator);
    }

    FORCEINLINE bool operator!=( const TReverseIterator& Other ) const noexcept
    {
        return (Iterator != Other.Iterator);
    }

    FORCEINLINE operator TReverseIterator<const IteratorType>() const noexcept
    {
        return TReverseIterator<const IteratorType>( Iterator );
    }

private:
    IteratorType* Iterator;
};