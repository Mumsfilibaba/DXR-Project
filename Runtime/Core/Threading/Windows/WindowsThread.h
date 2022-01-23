#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Interface/PlatformThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows-specific interface for threads

class CORE_API CWindowsThread final : public CPlatformThread
{
public:

    /* Create a new thread */
    static TSharedRef<CWindowsThread> Make(ThreadFunction InFunction);
    /* Create a new thread with a name */
    static TSharedRef<CWindowsThread> Make(ThreadFunction InFunction, const CString & InName);

    /* Start thread-execution */
    virtual bool Start() override final;

    /* Wait until function has finished running */
    virtual void WaitUntilFinished() override final;

    /* Set name of thread. Needs to be called before start on some platforms for the changes to take effect */
    virtual void SetName(const CString& InName) override final;

    /* Retrieve the platform specific handle for the thread */
    virtual PlatformThreadHandle GetPlatformHandle() override final;

private:

    CWindowsThread(ThreadFunction InFunction);
    CWindowsThread(ThreadFunction InFunction, const CString& InName);
    ~CWindowsThread();

    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE Thread;
    DWORD  hThreadID;

    CString Name;

    ThreadFunction Function;
};
#endif