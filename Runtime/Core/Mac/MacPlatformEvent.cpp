#include "Core/Mac/MacPlatformEvent.h"
#include "Core/Platform/PlatformAtomic.h"
#include "Core/Templates/NumericLimits.h"
#include <sys/time.h>
#include <Foundation/Foundation.h>

FGenericPlatformEvent* FMacPlatformEvent::Create(bool bManualReset)
{
    FMacPlatformEvent* NewEvent = new FMacPlatformEvent();
    if (!NewEvent->Initialize(bManualReset))
    {
        delete NewEvent;
        return nullptr;
    }

    return NewEvent;
}

void FMacPlatformEvent::Recycle(FGenericPlatformEvent* InEvent)
{
    FMacPlatformEvent* MacEvent = static_cast<FMacPlatformEvent*>(InEvent);
    if (MacEvent)
    {
        delete MacEvent;
    }
}

FMacPlatformEvent::FMacPlatformEvent()
    : bInitialized(false)
    , bManualReset(false)
    , Triggered(ETriggerType::None)
    , NumWaitingThreads(0)
    , Mutex()
    , Condition()
{
}

FMacPlatformEvent::~FMacPlatformEvent()
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

bool FMacPlatformEvent::Initialize(bool bInManualReset)
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

void FMacPlatformEvent::Trigger()
{
    CHECK(bInitialized == true);

    LockMutex();

    if (bManualReset)
    {
        Triggered = ETriggerType::All;
        const int32 Result = pthread_cond_broadcast(&Condition);
        CHECK(Result == 0);
    }
    else 
    {
        Triggered = ETriggerType::One;
        const int32 Result = pthread_cond_signal(&Condition);
        CHECK(Result == 0);
    }

    UnlockMutex();
}

void FMacPlatformEvent::Wait(uint64 Milliseconds)
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
            FPlatformAtomic::InterlockedIncrement(&NumWaitingThreads);
            
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
            
            FPlatformAtomic::InterlockedDecrement(&NumWaitingThreads);
            CHECK(NumWaitingThreads >= 0);
        }

    } while(!bResult && (Milliseconds != 0));

    UnlockMutex();
}

void FMacPlatformEvent::Reset()
{
    CHECK(bInitialized == true);
    LockMutex();
    Triggered = ETriggerType::None;
    UnlockMutex();
}
