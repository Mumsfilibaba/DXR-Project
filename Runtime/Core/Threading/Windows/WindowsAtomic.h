#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Interface/PlatformAtomic.h"
#include "Core/Threading/Platform/PlatformInterlocked.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows-specific interface for atomic operations

class CWindowsAtomic final : public CPlatformAtomic
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Read: Perform a atomic read. All loads and stores are synced

    static FORCEINLINE int8 Read(volatile const int8* Source)
    {
        return PlatformInterlocked::InterlockedCompareExchange((int8*)Source, 0, 0);
    }

    static FORCEINLINE int16 Read(volatile const int16* Source)
    {
        return PlatformInterlocked::InterlockedCompareExchange((int16*)Source, 0, 0);
    }

    static FORCEINLINE int32 Read(volatile const int32* Source)
    {
        return PlatformInterlocked::InterlockedCompareExchange((int32*)Source, 0, 0);
    }

    static FORCEINLINE int64 Read(volatile const int64* Source)
    {
        return PlatformInterlocked::InterlockedCompareExchange((int64*)Source, 0, 0);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // RelaxedRead: Performs a relaxed atomic read. No barriers or synchronization takes place. Only guaranteed to be atomic.

    static FORCEINLINE int8 RelaxedRead(volatile const int8* Source)
    {
        return *Source;
    }

    static FORCEINLINE int16 RelaxedRead(volatile const int16* Source)
    {
        return *Source;
    }

    static FORCEINLINE int32 RelaxedRead(volatile const int32* Source)
    {
        return *Source;
    }

    static FORCEINLINE int64 RelaxedRead(volatile const int64* Source)
    {
        return *Source;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Store: Perform a atomic Store. All loads and stores are synced.

    static FORCEINLINE void Store(volatile int8* Dest, int8 Value)
    {
        PlatformInterlocked::InterlockedExchange((int8*)Dest, Value);
    }

    static FORCEINLINE void Store(volatile int16* Dest, int16 Value)
    {
        PlatformInterlocked::InterlockedExchange((int16*)Dest, Value);
    }

    static FORCEINLINE void Store(volatile int32* Dest, int32 Value)
    {
        PlatformInterlocked::InterlockedExchange((int32*)Dest, Value);
    }

    static FORCEINLINE void Store(volatile int64* Dest, int64 Value)
    {
        PlatformInterlocked::InterlockedExchange((int64*)Dest, Value);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // RelaxedStore: Perform a relaxed atomic Store. No barriers or synchronization takes place. Only guaranteed to be atomic.

    static FORCEINLINE void RelaxedStore(volatile int8* Dest, int8 Value)
    {
        *Dest = Value;
    }

    static FORCEINLINE void RelaxedStore(volatile int16* Dest, int16 Value)
    {
        *Dest = Value;
    }

    static FORCEINLINE void RelaxedStore(volatile int32* Dest, int32 Value)
    {
        *Dest = Value;
    }

    static FORCEINLINE void RelaxedStore(volatile int64* Dest, int64 Value)
    {
        *Dest = Value;
    }
};

#endif