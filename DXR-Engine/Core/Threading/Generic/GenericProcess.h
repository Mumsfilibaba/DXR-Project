#pragma once
#include "GenericThread.h"

class GenericProcess
{
public:
    FORCEINLINE static uint32 GetNumProcessors()
    {
        return 1;
    }

    FORCEINLINE static ThreadID GetThreadID()
    {
        return INVALID_THREAD_ID;
    }

    FORCEINLINE static void Sleep( Timestamp Time )
    {
        UNREFERENCED_VARIABLE( Time );
    }
};