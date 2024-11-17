#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Platform/PlatformInterlocked.h"
#include "Core/Platform/PlatformAtomic.h"

template<typename T>
class TAtomicInt
{
    typedef typename TMakeSigned<typename TRemoveCV<T>::Type>::Type SignedType;
    static_assert(TIsIntegerNotBool<SignedType>::Value, "TAtomicInt only supports integer types");

public:
    typedef typename TRemoveCV<T>::Type IntegerType;
    typedef IntegerType Type;

    static_assert(TIsIntegerNotBool<Type>::Value, "TAtomicInt only supports integer types");

    /** @brief - Default constructor initializes to zero */
    FORCEINLINE TAtomicInt() noexcept
        : Value(0)
    {
    }

    /**
     * @brief       - Copy-constructor
     * @param Other - Instance to copy
     */
    FORCEINLINE TAtomicInt(const TAtomicInt& Other) noexcept
    {
        const IntegerType TempInteger = Other.Load();
        Store(TempInteger);
    }

    /**
     * @brief         - Construct with an initial value
     * @param InValue - Initial value
     */
    FORCEINLINE TAtomicInt(IntegerType InValue) noexcept
        : Value(static_cast<SignedType>(InValue))
    {
    }

    /**
     * @brief  - Atomically increments the integer
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType Increment() noexcept
    {
        return static_cast<IntegerType>(FPlatformInterlocked::InterlockedIncrement(&Value));
    }

    /**
     * @brief  - Atomically decrements the integer
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType Decrement() noexcept
    {
        return static_cast<IntegerType>(FPlatformInterlocked::InterlockedDecrement(&Value));
    }

    /**
     * @brief  - Atomically adds a value to the integer
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType Add(IntegerType RHS) noexcept
    {
        FPlatformInterlocked::InterlockedAdd(&Value, static_cast<SignedType>(RHS));
        return Load();
    }

    /**
     * @brief  - Atomically subtracts a value from the integer
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType Subtract(IntegerType RHS) noexcept
    {
        FPlatformInterlocked::InterlockedAdd(&Value, static_cast<SignedType>(-RHS));
        return Load();
    }

    /**
     * @brief  - Performs a bitwise AND atomically with a value and the integer
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType And(IntegerType RHS) noexcept
    {
        FPlatformInterlocked::InterlockedAnd(&Value, static_cast<SignedType>(RHS));
        return Load();
    }

    /**
     * @brief  - Performs a bitwise OR atomically with a value and the integer
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType Or(IntegerType RHS) noexcept
    {
        FPlatformInterlocked::InterlockedOr(&Value, static_cast<SignedType>(RHS));
        return Load();
    }

    /**
     * @brief  - Performs a bitwise XOR atomically with a value and the integer
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType Xor(IntegerType RHS) noexcept
    {
        FPlatformInterlocked::InterlockedXor(&Value, static_cast<SignedType>(RHS));
        return Load();
    }

    /**
     * @brief  - Retrieves the integer atomically and ensures that all prior accesses have completed 
     * @return - Returns the stored value
     */
    FORCEINLINE IntegerType Load() const noexcept
    {
        return static_cast<IntegerType>(FPlatformAtomic::Read(&Value));
    }

    /**
     * @brief  - Retrieves the integer atomically without ensuring that all prior accesses have completed
     * @return - Returns the stored value
     */
    FORCEINLINE IntegerType RelaxedLoad() const noexcept
    {
        return static_cast<IntegerType>(FPlatformAtomic::RelaxedRead(&Value));
    }

    /**
     * @brief         - Exchanges the integer to a new value and returns the original value
     * @param InValue - Value to exchange
     * @return        - Returns the original value
     */
    FORCEINLINE IntegerType Exchange(IntegerType InValue) noexcept
    {
        const SignedType OldVal = FPlatformInterlocked::InterlockedExchange(&Value, static_cast<SignedType>(InValue));
        return static_cast<IntegerType>(OldVal);
    }

    /**
     * @brief           - Compares and exchanges the integer to a new value if it matches the comparand
     * @param InValue   - Value to exchange
     * @param Comparand - Value to compare against
     * @return          - Returns true if the exchange was successful
     */
    FORCEINLINE bool CompareExchange(IntegerType InValue, IntegerType Comparand) noexcept
    {
        const SignedType Expected = static_cast<SignedType>(Comparand);
        const SignedType Original = FPlatformInterlocked::InterlockedCompareExchange(&Value, static_cast<SignedType>(InValue), Expected);
        return Original == Expected;
    }

