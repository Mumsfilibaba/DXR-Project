#pragma once
#include "Core/Containers/HashTable.h"
#include "Core/Threading/Spinlock.h"

#include "Renderer/RendererModule.h"

#include "RHI/RHITimestampQuery.h"
#include "RHI/RHICommandList.h"

#define ENABLE_GPU_PROFILER      (1)
#define NUM_GPU_PROFILER_SAMPLES (200)

#if ENABLE_GPU_PROFILER
    #define GPU_TRACE_SCOPE(CmdList, Name) FGPUScopedTrace PREPROCESS_CONCAT(GPUScopedTrace_Line_, __LINE__)(CmdList, Name)
#else
    #define GPU_TRACE_SCOPE(CmdList, Name)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

struct FGPUProfileSample
{
    void AddSample(float NewSample)
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

using GPUProfileSamplesTable = THashTable<FString, FGPUProfileSample, FStringHasher>;

class RENDERER_API FGPUProfiler
{
public:

     /** @brief: Creates the profiler, requires the RHI to be initialized */
    static bool Init();

     /** @brief: Release the resources */
    static void Release();

    static FORCEINLINE FGPUProfiler& Get() { return Instance; }

     /** @brief: Enables the collection of samples (Resume) */
    void Enable();

     /** @brief: Disables the collection of samples (Pause) */
    void Disable();

     /** @brief: Updates the profiler, should be called once per frame */
    void Tick();

     /** @brief: Resets all the samples */
    void Reset();

     /** @brief: Retrieve a copy of the GPU Profiler samples */
    void GetGPUSamples(GPUProfileSamplesTable& OutGPUSamples);

     /** @brief: Start the GPU frame */
    void BeginGPUFrame(FRHICommandList& CmdList);

     /** @brief: End the GPU frame */
    void EndGPUFrame(FRHICommandList& CmdList);

     /** @brief: Begin a GPU scope */
    void BeginGPUTrace(FRHICommandList& CmdList, const char* Name);

     /** @brief: End a GPU scope */
    void EndGPUTrace(FRHICommandList& CmdList, const char* Name);

    FORCEINLINE const FGPUProfileSample& GetGPUFrameTime() const
    {
        return FrameTime;
    }

private:

    FGPUProfiler();
    ~FGPUProfiler() = default;

     /** @brief: Queries for GPUTimeStamps */
    TSharedRef<FRHITimestampQuery> Timequeries;

    uint32 CurrentTimeQueryIndex = 0;

     /** @brief: Sample for the GPU FrameTime */
    FGPUProfileSample FrameTime;

     /** @brief: Lockable table for GPU- samples */
    GPUProfileSamplesTable Samples;
    FSpinLock              SamplesLock;

    bool bEnabled;

    static FGPUProfiler Instance;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

struct FGPUScopedTrace
{
public:

    FORCEINLINE FGPUScopedTrace(FRHICommandList& InCmdList, const char* InName)
        : CmdList(InCmdList)
        , Name(InName)
    {
        FGPUProfiler::Get().BeginGPUTrace(CmdList, Name);
    }

    FORCEINLINE ~FGPUScopedTrace()
    {
        FGPUProfiler::Get().EndGPUTrace(CmdList, Name);
    }

private:
    FRHICommandList& CmdList;
    const char* Name = nullptr;
};