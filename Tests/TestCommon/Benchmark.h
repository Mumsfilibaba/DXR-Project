#pragma once
#include <Core/CoreTypes.h>
#include <Core/Time/Time.h>
#include <Core/Platform/PlatformTime.h>
#include <Core/Misc/OutputDeviceLogger.h>

struct FClock
{
    friend struct FScopedClock;

public:
    FClock()
        : TimeFrequency(FPlatformTime::QueryPerformanceFrequency())
        , Duration(0)
        , TotalDuration(0)
    {
    }

    void Reset()
    {
        Duration      = 0;
        TotalDuration = 0;
    }

    int64 GetLastDuration() const
    {
        return Duration;
    }

    int64 GetTotalDuration() const
    {
        return TotalDuration;
    }

    const int64 TimeFrequency;

private:
    inline void AddDuration(int64 InDuration)
    {
        Duration       = InDuration;
        TotalDuration += Duration;
    }

    int64 Duration      = 0;
    int64 TotalDuration = 0;
};

struct FScopedClock
{
    FScopedClock(FClock& InParent)
        : Parent(InParent)
        , Start(FPlatformTime::QueryPerformanceCounter())
        , End()
    {
    }

    ~FScopedClock()
    {
        End = FPlatformTime::QueryPerformanceCounter();

        const uint64 Delta       = End - Start;
        const uint64 Nanoseconds = Time::FromSeconds(Delta) / Parent.TimeFrequency;
        Parent.AddDuration(Nanoseconds);
    }

    FClock& Parent;
    uint64  Start;
    uint64  End;
};

// ------------------------------------------------------------------------------------------------
// Benchmark - static-only helpers shared by all benchmarks.
// ------------------------------------------------------------------------------------------------
struct Benchmark
{
    // Pick a smaller count for slow Debug builds, the larger one otherwise.
    NODISCARD static uint32 ScaleCount(uint32 OptimizedCount, uint32 DebugCount)
    {
    #if defined(DEBUG_BUILD)
        (void)OptimizedCount;
        return DebugCount;
    #else
        (void)DebugCount;
        return OptimizedCount;
    #endif
    }

    // Logs "<Label> :<ns>ns" and, when Iterations > 0, an extra per-op line.
    static void Report(const CHAR* Label, int64 TotalNs, uint32 Iterations = 0)
    {
        LOG_INFO("%-27s:%lldns", Label, static_cast<long long>(TotalNs));
        if (Iterations > 0)
        {
            LOG_INFO("%-27s (per op):%lldns", Label, static_cast<long long>(TotalNs / Iterations));
        }
    }
};
