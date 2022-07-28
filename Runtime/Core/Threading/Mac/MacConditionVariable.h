#pragma once
#include "Core/Threading/Generic/GenericConditionVariable.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacConditionVariable

class FMacConditionVariable final 
    : public FGenericConditionVariable
{
public:
    typedef pthread_cond_t* PlatformHandle;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericConditionVariable Interface

    FORCEINLINE FMacConditionVariable()
        : ConditionVariable()
    {
        pthread_condattr_t Attributes;
        pthread_condattr_init(&Attributes);

        pthread_cond_init(&ConditionVariable, &Attributes);

        pthread_condattr_destroy(&Attributes);
    }

    FORCEINLINE ~FMacConditionVariable()
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

    FORCEINLINE bool Wait(TScopedLock<FCriticalSection>& Lock) noexcept
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
