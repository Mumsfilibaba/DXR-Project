#pragma once
#include "Core.h"

class GenericTime
{
public:
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        return 0;
    }

    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        return 1;
    }
};