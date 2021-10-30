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

#define GPU_TRACE_SCOPE(CmdList, Name) SGPUScopedTrace PREPROCESS_CONCAT(GPUScopedTrace_Line_, __LINE__)(CmdList, Name)

#else
#define TRACE_SCOPE(Name)
#define TRACE_FUNCTION_SCOPE()

#define GPU_TRACE_SCOPE(CmdList, Name)

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
        AddSample( Delta );

        TotalCalls++;
    }

    FORCEINLINE void AddSample( float NewSample )
    {
        Samples[CurrentSample] = NewSample;

        Min = NMath::Min( NewSample, Min );
        Max = NMath::Max( NewSample, Max );

        SampleCount = NMath::Min<int32>( Samples.Size(), SampleCount + 1 );

        CurrentSample++;
        if ( CurrentSample >= int32( Samples.Size() ) )
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE float GetAverage() const
    {
        if ( SampleCount < 1 )
        {
            return 0.0f;
        }

        float Average = 0.0f;
        for ( int32 n = 0; n < SampleCount; n++ )
        {
            Average += Samples[n];
        }

        return Average / float( SampleCount );
    }

    FORCEINLINE void Reset()
    {
        Samples.Fill( 0.0f );

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

struct SGPUProfileSample
{
    FORCEINLINE void AddSample( float NewSample )
    {
        Samples[CurrentSample] = NewSample;

        Min = NMath::Min( NewSample, Min );
        Max = NMath::Max( NewSample, Max );

        SampleCount = NMath::Min<int32>( Samples.Size(), SampleCount + 1 );

        CurrentSample++;
        if ( CurrentSample >= int32( Samples.Size() ) )
        {
            CurrentSample = 0;
        }
    }

    FORCEINLINE float GetAverage() const
    {
        if ( SampleCount < 1 )
        {
            return 0.0f;
        }

        float Average = 0.0f;
        for ( int32 n = 0; n < SampleCount; n++ )
        {
            Average += Samples[n];
        }

        return Average / float( SampleCount );
    }

    FORCEINLINE void Reset()
    {
        Samples.Fill( 0.0f );

        SampleCount = 0;
        CurrentSample = 0;
        TotalCalls = 0;

        Max = -FLT_MAX;
        Min = FLT_MAX;
    }

    TStaticArray<float, NUM_PROFILER_SAMPLES> Samples;

    float Max = -FLT_MAX;
    float Min = FLT_MAX;

    int32 SampleCount = 0;
    int32 CurrentSample = 0;
    int32 TotalCalls = 0;

    uint32 TimeQueryIndex = 0;
};

using ProfileSamplesTable    = THashTable<CString, SProfileSample, SStringHasher>;
using GPUProfileSamplesTable = THashTable<CString, SGPUProfileSample, SStringHasher>;

class CORE_API CFrameProfiler
{
public:

    static FORCEINLINE CFrameProfiler& Get()
    {
        return Instance;
    }
    
    /* Enables the collection of samples (Resume) */
    void Enable();

    /* Disables the collection of samples (Pause) */
    void Disable();

    /* Updates the profiler, should be called once per frame */
    void Tick();

    /* Resets all the samples */
    void Reset();

    /* Starts a scope for a function */
    void BeginTraceScope( const char* Name );
    void EndTraceScope( const char* Name );

    void BeginGPUFrame( CRHICommandList& CmdList );
    void BeginGPUTrace( CRHICommandList& CmdList, const char* Name );
    void EndGPUTrace( CRHICommandList& CmdList, const char* Name );
    void EndGPUFrame( CRHICommandList& CmdList );

    void SetGPUProfiler( class CRHITimestampQuery* Profiler );

    void GetCPUSamples( ProfileSamplesTable& OutCPUSamples );
    void GetGPUSamples( GPUProfileSamplesTable& OutGPUSamples );

    FORCEINLINE int32 GetFramesPerSecond() const
    {
        return Fps;
    }

    FORCEINLINE const SProfileSample& GetCPUFrameTime() const
    {
        return CPUFrameTime;
    }

    FORCEINLINE const SGPUProfileSample& GetGPUFrameTime() const
    {
        return GPUFrameTime;
    }

private:

    CFrameProfiler() = default;
    ~CFrameProfiler() = default;

    TSharedRef<CRHITimestampQuery> GPUProfiler;

    uint32 CurrentTimeQueryIndex = 0;

    SProfileSample    CPUFrameTime;
    SGPUProfileSample GPUFrameTime;

    CTimer Clock;

    int32 CurrentFps = 0;
    int32 Fps = 0;

    bool EnableProfiler = true;

    Lockable<ProfileSamplesTable>    CPUSamples;
    Lockable<GPUProfileSamplesTable> GPUSamples;

    static CFrameProfiler Instance;
};

struct SScopedTrace
{
public:
    FORCEINLINE SScopedTrace( const char* InName )
        : Name( InName )
    {
        CFrameProfiler::Get().BeginTraceScope( Name );
    }

    FORCEINLINE ~SScopedTrace()
    {
        CFrameProfiler::Get().EndTraceScope( Name );
    }

private:
    const char* Name = nullptr;
};

struct SGPUScopedTrace
{
public:
    FORCEINLINE SGPUScopedTrace( CRHICommandList& InCmdList, const char* InName )
        : CmdList( InCmdList )
        , Name( InName )
    {
        CFrameProfiler::Get().BeginGPUTrace( CmdList, Name );
    }

    FORCEINLINE ~SGPUScopedTrace()
    {
        CFrameProfiler::Get().EndGPUTrace( CmdList, Name );
    }

private:
    CRHICommandList& CmdList;
    const char* Name = nullptr;
};