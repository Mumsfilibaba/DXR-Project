#pragma once
#include "GenericThread.h"

class GenericProcess
{
public:
    static FORCEINLINE uint32 GetNumProcessors()
    {
        return 1;
    }

    static FORCEINLINE ThreadID GetThreadID()
    {
        return INVALID_THREAD_ID;
    }

    static FORCEINLINE void Sleep( Timestamp Time )
    {
        UNREFERENCED_VARIABLE( Time );
    }
};