#pragma once
#include "Time/Time.h"
#include "Core/Time/Generic/GenericTime.h"

#include <mach/mach_time.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacTime

class CMacTime : public CGenericTime
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericTime Interface

    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        return mach_absolute_time();
    }

    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        mach_timebase_info_data_t TimeBaseInfo = {};
        mach_timebase_info(&TimeBaseInfo);

        return NTime::FromSeconds<uint64>(uint64(TimeBaseInfo.numer)) / uint64(TimeBaseInfo.denom);
    }
};
