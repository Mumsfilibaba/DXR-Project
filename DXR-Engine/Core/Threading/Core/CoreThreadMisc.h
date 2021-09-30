#pragma once
#include "CoreThread.h"

class CCoreThreadMisc
{
public:
    static FORCEINLINE uint32 GetNumProcessors()
    {
        return 1;
    }

    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        return static_cast<PlatformThreadHandle>(INVALID_THREAD_ID);
    }

    static FORCEINLINE void Sleep( CTimestamp )
    {
    }
};