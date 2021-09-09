#pragma once
#include "Core.h"

#include "Core/Templates/IsSigned.h"

#include "Core/Threading/Platform/PlatformAtomic.h"

template<typename T>
class TInterlockedInt
{
public:
    typedef T Type;

    static_assert(TIsSigned<T>::Value, "InterlockedInt only supports signed types");

    FORCEINLINE TInterlockedInt() noexcept
        : Value( 0 )
    {
    }

    FORCEINLINE TInterlockedInt( const TInterlockedInt& Other)
            : Value( 0 )
    {
        T TempInteger = Other.Load();
        Store(TempInteger);
    }

    FORCEINLINE TInterlockedInt( T InValue ) noexcept
        : Value( InValue )
    {
    }

    FORCEINLINE T Increment() noexcept
    {
        return PlatformAtomic::InterlockedIncrement( &Value );
    }

    FORCEINLINE T Decrement() noexcept
    {
        return PlatformAtomic::InterlockedDecrement( &Value );
    }

    FORCEINLINE T Add( T RHS ) noexcept
    {
        PlatformAtomic::InterlockedAdd( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Sub( T RHS ) noexcept
    {
        PlatformAtomic::InterlockedSub( &Value, RHS );
        return Value;
    }

    FORCEINLINE T And( T RHS ) noexcept
    {
        PlatformAtomic::InterlockedAnd( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Or( T RHS ) noexcept
    {
        PlatformAtomic::InterlockedOr( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Xor( T RHS ) noexcept
    {
        PlatformAtomic::InterlockedXor( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Load() const noexcept
    {
        // Makes sure that all prior accesses has completed
        PlatformAtomic::InterlockedCompareExchange( &Value, 0, 0 );
        return Value;
    }

    FORCEINLINE void Store( T InValue ) noexcept
    {
        PlatformAtomic::InterlockedExchange( &Value, InValue );
    }

public:

    /* Operators */

    FORCEINLINE TInterlockedInt& operator=( const TInterlockedInt& Other )
    {
        T TempInteger = Other.Load();
        Store(TempInteger);
    }

    FORCEINLINE T operator=( T RHS ) noexcept
    {
        return Store( RHS );
    }

    FORCEINLINE T operator++( int32 ) noexcept
    {
        T TempValue = Load();
        Increment();
        return TempValue;
    }

    FORCEINLINE T operator++() noexcept
    {
        return Increment();
    }

    FORCEINLINE T operator--( int32 ) noexcept
    {
        T TempValue = Load();
        Decrement();
        return TempValue;
    }

    FORCEINLINE T operator--() noexcept
    {
        return Decrement();
    }

    FORCEINLINE T operator+=( T RHS ) noexcept
    {
        return Add( RHS );
    }

    FORCEINLINE T operator-=( T RHS ) noexcept
    {
        return Sub( RHS );
    }

    FORCEINLINE T operator&=( T RHS ) noexcept
    {
        return And( RHS );
    }

    FORCEINLINE T operator|=( T RHS ) noexcept
    {
        return Or( RHS );
    }

    FORCEINLINE T operator^=( T RHS ) noexcept
    {
        return Xor( RHS );
    }

private:
    mutable volatile T Value;
};

/* Predefined types*/
typedef TInterlockedInt<int8>  InterlockedInt8;
typedef TInterlockedInt<int16> InterlockedInt16;
typedef TInterlockedInt<int32> InterlockedInt32;
typedef TInterlockedInt<int64> InterlockedInt64;