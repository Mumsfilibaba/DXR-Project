#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Core/CoreThreadMisc.h"

class CWindowsThreadMisc : public CCoreThreadMisc
{
public:

    /* Retrieves the number of logical cores available on the system */
    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        Memory::Memzero( &SystemInfo );

        GetSystemInfo( &SystemInfo );

        return SystemInfo.dwNumberOfProcessors;
    }

    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        DWORD CurrentHandle = ::GetCurrentThreadId();
        return (PlatformThreadHandle)CurrentHandle;
    }

    /* Make the current thread sleep for a specified amount of time */
    static FORCEINLINE void Sleep( CTimestamp Time )
    {
        DWORD Milliseconds = (DWORD)Time.AsMilliSeconds();
        ::Sleep( Milliseconds );
    }
};
#endif