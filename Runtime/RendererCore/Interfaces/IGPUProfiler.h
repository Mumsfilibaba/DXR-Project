#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/StaticArray.h"
#include "RHI/RHIQuery.h"

#define NUM_GPU_PROFILER_SAMPLES (200)

struct FGPUProfileSample
{
    void AddSample(float NewSample)
    {
        Samples[CurrentSample] = NewSample;

        Min = Math::Min(NewSample, Min);
        Max = Math::Max(NewSample, Max);

        SampleCount = Math::Min<int32>(Samples.Size(), SampleCount + 1);

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

        SampleCount   = 0;
        CurrentSample = 0;
        TotalCalls    = 0;
        Max           = TNumericLimits<float>::Lowest();
        Min           = TNumericLimits<float>::Max();
    }

    TStaticArray<float, NUM_GPU_PROFILER_SAMPLES> Samples;

    float Max           = TNumericLimits<float>::Lowest();
    float Min           = TNumericLimits<float>::Max();
    int32 SampleCount   = 0;
    int32 CurrentSample = 0;
    int32 TotalCalls    = 0;
};

using GPUProfileSamplesMap = TMap<String, FGPUProfileSample>;

struct FPipelineStatisticsMinMax
{
    FPipelineStatisticsMinMax()
    {
        Reset();
    }

    void Update(const FRHIPipelineStatistics& Stats)
    {
        const auto UpdateMin = [](uint64& Current, uint64 New)
        {
            if (New != 0)
            {
                Current = Math::Min(Current, New); 
            }
        };

        UpdateMin(Min.IAVertices,    Stats.IAVertices);
        UpdateMin(Min.IAPrimitives,  Stats.IAPrimitives);
        UpdateMin(Min.VSInvocations, Stats.VSInvocations);
        UpdateMin(Min.GSInvocations, Stats.GSInvocations);
        UpdateMin(Min.GSPrimitives,  Stats.GSPrimitives);
        UpdateMin(Min.CInvocations,  Stats.CInvocations);
        UpdateMin(Min.CPrimitives,   Stats.CPrimitives);
        UpdateMin(Min.PSInvocations, Stats.PSInvocations);
        UpdateMin(Min.HSInvocations, Stats.HSInvocations);
        UpdateMin(Min.DSInvocations, Stats.DSInvocations);
        UpdateMin(Min.CSInvocations, Stats.CSInvocations);
        UpdateMin(Min.ASInvocations, Stats.ASInvocations);
        UpdateMin(Min.MSInvocations, Stats.MSInvocations);
        UpdateMin(Min.MSPrimitives,  Stats.MSPrimitives);

        Max.IAVertices    = Math::Max(Max.IAVertices,    Stats.IAVertices);
        Max.IAPrimitives  = Math::Max(Max.IAPrimitives,  Stats.IAPrimitives);
        Max.VSInvocations = Math::Max(Max.VSInvocations, Stats.VSInvocations);
        Max.GSInvocations = Math::Max(Max.GSInvocations, Stats.GSInvocations);
        Max.GSPrimitives  = Math::Max(Max.GSPrimitives,  Stats.GSPrimitives);
        Max.CInvocations  = Math::Max(Max.CInvocations,  Stats.CInvocations);
        Max.CPrimitives   = Math::Max(Max.CPrimitives,   Stats.CPrimitives);
        Max.PSInvocations = Math::Max(Max.PSInvocations, Stats.PSInvocations);
        Max.HSInvocations = Math::Max(Max.HSInvocations, Stats.HSInvocations);
        Max.DSInvocations = Math::Max(Max.DSInvocations, Stats.DSInvocations);
        Max.CSInvocations = Math::Max(Max.CSInvocations, Stats.CSInvocations);
        Max.ASInvocations = Math::Max(Max.ASInvocations, Stats.ASInvocations);
        Max.MSInvocations = Math::Max(Max.MSInvocations, Stats.MSInvocations);
        Max.MSPrimitives  = Math::Max(Max.MSPrimitives,  Stats.MSPrimitives);
    }

    void Reset()
    {
        Min = FRHIPipelineStatistics();
        Max = FRHIPipelineStatistics();

        Min.IAVertices    = Min.IAPrimitives  = Min.VSInvocations = Min.GSInvocations = TNumericLimits<uint64>::Max();
        Min.GSPrimitives  = Min.CInvocations  = Min.CPrimitives   = Min.PSInvocations = TNumericLimits<uint64>::Max();
        Min.HSInvocations = Min.DSInvocations = Min.CSInvocations = TNumericLimits<uint64>::Max();
        Min.ASInvocations = Min.MSInvocations = Min.MSPrimitives  = TNumericLimits<uint64>::Max();
    }

    FRHIPipelineStatistics Min;
    FRHIPipelineStatistics Max;
};

struct IGPUProfiler
{
    virtual ~IGPUProfiler() = default;

    virtual void Enable()  = 0;
    virtual void Disable() = 0;
    virtual void Reset()   = 0;

    virtual void EnablePipelineStatistics()  = 0;
    virtual void DisablePipelineStatistics() = 0;
    virtual bool IsPipelineStatisticsEnabled() const = 0;

    virtual void GetGPUSamples(GPUProfileSamplesMap& OutSamples) = 0;
    
    virtual const FGPUProfileSample&         GetGPUFrameTime()             const = 0;
    virtual const FRHIPipelineStatistics&    GetPipelineStatistics()       const = 0;
    virtual const FPipelineStatisticsMinMax& GetPipelineStatisticsMinMax() const = 0;
};
