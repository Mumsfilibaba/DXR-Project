#pragma once
#include "Core/Containers/Map.h"
#include "Core/Threading/Spinlock.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "Renderer/RendererModule.h"

#define ENABLE_GPU_PROFILER      (1)
#define NUM_GPU_PROFILER_SAMPLES (200)

#if ENABLE_GPU_PROFILER
    #define GPU_TRACE_SCOPE(CmdList, Name) FGPUScopedTrace STRING_CONCAT(GPUScopedTrace_Line_, __LINE__)(CmdList, Name)
#else
    #define GPU_TRACE_SCOPE(CmdList, Name)
#endif

struct FGPUProfileSample
{
    void AddSample(float NewSample)
    {
        Samples[CurrentSample] = NewSample;

        Min = FMath::Min(NewSample, Min);
        Max = FMath::Max(NewSample, Max);

        SampleCount = FMath::Min<int32>(Samples.Size(), SampleCount + 1);

        CurrentSample++;
        if (CurrentSample >= int32(Samples.Size()))
        {
            CurrentSample = 0;
        }
    }

    float GetAverage() const
    {
        if (SampleCount < 1)
            return 0.0f;

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

    TStaticArray<float, NUM_GPU_PROFILER_SAMPLES> Samples;

    FRHIQueryRef BeginQuery;
    FRHIQueryRef EndQuery;
    float        Max = TNumericLimits<float>::Lowest();
    float        Min = TNumericLimits<float>::Max();
    int32        SampleCount;
    int32        CurrentSample;
    int32        TotalCalls;
};

using GPUProfileSamplesMap = TMap<FString, FGPUProfileSample>;

class RENDERER_API FGPUProfiler
{
public:

    static FGPUProfiler& Get()
    {
        return Instance;
    }

    /** @brief Releases all query objects */
    void Release();

     /** @brief Enables the collection of samples (Resume) */
    void Enable();

     /** @brief Disables the collection of samples (Pause) */
    void Disable();

     /** @brief Updates the profiler, should be called once per frame */
    void Tick();

     /** @brief Resets all the samples */
    void Reset();

     /** @brief Retrieve a copy of the GPU Profiler samples */
    void GetGPUSamples(GPUProfileSamplesMap& OutGPUSamples);

     /** @brief Start the GPU frame */
    void BeginGPUFrame(FRHICommandList& CmdList);

     /** @brief End the GPU frame */
    void EndGPUFrame(FRHICommandList& CmdList);

     /** @brief Begin a GPU scope */
    void BeginGPUTrace(FRHICommandList& CmdList, const CHAR* Name);

     /** @brief End a GPU scope */
    void EndGPUTrace(FRHICommandList& CmdList, const CHAR* Name);

    const FGPUProfileSample& GetGPUFrameTime() const
    {
        return FrameTime;
    }

private:
    FGPUProfiler();

    FGPUProfileSample    FrameTime;
    GPUProfileSamplesMap Samples;
    FSpinLock            SamplesLock;
    bool                 bEnabled;

    static FGPUProfiler Instance;
};

struct FGPUScopedTrace
{
public:
    FORCEINLINE FGPUScopedTrace(FRHICommandList& InCommandList, const CHAR* InName)
        : CommandList(InCommandList)
        , Name(InName)
    {
        FGPUProfiler::Get().BeginGPUTrace(CommandList, Name);
    }

    FORCEINLINE ~FGPUScopedTrace()
    {
        FGPUProfiler::Get().EndGPUTrace(CommandList, Name);
    }

private:
    FRHICommandList& CommandList;
    const CHAR* Name = nullptr;
};
