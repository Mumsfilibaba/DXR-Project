#pragma once
#include "Core.h"
#include "Core/Templates/IsSigned.h"
#include "Core/Threading/Platform/PlatformInterlocked.h"

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

    FORCEINLINE TInterlockedInt( const TInterlockedInt& Other )
        : Value( 0 )
    {
        T TempInteger = Other.Load();
        Store( TempInteger );
    }

    FORCEINLINE TInterlockedInt( T InValue ) noexcept
        : Value( InValue )
    {
    }

    FORCEINLINE T Increment() noexcept
    {
        return PlatformInterlocked::Increment( &Value );
    }

    FORCEINLINE T Decrement() noexcept
    {
        return PlatformInterlocked::Decrement( &Value );
    }

    FORCEINLINE T Add( T RHS ) noexcept
    {
        PlatformInterlocked::Add( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Subtract( T RHS ) noexcept
    {
        PlatformInterlocked::Sub( &Value, RHS );
        return Value;
    }

    FORCEINLINE T And( T RHS ) noexcept
    {
        PlatformInterlocked::And( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Or( T RHS ) noexcept
    {
        PlatformInterlocked::Or( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Xor( T RHS ) noexcept
    {
        PlatformInterlocked::Xor( &Value, RHS );
        return Value;
    }

    FORCEINLINE T Load() const noexcept
    {
        // Makes sure that all prior accesses has completed
        PlatformInterlocked::CompareExchange( &Value, 0, 0 );
        return Value;
    }

    FORCEINLINE void Store( T InValue ) noexcept
    {
        PlatformInterlocked::Exchange( &Value, InValue );
    }

public:

    /* Operators */

    FORCEINLINE TInterlockedInt& operator=( const TInterlockedInt& Other )
    {
        T TempInteger = Other.Load();
        Store( TempInteger );
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
        return Subtract( RHS );
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
