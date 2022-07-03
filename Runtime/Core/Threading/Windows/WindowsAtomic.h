#pragma once
#include "Core/Threading/Generic/GenericAtomic.h"
#include "Core/Threading/Platform/PlatformInterlocked.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsAtomic

class FWindowsAtomic final : public FGenericAtomic
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericAtomic Interface

    static FORCEINLINE int8 Read(volatile const int8* Source)
    {
        return FPlatformInterlocked::InterlockedCompareExchange((int8*)Source, 0, 0);
    }

    static FORCEINLINE int16 Read(volatile const int16* Source)
    {
        return FPlatformInterlocked::InterlockedCompareExchange((int16*)Source, 0, 0);
    }

    static FORCEINLINE int32 Read(volatile const int32* Source)
    {
        return FPlatformInterlocked::InterlockedCompareExchange((int32*)Source, 0, 0);
    }

    static FORCEINLINE int64 Read(volatile const int64* Source)
    {
        return FPlatformInterlocked::InterlockedCompareExchange((int64*)Source, 0, 0);
    }

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

    static FORCEINLINE void Store(volatile int8* Dest, int8 Value)
    {
        FPlatformInterlocked::InterlockedExchange((int8*)Dest, Value);
    }

    static FORCEINLINE void Store(volatile int16* Dest, int16 Value)
    {
        FPlatformInterlocked::InterlockedExchange((int16*)Dest, Value);
    }

    static FORCEINLINE void Store(volatile int32* Dest, int32 Value)
    {
        FPlatformInterlocked::InterlockedExchange((int32*)Dest, Value);
    }

    static FORCEINLINE void Store(volatile int64* Dest, int64 Value)
    {
        FPlatformInterlocked::InterlockedExchange((int64*)Dest, Value);
    }

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