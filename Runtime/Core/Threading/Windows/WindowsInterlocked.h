#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Interface/PlatformInterlocked.h"

#include "Core/Windows/Windows.h"

#include <intrin.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows-specific interface for interlocked mathematical operations

class CWindowsInterlocked : public CPlatformInterlocked
{
public:

    // See: https://docs.microsoft.com/en-us/cpp/intrinsics/interlockedexchangeadd-intrinsic-functions?view=msvc-160
    //      https://docs.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-interlockedincrement16

    static_assert(sizeof(int32) == sizeof(long) && alignof(int32) == alignof(long), "int32 must have the same size and alignment as long");

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Adds two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedAdd(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8(static_cast<volatile char*>(Dest), static_cast<char>(Value)));
    }

    /**
     * @brief: Adds two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedAdd(volatile int16* Dest, int16 Value)
    {
        return static_cast<int16>(::_InterlockedExchangeAdd16(static_cast<volatile short*>(Dest), static_cast<short>(Value)));
    }

    /**
     * @brief: Adds two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedAdd(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(::_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value)));
    }

    /**
     * @brief: Adds two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedAdd(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(::_InterlockedExchangeAdd64(static_cast<volatile long long*>(Dest), static_cast<long long>(Value)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedSub(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8(static_cast<volatile char*>(Dest), -static_cast<char>(Value)));
    }

    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedSub(volatile int16* Dest, int16 Value)
    {
        return static_cast<int16>(::_InterlockedExchangeAdd16(static_cast<volatile short*>(Dest), -static_cast<short>(Value)));
    }

    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedSub(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(::_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(Dest), -static_cast<long>(Value)));
    }

    /**
     * @brief: Subtract two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedSub(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(::_InterlockedExchangeAdd64(static_cast<volatile long long*>(Dest), -static_cast<long long>(Value)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedAnd(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(::_InterlockedAnd8(static_cast<volatile char*>(Dest), static_cast<char>(Value)));
    }

    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedAnd(volatile int16* Dest, int16 Value)
    {
        return static_cast<int8>(::_InterlockedAnd16(static_cast<volatile short*>(Dest), static_cast<short>(Value)));;
    }

    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedAnd(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(::_InterlockedAnd(reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value)));
    }

    /**
     * @brief: Bitwise AND two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedAnd(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(::_InterlockedAnd64(static_cast<volatile long long*>(Dest), static_cast<long long>(Value)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedOr(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(::_InterlockedOr8(static_cast<volatile char*>(Dest), static_cast<char>(Value)));
    }

    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedOr(volatile int16* Dest, int16 Value)
    {
        return static_cast<int8>(::_InterlockedOr16(static_cast<volatile short*>(Dest), static_cast<short>(Value)));;
    }

    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedOr(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(::_InterlockedOr(reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value)));
    }

    /**
     * @brief: Bitwise OR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedOr(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(::_InterlockedOr64(static_cast<volatile long long*>(Dest), static_cast<long long>(Value)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int8 InterlockedXor(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(::_InterlockedXor8(static_cast<volatile char*>(Dest), static_cast<char>(Value)));
    }

    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int16 InterlockedXor(volatile int16* Dest, int16 Value)
    {
        return static_cast<int8>(::_InterlockedXor16(static_cast<volatile short*>(Dest), static_cast<short>(Value)));
    }

    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int32 InterlockedXor(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(::_InterlockedXor(reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value)));
    }

    /**
     * @brief: Bitwise XOR two integers atomically and return original value
     *
     * @param LHS: Pointer to first, which is also used to store the result
     * @param RHS: Second operand
     * @return: Returns the original value of LHS
     */
    static FORCEINLINE int64 InterlockedXor(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(::_InterlockedXor64(static_cast<volatile long long*>(Dest), static_cast<long long>(Value)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int8 InterlockedIncrement(volatile int8* Dest)
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8(static_cast<volatile char*>(Dest), 1)) + 1;
    }

    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int16 InterlockedIncrement(volatile int16* Dest)
    {
        return static_cast<int16>(::_InterlockedIncrement16(static_cast<volatile short*>(Dest)));
    }

    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int32 InterlockedIncrement(volatile int32* Dest)
    {
        return static_cast<int32>(::_InterlockedIncrement(reinterpret_cast<volatile long*>(Dest)));
    }

    /**
     * @brief: Increment an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to increment, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int64 InterlockedIncrement(volatile int64* Dest)
    {
        return static_cast<int64>(::_InterlockedIncrement64(static_cast<volatile long long*>(Dest)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int8 InterlockedDecrement(volatile int8* Dest)
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8(static_cast<volatile char*>(Dest), -1)) - 1;
    }

    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int16 InterlockedDecrement(volatile int16* Dest)
    {
        return static_cast<int16>(::_InterlockedDecrement16(static_cast<volatile short*>(Dest)));
    }

    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int32 InterlockedDecrement(volatile int32* Dest)
    {
        return static_cast<int32>(::_InterlockedDecrement(reinterpret_cast<volatile long*>(Dest)));
    }

    /**
     * @brief: Decrement an integer atomically and return new value
     *
     * @param Addend: Pointer to integer to decrement, which is also used to store the result
     * @return: Returns the new value
     */
    static FORCEINLINE int64 InterlockedDecrement(volatile int64* Dest)
    {
        return static_cast<int64>(::_InterlockedDecrement64(static_cast<volatile long long*>(Dest)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int8 InterlockedCompareExchange(volatile int8* Dest, int8 Exchange, int8 Comparand)
    {
        return static_cast<int8>(::_InterlockedCompareExchange8(static_cast<volatile char*>(Dest), static_cast<char>(Exchange), static_cast<char>(Comparand)));
    }

    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int16 InterlockedCompareExchange(volatile int16* Dest, int16 Exchange, int16 Comparand)
    {
        return static_cast<int16>(::_InterlockedCompareExchange16(static_cast<volatile short*>(Dest), static_cast<short>(Exchange), static_cast<short>(Comparand)));
    }

    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* Dest, int32 Exchange, int32 Comparand)
    {
        return static_cast<int32>(::_InterlockedCompareExchange(reinterpret_cast<volatile long*>(Dest), static_cast<long>(Exchange), static_cast<long>(Comparand)));
    }

    /**
     * @brief: Compares two values, if equal then one of them gets exchanged. Returns the original value.
     *
     * @param Dest: Pointer to destination
     * @param Exchange: Value to exchange
     * @param Comparand: Value to compare against Dest
     * @return: Returns the original value of Dest
     */
    static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* Dest, int64 Exchange, int64 Comparand)
    {
        return static_cast<int64>(::_InterlockedCompareExchange64(static_cast<volatile long long*>(Dest), static_cast<long>(Exchange), static_cast<long>(Comparand)));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/

    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int8 InterlockedExchange(volatile int8* Dest, int8 Value)
    {
        return static_cast<int8>(::_InterlockedExchange8(static_cast<volatile char*>(Dest), static_cast<char>(Value)));
    }

    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int16 InterlockedExchange(volatile int16* Dest, int16 Value)
    {
        return static_cast<int8>(::_InterlockedExchange16(static_cast<volatile short*>(Dest), static_cast<short>(Value)));
    }

    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int32 InterlockedExchange(volatile int32* Dest, int32 Value)
    {
        return static_cast<int32>(::_InterlockedExchange(reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value)));
    }

    /**
     * @brief: Sets an integer to a specified value atomically and returns the original value.
     *
     * @param Target: Pointer to target
     * @param Value: New value to for the target
     * @return: Returns the original value of Target
     */
    static FORCEINLINE int64 InterlockedExchange(volatile int64* Dest, int64 Value)
    {
        return static_cast<int64>(::_InterlockedExchange64(static_cast<volatile long long*>(Dest), static_cast<long long>(Value)));
    }
};

#endif