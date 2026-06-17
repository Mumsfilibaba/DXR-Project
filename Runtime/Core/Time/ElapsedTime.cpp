#include "Core/Time/ElapsedTime.h"
#include "Core/Platform/PlatformTime.h"

FElapsedTime::FElapsedTime()
    : TotalTime(0)
    , DeltaTime(0)
    , LastTime(FPlatformTime::QueryPerformanceCounter())
    , Frequency(FPlatformTime::QueryPerformanceFrequency())
{
}

void FElapsedTime::Tick()
{
    const uint64 Now   = FPlatformTime::QueryPerformanceCounter();
    const uint64 Delta = Now - LastTime;
    LastTime = Now;

    // Compute with double for accuracy, then store ns
    const double Seconds     = static_cast<double>(Delta) / static_cast<double>(Frequency);
    const uint64 Nanoseconds = static_cast<uint64>(Seconds * 1000.0 * 1000.0 * 1000.0);

    DeltaTime = FTimespan(Nanoseconds);
    TotalTime += DeltaTime;
}
