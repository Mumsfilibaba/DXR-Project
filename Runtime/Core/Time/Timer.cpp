#include "Timer.h"

#include "Platform/PlatformTime.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CTimer

CTimer::CTimer()
{
    Frequency = PlatformTime::QueryPerformanceFrequency();
    Tick();
}

void CTimer::Tick()
{
    const uint64 Now = PlatformTime::QueryPerformanceCounter();
    
    // TODO: Does this make sense? 
    uint64 Delta       = Now - LastTime;
    uint64 Nanoseconds = NTime::FromSeconds(Delta) / Frequency;

    DeltaTime = CTimestamp(Nanoseconds);
    LastTime = Now;
    TotalTime += DeltaTime;
}