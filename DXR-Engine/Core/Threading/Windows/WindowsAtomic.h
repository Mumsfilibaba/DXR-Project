#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Generic/GenericAtomic.h"

#include "Core/Windows/Windows.h"

/* Atomic operations on the windows platform */
class CWindowsAtomic : public CGenericAtomic
{
public:

    // See: https://docs.microsoft.com/en-us/cpp/intrinsics/interlockedexchangeadd-intrinsic-functions?view=msvc-160
    //      https://docs.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-interlockedincrement16
    
    static_assert(sizeof(int32) == sizeof(long) && alignof(int32) == alignof(long), "int32 must have the same size and alignment as LONG");

    /* InterlockedAdd: Adds value and return original value of Dest */

    static FORCEINLINE int8 InterlockedAdd( volatile int8* Dest, int32 Value )
    {
        return static_cast<int8>(_InterlockedExchangeAdd8( static_cast<volatile char*>(Dest), Value ));
    }
    
    static FORCEINLINE int16 InterlockedAdd( volatile int16* Dest, int32 Value )
    {
        return static_cast<int16>(_InterlockedExchangeAdd16( static_cast<volatile short*>(Dest), Value ));
    }

    static FORCEINLINE int32 InterlockedAdd( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(_InterlockedExchangeAdd( static_cast<volatile long*>(Dest), Value ));
    }

    static FORCEINLINE int64 InterlockedAdd( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(_InterlockedExchangeAdd64( static_cast<volatile long long*>(Dest), Value ));
    }

    /* InterlockedSub: Subtracts value and return original value of Dest */

    static FORCEINLINE int32 InterlockedSub( volatile int32* Dest, int32 Value )
    {
        return _InlineInterlockedAdd( (LONG*)Dest, -Value );
    }

    static FORCEINLINE int64 InterlockedSub( volatile int64* Dest, int64 Value )
    {
        return _InlineInterlockedAdd64( Dest, -Value );
    }

    static FORCEINLINE int32 InterlockedIncrement( volatile int32* Dest )
    {
        return _InterlockedIncrement( (LONG*)Dest );
    }

    static FORCEINLINE int64 InterlockedIncrement( volatile int64* Dest )
    {
        return _InterlockedIncrement64( Dest );
    }

    static FORCEINLINE int32 InterlockedDecrement( volatile int32* Dest )
    {
        return _InterlockedDecrement( (LONG*)Dest );
    }

    static FORCEINLINE int64 InterlockedDecrement( volatile int64* Dest )
    {
        return _InterlockedDecrement64( Dest );
    }

    static FORCEINLINE int32 InterlockedCompareExchange( volatile int32* Dest, int32 ExChange, int32 Comparand )
    {
        return _InterlockedCompareExchange( (LONG*)Dest, ExChange, Comparand );
    }

    static FORCEINLINE int64 InterlockedCompareExchange( volatile int64* Dest, int64 ExChange, int64 Comparand )
    {
        return _InterlockedCompareExchange64( Dest, ExChange, Comparand );
    }

    static FORCEINLINE int32 InterlockedExchange( volatile int32* Dest, int32 Value )
    {
        return _InterlockedExchange( (LONG*)Dest, Value );
    }

    static FORCEINLINE int64 InterlockedExchange( volatile int64* Dest, int64 Value )
    {
        return _InterlockedExchange64( Dest, Value );
    }
};

#endif