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
     * Default constructor
     */
    FORCEINLINE TAtomicInt() noexcept
        : Value(0)
    { }

    /**
     * Copy-constructor
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
     * Construct with a initial value
     * 
     * @param InValue: Initial value
     */
    FORCEINLINE TAtomicInt(T InValue) noexcept
        : Value(InValue)
    { }

    ~TAtomicInt() = default;

    /**
     * Atomically increments the integer
     * 
     * @return: Returns the new value
     */
    FORCEINLINE T Increment() noexcept
    {
        return PlatformInterlocked::InterlockedIncrement(&Value);
    }

    /**
     * Atomically decrements the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Decrement() noexcept
    {
        return PlatformInterlocked::InterlockedDecrement(&Value);
    }

    /**
     * Atomically adds a value to the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Add(T Rhs) noexcept
    {
        PlatformInterlocked::InterlockedAdd(&Value, Rhs);
        return Value;
    }

    /**
     * Atomically subtracts a value from the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Subtract(T Rhs) noexcept
    {
        PlatformInterlocked::InterlockedSub(&Value, Rhs);
        return Value;
    }

    /**
     * Performs a bitwise AND atomically with a value and the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T And(T Rhs) noexcept
    {
        PlatformInterlocked::InterlockedAnd(&Value, Rhs);
        return Value;
    }

    /**
     * Performs a bitwise OR atomically with a value and the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Or(T Rhs) noexcept
    {
        PlatformInterlocked::InterlockedOr(&Value, Rhs);
        return Value;
    }

    /**
     * Performs a bitwise XOR atomically with a value and the integer
     *
     * @return: Returns the new value
     */
    FORCEINLINE T Xor(T Rhs) noexcept
    {
        PlatformInterlocked::InterlockedXor(&Value, Rhs);
        return Value;
    }

    /**
     * Retrieves the integer atomically and makes sure that all prior accesses has completed 
     *
     * @return: Returns the stored value
     */
    FORCEINLINE T Load() const noexcept
    {
        return PlatformAtomic::Read(&Value);
    }

    /**
     * Retrieves the integer atomically without making sure that all prior accesses has completed
     *
     * @return: Returns the stored value
     */
    FORCEINLINE T RelaxedLoad() const noexcept
    {
        return PlatformAtomic::RelaxedRead(&Value);
    }

    /**
     * Exchanges the integer to a new value and return the original value
     * 
     * @param InValue: Value to exchange
     * @return: Returns the original value
     */
    FORCEINLINE T Exchange(T InValue) noexcept
    {
        return PlatformInterlocked::InterlockedExchange(&Value, InValue);
    }

    /**
     * Stores a new integer atomically and makes sure that all prior accesses has completed
     *
     * @param InValue: New value to store
     */
    FORCEINLINE void Store(T InValue) noexcept
    {
        PlatformAtomic::Store(&Value, InValue);
    }

    /**
     * Stores a new integer atomically without making sure that all prior accesses has completed
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
     * Copy-assignment operator
     * 
     * @param Rhs: Value to copy
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TAtomicInt& operator=(const TAtomicInt& Rhs)
    {
        T TempInteger = Rhs.Load();
        Store(TempInteger);
        return *this;
    }

    /**
     * Assign a new value
     *
     * @param Rhs: Value to assign
     * @return: Returns a reference to this instance
     */
    FORCEINLINE T operator=(T Rhs) noexcept
    {
        return Store(Rhs);
    }

    /**
     * Increment the integer with one
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
     * Increment the integer with one
     *
     * @return: Returns a the new value
     */
    FORCEINLINE T operator++() noexcept
    {
        return Increment();
    }

    /**
     * Decrement the integer with one
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
     * Decrement the integer with one
     *
     * @return: Returns a the new value
     */
    FORCEINLINE T operator--() noexcept
    {
        return Decrement();
    }

    /**
     * Add a value
     * 
     * @param Rhs: Value to add to the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator+=(T Rhs) noexcept
    {
        return Add(Rhs);
    }

    /**
     * Subtract a value
     *
     * @param Rhs: Value to subtract to the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator-=(T Rhs) noexcept
    {
        return Subtract(Rhs);
    }

    /**
     * Bitwise AND with a value
     *
     * @param Rhs: Value to AND with the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator&=(T Rhs) noexcept
    {
        return And(Rhs);
    }

    /**
     * Bitwise OR with a value
     *
     * @param Rhs: Value to OR with the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator|=(T Rhs) noexcept
    {
        return Or(Rhs);
    }

    /**
     * Bitwise XOR with a value
     *
     * @param Rhs: Value to XOR with the integer
     * @return: Returns a the new value
     */
    FORCEINLINE T operator^=(T Rhs) noexcept
    {
        return Xor(Rhs);
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
