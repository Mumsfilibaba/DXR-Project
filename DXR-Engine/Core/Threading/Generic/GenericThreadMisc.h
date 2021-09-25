#pragma once
#include "GenericThread.h"

class CGenericThreadMisc
{
public:
    static FORCEINLINE uint32 GetNumProcessors()
    {
        return 1;
    }

    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        return INVALID_THREAD_ID;
    }

    static FORCEINLINE void Sleep( CTimestamp )
    {
    }
};