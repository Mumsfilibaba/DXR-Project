#pragma once
#include "Core/Threading/Generic/GenericCriticalSection.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacCriticalSection

class FMacCriticalSection final : public FGenericCriticalSection
{
public:

    typedef pthread_mutex_t* PlatformHandle;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericCriticalSection Interface

    FORCEINLINE FMacCriticalSection()
        : Mutex()
    {
        // Create a critical section that is recursive, in order to have parity with WinApi CriticalSection
        pthread_mutexattr_t MutexAttributes;
        pthread_mutexattr_init(&MutexAttributes);
        pthread_mutexattr_settype(&MutexAttributes, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&Mutex, &MutexAttributes);
        pthread_mutexattr_destroy(&MutexAttributes);
    }

    FORCEINLINE ~FMacCriticalSection()
    {
        pthread_mutex_destroy(&Mutex);
    }

    FORCEINLINE void Lock() noexcept
    {
        pthread_mutex_lock(&Mutex);
    }

    FORCEINLINE bool TryLock() noexcept
    {
        return (pthread_mutex_trylock(&Mutex) == 0);
    }

    FORCEINLINE void Unlock() noexcept
    {
        pthread_mutex_unlock(&Mutex);
    }

    FORCEINLINE PlatformHandle GetPlatformHandle()
    {
        return &Mutex;
    }

private:
    pthread_mutex_t Mutex;
};

