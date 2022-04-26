#pragma once

#if PLATFORM_WINDOWS
#include "Core/Windows/Windows.h"
#include "Core/Time/Interface/PlatformTime.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows specific interface for high performance counters

class CWindowsTime : public CPlatformTime
{
public:

    /**
     * @brief: Query the current state of the performance counter
     *
     * @return: Returns the current value of the performance counter
     */
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceCounter(&Counter);
        return Counter.QuadPart;
    }

    /**
     * @brief: Query the frequency of the performance counter
     *
     * @return: Returns the frequency of the performance counter
     */
    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceFrequency(&Counter);
        return Counter.QuadPart;
    }
};
#endif