#pragma once

#if PLATFORM_MACOS
#include "MacRunLoop.h"

#include "Core/Threading/Interface/PlatformThreadMisc.h"

#include <unistd.h>
#include <pthread.h>

#if defined(__OBJC__)
#include <Foundation/Foundation.h>
#endif

class CMacThreadMisc : public CPlatformThreadMisc
{
public:

    /* Performs platform specific initialization of threadhandling */
    static FORCEINLINE bool Initialize() 
    { 
        // This must be executed on the mainthread
        MainThreadHandle = GetThreadHandle();

        // Then init the mainthread runloop
        CMacMainThread::Init();
		return true;
    }

    /* Releases platform specific resources for threadhandling */
    static FORCEINLINE void Release() 
    {
        CMacMainThread::Release();
    }

    /* Retreives the number of logical cores available on the system */
    static uint32 GetNumProcessors()
    {
        NSUInteger NumProcessors = [[NSProcessInfo processInfo] processorCount];
        return static_cast<uint32>(NumProcessors);
    }

    /* Retrieves the current thread's system ID */
    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        pthread_t CurrentThread = pthread_self();
        return reinterpret_cast<PlatformThreadHandle>(CurrentThread);
    }

    /* Make the current thread sleep for a specified amount of time */
    static FORCEINLINE void Sleep( CTimestamp Time )
    {
		// HACK: When the thread sleeps and we are on mainthread, run the mainloop
		if ( IsMainThread() )
		{
			CMacMainThread::Tick();
		}
		
        float MicroSeconds = Time.AsMicroSeconds();
        usleep( static_cast<useconds_t>(MicroSeconds) );
    }

    /* Checks weather or not the current thread is the main thread */
    static FORCEINLINE bool IsMainThread() 
    {
		return [NSThread isMainThread];
    }

private:
    static PlatformThreadHandle MainThreadHandle;
};
#endif
