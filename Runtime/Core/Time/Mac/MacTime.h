#pragma once

#if PLATFORM_MACOS
#include "Time/Time.h"
#include "Core/Time/Interface/PlatformTime.h"

#include <mach/mach_time.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific interface for high performance counters

class CMacTime : public CPlatformTime
{
public:

    /**
     * Query the current state of the performance counter
     *
     * @return: Returns the current value of the performance counter
     */
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        return mach_absolute_time();
    }

    /**
     * Query the frequency of the performance counter
     *
     * @return: Returns the frequency of the performance counter
     */
    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        mach_timebase_info_data_t TimeBaseInfo = {};
        mach_timebase_info(&TimeBaseInfo);

        return NTime::FromSeconds<uint64>(uint64(TimeBaseInfo.numer)) / uint64(TimeBaseInfo.denom);
    }
};

#endif
