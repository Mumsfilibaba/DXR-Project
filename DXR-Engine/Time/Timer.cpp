#include "Timer.h"

#include "Platform/PlatformTime.h"

Timer::Timer()
{
    Frequency = PlatformTime::QueryPerformanceFrequency();
    Tick();
}

void Timer::Tick()
{
    const uint64 Now = PlatformTime::QueryPerformanceCounter();	
    constexpr uint64 NANOSECONDS = 1000 * 1000 * 1000;
    uint64 Delta       = Now - LastTime;
    uint64 Nanoseconds = (Delta * NANOSECONDS) / Frequency;

    DeltaTime = Timestamp(Nanoseconds);
    LastTime  = Now;
    TotalTime += DeltaTime;
}