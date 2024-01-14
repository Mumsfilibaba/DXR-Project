#include "MacEvent.h"
#include "Core/Platform/PlatformInterlocked.h"
#include "Core/Templates/NumericLimits.h"

#include <sys/time.h>

FMacEvent::FMacEvent()
    : bInitialized(false)
    , bManualReset(false)
    , Triggered(ETriggerType::None)
    , NumWaitingThreads(0)
    , Mutex()
    , Condition()
{
}

FMacEvent::~FMacEvent()
{
    if (bInitialized)
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
    CHECK(bInitialized == false);
    
    Triggered    = ETriggerType::None;
    bManualReset = bInManualReset;

    bool bResult = false;
    if (pthread_mutex_init(&Mutex, nullptr) == 0)
    {
        if (pthread_cond_init(&Condition, nullptr) == 0)
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
    CHECK(bInitialized == true);

    LockMutex();

    if (bManualReset)
    {
        Triggered = ETriggerType::All;
        const auto Result = pthread_cond_broadcast(&Condition);
        CHECK(Result == 0);
    }
    else 
    {
        Triggered = ETriggerType::One;
        const auto Result = pthread_cond_signal(&Condition);
        CHECK(Result == 0);
    }

    UnlockMutex();
}

void FMacEvent::Wait(uint64 Milliseconds)
{
    CHECK(bInitialized == true);
    
    struct timeval StartTime;
    if ((Milliseconds > 0) && (Milliseconds != TNumericLimits<uint64>::Max()))
    {
        ::gettimeofday(&StartTime, nullptr);
    }

    LockMutex();

    bool bResult = false;
    do 
    {
        if (Triggered == ETriggerType::One)
        {
            Triggered = ETriggerType::None;
            bResult = true;
        }
        else if (Triggered == ETriggerType::All)
        {
            bResult = true;
        }
        else if (Milliseconds != 0)
        {
            FMacInterlocked::InterlockedIncrement(&NumWaitingThreads);
            
            if (Milliseconds == uint64(-1))
            {
                const auto Result = pthread_cond_wait(&Condition, &Mutex);
                CHECK(Result == 0);
            }
            else
            {
                const uint64 TimeMS = (StartTime.tv_usec / 1000) + Milliseconds;

                struct timespec TimeOut;
                TimeOut.tv_sec  = StartTime.tv_sec + (TimeMS / 1000);
                TimeOut.tv_nsec = (TimeMS % 1000) * 1000000;

                const auto Result = pthread_cond_timedwait(&Condition, &Mutex, &TimeOut);
                CHECK((Result == 0) || (Result == ETIMEDOUT));

                struct timeval Now;
                struct timeval Difference;
                gettimeofday(&Now, nullptr);

                SubtractTimevals(&Now, &StartTime, &Difference);

                const uint64 DifferenceMS = ((Difference.tv_sec * 1000) + (Difference.tv_usec / 1000));
                Milliseconds = ((DifferenceMS >= Milliseconds) ? 0 : (Milliseconds - DifferenceMS));
                StartTime    = Now;
            }
            
            FMacInterlocked::InterlockedDecrement(&NumWaitingThreads);
            CHECK(NumWaitingThreads >= 0);
        }

    } while(!bResult && (Milliseconds != 0));

    UnlockMutex();
}

void FMacEvent::Reset()
{
    CHECK(bInitialized == true);
    LockMutex();
    Triggered = ETriggerType::None;
    UnlockMutex();
}
