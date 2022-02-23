#pragma once

#if PLATFORM_WINDOWS
#include "Core/Core.h"
#include "Core/Threading/Interface/PlatformThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for miscellaneous thread utility functions

class CWindowsThreadMisc : public CPlatformThreadMisc
{
public:

    /**
     * Performs platform specific initialization of thread handling
     *
     * @return: Returns true if the initialization was successful, otherwise false
     */
    static FORCEINLINE bool Initialize()
    {
        // This must be executed on the main-thread
        MainThreadHandle = GetThreadHandle();
        return true;
    }

    /**
     * Retrieve the number of logical processors on the system
     *
     * @return: Returns the number of logical processors on the system
     */
    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        Memory::Memzero(&SystemInfo);

        GetSystemInfo(&SystemInfo);

        return static_cast<uint32>(SystemInfo.dwNumberOfProcessors);
    }

    /**
     * Retrieves the current thread's system ID
     *
     * @return: Returns a platform handle for the calling thread, return a invalid handle on failure
     */
    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        DWORD CurrentHandle = GetCurrentThreadId();
        return static_cast<PlatformThreadHandle>(CurrentHandle);
    }

    /**
     * Makes the calling thread sleep for a specified amount of time
     *
     * @param Time: Time to sleep
     */
    static FORCEINLINE void Sleep(CTimestamp Time)
    {
        DWORD Milliseconds = static_cast<DWORD>(Time.AsMilliSeconds());
        ::Sleep(Milliseconds);
    }

    /**
     * Checks weather or not the calling thread is the main thread
     *
     * @return: Returns true if the calling thread is the main-thread
     */
    static FORCEINLINE bool IsMainThread()
    {
        return (MainThreadHandle == GetThreadHandle());
    }

private:
    static CORE_API PlatformThreadHandle MainThreadHandle;
};

#endif