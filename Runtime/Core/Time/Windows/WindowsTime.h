#pragma once

#if PLATFORM_WINDOWS
#include "Core/Windows/Windows.h"
#include "Core/Time/Interface/PlatformTimeInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows specific interface for high performance counters

class CWindowsTime : public CPlatformTimeInterface
{
public:

    /* Query the current state of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceCounter(&Counter);
        return Counter.QuadPart;
    }

    /* Query the frequency of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceFrequency(&Counter);
        return Counter.QuadPart;
    }
};
#endif