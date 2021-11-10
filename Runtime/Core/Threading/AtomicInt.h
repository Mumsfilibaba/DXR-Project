#pragma once
#include "Core.h"
#include "Core/Templates/IsSigned.h"
#include "Core/Threading/Platform/PlatformInterlocked.h"
#include "Core/Threading/Platform/PlatformAtomic.h"

template<typename T>
class TAtomicInt
{
public:

    typedef T Type;

    static_assert(TIsSigned<T>::Value, "AtomicInt only supports signed types");

    FORCEINLINE TAtomicInt() noexcept
        : Value( 0 )
    {
    }

    FORCEINLINE TAtomicInt( const TAtomicInt& Other )
        : Value( 0 )
    {
        T TempInteger = Other.Load();
        Store( TempInteger );
    }

    FORCEINLINE TAtomicInt( T InValue ) noexcept
        : Value( InValue )
    {
    }

    ~TAtomicInt() = default;

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
        return PlatformAtomic::Read( &Value );
    }

    FORCEINLINE T RelaxedLoad() const noexcept
    {
        return PlatformAtomic::RelaxedRead( &Value );
    }

    FORCEINLINE T Exchange( T InValue ) noexcept
    {
        return PlatformInterlocked::Exchange( &Value, InValue );
    }

    FORCEINLINE void Store( T InValue ) noexcept
    {
        PlatformAtomic::Store( &Value, InValue );
    }

    FORCEINLINE void RelaxedStore( T InValue ) noexcept
    {
        PlatformAtomic::RelaxedStore( &Value, InValue );
    }

public:

    /* Operators */

    FORCEINLINE TAtomicInt& operator=( const TAtomicInt& Other )
    {
        T TempInteger = Other.Load();
        Store( TempInteger );
        return *this;
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
typedef TAtomicInt<int8>  AtomicInt8;
typedef TAtomicInt<int16> AtomicInt16;
typedef TAtomicInt<int32> AtomicInt32;
typedef TAtomicInt<int64> AtomicInt64;
