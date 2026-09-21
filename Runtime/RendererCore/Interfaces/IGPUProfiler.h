#pragma once
#include "Core/Containers/Array.h"
#include "Core/Misc/ProfilerTypes.h"
#include "RHI/RHIQuery.h"

class FRHICommandList;

struct FGPUProfilerInterval
{
    const CHAR* Name = nullptr;
    uint64      StartNanoseconds = 0;
    uint64      EndNanoseconds = 0;
    uint64      InclusiveNanoseconds = 0;
    uint64      ExclusiveNanoseconds = 0;
    int32       ParentIndex = -1;
    int32       Depth = 0;
    bool        bInstant = false;
    FRHIPipelineStatistics PipelineStats;
    bool        bHasPipelineStats = false;
};

struct FProfilerGpuFrame
{
    int32 CpuFrameIndex = -1;
    float GpuMilliseconds = 0.0f;
    TArray<FGPUProfilerInterval> Intervals;
};

struct IGPUProfiler
{
    virtual ~IGPUProfiler() = default;

    virtual void Enable()  = 0;
    virtual void Disable() = 0;
    virtual void Reset()   = 0;
    virtual void Release() = 0;

    virtual void BeginGPUFrame() = 0;
    virtual void EndGPUFrame() = 0;
    virtual void MarkGPUFrameBegin(FRHICommandList& CommandList) = 0;
    virtual void MarkGPUFrameEnd(FRHICommandList& CommandList) = 0;
    virtual void BeginGPUTrace(FRHICommandList& CommandList, const CHAR* Name) = 0;
    virtual void EndGPUTrace(FRHICommandList& CommandList, const CHAR* Name) = 0;

    virtual void SetRetainAllFrames(bool bInRetainAllFrames) = 0;
    virtual void EnablePipelineStatistics()  = 0;
    virtual void DisablePipelineStatistics() = 0;
    virtual bool IsPipelineStatisticsEnabled() const = 0;

    NODISCARD virtual int32 GetStoredFrameCount() const = 0;
    NODISCARD virtual bool GetStoredFrame(int32 OldestIndex, FProfilerGpuFrame& OutFrame) const = 0;
    NODISCARD virtual bool FindFrameForCpuFrame(int32 CpuFrameIndex, FProfilerGpuFrame& OutFrame) const = 0;
    NODISCARD virtual bool GetLatestFrame(FProfilerGpuFrame& OutFrame) const = 0;
};
