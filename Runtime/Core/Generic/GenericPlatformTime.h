#pragma once
#include "Core/Core.h"

struct FGenericPlatformTime
{
    /** @return - Returns the current value of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceCounter() { return 0; }

    /** @return - Returns the frequency of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceFrequency() { return 1; }
};