#pragma once

#if PLATFORM_MACOS
#include "Core/Threading/Interface/PlatformInterlocked.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation of interlocked math operations

class CMacInterlocked : public CPlatformInterlocked
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Adds two integers atomically and return original value
     * 
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedAdd(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(__sync_fetch_and_add(Dest, Value));
    }

    /**
     * Adds two integers atomically and return original value
     * 
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedAdd(volatile int16* Dest, int16 Value)
    {
        return static_cast<int16>(__sync_fetch_and_add(Dest, Value));
    }

    /**
     * Adds two integers atomically and return original value
     * 
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedAdd(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(__sync_fetch_and_add(Dest, Value));
    }

    /**
     * Adds two integers atomically and return original value
     * 
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedAdd(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(__sync_fetch_and_add(Dest, Value));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedSub(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(__sync_fetch_and_sub(Dest, Value));
    }

    /**
     * Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedSub(volatile int16* Dest, int16 Value)
    {
        return static_cast<int16>(__sync_fetch_and_sub(Dest, Value));
    }

    /**
     * Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedSub(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(__sync_fetch_and_sub(Dest, Value));
    }

    /**
     * Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedSub(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(__sync_fetch_and_sub(Dest, Value));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedAnd(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(__sync_fetch_and_and(Dest, Value));
    }

    /**
     * Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedAnd(volatile int16* Dest, int16 Value)
    {
        return static_cast<int16>(__sync_fetch_and_and(Dest, Value));
    }

    /**
     * Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedAnd(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(__sync_fetch_and_and(Dest, Value));
    }

    /**
     * Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedAnd(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(__sync_fetch_and_and(Dest, Value));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedOr(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(__sync_fetch_and_or(Dest, Value));
    }

    /**
     * Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedOr(volatile int16* Dest, int16 Value)
    {
        return static_cast<int16>(__sync_fetch_and_or(Dest, Value));
    }

    /**
     * Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedOr(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(__sync_fetch_and_or(Dest, Value));
    }

    /**
     * Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedOr(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(__sync_fetch_and_or(Dest, Value));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedXor(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(__sync_fetch_and_xor(Dest, Value));
    }

    /**
     * Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedXor(volatile int16* Dest, int16 Value)
    {
        return static_cast<int16>(__sync_fetch_and_xor(Dest, Value));
    }

    /**
     * Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedXor(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(__sync_fetch_and_xor(Dest, Value));
    }

    /**
     * Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedXor(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(__sync_fetch_and_xor(Dest, Value));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int8 InterlockedIncrement(volatile int8* Dest)
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int8>(__sync_fetch_and_add(Dest, 1)) + 1;
    }

    /**
     * Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int16 InterlockedIncrement(volatile int16* Dest)
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int16>(__sync_fetch_and_add(Dest, 1)) + 1;
    }

    /**
     * Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int32 InterlockedIncrement(volatile int32* Dest)
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int32>(__sync_fetch_and_add(Dest, 1)) + 1;
    }

    /**
     * Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int64 InterlockedIncrement(volatile int64* Dest)
    {
        // No built in increment, add one and then add one to the return value sin the original value is returned
        return static_cast<int64>(__sync_fetch_and_add(Dest, 1)) + 1;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int8 InterlockedDecrement(volatile int8* Dest)
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int8>(__sync_fetch_and_sub(Dest, 1)) - 1;
    }

    /**
     * Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int16 InterlockedDecrement(volatile int16* Dest)
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int16>(__sync_fetch_and_sub(Dest, 1)) - 1;
    }

    /**
     * Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int32 InterlockedDecrement(volatile int32* Dest)
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int32>(__sync_fetch_and_sub(Dest, 1)) - 1;
    }

    /**
     * Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int64 InterlockedDecrement(volatile int64* Dest)
    {
        // No built in decrement, subtract one and then subtract one to the return value sin the original value is returned
        return static_cast<int64>(__sync_fetch_and_sub(Dest, 1)) - 1;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int8 InterlockedCompareExchange(volatile int8* Dest, int8 Exchange, int8 Comparand)
    {
        return __sync_val_compare_and_swap(Dest, Comparand, Exchange);
    }

    /**
     * Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int16 InterlockedCompareExchange(volatile int16* Dest, int16 Exchange, int16 Comparand)
    {
        return __sync_val_compare_and_swap(Dest, Comparand, Exchange);
    }

    /**
     * Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* Dest, int32 Exchange, int32 Comparand)
    {
        return __sync_val_compare_and_swap(Dest, Comparand, Exchange);
    }

    /**
     * Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* Dest, int64 Exchange, int64 Comparand)
    {
        return __sync_val_compare_and_swap(Dest, Comparand, Exchange);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedExchange(volatile int8* Dest, int8 Exchange)
    {
        return __sync_lock_test_and_set(Dest, Exchange);
    }

    /**
     * Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedExchange(volatile int16* Dest, int16 Exchange)
    {
        return __sync_lock_test_and_set(Dest, Exchange);
    }

    /**
     * Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedExchange(volatile int32* Dest, int32 Exchange)
    {
        return __sync_lock_test_and_set(Dest, Exchange);
    }

    /**
     * Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedExchange(volatile int64* Dest, int64 Exchange)
    {
        return __sync_lock_test_and_set(Dest, Exchange);
    }
};

#endif
