#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Threading/Generic/GenericThreadMisc.h"

#include <unistd.h>
#include <pthread.h>

class CMacThreadMisc : public CGenericThreadMisc
{
public:

    /* Retreives the number of logical cores available on the system */
    static uint32 GetNumProcessors();

    static FORCEINLINE PlatformThreadHandle GetThreadHandle()
    {
        pthread_t CurrentThread = pthread_self();
        return reinterpret_cast<PlatformThreadHandle>(CurrentThread);
    }

    /* Make the current thread sleep for a specified amount of time */
    static FORCEINLINE void Sleep( CTimestamp Time )
    {
        float MicroSeconds = Time.AsMicroSeconds();
        usleep( static_cast<useconds_t>(MicroSeconds) );
    }
};
#endif
