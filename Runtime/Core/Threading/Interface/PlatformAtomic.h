#pragma once
#include "Core/Core.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for atomic operations

class CPlatformAtomic
{
public:

    /**
     * Reads a value atomically. All memory loads and stores are synced.
     * 
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int8  Read(volatile const int8* Source) { return 0; }

    /**
     * Reads a value atomically. All memory loads and stores are synced.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int16 Read(volatile const int16* Source) { return 0; }
    
    /**
     * Reads a value atomically. All memory loads and stores are synced.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int32 Read(volatile const int32* Source) { return 0; }
    
    /**
     * Reads a value atomically. All memory loads and stores are synced.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int64 Read(volatile const int64* Source) { return 0; }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int8  RelaxedRead(volatile const int8* Source) { return 0; }
    
    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int16 RelaxedRead(volatile const int16* Source) { return 0; }
    
    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int32 RelaxedRead(volatile const int32* Source) { return 0; }
    
    /**
     * Performs a relaxed atomic read. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Source: Pointer to variable to read from
     * @return: Returns the read value
     */
    static FORCEINLINE int64 RelaxedRead(volatile const int64* Source) { return 0; }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    
    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile const int8* Dest, int8  Value) { }
    
    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile const int16* Dest, int16 Value) { }
    
    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile const int32* Dest, int32 Value) { }
    
    /**
     * Stores a value atomically. All memory-loads and stores are synced.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void Store(volatile const int64* Dest, int64 Value) { }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile const int8* Dest, int8  Value) { }
    
    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile const int16* Dest, int16 Value) { }
    
    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile const int32* Dest, int32 Value) { }
    
    /**
     * Performs a relaxed atomic store. No memory-barriers or synchronization takes place. Only guaranteed to be atomic.
     *
     * @param Dest: Pointer to variable to store value in
     * @param Value: Value to store
     */
    static FORCEINLINE void RelaxedStore(volatile const int64* Dest, int64 Value) { }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
