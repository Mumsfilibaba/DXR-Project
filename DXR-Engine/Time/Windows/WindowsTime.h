#pragma once
#include "Time/Generic/GenericTime.h"

#include "Windows/Windows.h"

class WindowsTime : public GenericTime
{
public:
    static FORCEINLINE UInt64 QueryPerformanceCounter()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceCounter(&Counter);
        return Counter.QuadPart;
    }

    static FORCEINLINE UInt64 QueryPerformanceFrequency()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceFrequency(&Counter);
        return Counter.QuadPart;
    }
};