#pragma once
#include "Core/Core.h"
#include "Core/Templates/IsSigned.h"
#include "Core/Threading/Platform/PlatformInterlocked.h"
#include "Core/Threading/Platform/PlatformAtomic.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Templated integer that support atomic operations

template<typename T>
class TAtomicInt
{
public:

    typedef T Type;

    static_assert(TIsSigned<T>::Value, "AtomicInt only supports signed types");

    /**
     * @brief: Default constructor
     */
    FORCEINLINE TAtomicInt() noexcept
        : Value(0)
    { }

    /**
     * @brief: Copy-constructor
     * 
     * @param Other: Instance to copy
     */
    FORCEINLINE TAtomicInt(const TAtomicInt& Other)
        : Value(0)
    {
        T TempInteger = Other.Load();
        Store(TempInteger);
    }

    /**
     * @brief: Construct with a initial value
     * 
     * @param InValue: Initial value
     */
    FORCEINLINE TAtomicInt(T InValue) noexcept
        : Value(InValue)
    { }

    ~TAtomicInt() = default;

    /**
     * @brief: Atomically increments the integer
     * 
     * @return: Returns the new value
     */
    FORCEINLINE T Increment() noexcept
    {
        return PlatformInterlocked::InterlockedIncrement(&Value);
    }

    /**
     * @brief: Atomically decrements the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Decrement() noexcept
    {
        return PlatformInterlocked::InterlockedDecrement(&Value);
    }

    /**
     * @brief: Atomically adds a value to the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Add(T RHS) noexcept
    {
        PlatformInterlocked::InterlockedAdd(&Value, RHS);
        return Value;
    }

    /**
     * @brief: Atomically subtracts a value from the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Subtract(T RHS) noexcept
    {
        PlatformInterlocked::InterlockedSub(&Value, RHS);
        return Value;
    }

    /**
     * @brief: Performs a bitwise AND atomically with a value and the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T And(T RHS) noexcept
    {
        PlatformInterlocked::InterlockedAnd(&Value, RHS);
        return Value;
    }

    /**
     * @brief: Performs a bitwise OR atomically with a value and the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Or(T RHS) noexcept
    {
        PlatformInterlocked::InterlockedOr(&Value, RHS);
        return Value;
    }

    /**
     * @brief: Performs a bitwise XOR atomically with a value and the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Xor(T RHS) noexcept
    {
        PlatformInterlocked::InterlockedXor(&Value, RHS);
        return Value;
    }

    /**
     * @brief: Retrieves the integer atomically and makes sure that all prior accesses has completed 
     *
     * @return: Returns the stored value
     */
    FORCEINLINE T Load() const noexcept
    {
        return PlatformAtomic::Read(&Value);
    }

    /**
     * @brief: Retrieves the integer atomically without making sure that all prior accesses has completed
     *
     * @return: Returns the stored value
     */
    FORCEINLINE T RelaxedLoad() const noexcept
    {
        return PlatformAtomic::RelaxedRead(&Value);
    }

    /**
     * @brief: Exchanges the integer to a new value and return the original value
     * 
     * @param InValue: Value to exchange
     * @return: Returns the original value
     */
    FORCEINLINE T Exchange(T InValue) noexcept
    {
        return PlatformInterlocked::InterlockedExchange(&Value, InValue);
    }

    /**
     * @brief: Stores a new integer atomically and makes sure that all prior accesses has completed
     *
     * @param InValue: New value to store
     */
    FORCEINLINE void Store(T InValue) noexcept
    {
        PlatformAtomic::Store(&Value, InValue);
    }

    /**
     * @brief: Stores a new integer atomically without making sure that all prior accesses has completed
     *
     * @param InValue: New value to store
     */
    FORCEINLINE void RelaxedStore(T InValue) noexcept
    {
        PlatformAtomic::RelaxedStore(&Value, InValue);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Operators

    /**
     * @brief: Copy-assignment operator
     * 
     * @param RHS: Value to copy
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TAtomicInt& operator=(const TAtomicInt& RHS)
    {
        T TempInteger = RHS.Load();
        Store(TempInteger);
        return *this;
    }

    /**
     * @brief: Assign a new value
     *
     * @param RHS: Value to assign
     * @return: Returns a reference to this instance
     */
    FORCEINLINE T operator=(T RHS) noexcept
    {
        return Store(RHS);
    }

    /**
     * @brief: Increment the integer with one
     *
     * @return: Returns a the previous value
     */
    FORCEINLINE T operator++(int32) noexcept
    {
        T TempValue = Load();
        Increment();
        return TempValue;
    }

    /**
     * @brief: Increment the integer with one
     *
     * @return: Returns a the new value
     */
    FORCEINLINE T operator++() noexcept
    {
        return Increment();
    }

    /**
     * @brief: Decrement the integer with one
     *
     * @return: Returns a the previous value
     */
    FORCEINLINE T operator--(int32) noexcept
    {
        T TempValue = Load();
        Decrement();
        return TempValue;
    }

    /**
     * @brief: Decrement the integer with one
     *
     * @return: Returns a the new value
     */
    FORCEINLINE T operator--() noexcept
    {
        return Decrement();
    }

    /**
     * @brief: Add a value
     * 
     * @param RHS: Value to add to the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator+=(T RHS) noexcept
    {
        return Add(RHS);
    }

    /**
     * @brief: Subtract a value
     *
     * @param RHS: Value to subtract to the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator-=(T RHS) noexcept
    {
        return Subtract(RHS);
    }

    /**
     * @brief: Bitwise AND with a value
     *
     * @param RHS: Value to AND with the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator&=(T RHS) noexcept
    {
        return And(RHS);
    }

    /**
     * @brief: Bitwise OR with a value
     *
     * @param RHS: Value to OR with the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator|=(T RHS) noexcept
    {
        return Or(RHS);
    }

    /**
     * @brief: Bitwise XOR with a value
     *
     * @param RHS: Value to XOR with the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator^=(T RHS) noexcept
    {
        return Xor(RHS);
    }

private:
    mutable volatile T Value;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

typedef TAtomicInt<int8>  AtomicInt8;
typedef TAtomicInt<int16> AtomicInt16;
typedef TAtomicInt<int32> AtomicInt32;
typedef TAtomicInt<int64> AtomicInt64;
