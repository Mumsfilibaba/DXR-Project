#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Time/Generic/GenericTime.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsTime

class CWindowsTime : public CGenericTime
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericTime Interface

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