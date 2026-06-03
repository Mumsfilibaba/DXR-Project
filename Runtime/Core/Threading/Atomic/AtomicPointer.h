#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Platform/PlatformAtomic.h"
#include "Core/Threading/Atomic/AtomicMemoryOrder.h"

template<typename T>
class TAtomicPointer
{
    typedef typename TMakeSigned<UPTR_INT>::Type SignedType;

public:
    static_assert(TIsPointer<T>::Value, "TAtomicPointer can only be instantiated with pointer types.");

    typedef T PointerType;
    typedef PointerType Type;

    /** @brief Default constructor initializes to nullptr */
    FORCEINLINE TAtomicPointer() noexcept
        : Value(0)
    {
    }

    /**
     * @brief Copy-constructor
     * @param Other Instance to copy
     */
    FORCEINLINE TAtomicPointer(const TAtomicPointer& Other) noexcept
    {
        PointerType TempPointer = Other.Load();
        Store(TempPointer);
    }

    /**
     * @brief Construct with an initial pointer value
     * @param InPtr Initial pointer value
     */
    FORCEINLINE TAtomicPointer(PointerType InPtr) noexcept
        : Value(reinterpret_cast<SignedType>(InPtr))
    {
    }

    /**
     * @brief Atomically sets the pointer to a new value and returns the old value
     * @param InPtr New pointer value
     * @param Order Memory-order constraint
     * @return Returns the old pointer value
     */
    FORCEINLINE PointerType Exchange(PointerType InPtr, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Desired = reinterpret_cast<SignedType>(InPtr);

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

        return reinterpret_cast<PointerType>(OldVal);
    }

    /**
     * @brief Compares and exchanges the pointer to a new value if it matches the comparand
     * @param InPtr New pointer value to set
     * @param Comparand Pointer value to compare against
     * @param Order Memory-order constraint
     * @return Returns true if the exchange was successful
     */
    FORCEINLINE bool CompareExchange(PointerType InPtr, PointerType Comparand, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Desired  = reinterpret_cast<SignedType>(InPtr);
        const SignedType Expected = reinterpret_cast<SignedType>(Comparand);

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
     * @brief Retrieves the pointer atomically
     * @param Order Memory-order constraint (Relaxed / Acquire / SequentiallyConsistent)
     * @return Returns the stored pointer value
     */
    NODISCARD FORCEINLINE PointerType Load(EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) const noexcept
    {
        SignedType Current;
        switch (Order)
        {
        case EMemoryOrder::Relaxed:
            Current = FPlatformAtomic::Read<EMemoryOrder::Relaxed>(&Value);
            break;
        case EMemoryOrder::Acquire:
        case EMemoryOrder::AcquireRelease:
            Current = FPlatformAtomic::Read<EMemoryOrder::Acquire>(&Value);
            break;
        default:
            Current = FPlatformAtomic::Read(&Value);
            break;
        }
        
        return reinterpret_cast<PointerType>(Current);
    }

    /**
     * @brief Stores a new pointer atomically
     * @param InPtr New pointer value to store
     * @param Order Memory-order constraint (Relaxed / Release / SequentiallyConsistent)
     */
    FORCEINLINE void Store(PointerType InPtr, EMemoryOrder Order = EMemoryOrder::SequentiallyConsistent) noexcept
    {
        const SignedType Raw = reinterpret_cast<SignedType>(InPtr);
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
    FORCEINLINE TAtomicPointer& operator=(const TAtomicPointer& RHS) noexcept
    {
        const PointerType PtrValue = RHS.Load();
        Store(PtrValue);
        return *this;
    }

    /**
     * @brief Assign a new pointer value
     * @param RHS Pointer value to assign
     * @return Returns the assigned pointer value
     */
    FORCEINLINE PointerType operator=(PointerType RHS) noexcept
    {
        Store(RHS);
        return Load();
    }

private:
    mutable volatile SignedType Value{ 0 };
};
