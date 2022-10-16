#pragma once
#include "Mac.h"

#include "Core/Generic/GenericEvent.h"

typedef TSharedRef<class FMacEvent> FMacEventRef;

class FMacEvent final
    : public FGenericEvent
{
    enum class ETriggerType : uint8
    {
        None = 0,
        One  = 1,
        All  = 2,
    };

public:
    FMacEvent();
    ~FMacEvent();

    bool Create(bool bInManualReset);

    virtual void Trigger() override final;

    virtual void Wait(uint64 Milliseconds) override final;

    virtual void Reset() override final;

    virtual bool IsManualReset() const override final 
    { 
        return bManualReset;
    }

private:
    FORCEINLINE void LockMutex()
    {
        const auto Result = pthread_mutex_lock(&Mutex);
        CHECK(Result == 0);
    }

    FORCEINLINE void UnlockMutex()
    {
        const auto Result = pthread_mutex_unlock(&Mutex);
        CHECK(Result == 0);
    }

    static void SubtractTimevals(const struct timeval* This, struct timeval* Subtract, struct timeval* OutDifference)
    {
        if (This->tv_usec < Subtract->tv_usec)
        {
            const auto nsec = ((Subtract->tv_usec - This->tv_usec) / 1000000) + 1;
            Subtract->tv_usec -= 1000000 * nsec;
            Subtract->tv_sec  += nsec;
        }

        if (This->tv_usec - Subtract->tv_usec > 1000000)
        {
            const auto nsec = (This->tv_usec - Subtract->tv_usec) / 1000000;
            Subtract->tv_usec += 1000000 * nsec;
            Subtract->tv_sec  -= nsec;
        }

        OutDifference->tv_sec  = This->tv_sec - Subtract->tv_sec;
        OutDifference->tv_usec = This->tv_usec - Subtract->tv_usec;
    }  

    bool bInitialized;
    bool bManualReset;

    volatile ETriggerType Triggered;
    volatile int32        NumWaitingThreads;

    pthread_mutex_t Mutex;
    pthread_cond_t  Condition;
};

