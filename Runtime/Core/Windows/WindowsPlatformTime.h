#pragma once
#include "Windows.h"

#include "Core/Generic/GenericPlatformTime.h"

struct FWindowsPlatformTime 
    : public FGenericPlatformTime
{
    static FORCEINLINE uint64 QueryPerformanceCounter()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceCounter(&Counter);
        return Counter.QuadPart;
    }

    static FORCEINLINE uint64 QueryPerformanceFrequency()
    {
        LARGE_INTEGER Counter;
        ::QueryPerformanceFrequency(&Counter);
        return Counter.QuadPart;
    }
};