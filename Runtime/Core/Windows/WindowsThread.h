#pragma once
#include "Core/Containers/Function.h"
#include "Core/Generic/GenericThread.h"

class CORE_API FWindowsThread final : public FGenericThread
{
public:
    static FGenericThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true);

public:
    virtual ~FWindowsThread();
    
    virtual bool Start() override final;
    virtual void Kill(bool bWaitUntilCompletion) override final;
    virtual void Suspend() override final;
    virtual void Resume() override final;
    virtual void WaitForCompletion() override final;
    virtual void* GetPlatformHandle() override final;

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    FWindowsThread(FRunnable* InRunnable, const CHAR* InThreadName, bool bSuspended);

    HANDLE Thread;
    DWORD  hThreadID;
    bool   bIsSuspended;
};