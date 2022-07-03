#pragma once
#include "Core/Templates/Move.h"

#include "Platform/CriticalSection.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class encapsulating an object and a lock for less repetion when protecting a single object with a lock

template<typename T, typename LockType = FCriticalSection>
class Lockable
{
public:

    using ElementType = T;

    FORCEINLINE Lockable() noexcept
        : LockableItem()
        , ItemLock()
    { }

    template<typename... ArgTypes>
    FORCEINLINE Lockable(ArgTypes&&... Args) noexcept
        : LockableItem(Forward<ArgTypes>(Args)...)
        , ItemLock()
    { }

    FORCEINLINE void Lock() noexcept
    {
        ItemLock.Lock();
    }

    FORCEINLINE bool TryLock() noexcept
    {
        return ItemLock.TryLock();
    }

    FORCEINLINE void Unlock() noexcept
    {
        ItemLock.Unlock();
    }

    FORCEINLINE ElementType& Get() noexcept
    {
        return LockableItem;
    }

    FORCEINLINE const ElementType& Get() const noexcept
    {
        return LockableItem;
    }

    FORCEINLINE LockType& GetLock() noexcept
    {
        return ItemLock;
    }

    FORCEINLINE LockType GetLock() const noexcept
    {
        return ItemLock;
    }

public:

    FORCEINLINE bool operator==(const ElementType& RHS) const noexcept
    {
        return (LockableItem == RHS);
    }

    FORCEINLINE bool operator!=(const ElementType& RHS) const noexcept
    {
        return (LockableItem != RHS);
    }

    FORCEINLINE T* operator&() noexcept
    {
        return &LockableItem;
    }

    FORCEINLINE const T* operator&() const noexcept
    {
        return &LockableItem;
    }

    FORCEINLINE operator ElementType& () noexcept
    {
        return Get();
    }

    FORCEINLINE operator const ElementType& () const noexcept
    {
        return Get();
    }

private:
    T        LockableItem;
    LockType ItemLock;
};
