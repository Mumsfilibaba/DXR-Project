#include "Core/Mac/MacPlatformEvent.h"
#include "Core/Platform/PlatformAtomic.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Templates/NumericLimits.h"
#include <sys/time.h>
#include <time.h>
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

        while (FPlatformAtomic::Read(&NumWaitingThreads) > 0)
        {
            UnlockMutex();
            FPlatformThreadMisc::Yield();
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

    struct timespec StartTime = {};
    if ((Milliseconds > 0) && (Milliseconds != TNumericLimits<uint64>::Max()))
    {
        ::clock_gettime(CLOCK_MONOTONIC, &StartTime);
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
                struct timespec TimeOut;
                TimeOut.tv_sec  = static_cast<time_t>(Milliseconds / 1000);
                TimeOut.tv_nsec = static_cast<long>((Milliseconds % 1000) * 1000000);

                const auto Result = pthread_cond_timedwait_relative_np(&Condition, &Mutex, &TimeOut);
                CHECK((Result == 0) || (Result == ETIMEDOUT));

                struct timespec Now;
                ::clock_gettime(CLOCK_MONOTONIC, &Now);

                const int64  ElapsedNS = (static_cast<int64>(Now.tv_sec - StartTime.tv_sec) * 1000000000ll) + static_cast<int64>(Now.tv_nsec - StartTime.tv_nsec);
                const uint64 ElapsedMS = (ElapsedNS > 0) ? static_cast<uint64>(ElapsedNS / 1000000ll) : 0;

                Milliseconds = ((ElapsedMS >= Milliseconds) ? 0 : (Milliseconds - ElapsedMS));
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
