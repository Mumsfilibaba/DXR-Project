#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for high performance counters

class CPlatformTime
{
public:

    /**
     * @brief: Query the current state of the performance counter
     *
     * @return: Returns the current value of the performance counter
     */
    static FORCEINLINE uint64 QueryPerformanceCounter() { return 0; }

    /**
     * @brief: Query the frequency of the performance counter
     *
     * @return: Returns the frequency of the performance counter
     */
    static FORCEINLINE uint64 QueryPerformanceFrequency() { return 1; }
};