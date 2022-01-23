#pragma once

#if PLATFORM_MACOS
#include "Core/Threading/Interface/PlatformConditionVariable.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation of ConditionVariable

class CMacConditionVariable final : public CPlatformConditionVariable
{
public:

    typedef pthread_cond_t* PlatformHandle;

    CMacConditionVariable(const CMacConditionVariable&) = delete;
    CMacConditionVariable& operator=(const CMacConditionVariable&) = delete;

    FORCEINLINE CMacConditionVariable()
        : ConditionVariable()
    {
        pthread_condattr_t Attributes;
        pthread_condattr_init(&Attributes);

        pthread_cond_init(&ConditionVariable, &Attributes);

        pthread_condattr_destroy(&Attributes);
    }

    FORCEINLINE ~CMacConditionVariable()
    {
        pthread_cond_destroy(&ConditionVariable);
    }

   /* Notifies a single CriticalSection */
    FORCEINLINE void NotifyOne() noexcept
    {
        pthread_cond_signal(&ConditionVariable);
    }

    /* Notifies a all CriticalSections */
    FORCEINLINE void NotifyAll() noexcept
    {
        pthread_cond_broadcast(&ConditionVariable);
    }

    /* Make a CriticalSections wait until notified */
    FORCEINLINE bool Wait(TScopedLock<CCriticalSection>& Lock) noexcept
    {
        pthread_mutex_t* Mutex = Lock.GetLock().GetPlatformHandle();
        int Result = pthread_cond_wait(&ConditionVariable, Mutex);

        // TODO: Handle error
        Assert(Result == 0);
        return (Result == 0);
    }

    FORCEINLINE PlatformHandle GetPlatformHandle() { return &ConditionVariable; }

private:
    pthread_cond_t ConditionVariable;
};

#endif
