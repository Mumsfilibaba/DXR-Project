#pragma once
#include "Core/Core.h"
#include "Core/Templates/ClassUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TScopedLock

template<typename LockType>
class TScopedLock : CNonCopyable, CNonMovable
{
public:
    
    /**
     * @brief: Constructor that takes a lock and tries to lock it
     * 
     * @param InLock: Lock to lock
     */
    FORCEINLINE TScopedLock(LockType& InLock)
        : Lock(InLock)
    {
        Lock.Lock();
    }

    /** @brief: Destructor */
    FORCEINLINE ~TScopedLock()
    {
        Lock.Unlock();
    }

    /** @return: Returns a reference to the lock */
    FORCEINLINE LockType& GetLock()
    {
        return Lock;
    }

    /** @return: Returns a reference to the lock */
    FORCEINLINE const LockType& GetLock() const
    {
        return Lock;
    }

private:
    LockType& Lock;
};
