#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Remove Windows.h defines

#if defined(InterlockedAdd)
    #undef InterlockedAdd
#endif

#if defined(InterlockedSub)
    #undef InterlockedSub
#endif

#if defined(InterlockedAnd)
    #undef InterlockedAnd
#endif

#if defined(InterlockedOr)
    #undef InterlockedOr
#endif

#if defined(InterlockedXor)
    #undef InterlockedXor
#endif

#if defined(InterlockedIncrement)
    #undef InterlockedIncrement
#endif

#if defined(InterlockedDecrement)
    #undef InterlockedDecrement
#endif

#if defined(InterlockedCompareExchange)
    #undef InterlockedCompareExchange
#endif

#if defined(InterlockedExchange)
    #undef InterlockedExchange
#endif

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericInterlocked

struct FGenericInterlocked
{
    /**
     * @brief: Adds two integers atomically and return original value
     * 
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedAdd(volatile int8* LHS, int8  RHS) { return 0; }

    /**
     * @brief: Adds two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedAdd(volatile int16* LHS, int16 RHS) { return 0; }
    
    /**
     * @brief: Adds two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedAdd(volatile int32* LHS, int32 RHS) { return 0; }
    
    /**
     * @brief: Adds two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedAdd(volatile int64* LHS, int64 RHS) { return 0; }

    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedSub(volatile int8* LHS, int8  RHS) { return 0; }
    
    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedSub(volatile int16* LHS, int16 RHS) { return 0; }
    
    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedSub(volatile int32* LHS, int32 RHS) { return 0; }
    
    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedSub(volatile int64* LHS, int64 RHS) { return 0; }

    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8  InterlockedAnd(volatile int8* LHS, int8 RHS) { return 0; }
    
    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedAnd(volatile int16* LHS, int16 RHS) { return 0; }
    
    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedAnd(volatile int32* LHS, int32 RHS) { return 0; }
    
    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedAnd(volatile int64* LHS, int64 RHS) { return 0; }

    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8  InterlockedOr(volatile int8* LHS, int8 RHS) { return 0; }
    
    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedOr(volatile int16* LHS, int16 RHS) { return 0; }
    
    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedOr(volatile int32* LHS, int32 RHS) { return 0; }
    
    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedOr(volatile int64* LHS, int64 RHS) { return 0; }

    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedXor(volatile int8* LHS, int8 RHS) { return 0; }
    
    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedXor(volatile int16* LHS, int16 RHS) { return 0; }
    
    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedXor(volatile int32* LHS, int32 RHS) { return 0; }
    
    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first operand, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedXor(volatile int64* LHS, int64 RHS) { return 0; }
    
    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int8 InterlockedIncrement(volatile int8* Addend) { return 0; }
    
    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int16 InterlockedIncrement(volatile int16* Addend) { return 0; }
    
    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int32 InterlockedIncrement(volatile int32* Addend) { return 0; }
    
    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int64 InterlockedIncrement(volatile int64* Addend) { return 0; }
    
    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int8  InterlockedDecrement(volatile int8*  Addend) { return 0; }
    
    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int16 InterlockedDecrement(volatile int16* Addend) { return 0; }
    
    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int32 InterlockedDecrement(volatile int32* Addend) { return 0; }
    
    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int64 InterlockedDecrement(volatile int64* Addend) { return 0; }

    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int8  InterlockedCompareExchange(volatile int8*  Dest, int8  Exchange, int8  Comparand) { return 0; }
    
    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int16 InterlockedCompareExchange(volatile int16* Dest, int16 Exchange, int16 Comparand) { return 0; }
    
    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* Dest, int32 Exchange, int32 Comparand) { return 0; }
    
    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* Dest, int64 Exchange, int64 Comparand) { return 0; }

    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedExchange(volatile int8* Target, int8 Value) { return 0; }
    
    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedExchange(volatile int16* Target, int16 Value) { return 0; }
    
    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedExchange(volatile int32* Target, int32 Value) { return 0; }
    
    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedExchange(volatile int64* Target, int64 Value) { return 0; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif