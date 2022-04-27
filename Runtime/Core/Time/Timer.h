#pragma once
#include "Timestamp.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CTimer

class CORE_API CTimer
{
public:

    CTimer();
    ~CTimer() = default;

    /** @brief: Measure time between this and last call to tick */
    void Tick();

    /** @brief: Resets the time to zero */
    FORCEINLINE void Reset()
    {
        DeltaTime = CTimestamp(0);
        TotalTime = CTimestamp(0);
    }

    FORCEINLINE const CTimestamp& GetDeltaTime() const
    {
        return DeltaTime;
    }

    FORCEINLINE const CTimestamp& GetTotalTime() const
    {
        return TotalTime;
    }

private:
    CTimestamp TotalTime;
    CTimestamp DeltaTime;

    uint64     LastTime  = 0;
    uint64     Frequency = 0;
};