    /**
     * @brief         - Stores a new integer atomically and ensures that all prior accesses have completed
     * @param InValue - New value to store
     */
    FORCEINLINE void Store(IntegerType InValue) noexcept
    {
        FPlatformAtomic::Store(&Value, static_cast<SignedType>(InValue));
    }

    /**
     * @brief         - Stores a new integer atomically without ensuring that all prior accesses have completed
     * @param InValue - New value to store
     */
    FORCEINLINE void RelaxedStore(IntegerType InValue) noexcept
    {
        FPlatformAtomic::RelaxedStore(&Value, static_cast<SignedType>(InValue));
    }

public:

    /**
     * @brief     - Copy-assignment operator
     * @param RHS - Instance to copy
     * @return    - Returns a reference to this instance
     */
    FORCEINLINE TAtomicInt& operator=(const TAtomicInt& RHS) noexcept
    {
        const IntegerType TempInteger = RHS.Load();
        Store(TempInteger);
        return *this;
    }

    /**
     * @brief     - Assign a new value
     * @param RHS - Value to assign
     * @return    - Returns the assigned value
     */
    FORCEINLINE IntegerType operator=(IntegerType RHS) noexcept
    {
        Store(RHS);
        return Load();
    }

    /**
     * @brief  - Increment the integer by one (postfix)
     * @return - Returns the previous value
     */
    FORCEINLINE IntegerType operator++(int) noexcept
    {
        const IntegerType TempValue = Load();
        Increment();
        return TempValue;
    }

    /**
     * @brief  - Increment the integer by one (prefix)
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType operator++() noexcept
    {
        return Increment();
    }

    /**
     * @brief  - Decrement the integer by one (postfix)
     * @return - Returns the previous value
     */
    FORCEINLINE IntegerType operator--(int) noexcept
    {
        const IntegerType TempValue = Load();
        Decrement();
        return TempValue;
    }

    /**
     * @brief  - Decrement the integer by one (prefix)
     * @return - Returns the new value
     */
    FORCEINLINE IntegerType operator--() noexcept
    {
        return Decrement();
    }

    /**
     * @brief     - Add a value
     * @param RHS - Value to add to the integer
     * @return    - Returns the new value
     */
    FORCEINLINE IntegerType operator+=(IntegerType RHS) noexcept
    {
        return Add(RHS);
    }

    /**
     * @brief     - Subtract a value
     * @param RHS - Value to subtract from the integer
     * @return    - Returns the new value
     */
    FORCEINLINE IntegerType operator-=(IntegerType RHS) noexcept
    {
        return Subtract(RHS);
    }

    /**
     * @brief     - Bitwise AND with a value
     * @param RHS - Value to AND with the integer
     * @return    - Returns the new value
     */
    FORCEINLINE IntegerType operator&=(IntegerType RHS) noexcept
    {
        return And(RHS);
    }

    /**
     * @brief     - Bitwise OR with a value
     * @param RHS - Value to OR with the integer
     * @return    - Returns the new value
     */
    FORCEINLINE IntegerType operator|=(IntegerType RHS) noexcept
    {
        return Or(RHS);
    }

    /**
     * @brief     - Bitwise XOR with a value
     * @param RHS - Value to XOR with the integer
     * @return    - Returns the new value
     */
    FORCEINLINE IntegerType operator^=(IntegerType RHS) noexcept
    {
        return Xor(RHS);
    }

private:
    mutable volatile SignedType Value{0};
};

// Typedefs for signed integer types
using FAtomicInt8   = TAtomicInt<int8>;
using FAtomicInt16  = TAtomicInt<int16>;
using FAtomicInt32  = TAtomicInt<int32>;
using FAtomicInt64  = TAtomicInt<int64>;

// Typedefs for unsigned integer types
using FAtomicUInt8  = TAtomicInt<uint8>;
using FAtomicUInt16 = TAtomicInt<uint16>;
using FAtomicUInt32 = TAtomicInt<uint32>;
using FAtomicUInt64 = TAtomicInt<uint64>;
