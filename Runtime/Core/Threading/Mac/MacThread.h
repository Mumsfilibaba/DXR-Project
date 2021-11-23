#pragma once

#if PLATFORM_MACOS
#include "Core/Threading/Interface/PlatformThread.h"

#include <pthread.h>

class CMacThread final : public CPlatformThread
{
public:

    static TSharedRef<CMacThread> Make( ThreadFunction InFunction )
    {
        return new CMacThread( InFunction );
    }

    static TSharedRef<CMacThread> Make( ThreadFunction InFunction, const CString& InName )
    {
        return new CMacThread( InFunction, InName );
    }

	/* Starts the thread so that it can start perform work */
    virtual bool Start() override final;

    virtual void WaitUntilFinished() override final;

    /* On macOS it only works to call this before the thread is started */
    virtual void SetName( const CString& InName ) override final;

    virtual PlatformThreadHandle GetPlatformHandle() override final;

private:

    CMacThread( ThreadFunction InFunction );
    CMacThread( ThreadFunction InFunction, const CString& InName );
    ~CMacThread() = default;

    static void* ThreadRoutine( void* ThreadParameter );

    /* Native thread handle */
    pthread_t Thread;

    /* Function to run */
    ThreadFunction Function;

    /* Name of the thread */
    CString Name;

    /* Check if thread is running or not */
    bool IsRunning;
};

#endif
