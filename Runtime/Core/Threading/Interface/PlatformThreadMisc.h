#pragma once
#include "CoreThread.h"

#include "Core/Time/Timestamp.h"

class CPlatformThreadMisc
{
public:

    /* Returns the number of logical processors on the system */
    static FORCEINLINE uint32 GetNumProcessors() { return 1; }

    /* Retrieves the current thread's system ID */
    static FORCEINLINE PlatformThreadHandle GetThreadHandle() { return static_cast<PlatformThreadHandle>(INVALID_THREAD_ID); }

    /* Makes the current threas sleep for a specified amount of time */
    static FORCEINLINE void Sleep( CTimestamp ) { }
};