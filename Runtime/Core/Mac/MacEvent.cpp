#include "MacEvent.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacEvent

FMacEvent::FMacEvent()
    : bInitialized(false)
    , bManualReset(false)
    , Triggered(ETriggerType::None)
    , NumWaitingThreads(0)
    , Mutex()
    , Condition()
{ }

FMacEvent::~FMacEvent()
{
    if (bInitialize)
    {
        LockMutex();
        bManualReset = true;
        UnlockMutex();

        Trigger();

        LockMutex();

        bInitialized = false;
        while (NumWaitingThreads)
        {
            UnlockMutex();
            LockMutex();
        }

        pthread_cond_destroy(&Condition);

        UnlockMutex();
        
        pthread_mutex_destroy(&Mutex);
    }
}

bool FMacEvent::Create(bool bInManualReset)
{
    bool bResult = false;
    Triggered    = ETriggerType::None;
    bManualReset = bInManualReset;

    if (pthreads_mutex_init(&Mutex, nullptr) == 0)
    {
        if (pthread_cond_init(&Condition) == 0)
        {
            bInitialized = true;
            bResult      = true;
        }
        else
        {
            pthread_mutex_destroy(&Mutex);
        }
    }

    return bResult;
}

void FMacEvent::Trigger()
{
    Check(bInitialized == true);

    LockMutex();

    if (bManualReset)
    {
        Triggered = ETriggerType::All;
        const auto Result = pthread_cond_broadcast(&Condition);
        Check(Result == 0);
    }
    else 
    {
        Triggered = ETriggerType::One;
        const auto Result = pthread_cond_broadcast(&Condition);
        Check(Result == 0);
    }

    UnlockMutex();
}

void FMacEvent::Wait(uint64 Milliseconds)
{
    struct timeval StartTime;
    if ((Milliseconds > 0) && (Milliseconds != TNumericLimits<uint64>::Max()))
    {
        gettimeofday(&StartTime, nullptr);
    }

    LockMutex();

    bool bResult = false;
    do 
    {
        if (Triggered == ETriggered::One)
        {
            Triggered = ETriggered::None;
            bResult = true;
        }
        else if (Triggered == ETriggered::All)
        {
            bResult = true;
        }
        else if (Milliseconds != 0)
        {
            NumWaitingThreads++;

            if (Milliseconds == uint64(-1))
            {
                const auto Result = pthread_cond_wait(&Condition, &Mutex);
                Check(Result == 0);
            }
            else
            {
                const uint32 TimeMS = (StartTime.tv_usec / 1000) + Milliseconds;

                struct timespec TimeOut;
                TimeOut.tv_sec  = StartTime.tv_sec + (TimeMS / 1000);
                TimeOut.tv_nsec = (TimeMS % 1000) * 1000000;

                const auto Result = pthread_cond_timedwait(&Condition, &Mutex, &TimeOut);
                Check((Result == 0) || (Result == ETIMEDOUT));

                struct timeval Now;
                struct timeval Difference;
                gettimeofday(&Now, nullptr);

                SubtractTimevals(&Now, &StartTime, &Difference);

                const int32 DifferenceMS = ((Difference.tv_sec * 1000) + (Difference.tv_usec / 1000));
                Milliseconds = (((DifferenceMS >= Milliseconds) ? 0 : (Milliseconds - DifferenceMS)););
                StartTime    = Now;
            }
        }

    } while(!bResult && (Milliseconds != 0));

    UnlockMutex();
    return bResult;
}

void FMacEvent::Reset()
{
    Check(bInitialized == true);
    LockMutex();
    Triggered = ETriggerType::None;
    UnlockMutex();
}