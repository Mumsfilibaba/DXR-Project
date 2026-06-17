#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Platform/PlatformAtomic.h"
#include "Core/Threading/Atomic/AtomicMemoryOrder.h"

class AtomicBool
{
    typedef int8 IntegerType;

public:
    typedef bool Type;

    /** @brief Default constructor initializes to false */
    FORCEINLINE AtomicBool() noexcept
        : Value(false)
    {
    }

    /**
     * @brief Copy-constructor
     * @param Other Instance to copy
     */
    FORCEINLINE AtomicBool(const AtomicBool& Other) noexcept
    {
        bool bValue = Other.Load();
        Store(bValue);
    }

    /**
     * @brief Construct with an initial value
     * @param InValue Initial value
     */
    FORCEINLINE AtomicBool(bool bInValue) noexcept
        : Value(static_cast<IntegerType>(bInValue))
    {
    }

    /**
     * @brief Atomically sets the boolean to a new value and returns the previous value
     * @param bInValue New value to set
     * @param Order Memory-order constraint
     * @return Returns the previous value
     */
    FORCEINLINE bool Exchange(bool bInValue, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const IntegerType Desired = static_cast<IntegerType>(bInValue);

        IntegerType OldVal;
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            OldVal = FPlatformAtomic::InterlockedExchange<EMemoryOrder::Relaxed>(&Value, Desired);
            break;
        case EMemoryOrder::Acquire:
            OldVal = FPlatformAtomic::InterlockedExchange<EMemoryOrder::Acquire>(&Value, Desired);
            break;
        case EMemoryOrder::Release:
            OldVal = FPlatformAtomic::InterlockedExchange<EMemoryOrder::Release>(&Value, Desired);
            break;
        default:
            OldVal = FPlatformAtomic::InterlockedExchange(&Value, Desired);
            break;
        }

        return static_cast<bool>(OldVal);
    }

    /**
     * @brief Compares and exchanges the boolean to a new value if it matches the comparand
     * @param bInValue Value to exchange
     * @param bComparand Value to compare against
     * @param Order Memory-order constraint
     * @return Returns true if the exchange was successful
     */
    FORCEINLINE bool CompareExchange(bool bInValue, bool bComparand, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const IntegerType Desired  = static_cast<IntegerType>(bInValue);
        const IntegerType Expected = static_cast<IntegerType>(bComparand);

        IntegerType Original;
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            Original = FPlatformAtomic::InterlockedCompareExchange<EMemoryOrder::Relaxed>(&Value, Desired, Expected);
            break;
        case EMemoryOrder::Acquire:
            Original = FPlatformAtomic::InterlockedCompareExchange<EMemoryOrder::Acquire>(&Value, Desired, Expected);
            break;
        case EMemoryOrder::Release:
            Original = FPlatformAtomic::InterlockedCompareExchange<EMemoryOrder::Release>(&Value, Desired, Expected);
            break;
        default:
            Original = FPlatformAtomic::InterlockedCompareExchange(&Value, Desired, Expected);
            break;
        }
        
        return Original == Expected;
    }

    /**
     * @brief Retrieves the boolean atomically
     * @param Order Memory-order constraint (Relaxed / Acquire / SequentiallyConsistent)
     * @return Returns the stored boolean value
     */
    NODISCARD FORCEINLINE bool Load(EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) const noexcept
    {
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            return static_cast<bool>(FPlatformAtomic::Read<EMemoryOrder::Relaxed>(&Value));
        case EMemoryOrder::Acquire:
        case EMemoryOrder::AcquireRelease:
            return static_cast<bool>(FPlatformAtomic::Read<EMemoryOrder::Acquire>(&Value));
        default:
            return static_cast<bool>(FPlatformAtomic::Read(&Value));
        }
    }

    /**
     * @brief Stores a new boolean atomically
     * @param bInValue New value to store
     * @param Order Memory-order constraint (Relaxed / Release / SequentiallyConsistent)
     */
    FORCEINLINE void Store(bool bInValue, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const IntegerType Raw = static_cast<IntegerType>(bInValue);
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            FPlatformAtomic::Store<EMemoryOrder::Relaxed>(&Value, Raw);
            break;
        case EMemoryOrder::Release:
        case EMemoryOrder::AcquireRelease:
            FPlatformAtomic::Store<EMemoryOrder::Release>(&Value, Raw);
            break;
        default:
            FPlatformAtomic::Store(&Value, Raw);
            break;
        }
    }

public:

    /**
     * @brief Copy-assignment operator
     * @param RHS Instance to copy
     * @return Returns a reference to this instance
     */
    FORCEINLINE AtomicBool& operator=(const AtomicBool& RHS) noexcept
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
