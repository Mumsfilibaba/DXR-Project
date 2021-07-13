#pragma once
#include "Windows/Windows.h"

#include "Core/Threading/Generic/GenericAtomic.h"

class WindowsAtomic : public GenericAtomic
{
public:
    FORCEINLINE static int32 InterlockedIncrement( volatile int32* Dest )
    {
        return _InterlockedIncrement( (LONG*)Dest );
    }

    FORCEINLINE static int64 InterlockedIncrement( volatile int64* Dest )
    {
        return _InterlockedIncrement64( Dest );
    }

    FORCEINLINE static int32 InterlockedDecrement( volatile int32* Dest )
    {
        return _InterlockedDecrement( (LONG*)Dest );
    }

    FORCEINLINE static int64 InterlockedDecrement( volatile int64* Dest )
    {
        return _InterlockedDecrement64( Dest );
    }

    FORCEINLINE static int32 InterlockedAdd( volatile int32* Dest, int32 Value )
    {
        return _InlineInterlockedAdd( (LONG*)Dest, Value );
    }

    FORCEINLINE static int64 InterlockedAdd( volatile int64* Dest, int64 Value )
    {
        return _InlineInterlockedAdd64( Dest, Value );
    }

    FORCEINLINE static int32 InterlockedSub( volatile int32* Dest, int32 Value )
    {
        return _InlineInterlockedAdd( (LONG*)Dest, -Value );
    }

    FORCEINLINE static int64 InterlockedSub( volatile int64* Dest, int64 Value )
    {
        return _InlineInterlockedAdd64( Dest, -Value );
    }

    FORCEINLINE static int32 InterlockedCompareExchange( volatile int32* Dest, int32 ExChange, int32 Comparand )
    {
        return _InterlockedCompareExchange( (LONG*)Dest, ExChange, Comparand );
    }

    FORCEINLINE static int64 InterlockedCompareExchange( volatile int64* Dest, int64 ExChange, int64 Comparand )
    {
        return _InterlockedCompareExchange64( Dest, ExChange, Comparand );
    }

    FORCEINLINE static int32 InterlockedExchange( volatile int32* Dest, int32 Value )
    {
        return _InterlockedExchange( (LONG*)Dest, Value );
    }

    FORCEINLINE static int64 InterlockedExchange( volatile int64* Dest, int64 Value )
    {
        return _InterlockedExchange64( Dest, Value );
    }
};