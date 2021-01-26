#pragma once
#include "Timestamp.h"

class Clock
{
public:
    Clock();

    /*
    * Measures the deltatime between this and the latest call to Clock::Tick. It also updates the totalTime that the clock
    * has been active. This is the time between the last call to Clock::Reset and this call to Clock::Tick
    */
    void Tick();

    FORCEINLINE void Reset()
    {
        DeltaTime = Timestamp(0);
        TotalTime = Timestamp(0);
    }

    FORCEINLINE const Timestamp& GetDeltaTime() const
    {
        return DeltaTime;
    }

    FORCEINLINE const Timestamp& GetTotalTime() const
    {
        return TotalTime;
    }

private:
    Timestamp TotalTime = Timestamp(0);
    Timestamp DeltaTime = Timestamp(0);
    UInt64 LastTime  = 0;
    UInt64 Frequency = 0;
};