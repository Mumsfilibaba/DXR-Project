#pragma once
#include "Core.h"

#ifdef Add
#undef Add
#endif

#ifdef Increment
#undef Increment
#endif

#ifdef Decrement
#undef Decrement
#endif

#ifdef CompareExchange
#undef CompareExchange
#endif

#ifdef Exchange
#undef Exchange
#endif

#ifdef And
#undef And
#endif

#ifdef Or
#undef Or
#endif

#ifdef Xor
#undef Xor
#endif

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Base atomic operations, should not be used directly */
class CGenericInterlocked
{
public:

    /* Add: Adds value and return original value of Dest */

    static FORCEINLINE int8 Add( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 Add( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 Add( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 Add( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* Sub: Subtracts value and return original value of Dest */

    static FORCEINLINE int8 Sub( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 Sub( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 Sub( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 Sub( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* And: ANDs the Dest with Value and returns the original value */

    static FORCEINLINE int8 And( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 And( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 And( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 And( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* Or: ORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 Or( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 Or( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 Or( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 Or( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* Xor: XORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 Xor( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 Xor( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 Xor( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 Xor( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    /* Increment: Increments destination and returns the new value */

    static FORCEINLINE int8 Increment( volatile int8* Dest )
    {
        return 0;
    }

    static FORCEINLINE int16 Increment( volatile int16* Dest )
    {
        return 0;
    }

    static FORCEINLINE int32 Increment( volatile int32* Dest )
    {
        return 0;
    }

    static FORCEINLINE int64 Increment( volatile int64* Dest )
    {
        return 0;
    }

    /* Decrement: Decrements destination and returns the new value */

    static FORCEINLINE int8 Decrement( volatile int8* Dest )
    {
        return 0;
    }

    static FORCEINLINE int16 Decrement( volatile int16* Dest )
    {
        return 0;
    }

    static FORCEINLINE int32 Decrement( volatile int32* Dest )
    {
        return 0;
    }

    static FORCEINLINE int64 Decrement( volatile int64* Dest )
    {
        return 0;
    }

    /* CompareExchange: Compares Dest with Comparand, if equal Exchange gets stored in Dest. Returns the orignal value. */

    static FORCEINLINE int8 CompareExchange( volatile int8* Dest, int8 Exchange, int8 Comparand )
    {
        return 0;
    }

    static FORCEINLINE int16 CompareExchange( volatile int16* Dest, int16 Exchange, int16 Comparand )
    {
        return 0;
    }

    static FORCEINLINE int32 CompareExchange( volatile int32* Dest, int32 Exchange, int32 Comparand )
    {
        return 0;
    }

    static FORCEINLINE int64 CompareExchange( volatile int64* Dest, int64 Exchange, int64 Comparand )
    {
        return 0;
    }

    /* Exchange: Stores Value in Dest, and returns original value */

    static FORCEINLINE int8 Exchange( volatile int8* Dest, int8 Value )
    {
        return 0;
    }

    static FORCEINLINE int16 Exchange( volatile int16* Dest, int16 Value )
    {
        return 0;
    }

    static FORCEINLINE int32 Exchange( volatile int32* Dest, int32 Value )
    {
        return 0;
    }

    static FORCEINLINE int64 Exchange( volatile int64* Dest, int64 Value )
    {
        return 0;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
