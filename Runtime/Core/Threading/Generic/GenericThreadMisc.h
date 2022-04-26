#pragma once
#include "GenericThread.h"

#include "Core/Time/Timestamp.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericThreadMisc

class CGenericThreadMisc
{
public:

    /** @return: Performs platform specific thread-initialization and returns true if successful, otherwise false */
    static FORCEINLINE bool Initialize() { return true; }

    /** @brief: Releases platform specific resources for thread handling */
    static FORCEINLINE void Release() { }

    /** @return: Returns the number of logical processors on the system  */
    static FORCEINLINE uint32 GetNumProcessors() { return 1; }

    /** @return: Returns a platform handle for the calling thread, return a invalid handle on failure */
    static FORCEINLINE void* GetThreadHandle() { return nullptr; }

    /**
     * @brief: Makes the calling thread sleep for a specified amount of time 
     * 
     * @param Time: Time to sleep
     */
    static FORCEINLINE void Sleep(CTimestamp Time) { }

    /** @return: Returns true if the calling thread is the main-thread */
    static FORCEINLINE bool IsMainThread() { return false; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
