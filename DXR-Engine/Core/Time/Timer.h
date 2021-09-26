#pragma once
#include "Timestamp.h"

class CTimer
{
public:

    CTimer();
    ~CTimer() = default;

    /*
    * Measures the deltatime between this and the latest call to CTimer::Tick. It also updates the totalTime that the clock
    * has been active. This is the time between the last call to CTimer::Reset and this call to Clock::Tick
    */

    void Tick();

    FORCEINLINE void Reset()
    {
        DeltaTime = CTimestamp( 0 );
        TotalTime = CTimestamp( 0 );
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

    uint64 LastTime = 0;
    uint64 Frequency = 0;
};