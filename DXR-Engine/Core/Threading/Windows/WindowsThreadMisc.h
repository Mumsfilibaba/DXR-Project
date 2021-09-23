#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Generic/GenericThreadMisc.h"

class CWindowsThreadMisc : public CGenericThreadMisc
{
public:

    /* Retreives the number of logical cores available on the system */
    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        Memory::Memzero( &SystemInfo );

        GetSystemInfo( &SystemInfo );

        return SystemInfo.dwNumberOfProcessors;
    }

    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        DWORD CurrentHandle = GetCurrentThreadId();
        return (PlatformThreadHandle)CurrentID;
    }

	/* Make the current thread sleep for a specified amount of time */
    static FORCEINLINE void Sleep( Timestamp Time )
    {
        DWORD Milliseconds = (DWORD)Time.AsMilliSeconds();
        ::Sleep( Milliseconds );
    }
};
#endif