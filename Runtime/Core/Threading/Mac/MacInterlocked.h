#pragma once
#include "Core/Threading/Interface/PlatformInterlocked.h"

/* Atomic operations on the MacOS platform */
class CMacInterlocked : public CPlatformInterlocked
{
public:

    /* Add: Adds value and return original value of Dest */

    static FORCEINLINE int8 Add( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(__sync_fetch_and_add( Dest, Value ));
    }

    static FORCEINLINE int16 Add( volatile int16* Dest, int16 Value )
    {
        return static_cast<int16>(__sync_fetch_and_add( Dest, Value ));
    }

    static FORCEINLINE int32 Add( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(__sync_fetch_and_add( Dest, Value ));
    }

    static FORCEINLINE int64 Add( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(__sync_fetch_and_add( Dest, Value ));
    }

    /* Sub: Subtracts value and return original value of Dest */

    static FORCEINLINE int8 Sub( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(__sync_fetch_and_sub( Dest, Value ));
    }

    static FORCEINLINE int16 Sub( volatile int16* Dest, int16 Value )
    {
        return static_cast<int16>(__sync_fetch_and_sub( Dest, Value ));
    }

    static FORCEINLINE int32 Sub( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(__sync_fetch_and_sub( Dest, Value ));
    }

    static FORCEINLINE int64 Sub( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(__sync_fetch_and_sub( Dest, Value ));
    }

    /* And: ANDs the Dest with Value and returns the original value */

    static FORCEINLINE int8 And( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(__sync_fetch_and_and( Dest, Value ));
    }

    static FORCEINLINE int16 And( volatile int16* Dest, int16 Value )
    {
        return static_cast<int16>(__sync_fetch_and_and( Dest, Value ));
    }

    static FORCEINLINE int32 And( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(__sync_fetch_and_and( Dest, Value ));
    }

    static FORCEINLINE int64 And( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(__sync_fetch_and_and( Dest, Value ));
    }

    /* Or: ORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 Or( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(__sync_fetch_and_or( Dest, Value ));
    }

    static FORCEINLINE int16 Or( volatile int16* Dest, int16 Value )
    {
        return static_cast<int16>(__sync_fetch_and_or( Dest, Value ));
    }

    static FORCEINLINE int32 Or( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(__sync_fetch_and_or( Dest, Value ));
    }

    static FORCEINLINE int64 Or( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(__sync_fetch_and_or( Dest, Value ));
    }

    /* Xor: XORs the Dest with Value and returns the original value */

    static FORCEINLINE int8 Xor( volatile int8* Dest, int8 Value )
    {
        return static_cast<int8>(__sync_fetch_and_xor( Dest, Value ));
    }

    static FORCEINLINE int16 Xor( volatile int16* Dest, int16 Value )
    {
        return static_cast<int16>(__sync_fetch_and_xor( Dest, Value ));
    }

    static FORCEINLINE int32 Xor( volatile int32* Dest, int32 Value )
    {
        return static_cast<int32>(__sync_fetch_and_xor( Dest, Value ));
    }

    static FORCEINLINE int64 Xor( volatile int64* Dest, int64 Value )
    {
        return static_cast<int64>(__sync_fetch_and_xor( Dest, Value ));
    }

    /* Increment: Increments destination and returns the new value */

    static FORCEINLINE int8 Increment( volatile int8* Dest )
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int8>(__sync_fetch_and_add( Dest, 1 )) + 1;
    }

    static FORCEINLINE int16 Increment( volatile int16* Dest )
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int16>(__sync_fetch_and_add( Dest, 1 )) + 1;
    }

    static FORCEINLINE int32 Increment( volatile int32* Dest )
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int32>(__sync_fetch_and_add( Dest, 1 )) + 1;
    }

    static FORCEINLINE int64 Increment( volatile int64* Dest )
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int64>(__sync_fetch_and_add( Dest, 1 )) + 1;
    }

    /* Decrement: Decrements destination and returns the new value */

    static FORCEINLINE int8 Decrement( volatile int8* Dest )
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int8>(__sync_fetch_and_sub( Dest, 1 )) - 1;
    }

    static FORCEINLINE int16 Decrement( volatile int16* Dest )
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int16>(__sync_fetch_and_sub( Dest, 1 )) - 1;
    }

    static FORCEINLINE int32 Decrement( volatile int32* Dest )
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int32>(__sync_fetch_and_sub( Dest, 1 )) - 1;
    }

    static FORCEINLINE int64 Decrement( volatile int64* Dest )
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int64>(__sync_fetch_and_sub( Dest, 1 )) - 1;
    }

    /* CompareExchange: Compares Dest with Comparand, if equal Exchange gets stored in Dest. Returns the orignal value. */

    static FORCEINLINE int8 CompareExchange( volatile int8* Dest, int8 Exchange, int8 Comparand )
    {
        return __sync_val_compare_and_swap( Dest, Comparand, Exchange );
    }

    static FORCEINLINE int16 CompareExchange( volatile int16* Dest, int16 Exchange, int16 Comparand )
    {
        return __sync_val_compare_and_swap( Dest, Comparand, Exchange );
    }

    static FORCEINLINE int32 CompareExchange( volatile int32* Dest, int32 Exchange, int32 Comparand )
    {
        return __sync_val_compare_and_swap( Dest, Comparand, Exchange );
    }

    static FORCEINLINE int64 CompareExchange( volatile int64* Dest, int64 Exchange, int64 Comparand )
    {
        return __sync_val_compare_and_swap( Dest, Comparand, Exchange );
    }

    /* Exchange: Stores Value in Dest, and returns original value */

    static FORCEINLINE int8 Exchange( volatile int8* Dest, int8 Exchange )
    {
        return __sync_lock_test_and_set( Dest, Exchange );
    }

    static FORCEINLINE int16 Exchange( volatile int16* Dest, int16 Exchange )
    {
        return __sync_lock_test_and_set( Dest, Exchange );
    }

    static FORCEINLINE int32 Exchange( volatile int32* Dest, int32 Exchange )
    {
        return __sync_lock_test_and_set( Dest, Exchange );
    }

    static FORCEINLINE int64 Exchange( volatile int64* Dest, int64 Exchange )
    {
        return __sync_lock_test_and_set( Dest, Exchange );
    }
};
