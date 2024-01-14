#pragma once
#include "Core/Generic/GenericPlatformAtomic.h"

struct FMacPlatformAtomic final : public FGenericPlatformAtomic
{
    static FORCEINLINE int8 Read(volatile const int8* Source)
    {
        int8 Result;
        __atomic_load((volatile int8*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    static FORCEINLINE int16 Read(volatile const int16* Source)
    {
        int16 Result;
        __atomic_load((volatile int16*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    static FORCEINLINE int32 Read(volatile const int32* Source)
    {
        int32 Result;
        __atomic_load((volatile int32*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    static FORCEINLINE int64 Read(volatile const int64* Source)
    {
        int64 Result;
        __atomic_load((volatile int64*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    static FORCEINLINE int8 RelaxedRead(volatile const int8* Source)
    {
        int8 Result;
        __atomic_load((volatile int8*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    static FORCEINLINE int16 RelaxedRead(volatile const int16* Source)
    {
        int16 Result;
        __atomic_load((volatile int16*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    static FORCEINLINE int32 RelaxedRead(volatile const int32* Source)
    {
        int32 Result;
        __atomic_load((volatile int32*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    static FORCEINLINE int64 RelaxedRead(volatile const int64* Source)
    {
        int64 Result;
        __atomic_load((volatile int64*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    static FORCEINLINE void Store(volatile int8* Dest, int8 Value)
    {
        __atomic_store((volatile int8*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    static FORCEINLINE void Store(volatile int16* Dest, int16 Value )
    {
        __atomic_store((volatile int16*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    static FORCEINLINE void Store(volatile int32* Dest, int32 Value )
    {
        __atomic_store((volatile int32*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    static FORCEINLINE void Store(volatile int64* Dest, int64 Value )
    {
        __atomic_store((volatile int64*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    static FORCEINLINE void RelaxedStore(volatile int8* Dest, int8 Value)
    {
        __atomic_store((volatile int8*)Dest, &Value, __ATOMIC_RELAXED);
    }

    static FORCEINLINE void RelaxedStore(volatile int16* Dest, int16 Value )
    {
        __atomic_store((volatile int16*)Dest, &Value, __ATOMIC_RELAXED);
    }

    static FORCEINLINE void RelaxedStore(volatile int32* Dest, int32 Value )
    {
        __atomic_store((volatile int32*)Dest, &Value, __ATOMIC_RELAXED);
    }

    static FORCEINLINE void RelaxedStore(volatile int64* Dest, int64 Value )
    {
        __atomic_store((volatile int64*)Dest, &Value, __ATOMIC_RELAXED);
    }
};
