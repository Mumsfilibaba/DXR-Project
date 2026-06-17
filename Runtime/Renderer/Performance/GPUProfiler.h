#pragma once
#include "Core/Containers/Array.h"
#include "Core/Threading/Spinlock.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "RendererCore/Interfaces/IGPUProfiler.h"
#include "Renderer/RendererModule.h"

#define ENABLE_GPU_PROFILER (1)

#if ENABLE_GPU_PROFILER
    #define GPU_TRACE_SCOPE(CmdList, Name) FGPUScopedTrace STRING_CONCAT(GPUScopedTrace_Line_, __LINE__)(CmdList, Name)
#else
    #define GPU_TRACE_SCOPE(CmdList, Name)
#endif

static constexpr int32 GPU_PROFILER_BUFFER_COUNT = 3;

struct FGPUProfileScopeQueries
{
    FRHIQueryRef BeginQuery[GPU_PROFILER_BUFFER_COUNT];
    FRHIQueryRef EndQuery[GPU_PROFILER_BUFFER_COUNT];
    FRHIQueryRef PipelineStatsQuery[GPU_PROFILER_BUFFER_COUNT];
};

class FGPUProfiler : public IGPUProfiler
{
public:
    static FORCEINLINE FGPUProfiler& Get()
    {
        return GGpuProfiler;
    }

public:

    // IGPUProfiler interface
    virtual void Enable()  override final;
    virtual void Disable() override final;
    virtual void Reset()   override final;

    virtual void EnablePipelineStatistics()    override final;
    virtual void DisablePipelineStatistics()   override final;
    virtual bool IsPipelineStatisticsEnabled() const override final;

    virtual void GetGPUSamples(GPUProfileSamplesMap& OutGPUSamples) override final;
    
    virtual const FGPUProfileSample& GetGPUFrameTime() const override final
    {
        return FrameTime;
    }
    
    virtual const FRHIPipelineStatistics& GetPipelineStatistics() const override final
    {
        return LastPipelineStats;
    }

    virtual const FPipelineStatisticsMinMax& GetPipelineStatisticsMinMax() const override final
    {
        return PipelineStatsMinMax;
    }
    
    /** @brief Releases all query objects */
    void Release();

    /** @brief Start the GPU frame */
    void BeginGPUFrame(FRHICommandList& CmdList);

    /** @brief End the GPU frame */
    void EndGPUFrame(FRHICommandList& CmdList);

    /** @brief Begin a GPU scope */
    void BeginGPUTrace(FRHICommandList& CmdList, const CHAR* Name);

    /** @brief End a GPU scope */
    void EndGPUTrace(FRHICommandList& CmdList, const CHAR* Name);

private:
    FGPUProfiler();
    ~FGPUProfiler();

    void CollectResults();

    FGPUProfileSample                     FrameTime;
    GPUProfileSamplesMap                  Samples;
    FSpinLock                             SamplesLock;
    FRHIQueryRef                          FrameBeginQuery[GPU_PROFILER_BUFFER_COUNT];
    FRHIQueryRef                          FrameEndQuery[GPU_PROFILER_BUFFER_COUNT];
    TMap<String, FGPUProfileScopeQueries> ScopeQueries;
    FRHIPipelineStatistics                LastPipelineStats;
    FPipelineStatisticsMinMax             PipelineStatsMinMax;
    bool                                  bEnabled;
    bool                                  bPipelineStatsEnabled;
    int32                                 WriteIndex;
    int32                                 PipelineStatsNestingDepth;

    static FGPUProfiler GGpuProfiler;
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
    const CHAR*      Name;
};
