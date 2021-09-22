#pragma once
#include "Core/Time/Generic/GenericTime.h"
#include "Core/Windows/Windows.h"

class WindowsTime : public GenericTime
{
public:
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceCounter( &Counter );
        return Counter.QuadPart;
    }

    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceFrequency( &Counter );
        return Counter.QuadPart;
    }
};