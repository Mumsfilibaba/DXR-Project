#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Platform/PlatformAtomic.h"
#include "Core/Threading/Atomic/AtomicMemoryOrder.h"

template<typename T>
class TAtomicEnum
{
    static_assert(TIsEnum<T>::Value, "TAtomicEnum only supports enum types");

    typedef typename TUnderlyingType<T>::Type           UnderlyingType;
    typedef typename TMakeSigned<UnderlyingType>::Type  SignedType;

public:
    typedef T EnumType;
    typedef T Type;

    /** @brief Default constructor zero-initializes the enum value. */
    FORCEINLINE TAtomicEnum() noexcept
        : Value(0)
    {
    }

    /** @brief Construct with an initial enum value. */
    FORCEINLINE TAtomicEnum(EnumType InValue) noexcept
        : Value(static_cast<SignedType>(InValue))
    {
    }

    /** @brief Copy-constructor. */
    FORCEINLINE TAtomicEnum(const TAtomicEnum& Other) noexcept
    {
        Store(Other.Load());
    }

    /**
     * @brief Atomically loads the enum value.
     * @param Order Memory-order constraint (Relaxed / Acquire / SequentiallyConsistent).
     */
    NODISCARD FORCEINLINE EnumType Load(EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) const noexcept
    {
        return static_cast<EnumType>(LoadInternal(Order));
    }

    /**
     * @brief Atomically stores a new enum value.
     * @param Order Memory-order constraint (Relaxed / Release / SequentiallyConsistent).
     */
    FORCEINLINE void Store(EnumType InValue, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        StoreInternal(static_cast<SignedType>(InValue), Order);
    }

    /**
     * @brief Atomically exchanges the value, returning the old one.
     */
    FORCEINLINE EnumType Exchange(EnumType InValue, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Desired = static_cast<SignedType>(InValue);

        SignedType Old;
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            Old = FPlatformAtomic::InterlockedExchange<EMemoryOrder::Relaxed>(&Value, Desired);
            break;
        case EMemoryOrder::Acquire:
            Old = FPlatformAtomic::InterlockedExchange<EMemoryOrder::Acquire>(&Value, Desired);
            break;
        case EMemoryOrder::Release:
            Old = FPlatformAtomic::InterlockedExchange<EMemoryOrder::Release>(&Value, Desired);
            break;
        default:
            Old = FPlatformAtomic::InterlockedExchange(&Value, Desired);
            break;
        }

        return static_cast<EnumType>(Old);
    }

    /**
     * @brief Atomically compares and exchanges. Returns true if the swap took place.
     */
    FORCEINLINE bool CompareExchange(EnumType Desired, EnumType Comparand, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType ExpectedRaw = static_cast<SignedType>(Comparand);
        const SignedType DesiredRaw  = static_cast<SignedType>(Desired);

        SignedType Original;
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            Original = FPlatformAtomic::InterlockedCompareExchange<EMemoryOrder::Relaxed>(&Value, DesiredRaw, ExpectedRaw);
            break;
        case EMemoryOrder::Acquire:
            Original = FPlatformAtomic::InterlockedCompareExchange<EMemoryOrder::Acquire>(&Value, DesiredRaw, ExpectedRaw);
            break;
        case EMemoryOrder::Release:
            Original = FPlatformAtomic::InterlockedCompareExchange<EMemoryOrder::Release>(&Value, DesiredRaw, ExpectedRaw);
            break;
        default:
            Original = FPlatformAtomic::InterlockedCompareExchange(&Value, DesiredRaw, ExpectedRaw);
            break;
        }

        return Original == ExpectedRaw;
    }

    /** @brief Bitwise AND of the stored value with RHS, atomically. Returns the new value. Only available when EnumType has operator&. */
    FORCEINLINE EnumType And(EnumType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
        requires(THasBitwiseAnd<EnumType>::Value)
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

    /** @brief Bitwise OR of the stored value with RHS, atomically. Returns the new value. Only available when EnumType has operator|. */
    FORCEINLINE EnumType Or(EnumType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
        requires(THasBitwiseOr<EnumType>::Value)
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

    /** @brief Bitwise XOR of the stored value with RHS, atomically. Returns the new value. Only available when EnumType has operator^. */
    FORCEINLINE EnumType Xor(EnumType RHS, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
        requires(THasBitwiseXor<EnumType>::Value)
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

    FORCEINLINE TAtomicEnum& operator=(const TAtomicEnum& RHS) noexcept
    {
        Store(RHS.Load());
        return *this;
    }

    FORCEINLINE EnumType operator=(EnumType RHS) noexcept
    {
        Store(RHS);
        return RHS;
    }

    FORCEINLINE EnumType operator&=(EnumType RHS) noexcept requires(THasBitwiseAnd<EnumType>::Value)
    {
        return And(RHS);
    }

    FORCEINLINE EnumType operator|=(EnumType RHS) noexcept requires(THasBitwiseOr<EnumType>::Value)
    {
        return Or(RHS);
    }
    
    FORCEINLINE EnumType operator^=(EnumType RHS) noexcept requires(THasBitwiseXor<EnumType>::Value)
    {
        return Xor(RHS);
    }

private:
    FORCEINLINE SignedType LoadInternal(EMemoryOrder Order) const noexcept
    {
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            return FPlatformAtomic::Read<EMemoryOrder::Relaxed>(&Value);
        case EMemoryOrder::Acquire:
        case EMemoryOrder::AcquireRelease:
            return FPlatformAtomic::Read<EMemoryOrder::Acquire>(&Value);
        default:
            return FPlatformAtomic::Read(&Value);
        }
    }

    FORCEINLINE void StoreInternal(SignedType InValue, EMemoryOrder Order) noexcept
    {
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            FPlatformAtomic::Store<EMemoryOrder::Relaxed>(&Value, InValue);
            break;
        case EMemoryOrder::Release:
        case EMemoryOrder::AcquireRelease:
            FPlatformAtomic::Store<EMemoryOrder::Release>(&Value, InValue);
            break;
        default:
            FPlatformAtomic::Store(&Value, InValue);
            break;
        }
    }

    mutable volatile SignedType Value;
};
