#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
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
    FRHIQueryRef PipelineStatsQuery[GPU_PROFILER_BUFFER_COUNT];
};

struct FGPUPendingScope
{
    const CHAR*  Name        = nullptr;
    int32        ParentIndex = -1;
    int32        Depth       = 0;
    FRHIQueryRef BeginQuery;
    FRHIQueryRef EndQuery;
    bool         bOwnsPipelineStats = false;
};

class FGPUProfiler : public IGPUProfiler
{
public:
    static FORCEINLINE FGPUProfiler& Get()
    {
        return GPUProfiler;
    }

public:

    virtual void Enable()  override final;
    virtual void Disable() override final;
    virtual void Reset()   override final;
    virtual void Release() override final;

    virtual void BeginGPUFrame() override final;
    virtual void EndGPUFrame() override final;
    virtual void MarkGPUFrameBegin(FRHICommandList& CmdList) override final;
    virtual void MarkGPUFrameEnd(FRHICommandList& CmdList) override final;
    virtual void BeginGPUTrace(FRHICommandList& CmdList, const CHAR* Name) override final;
    virtual void EndGPUTrace(FRHICommandList& CmdList, const CHAR* Name) override final;

    virtual void SetRetainAllFrames(bool bInRetainAllFrames) override final;
    virtual void EnablePipelineStatistics()    override final;
    virtual void DisablePipelineStatistics()   override final;
    virtual bool IsPipelineStatisticsEnabled() const override final;

    virtual int32 GetStoredFrameCount() const override final;
    virtual bool GetStoredFrame(int32 OldestIndex, FProfilerGpuFrame& OutFrame) const override final;
    virtual bool FindFrameForCpuFrame(int32 CpuFrameIndex, FProfilerGpuFrame& OutFrame) const override final;
    virtual bool GetLatestFrame(FProfilerGpuFrame& OutFrame) const override final;

private:
    FGPUProfiler();
    ~FGPUProfiler();

    void CollectResults();
    void ReleaseQueries();

    mutable FSpinLock                     StateLock;
    FRHIQueryRef                          FrameBeginQuery[GPU_PROFILER_BUFFER_COUNT];
    FRHIQueryRef                          FrameEndQuery[GPU_PROFILER_BUFFER_COUNT];
    int32                                 PendingCpuFrameIndex[GPU_PROFILER_BUFFER_COUNT];
    bool                                  bFrameBeginRecorded[GPU_PROFILER_BUFFER_COUNT];
    bool                                  bFrameEndRecorded[GPU_PROFILER_BUFFER_COUNT];
    TMap<String, FGPUProfileScopeQueries> ScopeQueries;
    TArray<FGPUPendingScope>              PendingScopes[GPU_PROFILER_BUFFER_COUNT];
    int32                                 NumPendingScopes[GPU_PROFILER_BUFFER_COUNT];
    TArray<int32>                         OpenTraceStack;
    TProfilerFrameRing<FProfilerGpuFrame> StoredFrames;
    FRHIQuery*                            OpenPipelineStatsQuery;
    int32                                 PipelineStatsScopeIndex;
    bool                                  bEnabled;
    bool                                  bPipelineStatsEnabled;
    bool                                  bFrameOpen;
    int32                                 WriteIndex;

    static FGPUProfiler GPUProfiler;
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
