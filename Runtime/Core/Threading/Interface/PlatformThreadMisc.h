#pragma once
#include "PlatformThread.h"

#include "Core/Time/Timestamp.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for miscellaneous thread utility functions

class CPlatformThreadMisc
{
public:

    /**
     * Performs platform specific initialization of thread handling 
     * 
     * @return: Returns true if the initialization was successful, otherwise false
     */
    static FORCEINLINE bool Initialize() { return true; }

    /** Releases platform specific resources for thread handling */
    static FORCEINLINE void Release() { }

    /**
     * Retrieve the number of logical processors on the system 
     * 
     * @return: Returns the number of logical processors on the system 
     */
    static FORCEINLINE uint32 GetNumProcessors() { return 1; }

    /**
     * Retrieves the current thread's system ID 
     * 
     * @return: Returns a platform handle for the calling thread, return a invalid handle on failure
     */
    static FORCEINLINE PlatformThreadHandle GetThreadHandle() { return static_cast<PlatformThreadHandle>(INVALID_THREAD_ID); }

    /**
     * Makes the calling thread sleep for a specified amount of time 
     * 
     * @param Time: Time to sleep
     */
    static FORCEINLINE void Sleep(CTimestamp Time) { }

    /**
     * Checks weather or not the calling thread is the main thread 
     * 
     * @return: Returns true if the calling thread is the main-thread
     */
    static FORCEINLINE bool IsMainThread() { return false; }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
