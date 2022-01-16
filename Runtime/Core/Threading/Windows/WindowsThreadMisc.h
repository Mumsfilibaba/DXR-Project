#pragma once

#if PLATFORM_WINDOWS
#include "Core/CoreModule.h"
#include "Core/Threading/Interface/PlatformThreadMisc.h"

class CWindowsThreadMisc : public CPlatformThreadMisc
{
public:

    /* Performs platform specific initialization of threadhandling */
    static FORCEINLINE bool Initialize()
    {
        // This must be executed on the mainthread
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