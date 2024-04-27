#pragma once
#include "Core/Generic/GenericThread.h"

#include <pthread.h>

class FMacThread final : public FGenericThread
{
public:
    static FGenericThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true);

    virtual ~FMacThread() = default;
    
    virtual bool Start() override final;
    virtual void Kill(bool bWaitUntilCompletion) override final;

    virtual void Suspend() override final
    {
        // NOTE: Not supported on macOS
    }

    virtual void Resume() override final
    {
        // NOTE: Not supported on macOS
    }

    virtual void WaitForCompletion() override final;
    virtual void* GetPlatformHandle() override final;

private:
    static void* ThreadRoutine(void* ThreadParameter);

    FMacThread(FRunnable* InRunnable, const CHAR* ThreadName);

    pthread_t Thread;
    bool      bIsRunning;
};
