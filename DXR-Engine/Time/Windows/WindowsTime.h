#pragma once
#include "Time/Generic/GenericTime.h"

#include "Windows/Windows.h"

class WindowsTime : public GenericTime
{
public:
    FORCEINLINE static uint64 QueryPerformanceCounter()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceCounter( &Counter );
        return Counter.QuadPart;
    }

    FORCEINLINE static uint64 QueryPerformanceFrequency()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceFrequency( &Counter );
        return Counter.QuadPart;
    }
};