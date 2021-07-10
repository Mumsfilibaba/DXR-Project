#pragma once
#include "Core/Threading/Generic/GenericProcess.h"

class WindowsProcess : public GenericProcess
{
public:
    FORCEINLINE static uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        Memory::Memzero( &SystemInfo );

        GetSystemInfo( &SystemInfo );

        return SystemInfo.dwNumberOfProcessors;
    }

    FORCEINLINE static ThreadID GetThreadID()
    {
        DWORD CurrentID = GetCurrentThreadId();
        return (ThreadID)CurrentID;
    }

    FORCEINLINE static void Sleep( Timestamp Time )
    {
        DWORD Milliseconds = (DWORD)Time.AsMilliSeconds();
        ::Sleep( Milliseconds );
    }
};