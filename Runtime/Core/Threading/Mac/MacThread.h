#pragma once

#if PLATFORM_MACOS
#include "Core/Threading/Interface/PlatformThread.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac interface for threads

class CMacThread final : public CPlatformThread
{
public:

    /**
     * Create a new thread 
     * 
     * @param InFunction: Entry-point for the new thread
     * @return: An newly created thread interface
     */
    static TSharedRef<CMacThread> Make(ThreadFunction InFunction) { return new CMacThread(InFunction); }
    
    /**
      * Create a new thread with a name
      *
      * @param InFunction: Entry-point for the new thread
      * @param InName: Name of the new thread
      * @return: An newly created thread interface
      */
    static TSharedRef<CMacThread> Make(ThreadFunction InFunction, const String& InName) { return new CMacThread(InFunction, InName); }

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
    virtual void SetName(const String& InName) override final;

    /**
     * Retrieve platform specific handle
     *
     * @return: Returns a platform specific handle or zero if no platform handle is defined
     */
    virtual PlatformThreadHandle GetPlatformHandle() override final;

private:

    CMacThread(ThreadFunction InFunction);
    CMacThread(ThreadFunction InFunction, const String& InName);
    ~CMacThread() = default;

    static void* ThreadRoutine(void* ThreadParameter);

    /* Native thread handle */
    pthread_t Thread;

    /* Function to run */
    ThreadFunction Function;

    /* Name of the thread */
    String Name;

    /* Check if thread is running or not */
    bool bIsRunning;
};

#endif
