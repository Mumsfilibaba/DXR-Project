#pragma once

#if PLATFORM_MACOS
#include "Core/Threading/Interface/PlatformCriticalSection.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation of CriticalSection

class CMacCriticalSection final : public CPlatformCriticalSection
{
public:

    typedef pthread_mutex_t* PlatformHandle;

    CMacCriticalSection(const CMacCriticalSection&) = delete;
    CMacCriticalSection& operator=(const CMacCriticalSection&) = delete;

    /**
     * @brief: Default constructor 
     */
    FORCEINLINE CMacCriticalSection()
        : Mutex()
    {
        /* Create a critical section that is recursive, in order to have parity with winapi CriticalSection */
        pthread_mutexattr_t MutexAttributes;
        pthread_mutexattr_init(&MutexAttributes);
        pthread_mutexattr_settype(&MutexAttributes, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&Mutex, &MutexAttributes);
        pthread_mutexattr_destroy(&MutexAttributes);
    }

    /**
     * @brief: Destructor 
     */
    FORCEINLINE ~CMacCriticalSection()
    {
        pthread_mutex_destroy(&Mutex);
    }

    /** Lock CriticalSection for other threads */
    FORCEINLINE void Lock() noexcept
    {
        pthread_mutex_lock(&Mutex);
    }

    /**
     * @brief: Try to lock CriticalSection for other threads
     * 
     * @return:; Returns true if the lock is successful
     */
    FORCEINLINE bool TryLock() noexcept
    {
        return (pthread_mutex_trylock(&Mutex) == 0);
    }

    /** Unlock CriticalSection for other threads */
    FORCEINLINE void Unlock() noexcept
    {
        pthread_mutex_unlock(&Mutex);
    }

    /**
     * @brief: Retrieve platform specific handle
     *
     * @return: Returns a platform specific handle or nullptr if no platform handle is defined
     */
    FORCEINLINE PlatformHandle GetPlatformHandle()
    {
        return &Mutex;
    }

private:
    pthread_mutex_t Mutex;
};

#endif
