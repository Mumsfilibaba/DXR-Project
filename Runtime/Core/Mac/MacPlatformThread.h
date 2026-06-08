#pragma once
#include "Core/Generic/GenericPlatformThread.h"
#include <pthread.h>

class FMacPlatformThread final : public FGenericPlatformThread
{
public:

    // Create a new MacThread
    static FGenericPlatformThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true);

public:
    virtual ~FMacPlatformThread() = default;

    virtual bool Start() override final;
    virtual void Kill(bool bWaitUntilCompletion) override final;

    // NOTE: Not supported on macOS
    virtual void Suspend() override final { }

    // NOTE: Not supported on macOS
    virtual void Resume() override final { }

    virtual void WaitForCompletion() override final;
    virtual void* GetPlatformHandle() override final;

private:
    static void* ThreadRoutine(void* ThreadParameter);

    FMacPlatformThread(FRunnable* InRunnable, const CHAR* ThreadName);

    pthread_t Thread;
};
