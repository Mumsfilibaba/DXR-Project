#pragma once
#include "Core/Core.h"
#include "Core/Threading/Atomic/AtomicMemoryOrder.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

/**
 * @brief Returns true if Order is a valid memory ordering for an atomic load.
 *
 * Loads accept Relaxed, Acquire and SequentiallyConsistent orderings.
 * Release and AcquireRelease are not meaningful on a pure load.
 */
template<EMemoryOrder Order>
static constexpr bool IsLoadOrderingValid()
{
    return Order == EMemoryOrder::Relaxed
        || Order == EMemoryOrder::Acquire
        || Order == EMemoryOrder::SequentiallyConsistent;
}

/**
 * @brief Returns true if Order is a valid memory ordering for an atomic store.
 *
 * Stores accept Relaxed, Release and SequentiallyConsistent orderings.
 * Acquire and AcquireRelease are not meaningful on a pure store.
 */
template<EMemoryOrder Order>
static constexpr bool IsStoreOrderingValid()
{
    return Order == EMemoryOrder::Relaxed
        || Order == EMemoryOrder::Release
        || Order == EMemoryOrder::SequentiallyConsistent;
}

/**
 * @brief Returns true if Order is a valid memory ordering for a read-modify-write operation.
 *
 * Read-modify-write operations have both a read-half and a write-half, so all
 * five orderings are valid.
 */
template<EMemoryOrder Order>
static constexpr bool IsReadModifyWriteOrderingValid()
{
    return Order == EMemoryOrder::Relaxed
        || Order == EMemoryOrder::Acquire
        || Order == EMemoryOrder::Release
        || Order == EMemoryOrder::AcquireRelease
        || Order == EMemoryOrder::SequentiallyConsistent;
}

struct FGenericPlatformAtomic
{
    /**
     * @brief Atomically reads a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Source Pointer to variable to read from
     * @return Returns the read value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 Read(volatile const int8* Source) requires(IsLoadOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically reads a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Source Pointer to variable to read from
     * @return Returns the read value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 Read(volatile const int16* Source) requires(IsLoadOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically reads a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Source Pointer to variable to read from
     * @return Returns the read value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 Read(volatile const int32* Source) requires(IsLoadOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically reads a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Source Pointer to variable to read from
     * @return Returns the read value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 Read(volatile const int64* Source) requires(IsLoadOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically stores a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Dest Pointer to variable to store value in
     * @param Value Value to store
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int8* Dest, int8 Value) requires(IsStoreOrderingValid<Order>())
    {
    }

    /**
     * @brief Atomically stores a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Dest Pointer to variable to store value in
     * @param Value Value to store
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int16* Dest, int16 Value) requires(IsStoreOrderingValid<Order>())
    {
    }

    /**
     * @brief Atomically stores a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Dest Pointer to variable to store value in
     * @param Value Value to store
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int32* Dest, int32 Value) requires(IsStoreOrderingValid<Order>())
    {
    }

    /**
     * @brief Atomically stores a value with the supplied memory ordering
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Dest Pointer to variable to store value in
     * @param Value Value to store
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int64* Dest, int64 Value) requires(IsStoreOrderingValid<Order>())
    {
    }

    /**
     * @brief Atomically adds two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedAdd(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically adds two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedAdd(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically adds two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedAdd(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically adds two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedAdd(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise AND of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedAnd(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise AND of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedAnd(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise AND of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedAnd(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise AND of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedAnd(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise OR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedOr(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise OR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedOr(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise OR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedOr(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise OR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedOr(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise XOR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedXor(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise XOR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedXor(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise XOR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedXor(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically performs a bitwise XOR of two integers with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to first, which is also used to store the result
     * @param Value Second operand
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedXor(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically increments an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to increment, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedIncrement(volatile int8* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically increments an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to increment, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedIncrement(volatile int16* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically increments an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to increment, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedIncrement(volatile int32* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically increments an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to increment, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedIncrement(volatile int64* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically decrements an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to decrement, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedDecrement(volatile int8* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically decrements an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to decrement, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedDecrement(volatile int16* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically decrements an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to decrement, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedDecrement(volatile int32* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically decrements an integer with the supplied memory ordering and returns the new value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Addend Pointer to integer to decrement, which is also used to store the result
     * @return Returns the new value
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedDecrement(volatile int64* Addend) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically compares two values with the supplied memory ordering, exchanging if equal. Returns the original value.
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to destination
     * @param Exchange Value to exchange
     * @param Comparand Value to compare against Target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedCompareExchange(volatile int8* Target, int8 Exchange, int8 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically compares two values with the supplied memory ordering, exchanging if equal. Returns the original value.
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to destination
     * @param Exchange Value to exchange
     * @param Comparand Value to compare against Target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedCompareExchange(volatile int16* Target, int16 Exchange, int16 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically compares two values with the supplied memory ordering, exchanging if equal. Returns the original value.
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to destination
     * @param Exchange Value to exchange
     * @param Comparand Value to compare against Target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* Target, int32 Exchange, int32 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically compares two values with the supplied memory ordering, exchanging if equal. Returns the original value.
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to destination
     * @param Exchange Value to exchange
     * @param Comparand Value to compare against Target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* Target, int64 Exchange, int64 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically sets an integer to a specified value with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to target
     * @param Value New value for the target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedExchange(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically sets an integer to a specified value with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to target
     * @param Value New value for the target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedExchange(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically sets an integer to a specified value with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to target
     * @param Value New value for the target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedExchange(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically sets an integer to a specified value with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to target
     * @param Value New value for the target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedExchange(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return 0;
    }

    /**
     * @brief Atomically sets a pointer to a specified value with the supplied memory ordering and returns the original value
     * @tparam Order Memory ordering. Defaults to sequentially consistent.
     * @param Target Pointer to target
     * @param Value New value for the target
     * @return Returns the original value of Target
     */
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void* InterlockedExchangePointer(void* volatile* Target, void* Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return nullptr;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
