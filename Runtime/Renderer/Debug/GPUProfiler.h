#pragma once
#include "RHI/RHITimestampQuery.h"
#include "RHI/RHICommandList.h"

#include "Renderer/RendererAPI.h"

#include "Core/Containers/HashTable.h"
#include "Core/Threading/Lockable.h"

#define ENABLE_GPU_PROFILER      (1)
#define NUM_GPU_PROFILER_SAMPLES (200)

#if ENABLE_GPU_PROFILER
#define GPU_TRACE_SCOPE(CmdList, Name) SGPUScopedTrace PREPROCESS_CONCAT(GPUScopedTrace_Line_, __LINE__)(CmdList, Name)
#else
#define GPU_TRACE_SCOPE(CmdList, Name)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SGPUProfileSample
{
    void AddSample( float NewSample )
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

    float GetAverage() const
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

    void Reset()
    {
        Samples.Fill( 0.0f );

        SampleCount = 0;
        CurrentSample = 0;
        TotalCalls = 0;

        Max = -FLT_MAX;
        Min = FLT_MAX;
    }

    TStaticArray<float, NUM_GPU_PROFILER_SAMPLES> Samples;

    float Max = -FLT_MAX;
    float Min = FLT_MAX;

    int32 SampleCount = 0;
    int32 CurrentSample = 0;
    int32 TotalCalls = 0;

    uint32 TimeQueryIndex = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

using GPUProfileSamplesTable = THashTable<CString, SGPUProfileSample, SStringHasher>;

class RENDERER_API CGPUProfiler
{
public:

    /* Creates the profiler, requires the RHI to be initialized */
    static bool Init();

    static FORCEINLINE CGPUProfiler& Get()
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

    /* Retrieve a copy of the GPU Profiler samples */
    void GetGPUSamples( GPUProfileSamplesTable& OutGPUSamples );

    /* Start the GPU frame */
    void BeginGPUFrame( CRHICommandList& CmdList );

    /* End the GPU frame */
    void EndGPUFrame( CRHICommandList& CmdList );

    /* Begin a GPU scope */
    void BeginGPUTrace( CRHICommandList& CmdList, const char* Name );

    /* End a GPU scope */
    void EndGPUTrace( CRHICommandList& CmdList, const char* Name );

    FORCEINLINE const SGPUProfileSample& GetGPUFrameTime() const
    {
        return FrameTime;
    }

private:

    CGPUProfiler();
    ~CGPUProfiler() = default;

    /* Queries for GPUTimeStamps */
    TSharedRef<CRHITimestampQuery> Timequeries;

    uint32 CurrentTimeQueryIndex = 0;

    /* Sample for the GPU FrameTime */
    SGPUProfileSample FrameTime;

    /* Lockable table for GPU- samples */
    Lockable<GPUProfileSamplesTable> Samples;

    bool Enabled;

    static CGPUProfiler Instance;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SGPUScopedTrace
{
public:

    FORCEINLINE SGPUScopedTrace( CRHICommandList& InCmdList, const char* InName )
        : CmdList( InCmdList )
        , Name( InName )
    {
        CGPUProfiler::Get().BeginGPUTrace( CmdList, Name );
    }

    FORCEINLINE ~SGPUScopedTrace()
    {
        CGPUProfiler::Get().EndGPUTrace( CmdList, Name );
    }

private:
    CRHICommandList& CmdList;
    const char* Name = nullptr;
};