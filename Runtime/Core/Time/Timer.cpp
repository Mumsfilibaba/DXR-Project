#include "Timer.h"

#include "Platform/PlatformTime.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTimer

FTimer::FTimer()
{
    Frequency = FPlatformTime::QueryPerformanceFrequency();
    Tick();
}

void FTimer::Tick()
{
    const uint64 Now = FPlatformTime::QueryPerformanceCounter();
    
    // TODO: Does this make sense? 
    uint64 Delta       = Now - LastTime;
    uint64 Nanoseconds = NTime::FromSeconds(Delta) / Frequency;

    DeltaTime = FTimespan(Nanoseconds);
    LastTime  = Now;
    TotalTime += DeltaTime;
}