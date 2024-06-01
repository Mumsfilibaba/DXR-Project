#pragma once
#include "Core/Time/Stopwatch.h"
#include "Core/Threading/Spinlock.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Templates/NumericLimits.h"

#define ENABLE_PROFILER 1
#define NUM_PROFILER_SAMPLES 200

#if ENABLE_PROFILER
    #define TRACE_SCOPE(Name) FFrameProfilerScopedTrace STRING_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
    #define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(FUNCTION_SIGNATURE)
#else
    #define TRACE_SCOPE(Name)
    #define TRACE_FUNCTION_SCOPE()
#endif

struct FFrameProfilerSample
{
    FFrameProfilerSample()
        : Name(nullptr)
        , ThreadHandle(nullptr)
        , StartTimeStamp(0)
        , EndTimeStamp(0)
    {
    }

    FFrameProfilerSample(const CHAR* InName)
        : Name(InName)
        , ThreadHandle(nullptr)
        , StartTimeStamp(0)
        , EndTimeStamp(0)
    {
    }

    const CHAR* Name;
    void*       ThreadHandle;
    uint64      StartTimeStamp;
    uint64      EndTimeStamp;
};

struct FFrameProfilerFunctionInfo
{
    void AddSample(float NewSample)
    {
        Samples[CurrentSample] = NewSample;

        Min         = FMath::Min(NewSample, Min);
        Max         = FMath::Max(NewSample, Max);
        SampleCount = FMath::Min<int32>(Samples.Size(), SampleCount + 1);

        CurrentSample++;
        if (CurrentSample >= int32(Samples.Size()))
        {
            CurrentSample = 0;
        }

        TotalCalls++;
    }

    float GetAverage() const
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

    void Reset()
    {
        Samples.Fill(0.0f);

        SampleCount = 0;
        CurrentSample = 0;
        TotalCalls = 0;
        Max = TNumericLimits<float>::Lowest();
        Min = TNumericLimits<float>::Max();
    }

    TStaticArray<float, NUM_PROFILER_SAMPLES> Samples;

    float Max           = TNumericLimits<float>::Lowest();
    float Min           = TNumericLimits<float>::Max();
    int32 SampleCount   = 0;
    int32 CurrentSample = 0;
    int32 TotalCalls    = 0;
};

using FFrameProfileFunctionInfoMap = TMap<FString, FFrameProfilerFunctionInfo>;

struct FFrameProfilerThreadInfo
{
    FFrameProfilerThreadInfo()
        : ThreadHandle(nullptr)
        , FunctionInfoMap()
    {
    }

    FFrameProfilerThreadInfo(void* InThreadHandle)
        : ThreadHandle(InThreadHandle)
        , FunctionInfoMap()
    {
    }

    void* ThreadHandle;
    FFrameProfileFunctionInfoMap FunctionInfoMap;
};

class CORE_API FFrameProfiler
{
public:
    static FFrameProfiler& Get();
    
    void Enable();
    void Disable();
    void Tick();
    void Reset();
    void AddSample(const FFrameProfilerSample& InSample);
    void GetFunctionInfo(TArray<FFrameProfilerThreadInfo>& OutFunctionThreadInfo);

    int32 GetFramesPerSecond() const
    {
        return Fps;
    }

    const FFrameProfilerFunctionInfo& GetCPUFrameTime() const
    {
        return CPUFrameTime;
    }

private:
    FFrameProfiler();
    ~FFrameProfiler();

    FFrameProfilerFunctionInfo       CPUFrameTime;
    FStopwatch                       Clock;
    int32                            CurrentFps;
    int32                            Fps;
    bool                             bEnabled;
    uint64                           Frequency;
    TArray<FFrameProfilerSample>     CurrentSamples;
    FSpinLock                        CurrentSamplesLock;
    TArray<FFrameProfilerThreadInfo> FunctionInfoTable;
    TMap<void*, int32>               ThreadHandleToIndexMap;
};

struct FFrameProfilerScopedTrace
{
public:
    FORCEINLINE FFrameProfilerScopedTrace(const CHAR* InName)
        : Sample(InName)
    {
        Sample.ThreadHandle = FPlatformThreadMisc::GetCurrentThreadHandle();
        Sample.StartTimeStamp = FPlatformTime::QueryPerformanceCounter();
    }

    FORCEINLINE ~FFrameProfilerScopedTrace()
    {
        Sample.EndTimeStamp = FPlatformTime::QueryPerformanceCounter();
        FFrameProfiler::Get().AddSample(Sample);
    }

private:
    FFrameProfilerSample Sample;
};
