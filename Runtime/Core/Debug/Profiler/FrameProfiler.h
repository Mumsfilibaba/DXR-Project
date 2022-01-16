#pragma once
#include "Core/Time/Timer.h"

#include "RHI/RHICommandList.h"
#include "RHI/RHITimestampQuery.h"

#include "Core/Threading/Lockable.h"
#include "Core/Containers/HashTable.h"

#define ENABLE_PROFILER      (1)
#define NUM_PROFILER_SAMPLES (200)

#if ENABLE_PROFILER
#define TRACE_SCOPE(Name)      SScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
#define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(FUNCTION_SIGNATURE)
#else
#define TRACE_SCOPE(Name)
#define TRACE_FUNCTION_SCOPE()
#endif

struct SProfileSample
{
    FORCEINLINE void Begin()
    {
        Clock.Tick();
    }

    FORCEINLINE void End()
    {
        Clock.Tick();

        float Delta = static_cast<float>(Clock.GetDeltaTime().AsNanoSeconds());
        AddSample(Delta);

        TotalCalls++;
    }

    FORCEINLINE void AddSample(float NewSample)
    {
        Samples[CurrentSample] = NewSample;

        Min = NMath::Min(NewSample, Min);
        Max = NMath::Max(NewSample, Max);

        SampleCount = NMath::Min<int32>(Samples.Size(), SampleCount + 1);

        CurrentSample++;
        if (CurrentSample >= int32(Samples.Size()))
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE float GetAverage() const
    {
        if (SampleCount < 1)
        {
            return 0.0f;
        }

        float Average = 0.0f;
        for (int32 n = 0; n < SampleCount; n++)
        {
            Average += Samples[n];
        }

        return Average / float(SampleCount);
    }

    FORCEINLINE void Reset()
    {
        Samples.Fill(0.0f);

        SampleCount = 0;
        CurrentSample = 0;
        TotalCalls = 0;

        Max = -FLT_MAX;
        Min = FLT_MAX;

        Clock.Reset();
    }

    TStaticArray<float, NUM_PROFILER_SAMPLES> Samples;

    CTimer Clock;

    float Max = -FLT_MAX;
    float Min = FLT_MAX;

    int32 SampleCount = 0;
    int32 CurrentSample = 0;
    int32 TotalCalls = 0;
};

using ProfileSamplesTable = THashTable<CString, SProfileSample, SStringHasher>;

class CORE_API CFrameProfiler
{
public:

    static FORCEINLINE CFrameProfiler& Get() { return Instance; }

    /* Enables the collection of samples (Resume) */
    static void Enable();

    /* Disables the collection of samples (Pause) */
    static void Disable();

    /* Updates the profiler, should be called once per frame */
    void Tick();

    /* Resets all the samples */
    void Reset();

    /* Starts a scope for a function */
    void BeginTraceScope(const char* Name);

    /* Ends a scope for a function */
    void EndTraceScope(const char* Name);

    /* CPU Profiler samples */
    void GetCPUSamples(ProfileSamplesTable& OutCPUSamples);

    FORCEINLINE int32 GetFramesPerSecond() const
    {
        return Fps;
    }

    FORCEINLINE const SProfileSample& GetCPUFrameTime() const
    {
        return CPUFrameTime;
    }

private:

    CFrameProfiler() = default;
    ~CFrameProfiler() = default;

    SProfileSample CPUFrameTime;

    CTimer Clock;

    int32 CurrentFps = 0;
    int32 Fps = 0;

    bool bEnabled = true;

    Lockable<ProfileSamplesTable> CPUSamples;

    static CFrameProfiler Instance;
};

struct SScopedTrace
{
public:

    FORCEINLINE SScopedTrace(const char* InName)
        : Name(InName)
    {
        CFrameProfiler::Get().BeginTraceScope(Name);
    }

    FORCEINLINE ~SScopedTrace()
    {
        CFrameProfiler::Get().EndTraceScope(Name);
    }

private:
    const char* Name = nullptr;
};