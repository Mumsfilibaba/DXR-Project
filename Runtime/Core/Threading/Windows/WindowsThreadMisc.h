#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Interface/PlatformThreadMisc.h"

class CWindowsThreadMisc : public CPlatformThreadMisc
{
public:

    /* Retrieves the number of logical cores available on the system */
    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        CMemory::Memzero( &SystemInfo );

        GetSystemInfo( &SystemInfo );

        return static_cast<uint32>(SystemInfo.dwNumberOfProcessors);
    }

    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        DWORD CurrentHandle = ::GetCurrentThreadId();
        return static_cast<PlatformThreadHandle>(CurrentHandle);
    }

    /* Make the current thread sleep for a specified amount of time */
    static FORCEINLINE void Sleep( CTimestamp Time )
    {
        DWORD Milliseconds = static_cast<DWORD>(Time.AsMilliSeconds());
        ::Sleep( Milliseconds );
    }
};
#endif