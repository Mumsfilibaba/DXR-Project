#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Time/Core/CoreTime.h"

#include <mach/mach_time.h>

class CMacTime : public CCoreTime
{
public:

    /* Query the current state of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        return mach_absolute_time();
    }

    /* Query the frequency of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        mach_timebase_info_data_t TimeBaseInfo = {};
        mach_timebase_info( &TimeBaseInfo );

        // Ensure that the frequency returns nanoseconds
        constexpr uint64 NANOSECONDS = 1000 * 1000 * 1000;
        return ((NANOSECONDS * uint64( TimeBaseInfo.numer )) / uint64( TimeBaseInfo.denom ));
    }
};

#endif
