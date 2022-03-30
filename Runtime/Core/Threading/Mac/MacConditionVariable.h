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

    /**
     * @brief: Default constructor 
     */
    FORCEINLINE CMacConditionVariable()
        : ConditionVariable()
    {
        pthread_condattr_t Attributes;
        pthread_condattr_init(&Attributes);

        pthread_cond_init(&ConditionVariable, &Attributes);

        pthread_condattr_destroy(&Attributes);
    }

    /**
     * @brief: Destructor 
     */
    FORCEINLINE ~CMacConditionVariable()
    {
        pthread_cond_destroy(&ConditionVariable);
    }

    /** Notifies a single CriticalSection */
    FORCEINLINE void NotifyOne() noexcept
    {
        pthread_cond_signal(&ConditionVariable);
    }

    /** Notifies a all CriticalSections */
    FORCEINLINE void NotifyAll() noexcept
    {
        pthread_cond_broadcast(&ConditionVariable);
    }

    /**
     * @brief: Make a CriticalSections wait until notified 
     * 
     * @param Lock: Lock that should wait for condition to be met
     * @return: Returns true if the wait is successful
     */
    FORCEINLINE bool Wait(TScopedLock<CCriticalSection>& Lock) noexcept
    {
        pthread_mutex_t* Mutex = Lock.GetLock().GetPlatformHandle();
        int Result = pthread_cond_wait(&ConditionVariable, Mutex);

        // TODO: Handle error
        Assert(Result == 0);
        return (Result == 0);
    }

    /**
     * @brief: Retrieve platform specific handle 
     * 
     * @return: Returns a platform specific handle or nullptr if no platform handle is defined
     */
    FORCEINLINE PlatformHandle GetPlatformHandle() 
    { 
        return &ConditionVariable;
    }

private:
    pthread_cond_t ConditionVariable;
};

#endif
