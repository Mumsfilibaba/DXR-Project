#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Generic/GenericPlatformEvent.h"

class FMacPlatformEvent final : public FGenericPlatformEvent
{
    enum class ETriggerType : uint8
    {
        None = 0,
        One  = 1,
        All  = 2,
    };

public:
    static FGenericPlatformEvent* Create(bool bManualReset);
    static void Recycle(FGenericPlatformEvent* InEvent);

public:
    virtual void Trigger() override final;
    virtual void Wait(uint64 Milliseconds) override final;
    virtual void Reset() override final;

    virtual bool IsManualReset() const override final 
    { 
        return bManualReset;
    }

private:
    FMacPlatformEvent();
    ~FMacPlatformEvent();

    bool Initialize(bool bInManualReset);

    FORCEINLINE void LockMutex()
    {
        const int32 Result = pthread_mutex_lock(&Mutex);
        CHECK(Result == 0);
    }

    FORCEINLINE void UnlockMutex()
    {
        const int32 Result = pthread_mutex_unlock(&Mutex);
        CHECK(Result == 0);
    }

    bool                  bInitialized : 1;
    bool                  bManualReset : 1;
    volatile ETriggerType Triggered;
    volatile int32        NumWaitingThreads;
    pthread_mutex_t       Mutex;
    pthread_cond_t        Condition;
};

