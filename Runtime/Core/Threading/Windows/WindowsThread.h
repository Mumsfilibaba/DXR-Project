#pragma once
#include "Core/Threading/Generic/GenericThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsThread

class CORE_API CWindowsThread final : public CGenericThread
{
private:

    CWindowsThread(ThreadFunction InFunction);
    CWindowsThread(ThreadFunction InFunction, const String& InName);
    ~CWindowsThread();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericThread Interface

    static TSharedRef<CWindowsThread> Make(ThreadFunction InFunction);
    static TSharedRef<CWindowsThread> Make(ThreadFunction InFunction, const String & InName);

    virtual bool Start() override final;

    virtual void WaitUntilFinished() override final;

    virtual void SetName(const String& InName) override final;

    virtual void* GetPlatformHandle() override final;

private:

    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE Thread;
    DWORD  hThreadID;

    String Name;

    ThreadFunction Function;
};