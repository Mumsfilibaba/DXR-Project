#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Threading/Generic/GenericThread.h"

#include <pthread.h>

class CMacThread : public CGenericThread
{
public:

    static FORCEINLINE CMacThread* Make( ThreadFunction InFunction )
    {
        return new CMacThread( InFunction );
    }

    virtual bool Start() override final;

    virtual void WaitUntilFinished() override final;

    virtual void SetName( const std::string& Name ) override final;

    virtual PlatformThreadHandle GetPlatformHandle() override final;

private:
     CMacThread( ThreadFunction InFunction );
     ~CMacThread();

    static void* ThreadRoutine( void* ThreadParameter );

    pthread_t Thread;

    ThreadFunction Function;
};

#endif