#pragma once
#include "Timestamp.h"

class Timer
{
public:
    Timer();

    /*
    * Measures the deltatime between this and the latest call to Clock::Tick. It also updates the totalTime that the clock
    * has been active. This is the time between the last call to Clock::Reset and this call to Clock::Tick
    */
    void Tick();

    void Reset()
    {
        DeltaTime = Timestamp( 0 );
        TotalTime = Timestamp( 0 );
    }

    const Timestamp& GetDeltaTime() const
    {
        return DeltaTime;
    }
    const Timestamp& GetTotalTime() const
    {
        return TotalTime;
    }

private:
    Timestamp TotalTime = Timestamp( 0 );
    Timestamp DeltaTime = Timestamp( 0 );
    uint64 LastTime = 0;
    uint64 Frequency = 0;
};