#pragma once

#if PLATFORM_WINDOWS
#include "Core/Core.h"
#include "Core/Threading/Interface/PlatformThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for miscellaneous thread utility functions

class CWindowsThreadMisc : public CPlatformThreadMisc
{
public:

    /* Performs platform specific initialization of thread handling */
    static FORCEINLINE bool Initialize()
    {
        // This must be executed on the main-thread
        MainThreadHandle = GetThreadHandle();
        return true;
    }

    /* Retrieves the number of logical cores available on the system */
    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        CMemory::Memzero(&SystemInfo);

        GetSystemInfo(&SystemInfo);

        return static_cast<uint32>(SystemInfo.dwNumberOfProcessors);
    }

    /* Retrieves the current thread's system ID */
    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        DWORD CurrentHandle = GetCurrentThreadId();
        return static_cast<PlatformThreadHandle>(CurrentHandle);
    }

    /* Make the current thread sleep for a specified amount of time */
    static FORCEINLINE void Sleep(CTimestamp Time)
    {
        DWORD Milliseconds = static_cast<DWORD>(Time.AsMilliSeconds());
        ::Sleep(Milliseconds);
    }

    /* Checks weather or not the current thread is the main thread */
    static FORCEINLINE bool IsMainThread()
    {
        return (MainThreadHandle == GetThreadHandle());
    }

private:
    static CORE_API PlatformThreadHandle MainThreadHandle;
};

#endif