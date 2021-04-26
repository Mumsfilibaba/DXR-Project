#pragma once
#include "Time/Clock.h"

#include <unordered_map>
#include <algorithm>

#define ENABLE_PROFILER      1
#define NUM_PROFILER_SAMPLES 5000

#if ENABLE_PROFILER
    #define TRACE_SCOPE(Name)      ScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
    #define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(__FUNCTION_SIG__)

    #define GPU_TRACE_SCOPE(CmdList, Name) GPUScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(CmdList, Name)
#else
    #define TRACE_SCOPE(Name)
    #define TRACE_FUNCTION_SCOPE()

    #define GPU_TRACE_SCOPE(CmdList, Name)
#endif

class CommandList;

class Profiler
{
public:
    static void Init();
    static void Tick();

    static void Enable();
    static void Disable();
    static void Reset();

    static void BeginTraceScope(const Char* Name);
    static void EndTraceScope(const Char* Name);
    
    static void BeginGPUFrame(CommandList& CmdList);
    static void BeginGPUTrace(CommandList& CmdList, const Char* Name);
    static void EndGPUTrace(CommandList& CmdList, const Char* Name);
    static void EndGPUFrame(CommandList& CmdList);

    static void SetGPUProfiler(class GPUProfiler* Profiler);

    static const struct ProfileSample*    GetSample(const Char* Name);
    static const struct GPUProfileSample* GetGPUSample(const Char* Name);

    static const struct ProfileSample*    GetFrameTimeSamples();
    static const struct GPUProfileSample* GetGPUFrameTimeSamples();
};

struct ScopedTrace
{
public:
    ScopedTrace(const Char* InName)
        : Name(InName)
    {
        Profiler::BeginTraceScope(Name);
    }

    ~ScopedTrace()
    {
        Profiler::EndTraceScope(Name);
    }

private:
    const Char* Name = nullptr;
};

struct GPUScopedTrace
{
public:
    GPUScopedTrace(CommandList& InCmdList, const Char* InName)
        : CmdList(InCmdList)
        , Name(InName)
    {
        Profiler::BeginGPUTrace(CmdList, Name);
    }

    ~GPUScopedTrace()
    {
        Profiler::EndGPUTrace(CmdList, Name);
    }

private:
    CommandList& CmdList;
    const Char* Name = nullptr;
};

struct ProfileSample
{
    FORCEINLINE void Begin()
    {
        Clock.Tick();
    }

    FORCEINLINE void End()
    {
        Clock.Tick();

        Float Delta = (Float)Clock.GetDeltaTime().AsNanoSeconds();
        AddSample(Delta);

        TotalCalls++;
    }

    FORCEINLINE void AddSample(Float NewSample)
    {
        Samples[CurrentSample] = NewSample;
        Min = Math::Min(NewSample, Min);
        Max = Math::Max(NewSample, Max);

        CurrentSample++;
        SampleCount = Math::Min<Int32>(Samples.Size(), SampleCount + 1);

        if (CurrentSample >= Int32(Samples.Size()))
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE Float GetAverage() const
    {
        if (SampleCount < 1)
        {
            return 0.0f;
        }

        Float Average = 0.0f;
        for (Int32 n = 0; n < SampleCount; n++)
        {
            Average += Samples[n];
        }

        return Average / Float(SampleCount);
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

    TStaticArray<Float, NUM_PROFILER_SAMPLES> Samples;
    Clock Clock;
    Float Max = -FLT_MAX;
    Float Min = FLT_MAX;
    Int32 SampleCount = 0;
    Int32 CurrentSample = 0;
    Int32 TotalCalls = 0;
};

struct GPUProfileSample
{
    FORCEINLINE void AddSample(Float NewSample)
    {
        Samples[CurrentSample] = NewSample;
        Min = Math::Min(NewSample, Min);
        Max = Math::Max(NewSample, Max);

        CurrentSample++;
        SampleCount = Math::Min<Int32>(Samples.Size(), SampleCount + 1);

        if (CurrentSample >= Int32(Samples.Size()))
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE Float GetAverage() const
    {
        if (SampleCount < 1)
        {
            return 0.0f;
        }

        Float Average = 0.0f;
        for (Int32 n = 0; n < SampleCount; n++)
        {
            Average += Samples[n];
        }

        return Average / Float(SampleCount);
    }

    FORCEINLINE Float GetMin() const
    {
        if (SampleCount < 1)
        {
            return 0.0f;
        }

        Float m = FLT_MAX;
        for (Int32 n = 0; n < SampleCount; n++)
        {
            if (Samples[n] != 0.0f)
            {
                m = Math::Min(m, Samples[n]);
            }
        }

        return m == FLT_MAX ? 0.0f : m;
    }

    FORCEINLINE Float GetMax() const
    {
        if (SampleCount < 1)
        {
            return 0.0f;
        }

        Float m = -FLT_MAX;
        for (Int32 n = 0; n < SampleCount; n++)
        {
            m = Math::Max(m, Samples[n]);
        }

        return m;
    }

    FORCEINLINE Float GetMedian() const
    {
        if (SampleCount < 1)
        {
            return 0.0f;
        }

        TStaticArray<Float, NUM_PROFILER_SAMPLES> TempSamples = Samples;
        std::sort(TempSamples.begin(), TempSamples.end(), 
            [](Float a, Float b) 
            {
                return a < b;
            });

        Int32 Median = SampleCount / 2;
        return TempSamples[Median];
    }

    FORCEINLINE void Reset()
    {
        Samples.Fill(0.0f);
        SampleCount = 0;
        CurrentSample = 0;
        TotalCalls = 0;
        Max = -FLT_MAX;
        Min = FLT_MAX;
    }

    TStaticArray<Float, NUM_PROFILER_SAMPLES> Samples;
    Float  Max = -FLT_MAX;
    Float  Min = FLT_MAX;
    Int32  SampleCount = 0;
    Int32  CurrentSample = 0;
    Int32  TotalCalls = 0;
    UInt32 TimeQueryIndex = 0;
};
