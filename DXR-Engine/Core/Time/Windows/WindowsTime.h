#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Time/Core/CoreTime.h"
#include "Core/Windows/Windows.h"

class CWindowsTime : public CCoreTime
{
public:

    /* Query the current state of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceCounter( &Counter );
        return Counter.QuadPart;
    }

    /* Query the frequency of the performance counter */
    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceFrequency( &Counter );
        return Counter.QuadPart;
    }
};
#endif