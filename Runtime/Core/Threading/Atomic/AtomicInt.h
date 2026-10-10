#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Platform/PlatformAtomic.h"
#include "Core/Threading/Atomic/AtomicMemoryOrder.h"

template<typename T>
class TAtomicInt
{
    typedef typename TMakeSigned<typename TRemoveCV<T>::Type>::Type SignedType;
    static_assert(TIsIntegerNotBool<SignedType>::Value, "TAtomicInt only supports integer types");

public:
    typedef typename TRemoveCV<T>::Type IntegerType;
    typedef IntegerType Type;

    static_assert(TIsIntegerNotBool<Type>::Value, "TAtomicInt only supports integer types");

    /** @brief Default constructor initializes to zero */
    FORCEINLINE TAtomicInt() noexcept
        : Value(0)
    {
    }

    /**
     * @brief Copy-constructor
     * @param Other Instance to copy
     */
    FORCEINLINE TAtomicInt(const TAtomicInt& Other) noexcept
    {
        const IntegerType TempInteger = Other.Load();
        Store(TempInteger);
    }

    /**
     * @brief Construct with an initial value
     * @param InValue Initial value
     */
    FORCEINLINE TAtomicInt(IntegerType InValue) noexcept
        : Value(static_cast<SignedType>(InValue))
    {
    }

    /**
     * @brief Atomically increments the integer
     * @param Order Memory-order constraint
     * @return Returns the new value
     */
    FORCEINLINE IntegerType Increment(EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        SignedType NewValue;
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            NewValue = FPlatformAtomic::InterlockedIncrement<EMemoryOrder::Relaxed>(&Value);
            break;
        case EMemoryOrder::Acquire:
            NewValue = FPlatformAtomic::InterlockedIncrement<EMemoryOrder::Acquire>(&Value);
            break;
        case EMemoryOrder::Release:
            NewValue = FPlatformAtomic::InterlockedIncrement<EMemoryOrder::Release>(&Value);
            break;
        default:
            NewValue = FPlatformAtomic::InterlockedIncrement(&Value);
            break;
        }

