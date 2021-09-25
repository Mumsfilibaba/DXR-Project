#include "Timer.h"

#include "Platform/PlatformTime.h"

CTimer::CTimer()
{
    Frequency = PlatformTime::QueryPerformanceFrequency();
    Tick();
}

void CTimer::Tick()
{
    const uint64 Now = PlatformTime::QueryPerformanceCounter();
    constexpr uint64 NANOSECONDS = 1000 * 1000 * 1000;
    uint64 Delta = Now - LastTime;
    uint64 Nanoseconds = (Delta * NANOSECONDS) / Frequency;

    DeltaTime = CTimestamp( Nanoseconds );
    LastTime = Now;
    TotalTime += DeltaTime;
}