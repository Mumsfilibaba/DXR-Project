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
    
    /**
     * Constructor that takes a lock and tries to lock it
     * 
     * @param InLock: Lock to lock
     */
    FORCEINLINE TScopedLock(LockType& InLock)
        : Lock(InLock)
    {
        Lock.Lock();
    }

    /**
     * Destructor
     */
    FORCEINLINE ~TScopedLock()
    {
        Lock.Unlock();
    }

    /**
     * Retrieve the lock
     * 
     * @return: Returns a reference to the lock
     */
    FORCEINLINE LockType& GetLock()
    {
        return Lock;
    }

    /**
     * Retrieve the lock
     *
     * @return: Returns a reference to the lock
     */
    FORCEINLINE const LockType& GetLock() const
    {
        return Lock;
    }

private:
    LockType& Lock;
};
