#pragma once
#include "Core/PlatformInterface/IPlatformThread.h"
#include <pthread.h>
#include <pthread/qos.h>

class FMacPlatformThread final : public IPlatformThread
{
public:
    static IPlatformThread* Create(FRunnable* Runnable, const CHAR_T* ThreadName, bool bSuspended = true);

public:
    virtual ~FMacPlatformThread();

    virtual bool Start() override final;
    virtual void Kill(bool bWaitUntilCompletion) override final;

    virtual void Suspend() override final { }
    virtual void Resume() override final { }

    virtual void WaitForCompletion() override final;
    virtual void* GetPlatformHandle() override final;

    virtual const FString& GetName() const override final
    {
        return Name;
    }

    virtual FRunnable* GetRunnable() const override final
    {
        return Runnable;
    }

private:
    static void* ThreadRoutine(void* ThreadParameter);

    FMacPlatformThread(FRunnable* InRunnable, const CHAR_T* ThreadName);

    FString    Name;
    FRunnable* Runnable;
    pthread_t  Thread;
    bool       bIsJoinable;
};
