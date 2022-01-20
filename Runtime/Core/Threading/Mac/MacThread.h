#pragma once

#if PLATFORM_MACOS
#include "Core/Threading/Interface/PlatformThread.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac interface for threads

class CMacThread final : public CPlatformThread
{
public:

    /* Create a new thread */
    static TSharedRef<CMacThread> Make(ThreadFunction InFunction) { return new CMacThread(InFunction); }
    /* Create a new thread with a name*/
    static TSharedRef<CMacThread> Make(ThreadFunction InFunction, const CString& InName) { return new CMacThread(InFunction, InName); }

	/* Starts the thread so that it can start perform work */
    virtual bool Start() override final;

    /* Wait until function has finished running */
    virtual void WaitUntilFinished() override final;

    /* On macOS it only works to call this before the thread is started */
    virtual void SetName(const CString& InName) override final;

    virtual PlatformThreadHandle GetPlatformHandle() override final;

private:

    CMacThread(ThreadFunction InFunction);
    CMacThread(ThreadFunction InFunction, const CString& InName);
    ~CMacThread() = default;

    static void* ThreadRoutine(void* ThreadParameter);

    /* Native thread handle */
    pthread_t Thread;

    /* Function to run */
    ThreadFunction Function;

    /* Name of the thread */
    CString Name;

    /* Check if thread is running or not */
    bool bIsRunning;
};

#endif
