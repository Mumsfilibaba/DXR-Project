#include "Application/Animation/UIAnimation.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformTime.h"

static double GPinnedFrameTime = 0.0;
static bool   GIsFrameTimePinned = false;

static double ReadWallClock()
{
    return static_cast<double>(FPlatformTime::QueryPerformanceCounter()) / static_cast<double>(FPlatformTime::QueryPerformanceFrequency());
}

double FUIFrameClock::Now()
{
    return GIsFrameTimePinned ? GPinnedFrameTime : ReadWallClock();
}

void FUIFrameClock::BeginFrame()
{
    GPinnedFrameTime   = ReadWallClock();
    GIsFrameTimePinned = true;
}

void FUIFrameClock::EndFrame()
{
    GIsFrameTimePinned = false;
}

void FUIAnimation::Start(float InDurationSeconds, float InStartValue, float InTargetValue)
{
    StartSeconds    = FUIFrameClock::Now();
    DurationSeconds = InDurationSeconds;
    StartValue      = InStartValue;
    TargetValue     = InTargetValue;
}

void FUIAnimation::Settle(float InValue)
{
    StartSeconds    = FUIFrameClock::Now();
    DurationSeconds = 0.0f;
    StartValue      = InValue;
    TargetValue     = InValue;
}

float FUIAnimation::GetProgress() const
{
    if (DurationSeconds <= 0.0f)
    {
        return 1.0f;
    }

    return Math::Clamp(static_cast<float>(FUIFrameClock::Now() - StartSeconds) / DurationSeconds, 0.0f, 1.0f);
}

float FUIAnimation::Evaluate() const
{
    return Math::Lerp(StartValue, TargetValue, GetProgress());
}

bool FUIAnimation::IsRunning() const
{
    return GetProgress() < 1.0f;
}
