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

	static FORCEINLINE CMacThread* Make( ThreadFunction InFunction, const CString& InName )
	{
		return new CMacThread( InFunction, InName );
	}
	
    virtual bool Start() override final;

    virtual void WaitUntilFinished() override final;

	/* On macOS it only works to call this before the thread is started */
    virtual void SetName( const CString& InName ) override final;

    virtual PlatformThreadHandle GetPlatformHandle() override final;

private:
     CMacThread( ThreadFunction InFunction );
	 CMacThread( ThreadFunction InFunction, const CString& InName );
     ~CMacThread();

    static void* ThreadRoutine( void* ThreadParameter );

    pthread_t Thread;
	
	ThreadFunction Function;

	CString Name;
		
	bool IsRunning;
};

#endif
