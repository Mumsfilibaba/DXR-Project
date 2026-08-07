#pragma once
#include "Core/Generic/GenericPlatformTime.h"
#include "Core/Time/Time.h"
#include <mach/mach_time.h>

struct FMacPlatformTime : public FGenericPlatformTime
{
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        return mach_absolute_time();
    }

    static FORCEINLINE uint64 TicksPerSecondFromTimebase(uint32 Numerator, uint32 Denominator)
    {
        return Time::FromSeconds<uint64>(uint64(Denominator)) / uint64(Numerator);
    }

    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        mach_timebase_info_data_t TimeBaseInfo = {};
        mach_timebase_info(&TimeBaseInfo);

        return TicksPerSecondFromTimebase(TimeBaseInfo.numer, TimeBaseInfo.denom);
    }
};