        return static_cast<IntegerType>(NewValue);
    }

    /**
     * @brief Atomically decrements the integer
     * @param Order Memory-order constraint
     * @return Returns the new value
     */
    FORCEINLINE IntegerType Decrement(EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        SignedType NewValue;
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            NewValue = FPlatformAtomic::InterlockedDecrement<EMemoryOrder::Relaxed>(&Value);
            break;
        case EMemoryOrder::Acquire:
            NewValue = FPlatformAtomic::InterlockedDecrement<EMemoryOrder::Acquire>(&Value);
            break;
        case EMemoryOrder::Release:
            NewValue = FPlatformAtomic::InterlockedDecrement<EMemoryOrder::Release>(&Value);
            break;
        default:
            NewValue = FPlatformAtomic::InterlockedDecrement(&Value);
            break;
        }

        return static_cast<IntegerType>(NewValue);
    }

    /**
     * @brief Atomically adds a value to the integer
     * @param Order Memory-order constraint
     * @return Returns the new value
     */
    FORCEINLINE IntegerType Add(IntegerType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Operand = static_cast<SignedType>(RHS);
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            FPlatformAtomic::InterlockedAdd<EMemoryOrder::Relaxed>(&Value, Operand);
            break;
        case EMemoryOrder::Acquire:
            FPlatformAtomic::InterlockedAdd<EMemoryOrder::Acquire>(&Value, Operand);
            break;
        case EMemoryOrder::Release:
            FPlatformAtomic::InterlockedAdd<EMemoryOrder::Release>(&Value, Operand);
            break;
        default:
            FPlatformAtomic::InterlockedAdd(&Value, Operand);
            break;
        }

        return Load(Order);
    }

    /**
     * @brief Atomically subtracts a value from the integer
     * @param Order Memory-order constraint
     * @return Returns the new value
     */
    FORCEINLINE IntegerType Subtract(IntegerType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        return Add(static_cast<IntegerType>(-static_cast<SignedType>(RHS)), Order);
    }

    /**
     * @brief Performs a bitwise AND atomically with a value and the integer
     * @param Order Memory-order constraint
     * @return Returns the new value
     */
    FORCEINLINE IntegerType And(IntegerType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Mask = static_cast<SignedType>(RHS);
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            FPlatformAtomic::InterlockedAnd<EMemoryOrder::Relaxed>(&Value, Mask);
            break;
        case EMemoryOrder::Acquire:
            FPlatformAtomic::InterlockedAnd<EMemoryOrder::Acquire>(&Value, Mask);
            break;
        case EMemoryOrder::Release:
            FPlatformAtomic::InterlockedAnd<EMemoryOrder::Release>(&Value, Mask);
            break;
        default:
            FPlatformAtomic::InterlockedAnd(&Value, Mask);
            break;
        }

        return Load(Order);
    }

    /**
     * @brief Performs a bitwise OR atomically with a value and the integer
     * @param Order Memory-order constraint
     * @return Returns the new value
     */
    FORCEINLINE IntegerType Or(IntegerType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Mask = static_cast<SignedType>(RHS);
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            FPlatformAtomic::InterlockedOr<EMemoryOrder::Relaxed>(&Value, Mask);
            break;
        case EMemoryOrder::Acquire:
            FPlatformAtomic::InterlockedOr<EMemoryOrder::Acquire>(&Value, Mask);
            break;
        case EMemoryOrder::Release:
            FPlatformAtomic::InterlockedOr<EMemoryOrder::Release>(&Value, Mask);
            break;
        default:
            FPlatformAtomic::InterlockedOr(&Value, Mask);
            break;
        }

        return Load(Order);
    }

    /**
     * @brief Performs a bitwise XOR atomically with a value and the integer
     * @param Order Memory-order constraint
     * @return Returns the new value
     */
    FORCEINLINE IntegerType Xor(IntegerType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Mask = static_cast<SignedType>(RHS);
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            FPlatformAtomic::InterlockedXor<EMemoryOrder::Relaxed>(&Value, Mask);
            break;
        case EMemoryOrder::Acquire:
            FPlatformAtomic::InterlockedXor<EMemoryOrder::Acquire>(&Value, Mask);
            break;
        case EMemoryOrder::Release:
            FPlatformAtomic::InterlockedXor<EMemoryOrder::Release>(&Value, Mask);
            break;
        default:
            FPlatformAtomic::InterlockedXor(&Value, Mask);
            break;
        }

        return Load(Order);
    }

    /**
     * @brief Retrieves the integer atomically
     * @param Order Memory-order constraint (Relaxed / Acquire / SequentiallyConsistent)
     * @return Returns the stored value
     */
    NODISCARD FORCEINLINE IntegerType Load(EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) const noexcept
    {
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            return static_cast<IntegerType>(FPlatformAtomic::Read<EMemoryOrder::Relaxed>(&Value));
        case EMemoryOrder::Acquire:
        case EMemoryOrder::AcquireRelease:
            return static_cast<IntegerType>(FPlatformAtomic::Read<EMemoryOrder::Acquire>(&Value));
        default:
            return static_cast<IntegerType>(FPlatformAtomic::Read(&Value));
        }
    }

    /**
     * @brief Exchanges the integer to a new value and returns the original value
     * @param InValue Value to exchange
     * @param Order Memory-order constraint
     * @return Returns the original value
     */
    FORCEINLINE IntegerType Exchange(IntegerType InValue, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Desired = static_cast<SignedType>(InValue);
        
        SignedType OldVal;
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

        return static_cast<IntegerType>(OldVal);
    }

    /**
     * @brief Compares and exchanges the integer to a new value if it matches the comparand
     * @param InValue Value to exchange
     * @param Comparand Value to compare against
     * @param Order Memory-order constraint
     * @return Returns true if the exchange was successful
     */
    FORCEINLINE bool CompareExchange(IntegerType InValue, IntegerType Comparand, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Expected = static_cast<SignedType>(Comparand);
        const SignedType Desired  = static_cast<SignedType>(InValue);

        SignedType Original;
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
     * @brief Stores a new integer atomically
     * @param InValue New value to store
     * @param Order Memory-order constraint (Relaxed / Release / SequentiallyConsistent)
     */
    FORCEINLINE void Store(IntegerType InValue, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Raw = static_cast<SignedType>(InValue);
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
    FORCEINLINE TAtomicInt& operator=(const TAtomicInt& RHS) noexcept
    {
        const IntegerType TempInteger = RHS.Load();
        Store(TempInteger);
        return *this;
    }

    /**
     * @brief Assign a new value
     * @param RHS Value to assign
     * @return Returns the assigned value
     */
    FORCEINLINE IntegerType operator=(IntegerType RHS) noexcept
    {
        Store(RHS);
        return Load();
    }

    /**
     * @brief Increment the integer by one (postfix)
     * @return Returns the previous value
     */
    FORCEINLINE IntegerType operator++(int) noexcept
    {
        const IntegerType TempValue = Load();
        Increment();
        return TempValue;
    }

    /**
     * @brief Increment the integer by one (prefix)
     * @return Returns the new value
     */
    FORCEINLINE IntegerType operator++() noexcept
    {
        return Increment();
    }

    /**
     * @brief Decrement the integer by one (postfix)
     * @return Returns the previous value
     */
    FORCEINLINE IntegerType operator--(int) noexcept
    {
        const IntegerType TempValue = Load();
        Decrement();
        return TempValue;
    }

    /**
     * @brief Decrement the integer by one (prefix)
     * @return Returns the new value
     */
    FORCEINLINE IntegerType operator--() noexcept
    {
        return Decrement();
    }

    /**
     * @brief Add a value
     * @param RHS Value to add to the integer
     * @return Returns the new value
     */
    FORCEINLINE IntegerType operator+=(IntegerType RHS) noexcept
    {
        return Add(RHS);
    }

    /**
     * @brief Subtract a value
     * @param RHS Value to subtract from the integer
     * @return Returns the new value
     */
    FORCEINLINE IntegerType operator-=(IntegerType RHS) noexcept
    {
        return Subtract(RHS);
    }

    /**
     * @brief Bitwise AND with a value
     * @param RHS Value to AND with the integer
     * @return Returns the new value
     */
    FORCEINLINE IntegerType operator&=(IntegerType RHS) noexcept
    {
        return And(RHS);
    }

    /**
     * @brief Bitwise OR with a value
     * @param RHS Value to OR with the integer
     * @return Returns the new value
     */
    FORCEINLINE IntegerType operator|=(IntegerType RHS) noexcept
    {
        return Or(RHS);
    }

    /**
     * @brief Bitwise XOR with a value
     * @param RHS Value to XOR with the integer
     * @return Returns the new value
     */
    FORCEINLINE IntegerType operator^=(IntegerType RHS) noexcept
    {
        return Xor(RHS);
    }

private:
    mutable volatile SignedType Value;
};

// Typedefs for signed integer types
using FAtomicInt8  = TAtomicInt<int8>;
using FAtomicInt16 = TAtomicInt<int16>;
using FAtomicInt32 = TAtomicInt<int32>;
using FAtomicInt64 = TAtomicInt<int64>;

// Typedefs for unsigned integer types
using FAtomicUInt8  = TAtomicInt<uint8>;
using FAtomicUInt16 = TAtomicInt<uint16>;
using FAtomicUInt32 = TAtomicInt<uint32>;
using FAtomicUInt64 = TAtomicInt<uint64>;
