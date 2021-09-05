#pragma once
#include "Core/Threading/Generic/GenericProcess.h"

class WindowsProcess : public GenericProcess
{
public:
    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        Memory::Memzero( &SystemInfo );

        GetSystemInfo( &SystemInfo );

        return SystemInfo.dwNumberOfProcessors;
    }

    static FORCEINLINE ThreadID GetThreadID()
    {
        DWORD CurrentID = GetCurrentThreadId();
        return (ThreadID)CurrentID;
    }

    static FORCEINLINE void Sleep( Timestamp Time )
    {
        DWORD Milliseconds = (DWORD)Time.AsMilliSeconds();
        ::Sleep( Milliseconds );
    }
};