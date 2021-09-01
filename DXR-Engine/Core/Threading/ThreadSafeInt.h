#pragma once
#include "Core.h"

#include "Core/Threading/Platform/PlatformAtomic.h"

// TODO: Maybe have an unsigned version?
template<typename T>
class TThreadSafeInt
{
public:
    typedef T Type;

    TThreadSafeInt( const TThreadSafeInt& ) = delete;

    FORCEINLINE TThreadSafeInt() noexcept
        : Value( 0 )
    {
    }

    FORCEINLINE TThreadSafeInt( T InValue ) noexcept
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
        return PlatformAtomic::InterlockedAdd( &Value, RHS );
    }

    FORCEINLINE T Sub( T RHS ) noexcept
    {
        return PlatformAtomic::InterlockedSub( &Value, RHS );
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

    TThreadSafeInt& operator=( const TThreadSafeInt& ) = delete;

    FORCEINLINE T operator=( T RHS ) noexcept
    {
        return Store( RHS );
    }

    FORCEINLINE T operator++( int32 ) noexcept
    {
        T TempValue = Value;
        Increment();
        return TempValue;
    }

    FORCEINLINE T operator++() noexcept
    {
        return Increment();
    }

    FORCEINLINE T operator--( int32 ) noexcept
    {
        T TempValue = Value;
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

    FORCEINLINE T operator*=( T RHS ) noexcept
    {
        return Mul( RHS );
    }

    FORCEINLINE T operator/=( T RHS ) noexcept
    {
        return Div( RHS );
    }

private:
    mutable volatile T Value;
};

typedef TThreadSafeInt<int32> ThreadSafeInt32;
typedef TThreadSafeInt<int64> ThreadSafeInt64;