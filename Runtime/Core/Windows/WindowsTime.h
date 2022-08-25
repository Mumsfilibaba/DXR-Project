#pragma once
#include "Core/Generic/GenericTime.h"
#include "Core/Windows/Windows.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsTime

struct FWindowsTime 
    : public FGenericTime
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