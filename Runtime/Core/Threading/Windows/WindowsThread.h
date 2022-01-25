#pragma once

#if PLATFORM_WINDOWS
#include "Core/Threading/Interface/PlatformThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows-specific interface for threads

class CORE_API CWindowsThread final : public CPlatformThread
{
public:

    /**
     * Create a new thread
     *
     * @param InFunction: Entry-point for the new thread
     * @return: An newly created thread interface
     */
    static TSharedRef<CWindowsThread> Make(ThreadFunction InFunction);
    
    /**
      * Create a new thread with a name
      *
      * @param InFunction: Entry-point for the new thread
      * @param InName: Name of the new thread
      * @return: An newly created thread interface
      */
    static TSharedRef<CWindowsThread> Make(ThreadFunction InFunction, const CString & InName);

    /**
     * Start thread-execution
     *
     * @return: Returns true if the thread started successfully
     */
    virtual bool Start() override final;

    /** Wait until thread has finished running */
    virtual void WaitUntilFinished() override final;

    /**
     * Set name of thread. Needs to be called before start on some platforms for the changes to take effect
     *
     * @param InName: New name of the thread
     */
    virtual void SetName(const CString& InName) override final;

    /**
     * Retrieve platform specific handle
     *
     * @return: Returns a platform specific handle or zero if no platform handle is defined
     */
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