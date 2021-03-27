#pragma once
#include "Core.h"

class GenericTime
{
public:
    FORCEINLINE static uint64 QueryPerformanceCounter()
    {
        return 0;
    }

    FORCEINLINE static uint64 QueryPerformanceFrequency()
    {
        return 1;
    }
};