#pragma once

#if PLATFORM_MACOS
#include "Core/Threading/Interface/PlatformAtomic.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation for atomic operations

class CMacAtomic final : public CPlatformAtomic
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Reads a value atomically. All memory loads and stores are synced.
     * 
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int8 Read(volatile const int8* Source)
    {
        int8 Result;
        __atomic_load((volatile int8*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    /**
     * Reads a value atomically. All memory loads and stores are synced.
     * 
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int16 Read(volatile const int16* Source)
    {
        int16 Result;
        __atomic_load((volatile int16*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    /**
     * Reads a value atomically. All memory loads and stores are synced.
     * 
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int32 Read(volatile const int32* Source)
    {
        int32 Result;
        __atomic_load((volatile int32*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    /**
     * Reads a value atomically. All memory loads and stores are synced.
     * 
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int64 Read(volatile const int64* Source)
    {
        int64 Result;
        __atomic_load((volatile int64*)Source, &Result, __ATOMIC_SEQ_CST);
        return Result;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int8 RelaxedRead(volatile const int8* Source)
    {
        int8 Result;
        __atomic_load((volatile int8*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int16 RelaxedRead(volatile const int16* Source)
    {
        int16 Result;
        __atomic_load((volatile int16*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int32 RelaxedRead(volatile const int32* Source)
    {
        int32 Result;
        __atomic_load((volatile int32*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int64 RelaxedRead(volatile const int64* Source)
    {
        int64 Result;
        __atomic_load((volatile int64*)Source, &Result, __ATOMIC_RELAXED);
        return Result;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile int8* Dest, int8 Value)
    {
        __atomic_store((volatile int8*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile int16* Dest, int16 Value )
    {
        __atomic_store((volatile int16*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile int32* Dest, int32 Value )
    {
        __atomic_store((volatile int32*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile int64* Dest, int64 Value )
    {
        __atomic_store((volatile int64*)Dest, &Value, __ATOMIC_SEQ_CST);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile int8* Dest, int8 Value)
    {
        __atomic_store((volatile int8*)Dest, &Value, __ATOMIC_RELAXED);
    }

    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile int16* Dest, int16 Value )
    {
        __atomic_store((volatile int16*)Dest, &Value, __ATOMIC_RELAXED);
    }

    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile int32* Dest, int32 Value )
    {
        __atomic_store((volatile int32*)Dest, &Value, __ATOMIC_RELAXED);
    }

    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile int64* Dest, int64 Value )
    {
        __atomic_store((volatile int64*)Dest, &Value, __ATOMIC_RELAXED);
    }
};

#endif