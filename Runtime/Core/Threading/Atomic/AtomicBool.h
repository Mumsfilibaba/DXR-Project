#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Platform/PlatformInterlocked.h"
#include "Core/Platform/PlatformAtomic.h"

class FAtomicBool
{
    typedef int8 IntegerType;

public:
    typedef bool Type;

    /** @brief Default constructor initializes to false */
    FORCEINLINE FAtomicBool() noexcept
        : Value(false)
    {
    }

    /**
     * @brief Copy-constructor
     * @param Other Instance to copy
     */
    FORCEINLINE FAtomicBool(const FAtomicBool& Other) noexcept
    {
        bool bValue = Other.Load();
        Store(bValue);
    }

    /**
     * @brief Construct with an initial value
     * @param InValue Initial value
     */
    FORCEINLINE FAtomicBool(bool bInValue) noexcept
        : Value(static_cast<IntegerType>(bInValue))
    {
    }

    /**
     * @brief Atomically sets the boolean to a new value and returns the previous value
     * @param InValue New value to set
     * @return Returns the previous value
     */
    FORCEINLINE bool Exchange(bool bInValue) noexcept
    {
        const IntegerType OldVal = FPlatformInterlocked::InterlockedExchange(&Value, static_cast<IntegerType>(bInValue));
        return static_cast<bool>(OldVal);
    }

    /**
     * @brief Compares and exchanges the boolean to a new value if it matches the comparand
     * @param InValue Value to exchange
     * @param Comparand Value to compare against
     * @return Returns true if the exchange was successful
     */
    FORCEINLINE bool CompareExchange(bool bInValue, bool bComparand) noexcept
    {
        const IntegerType Desired  = static_cast<IntegerType>(bInValue);
        const IntegerType Expected = static_cast<IntegerType>(bComparand);
        const IntegerType Original = FPlatformInterlocked::InterlockedCompareExchange(&Value, Desired, Expected);
        return Original == Expected;
    }

    /**
     * @brief Retrieves the boolean atomically
     * @return Returns the stored boolean value
     */
    FORCEINLINE bool Load() const noexcept
    {
        return static_cast<bool>(FPlatformAtomic::Read(&Value));
    }

    /**
     * @brief Stores a new boolean atomically
     * @param InValue New value to store
     */
    FORCEINLINE void Store(bool bInValue) noexcept
    {
        FPlatformAtomic::Store(&Value, static_cast<IntegerType>(bInValue));
    }

public:

    /**
     * @brief Copy-assignment operator
     * @param RHS Instance to copy
     * @return Returns a reference to this instance
     */
    FORCEINLINE FAtomicBool& operator=(const FAtomicBool& RHS) noexcept
    {
        const bool bValue = RHS.Load();
        Store(bValue);
        return *this;
    }

    /**
     * @brief Assign a new value
     * @param RHS Value to assign
     * @return Returns the assigned value
     */
    FORCEINLINE bool operator=(bool bValue) noexcept
    {
        Store(bValue);
        return Load();
    }

private:
    mutable volatile IntegerType Value{ static_cast<IntegerType>(false) };
};
