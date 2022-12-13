#include "Stopwatch.h"

#include "Core/Platform/PlatformTime.h"

FStopwatch::FStopwatch()
{
    Frequency = FPlatformTime::QueryPerformanceFrequency();
    Tick();
}

void FStopwatch::Tick()
{
    const uint64 Now = FPlatformTime::QueryPerformanceCounter();
    
    // TODO: Does this make sense? 
    uint64 Delta       = Now - LastTime;
    uint64 Nanoseconds = NTime::FromSeconds(Delta) / Frequency;

    DeltaTime = FTimespan(Nanoseconds);
    LastTime  = Now;
    TotalTime += DeltaTime;
}