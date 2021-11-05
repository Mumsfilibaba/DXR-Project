#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Threading/Interface/PlatformCriticalSection.h"

#include <pthread.h>

class CMacCriticalSection final : public CPlatformCriticalSection
{
public:

    typedef pthread_mutex_t* PlatformHandle;

    CMacCriticalSection( const CMacCriticalSection& ) = delete;
    CMacCriticalSection& operator=( const CMacCriticalSection& ) = delete;

    /* Constructing a critical section */
    FORCEINLINE CMacCriticalSection()
        : Mutex()
    {
        /* Create a critical section that is recursive, in order to have parity with winapi CriticalSection */
        pthread_mutexattr_t MutexAttributes;
        pthread_mutexattr_init( &MutexAttributes );
        pthread_mutexattr_settype( &MutexAttributes, PTHREAD_MUTEX_RECURSIVE );

        pthread_mutex_init( &Mutex, &MutexAttributes );
        pthread_mutexattr_destroy( &MutexAttributes );
    }

    FORCEINLINE ~CMacCriticalSection()
    {
        pthread_mutex_destroy( &Mutex );
    }

    FORCEINLINE void Lock() noexcept
    {
        pthread_mutex_lock( &Mutex );
    }

    FORCEINLINE bool TryLock() noexcept
    {
        return (pthread_mutex_trylock( &Mutex ) == 0);
    }

    FORCEINLINE void Unlock() noexcept
    {
        pthread_mutex_unlock( &Mutex );
    }

    FORCEINLINE PlatformHandle GetPlatformHandle()
    {
        return &Mutex;
    }

private:
    pthread_mutex_t Mutex;
};

#endif
