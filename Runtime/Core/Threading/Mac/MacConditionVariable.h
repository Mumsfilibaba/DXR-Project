#pragma once
#include "Core/Threading/Generic/GenericConditionVariable.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacConditionVariable

class CMacConditionVariable final : public CGenericConditionVariable
{
public:

    typedef pthread_cond_t* PlatformHandle;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericConditionVariable Interface

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

    FORCEINLINE void NotifyOne() noexcept
    {
        pthread_cond_signal(&ConditionVariable);
    }

    FORCEINLINE void NotifyAll() noexcept
    {
        pthread_cond_broadcast(&ConditionVariable);
    }

    FORCEINLINE bool Wait(TScopedLock<CCriticalSection>& Lock) noexcept
    {
        pthread_mutex_t* Mutex = Lock.GetLock().GetPlatformHandle();
        return (pthread_cond_wait(&ConditionVariable, Mutex) == 0);
    }

    FORCEINLINE PlatformHandle GetPlatformHandle() 
    { 
        return &ConditionVariable;
    }

private:
    pthread_cond_t ConditionVariable;
};
