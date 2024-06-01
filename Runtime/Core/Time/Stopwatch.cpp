#include "Stopwatch.h"

#include "Core/Platform/PlatformTime.h"

FStopwatch::FStopwatch()
    : Frequency(FPlatformTime::QueryPerformanceFrequency())
{
    Tick();
}

void FStopwatch::Tick()
{
    const uint64 Now = FPlatformTime::QueryPerformanceCounter();
    uint64 Delta       = Now - LastTime;
    uint64 Nanoseconds = TimeUtilities::FromSeconds(Delta) / Frequency;

    DeltaTime = FTimespan(Nanoseconds);
    LastTime  = Now;
    TotalTime += DeltaTime;
}