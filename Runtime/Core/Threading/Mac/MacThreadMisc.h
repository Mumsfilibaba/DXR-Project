#pragma once

#if PLATFORM_MACOS
#include "MacRunLoop.h"

#include "Core/Threading/Interface/PlatformThreadMisc.h"

#include <unistd.h>
#include <pthread.h>
#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific interface for miscellaneous thread utility functions 

class CMacThreadMisc : public CPlatformThreadMisc
{
public:

    /**
     * @brief: Performs platform specific initialization of thread handling 
     * 
     * @return: Returns true if the initialization was successful, otherwise false
     */
    static FORCEINLINE bool Initialize() 
    { 
        // This must be executed on the mainthread
        MainThreadHandle = GetThreadHandle();

        // Then init the mainthread runloop
		    return RegisterMainRunLoop();
    }

    /** Releases platform specific resources for thread handling */
    static FORCEINLINE void Release() 
    {
		    UnregisterMainRunLoop();
    }

    /**
     * @brief: Retrieve the number of logical processors on the system 
     * 
     * @return: Returns the number of logical processors on the system 
     */
    static uint32 GetNumProcessors()
    {
        NSUInteger NumProcessors = [[NSProcessInfo processInfo] processorCount];
        return static_cast<uint32>(NumProcessors);
    }

    /**
     * @brief: Retrieves the current thread's system ID 
     * 
     * @return: Returns a platform handle for the calling thread, return a invalid handle on failure
     */
    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        pthread_t CurrentThread = pthread_self();
        return reinterpret_cast<PlatformThreadHandle>(CurrentThread);
    }

    /**
     * @brief: Makes the calling thread sleep for a specified amount of time 
     * 
     * @param Time: Time to sleep
     */
    static FORCEINLINE void Sleep(CTimestamp Time)
    {
		    // HACK: When the thread sleeps and we are on mainthread, run the mainloop
		    CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
		    CFRunLoopWakeUp(RunLoop);
		    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
		
        float MicroSeconds = Time.AsMicroSeconds();
        usleep(static_cast<useconds_t>(MicroSeconds));
    }

    /**
     * @brief: Checks weather or not the calling thread is the main thread 
     * 
     * @return: Returns true if the calling thread is the main-thread
     */
    static FORCEINLINE bool IsMainThread() 
    {
		    return [NSThread isMainThread];
    }

private:
    static PlatformThreadHandle MainThreadHandle;
};
#endif
