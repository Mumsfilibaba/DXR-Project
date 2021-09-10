#pragma once
#include "Core.h"

#ifdef InterlockedAdd
#undef InterlockedAdd
#endif

#ifdef InterlockedIncrement
#undef InterlockedIncrement
#endif

#ifdef InterlockedDecrement
#undef InterlockedDecrement
#endif

#ifdef InterlockedCompareExchange
#undef InterlockedCompareExchange
#endif

#ifdef InterlockedExchange
#undef InterlockedExchange
#endif

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Base atomic operations, should not be used directly */
class CGenericAtomic
{
public:

    /* InterlockedAdd: Adds value and return original value of Dest */

    static FORCEINLINE int8 InterlockedAdd( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedAdd( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedAdd( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedAdd( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* InterlockedSub: Subtracts value and return original value of Dest */

    static FORCEINLINE int8 InterlockedSub( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedSub( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedSub( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedSub( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* InterlockedAnd: ANDs the Dest with Value and returns the original value */

    static FORCEINLINE int8 InterlockedAnd( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedAnd( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedAnd( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedAnd( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* InterlockedOr: ORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 InterlockedOr( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedOr( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedOr( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedOr( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* InterlockedXor: XORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 InterlockedXor( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedXor( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedXor( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedXor( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* InterlockedIncrement: Increments destination and returns the new value */

    static FORCEINLINE int8 InterlockedIncrement( volatile int8* Dest )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedIncrement( volatile int16* Dest )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedIncrement( volatile int32* Dest )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedIncrement( volatile int64* Dest )
    {
        return 0;
    }

    /* InterlockedDecrement: Decrements destination and returns the new value */

    static FORCEINLINE int8 InterlockedDecrement( volatile int8* Dest )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedDecrement( volatile int16* Dest )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedDecrement( volatile int32* Dest )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedDecrement( volatile int64* Dest )
    {
        return 0;
    }

    /* InterlockedCompareExchange: Compares Dest with Comparand, if equal Exchange gets stored in Dest. Returns the orignal value. */

    static FORCEINLINE int8 InterlockedCompareExchange( volatile int8* Dest, int8 Exchange, int8 Comparand )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedCompareExchange( volatile int16* Dest, int16 Exchange, int16 Comparand )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedCompareExchange( volatile int32* Dest, int32 Exchange, int32 Comparand )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedCompareExchange( volatile int64* Dest, int64 Exchange, int64 Comparand )
    {
        return 0;
    }

    /* InterlockedExchange: Stores Value in Dest, and returns original value */

    static FORCEINLINE int8 InterlockedExchange( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 InterlockedExchange( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 InterlockedExchange( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 InterlockedExchange( volatile int64* Dest, int64 Value )
    {
        return 0;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
