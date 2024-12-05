#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Platform/PlatformInterlocked.h"
#include "Core/Platform/PlatformAtomic.h"

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
     * @return Returns the old pointer value
     */
    FORCEINLINE PointerType Exchange(PointerType InPtr) noexcept
    {
        const SignedType OldVal = FPlatformInterlocked::InterlockedExchange(&Value, reinterpret_cast<SignedType>(InPtr));
        return reinterpret_cast<PointerType>(OldVal);
    }

    /**
     * @brief Compares and exchanges the pointer to a new value if it matches the comparand
     * @param InPtr New pointer value to set
     * @param Comparand Pointer value to compare against
     * @return Returns true if the exchange was successful
     */
    FORCEINLINE bool CompareExchange(PointerType InPtr, PointerType Comparand) noexcept
    {
        const SignedType Desired  = reinterpret_cast<SignedType>(InPtr);
        const SignedType Expected = reinterpret_cast<SignedType>(Comparand);
        const SignedType Original = FPlatformInterlocked::InterlockedCompareExchange(&Value, Desired, Expected);
        return Original == Expected;
    }

    /**
     * @brief Retrieves the pointer atomically
     * @return Returns the stored pointer value
     */
    FORCEINLINE PointerType Load() const noexcept
    {
        const SignedType Current = FPlatformAtomic::Read(&Value);
        return reinterpret_cast<PointerType>(Current);
    }

    /**
     * @brief Stores a new pointer atomically
     * @param InPtr New pointer value to store
     */
    FORCEINLINE void Store(PointerType InPtr) noexcept
    {
        FPlatformAtomic::Store(&Value, reinterpret_cast<SignedType>(InPtr));
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
