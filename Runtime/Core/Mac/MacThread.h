#pragma once
#include "Core/Generic/GenericThread.h"
#include <pthread.h>

class FMacThread final : public FGenericThread
{
public:

    // Create a new MacThread
    static FGenericThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true);

public:
    virtual ~FMacThread() = default;

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

    FMacThread(FRunnable* InRunnable, const CHAR* ThreadName);

    pthread_t Thread;
};
