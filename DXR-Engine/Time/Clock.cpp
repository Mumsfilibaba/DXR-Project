#include "Clock.h"

#include "Platform/PlatformTime.h"

Clock::Clock()
{
    Frequency = PlatformTime::QueryPerformanceFrequency();
    Tick();
}

void Clock::Tick()
{
    const UInt64 Now = PlatformTime::QueryPerformanceCounter();	
    constexpr UInt64 NANOSECONDS = 1000 * 1000 * 1000;
    UInt64 Delta       = Now - LastTime;
    UInt64 Nanoseconds = (Delta * NANOSECONDS) / Frequency;

    DeltaTime = Timestamp(Nanoseconds);
    LastTime  = Now;
    TotalTime += DeltaTime;
}