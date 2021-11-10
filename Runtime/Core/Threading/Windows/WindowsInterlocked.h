#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Interface/PlatformInterlocked.h"

#include "Core/Windows/Windows.h"

#include <intrin.h>

/* Atomic operations on the windows platform */
class CWindowsInterlocked : public CPlatformInterlocked
{
public:

    // See: https://docs.microsoft.com/en-us/cpp/intrinsics/interlockedexchangeadd-intrinsic-functions?view=msvc-160
    //      https://docs.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-interlockedincrement16

    static_assert(sizeof( int32 ) == sizeof( long ) && alignof(int32) == alignof(long), "int32 must have the same size and alignment as long");

    /* Add: Adds value and return original value of Dest */

    static FORCEINLINE int8 Add( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8( static_cast<volatile char*>(Dest), static_cast<char>(Value) ));
    }

    static FORCEINLINE int16 Add( volatile int16* Dest, int16 Value )
    {
        return static_cast<int16>(::_InterlockedExchangeAdd16( static_cast<volatile short*>(Dest), static_cast<short>(Value) ));
    }

    static FORCEINLINE int32 Add( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(::_InterlockedExchangeAdd( reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value) ));
    }

    static FORCEINLINE int64 Add( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(::_InterlockedExchangeAdd64( static_cast<volatile long long*>(Dest), static_cast<long long>(Value) ));
    }

    /* Sub: Subtracts value and return original value of Dest */

    static FORCEINLINE int8 Sub( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8( static_cast<volatile char*>(Dest), -static_cast<char>(Value) ));
    }

    static FORCEINLINE int16 Sub( volatile int16* Dest, int16 Value )
    {
        return static_cast<int16>(::_InterlockedExchangeAdd16( static_cast<volatile short*>(Dest), -static_cast<short>(Value) ));
    }

    static FORCEINLINE int32 Sub( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(::_InterlockedExchangeAdd( reinterpret_cast<volatile long*>(Dest), -static_cast<long>(Value) ));
    }

    static FORCEINLINE int64 Sub( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(::_InterlockedExchangeAdd64( static_cast<volatile long long*>(Dest), -static_cast<long long>(Value) ));
    }

    /* And: ANDs the Dest with Value and returns the original value */

    static FORCEINLINE int8 And( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(::_InterlockedAnd8( static_cast<volatile char*>(Dest), static_cast<char>(Value) ));
    }

    static FORCEINLINE int16 And( volatile int16* Dest, int16 Value )
    {
        return static_cast<int8>(::_InterlockedAnd16( static_cast<volatile short*>(Dest), static_cast<short>(Value) ));;
    }

    static FORCEINLINE int32 And( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(::_InterlockedAnd( reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value) ));
    }

    static FORCEINLINE int64 And( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(::_InterlockedAnd64( static_cast<volatile long long*>(Dest), static_cast<long long>(Value) ));
    }

    /* Or: ORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 Or( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(::_InterlockedOr8( static_cast<volatile char*>(Dest), static_cast<char>(Value) ));
    }

    static FORCEINLINE int16 Or( volatile int16* Dest, int16 Value )
    {
        return static_cast<int8>(::_InterlockedOr16( static_cast<volatile short*>(Dest), static_cast<short>(Value) ));;
    }

    static FORCEINLINE int32 Or( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(::_InterlockedOr( reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value) ));
    }

    static FORCEINLINE int64 Or( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(::_InterlockedOr64( static_cast<volatile long long*>(Dest), static_cast<long long>(Value) ));
    }

    /* Xor: XORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 Xor( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(::_InterlockedXor8( static_cast<volatile char*>(Dest), static_cast<char>(Value) ));
    }

    static FORCEINLINE int16 Xor( volatile int16* Dest, int16 Value )
    {
        return static_cast<int8>(::_InterlockedXor16( static_cast<volatile short*>(Dest), static_cast<short>(Value) ));
    }

    static FORCEINLINE int32 Xor( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(::_InterlockedXor( reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value) ));
    }

    static FORCEINLINE int64 Xor( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(::_InterlockedXor64( static_cast<volatile long long*>(Dest), static_cast<long long>(Value) ));
    }

    /* Increment: Increments destination and returns the new value */

    static FORCEINLINE int8 Increment( volatile int8* Dest )
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8( static_cast<volatile char*>(Dest), 1 )) + 1;
    }

    static FORCEINLINE int16 Increment( volatile int16* Dest )
    {
        return static_cast<int16>(::_InterlockedIncrement16( static_cast<volatile short*>(Dest) ));
    }

    static FORCEINLINE int32 Increment( volatile int32* Dest )
    {
        return static_cast<int32>(::_InterlockedIncrement( reinterpret_cast<volatile long*>(Dest) ));
    }

    static FORCEINLINE int64 Increment( volatile int64* Dest )
    {
        return static_cast<int64>(::_InterlockedIncrement64( static_cast<volatile long long*>(Dest) ));
    }

    /* Decrement: Decrements destination and returns the new value */

    static FORCEINLINE int8 Decrement( volatile int8* Dest )
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8( static_cast<volatile char*>(Dest), -1 )) - 1;
    }

    static FORCEINLINE int16 Decrement( volatile int16* Dest )
    {
        return static_cast<int16>(::_InterlockedDecrement16( static_cast<volatile short*>(Dest) ));
    }

    static FORCEINLINE int32 Decrement( volatile int32* Dest )
    {
        return static_cast<int32>(::_InterlockedDecrement( reinterpret_cast<volatile long*>(Dest) ));
    }

    static FORCEINLINE int64 Decrement( volatile int64* Dest )
    {
        return static_cast<int64>(::_InterlockedDecrement64( static_cast<volatile long long*>(Dest) ));
    }

    /* CompareExchange: Compares Dest with Comparand, if equal Exchange gets stored in Dest. Returns the original value. */

    static FORCEINLINE int8 CompareExchange( volatile int8* Dest, int8 Exchange, int8 Comparand )
    {
        return static_cast<int8>(::_InterlockedCompareExchange8( static_cast<volatile char*>(Dest), static_cast<char>(Exchange), static_cast<char>(Comparand) ));
    }

    static FORCEINLINE int16 CompareExchange( volatile int16* Dest, int16 Exchange, int16 Comparand )
    {
        return static_cast<int16>(::_InterlockedCompareExchange16( static_cast<volatile short*>(Dest), static_cast<short>(Exchange), static_cast<short>(Comparand) ));
    }

    static FORCEINLINE int32 CompareExchange( volatile int32* Dest, int32 Exchange, int32 Comparand )
    {
        return static_cast<int32>(::_InterlockedCompareExchange( reinterpret_cast<volatile long*>(Dest), static_cast<long>(Exchange), static_cast<long>(Comparand) ));
    }

    static FORCEINLINE int64 CompareExchange( volatile int64* Dest, int64 Exchange, int64 Comparand )
    {
        return static_cast<int64>(::_InterlockedCompareExchange64( static_cast<volatile long long*>(Dest), static_cast<long>(Exchange), static_cast<long>(Comparand) ));
    }

    /* Exchange: Stores Value in Dest, and returns original value */

    static FORCEINLINE int8 Exchange( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(::_InterlockedExchange8( static_cast<volatile char*>(Dest), static_cast<char>(Value) ));
    }

    static FORCEINLINE int16 Exchange( volatile int16* Dest, int16 Value )
    {
        return static_cast<int8>(::_InterlockedExchange16( static_cast<volatile short*>(Dest), static_cast<short>(Value) ));
    }

    static FORCEINLINE int32 Exchange( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(::_InterlockedExchange( reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value) ));
    }

    static FORCEINLINE int64 Exchange( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(::_InterlockedExchange64( static_cast<volatile long long*>(Dest), static_cast<long long>(Value) ));
    }
};

#endif