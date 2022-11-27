#pragma once
#include "Core/Core.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING


struct FGenericInterlocked
{
    /**
     * @brief        - Adds two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedAdd(volatile int8* Target, int8  Value) { return 0; }

    /**
     * @brief        - Adds two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedAdd(volatile int16* Target, int16 Value) { return 0; }
    
    /**
     * @brief        - Adds two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedAdd(volatile int32* Target, int32 Value) { return 0; }
    
    /**
     * @brief        - Adds two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedAdd(volatile int64* Target, int64 Value) { return 0; }

    /**
     * @brief        - Subtract two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedSub(volatile int8* Target, int8  Value) { return 0; }
    
    /**
     * @brief        - Subtract two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedSub(volatile int16* Target, int16 Value) { return 0; }
    
    /**
     * @brief        - Subtract two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedSub(volatile int32* Target, int32 Value) { return 0; }
    
    /**
     * @brief        - Subtract two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedSub(volatile int64* Target, int64 Value) { return 0; }

    /**
     * @brief        - Bitwise AND two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedAnd(volatile int8* Target, int8 Value) { return 0; }
    
    /**
     * @brief        - Bitwise AND two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedAnd(volatile int16* Target, int16 Value) { return 0; }
    
    /**
     * @brief        - Bitwise AND two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedAnd(volatile int32* Target, int32 Value) { return 0; }
    
    /**
     * @brief        - Bitwise AND two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedAnd(volatile int64* Target, int64 Value) { return 0; }

    /**
     * @brief        - Bitwise OR two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedOr(volatile int8* Target, int8 Value) { return 0; }
    
    /**
     * @brief        - Bitwise OR two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedOr(volatile int16* Target, int16 Value) { return 0; }
    
    /**
     * @brief        - Bitwise OR two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedOr(volatile int32* Target, int32 Value) { return 0; }
    
    /**
     * @brief        - Bitwise OR two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedOr(volatile int64* Target, int64 Value) { return 0; }

    /**
     * @brief        - Bitwise XOR two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedXor(volatile int8* Target, int8 Value) { return 0; }
    
    /**
     * @brief        - Bitwise XOR two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedXor(volatile int16* Target, int16 Value) { return 0; }
    
    /**
     * @brief        - Bitwise XOR two integers atomically and return original value
     * @param Target - Pointer to first, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedXor(volatile int32* Target, int32 Value) { return 0; }
    
    /**
     * @brief        - Bitwise XOR two integers atomically and return original value
     * @param Target - Pointer to first operand, which is also used to store the result
     * @param Value  - Second operand
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedXor(volatile int64* Target, int64 Value) { return 0; }
    
    /**
     * @brief        - Increment an integer atomically and return new value
     * @param Addend - Pointer to integer to increment, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int8 InterlockedIncrement(volatile int8* Addend) { return 0; }
    
    /**
     * @brief        - Increment an integer atomically and return new value
     * @param Addend - Pointer to integer to increment, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int16 InterlockedIncrement(volatile int16* Addend) { return 0; }
    
    /**
     * @brief        - Increment an integer atomically and return new value
     * @param Addend - Pointer to integer to increment, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int32 InterlockedIncrement(volatile int32* Addend) { return 0; }
    
    /**
     * @brief        - Increment an integer atomically and return new value
     * @param Addend - Pointer to integer to increment, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int64 InterlockedIncrement(volatile int64* Addend) { return 0; }
    
    /**
     * @brief        - Decrement an integer atomically and return new value
     * @param Addend - Pointer to integer to decrement, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int8 InterlockedDecrement(volatile int8* Addend) { return 0; }
    
    /**
     * @brief        - Decrement an integer atomically and return new value
     * @param Addend - Pointer to integer to decrement, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int16 InterlockedDecrement(volatile int16* Addend) { return 0; }
    
    /**
     * @brief        - Decrement an integer atomically and return new value
     * @param Addend - Pointer to integer to decrement, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int32 InterlockedDecrement(volatile int32* Addend) { return 0; }
    
    /**
     * @brief        - Decrement an integer atomically and return new value
     * @param Addend - Pointer to integer to decrement, which is also used to store the result
     * @return       - Returns the new value
     */
    static FORCEINLINE int64 InterlockedDecrement(volatile int64* Addend) { return 0; }

    /**
     * @brief           - Compares two values, if equal then one of them gets exchanged. Returns the original value.
     * @param Target    - Pointer to destination
     * @param Exchange  - Value to exchange
     * @param Comparand - Value to compare against Target
     * @return          - Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedCompareExchange(volatile int8* Target, int8 Exchange, int8 Comparand) { return 0; }
    
    /**
     * @brief           - Compares two values, if equal then one of them gets exchanged. Returns the original value.
     * @param Target    - Pointer to destination
     * @param Exchange  - Value to exchange
     * @param Comparand - Value to compare against Target
     * @return          - Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedCompareExchange(volatile int16* Target, int16 Exchange, int16 Comparand) { return 0; }
    
    /**
     * @brief           - Compares two values, if equal then one of them gets exchanged. Returns the original value.
     * @param Target    - Pointer to destination
     * @param Exchange  - Value to exchange
     * @param Comparand - Value to compare against Target
     * @return          - Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* Target, int32 Exchange, int32 Comparand) { return 0; }
    
    /**
     * @brief           - Compares two values, if equal then one of them gets exchanged. Returns the original value.
     * @param Target    - Pointer to destination
     * @param Exchange  - Value to exchange
     * @param Comparand - Value to compare against Target
     * @return          - Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* Target, int64 Exchange, int64 Comparand) { return 0; }

    /**
     * @brief        - Sets an integer to a specified value atomically and returns the original value.
     * @param Target - Pointer to target
     * @param Value  - New value to for the target
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedExchange(volatile int8* Target, int8 Value) { return 0; }
    
    /**
     * @brief        - Sets an integer to a specified value atomically and returns the original value.
     * @param Target - Pointer to target
     * @param Value  - New value to for the target
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedExchange(volatile int16* Target, int16 Value) { return 0; }
    
    /**
     * @brief        - Sets an integer to a specified value atomically and returns the original value.
     * @param Target - Pointer to target
     * @param Value  - New value to for the target
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedExchange(volatile int32* Target, int32 Value) { return 0; }
    
    /**
     * @brief        - Sets an integer to a specified value atomically and returns the original value.
     * @param Target - Pointer to target
     * @param Value  - New value to for the target
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedExchange(volatile int64* Target, int64 Value) { return 0; }

    /**
     * @brief        - Sets a pointer to a specified value atomically and returns the original value.
     * @param Target - Pointer to target
     * @param Value  - New value to for the target
     * @return       - Returns the original value of Target
     */
    static FORCEINLINE void* InterlockedExchangePointer(void* volatile* Target, void* Value) { return nullptr; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
