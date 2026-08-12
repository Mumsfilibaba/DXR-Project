#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/Function.h"
#include "Core/PlatformInterface/IPlatformThread.h"

class CORE_API FWindowsPlatformThread final : public IPlatformThread
{
public:
    static IPlatformThread* Create(FRunnable* Runnable, const CHAR* ThreadName, bool bSuspended = true);

public:
    virtual ~FWindowsPlatformThread();

    virtual bool Start() override final;
    virtual void Kill(bool bWaitUntilCompletion) override final;
    virtual void Suspend() override final;
    virtual void Resume() override final;
    virtual void WaitForCompletion() override final;
    virtual void* GetPlatformHandle() override final;

    virtual const String& GetName() const override final
    {
        return Name;
    }

    virtual FRunnable* GetRunnable() const override final
    {
        return Runnable;
    }

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    FWindowsPlatformThread(FRunnable* InRunnable, const CHAR* InThreadName, bool bSuspended);

    String     Name;
    FRunnable* Runnable;
    HANDLE     Thread;
    DWORD      hThreadID;
    bool       bIsSuspended;
};
