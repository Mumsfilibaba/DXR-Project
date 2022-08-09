#pragma once
#include "Core/Time/Timer.h"

#include "RHI/RHICommandList.h"
#include "RHI/RHITimestampQuery.h"

#include "Core/Threading/Spinlock.h"
#include "Core/Containers/HashTable.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Config

#define ENABLE_PROFILER      (1)
#define NUM_PROFILER_SAMPLES (200)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Macros

#if ENABLE_PROFILER
    #define TRACE_SCOPE(Name)      FScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
    #define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(FUNCTION_SIGNATURE)
#else
    #define TRACE_SCOPE(Name)
    #define TRACE_FUNCTION_SCOPE()
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FProfileSample

struct FProfileSample
{
    FORCEINLINE void Begin()
    {
        Clock.Tick();
    }

    FORCEINLINE void End()
    {
        Clock.Tick();

        const float Delta = static_cast<float>(Clock.GetDeltaTime().AsNanoseconds());
        AddSample(Delta);

        TotalCalls++;
    }

    FORCEINLINE void AddSample(float NewSample)
    {
        Samples[CurrentSample] = NewSample;

        Min = NMath::Min(NewSample, Min);
        Max = NMath::Max(NewSample, Max);

        SampleCount = NMath::Min<int32>(Samples.GetSize(), SampleCount + 1);

        CurrentSample++;
        if (CurrentSample >= int32(Samples.GetSize()))
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

    FTimer Clock;

    float Max = -FLT_MAX;
    float Min = FLT_MAX;

    int32 SampleCount   = 0;
    int32 CurrentSample = 0;
    int32 TotalCalls    = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FFrameProfiler

using ProfileSamplesTable = THashTable<FString, FProfileSample, FStringHasher>;

class CORE_API FFrameProfiler
{
public:

    /**
     * @brief: Retrieve the Frame-Profiler instance
     * 
     * @return: Returns a reference to the Frame-Profiler instance
     */
    static FFrameProfiler& Get();

    /** Enables the collection of samples (Resume) */
    static void Enable();

    /** Disables the collection of samples (Pause) */
    static void Disable();

    /** Updates the profiler, should be called once per frame */
    void Tick();

    /** Resets all the samples */
    void Reset();

    /**
     * @brief: Starts a scope for a function 
     * 
     * @param Name: Name of the scope
     */ 
    void BeginTraceScope(const char* Name);

    /**
     * @brief: Ends a scope for a function
     *
     * @param Name: Name of the scope
     */
    void EndTraceScope(const char* Name);

    /** CPU Profiler samples */
    void GetCPUSamples(ProfileSamplesTable& OutCPUSamples);

    FORCEINLINE int32 GetFramesPerSecond() const
    {
        return Fps;
    }

    FORCEINLINE const FProfileSample& GetCPUFrameTime() const
    {
        return CPUFrameTime;
    }

private:

    FProfileSample CPUFrameTime;

    FTimer Clock;

    int32 CurrentFps = 0;
    int32 Fps        = 0;

    bool bEnabled = true;

    ProfileSamplesTable SamplesTable;
    FSpinLock           SamplesTableLock;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FScopedTrace

struct FScopedTrace
{
public:
    FORCEINLINE FScopedTrace(const char* InName)
        : Name(InName)
    {
        FFrameProfiler::Get().BeginTraceScope(Name);
    }

    FORCEINLINE ~FScopedTrace()
    {
        FFrameProfiler::Get().EndTraceScope(Name);
    }

private:
    const char* Name = nullptr;
};