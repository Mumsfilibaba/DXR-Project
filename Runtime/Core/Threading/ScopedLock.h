#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper class that locks a lock in constructor and releases it in the destructor

template<typename LockType>
class TScopedLock
{
public:

    TScopedLock(TScopedLock&&) = delete;
    TScopedLock(const TScopedLock&) = delete;

    TScopedLock& operator=(TScopedLock&&) = delete;
    TScopedLock& operator=(const TScopedLock&) = delete;

    FORCEINLINE TScopedLock(LockType& InLock)
        : Lock(InLock)
    {
        Lock.Lock();
    }

    FORCEINLINE ~TScopedLock()
    {
        Lock.Unlock();
    }

    FORCEINLINE LockType& GetLock()
    {
        return Lock;
    }

    FORCEINLINE const LockType& GetLock() const
    {
        return Lock;
    }

private:
    LockType& Lock;
};
