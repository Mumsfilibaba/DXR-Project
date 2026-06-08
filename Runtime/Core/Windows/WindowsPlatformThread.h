#pragma once
#include "Core/Containers/Function.h"
#include "Core/Generic/GenericPlatformThread.h"

class CORE_API FWindowsPlatformThread final : public FGenericPlatformThread
{
public:
    static FGenericPlatformThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true);

public:
    virtual ~FWindowsPlatformThread();
    
    virtual bool Start() override final;
    virtual void Kill(bool bWaitUntilCompletion) override final;
    virtual void Suspend() override final;
    virtual void Resume() override final;
    virtual void WaitForCompletion() override final;
    virtual void* GetPlatformHandle() override final;

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    FWindowsPlatformThread(FRunnable* InRunnable, const CHAR* InThreadName, bool bSuspended);

    HANDLE Thread;
    DWORD  hThreadID;
    bool   bIsSuspended;
};
