#include "Core/Math/Math.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ProfilerTypes.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Time/Time.h"
#include "Core/Time/Timespan.h"
#include "RHI/RHI.h"
#include "Renderer/Performance/GPUProfiler.h"

FGPUProfiler FGPUProfiler::GPUProfiler;

FGPUProfiler::FGPUProfiler()
    : OpenPipelineStatsQuery(nullptr)
    , PipelineStatsScopeIndex(-1)
    , bEnabled(false)
    , bPipelineStatsEnabled(false)
    , bFrameOpen(false)
    , WriteIndex(0)
{
    for (int32 Index = 0; Index < GPU_PROFILER_BUFFER_COUNT; ++Index)
    {
        NumPendingScopes[Index]     = 0;
        PendingCpuFrameIndex[Index] = -1;
    }
}

FGPUProfiler::~FGPUProfiler()
{
}

void FGPUProfiler::Release()
{
    TScopedLock Lock(StateLock);

    ReleaseQueries();
    StoredFrames.Clear();
}

void FGPUProfiler::ReleaseQueries()
{
    for (int32 Index = 0; Index < GPU_PROFILER_BUFFER_COUNT; ++Index)
    {
        FrameBeginQuery[Index].Reset();
        FrameEndQuery[Index].Reset();

        for (FGPUPendingScope& Scope : PendingScopes[Index])
        {
            Scope.BeginQuery.Reset();
            Scope.EndQuery.Reset();
        }

        PendingScopes[Index].Clear();

        NumPendingScopes[Index]     = 0;
        PendingCpuFrameIndex[Index] = -1;
    }

    for (auto ScopeQuery : ScopeQueries)
    {
        for (int32 Index = 0; Index < GPU_PROFILER_BUFFER_COUNT; ++Index)
        {
            ScopeQuery.Second.PipelineStatsQuery[Index].Reset();
        }
    }

    ScopeQueries.Clear();
    OpenTraceStack.Clear();

    OpenPipelineStatsQuery  = nullptr;
    PipelineStatsScopeIndex = -1;
    bFrameOpen              = false;
    WriteIndex              = 0;
}

void FGPUProfiler::Enable()
{
    TScopedLock Lock(StateLock);
    bEnabled = true;
}

void FGPUProfiler::Disable()
{
    TScopedLock Lock(StateLock);
    bEnabled = false;
}

void FGPUProfiler::SetRetainAllFrames(bool bInRetainAllFrames)
{
    TScopedLock Lock(StateLock);
    StoredFrames.SetRetainAll(bInRetainAllFrames);
}

void FGPUProfiler::EnablePipelineStatistics()
{
    TScopedLock Lock(StateLock);
    bPipelineStatsEnabled = true;
}

void FGPUProfiler::DisablePipelineStatistics()
{
    TScopedLock Lock(StateLock);
    bPipelineStatsEnabled = false;
}

bool FGPUProfiler::IsPipelineStatisticsEnabled() const
{
    TScopedLock Lock(StateLock);
    return bPipelineStatsEnabled;
}

void FGPUProfiler::Reset()
{
    TScopedLock Lock(StateLock);
    StoredFrames.Clear();

    for (int32 Index = 0; Index < GPU_PROFILER_BUFFER_COUNT; ++Index)
    {
        PendingCpuFrameIndex[Index] = -1;
        if (!bFrameOpen || Index != WriteIndex)
        {
            NumPendingScopes[Index] = 0;
        }
    }

    if (!bFrameOpen)
    {
        OpenTraceStack.Clear();

        OpenPipelineStatsQuery  = nullptr;
        PipelineStatsScopeIndex = -1;
    }
}

int32 FGPUProfiler::GetStoredFrameCount() const
{
    TScopedLock Lock(StateLock);
    return StoredFrames.Num();
}

bool FGPUProfiler::GetStoredFrame(int32 OldestIndex, FProfilerGpuFrame& OutFrame) const
{
    TScopedLock Lock(StateLock);
    if (const FProfilerGpuFrame* Frame = StoredFrames.GetOldest(OldestIndex))
    {
        OutFrame = *Frame;
        return true;
    }

    return false;
}

bool FGPUProfiler::FindFrameForCpuFrame(int32 CpuFrameIndex, FProfilerGpuFrame& OutFrame) const
{
    TScopedLock Lock(StateLock);
    if (const FProfilerGpuFrame* Frame = StoredFrames.FindIf([CpuFrameIndex](const FProfilerGpuFrame& Stored)
        {
            return Stored.CpuFrameIndex == CpuFrameIndex;
        }))
    {
        OutFrame = *Frame;
        return true;
    }

    return false;
}

bool FGPUProfiler::GetLatestFrame(FProfilerGpuFrame& OutFrame) const
{
    TScopedLock Lock(StateLock);
    if (const FProfilerGpuFrame* Frame = StoredFrames.GetLatest())
    {
        OutFrame = *Frame;
        return true;
    }

    return false;
}

void FGPUProfiler::CollectResults()
{
    const int32 ReadIndex = (WriteIndex + 1) % GPU_PROFILER_BUFFER_COUNT;
    if (!FrameBeginQuery[ReadIndex] || PendingCpuFrameIndex[ReadIndex] < 0)
    {
        return;
    }

    uint64 FrameBegin = 0;
    float  FrameMs    = 0.0f;

    if (FrameBeginQuery[ReadIndex] && FrameEndQuery[ReadIndex])
    {
        uint64 BeginResult = 0;
        uint64 EndResult   = 0;

        bool bBeginReady = RHI::Device->GetQueryResult(FrameBeginQuery[ReadIndex].Get(), BeginResult);
        bool bEndReady   = RHI::Device->GetQueryResult(FrameEndQuery[ReadIndex].Get(), EndResult);

        if (!bBeginReady || !bEndReady)
        {
            bBeginReady = RHI::Device->GetQueryResult(FrameBeginQuery[ReadIndex].Get(), BeginResult, EQueryResultMode::Wait);
            bEndReady   = RHI::Device->GetQueryResult(FrameEndQuery[ReadIndex].Get(), EndResult, EQueryResultMode::Wait);
        }

        if (!bBeginReady || !bEndReady)
        {
            return;
        }

        if (EndResult > BeginResult)
        {
            FrameBegin = BeginResult;
            FrameMs    = static_cast<float>(Time::ToMilliseconds(static_cast<double>(EndResult - BeginResult)));
        }
    }

    const int32 NumScopes = NumPendingScopes[ReadIndex];

    TArray<FGPUProfilerInterval> Collected;
    Collected.Reserve(NumScopes);

    TArray<int32> MappedIndices;
    MappedIndices.Resize(NumScopes);

    uint64 EarliestStart = TNumericLimits<uint64>::Max();
    for (int32 ScopeIndex = 0; ScopeIndex < NumScopes; ++ScopeIndex)
    {
        const FGPUPendingScope& Scope = PendingScopes[ReadIndex][ScopeIndex];
        const int32 MappedParent = (Scope.ParentIndex >= 0 && Scope.ParentIndex < ScopeIndex) ? MappedIndices[Scope.ParentIndex] : -1;

        uint64 BeginResult = 0;
        uint64 EndResult   = 0;

        if (Scope.BeginQuery && Scope.EndQuery)
        {
            bool bBeginReady = RHI::Device->GetQueryResult(Scope.BeginQuery.Get(), BeginResult);
            bool bEndReady   = RHI::Device->GetQueryResult(Scope.EndQuery.Get(), EndResult);

            if (!bBeginReady || !bEndReady)
            {
                bBeginReady = RHI::Device->GetQueryResult(Scope.BeginQuery.Get(), BeginResult, EQueryResultMode::Wait);
                bEndReady   = RHI::Device->GetQueryResult(Scope.EndQuery.Get(), EndResult, EQueryResultMode::Wait);
            }

            if (!bBeginReady || !bEndReady)
            {
                MappedIndices[ScopeIndex] = MappedParent;
                continue;
            }
        }

        if (EndResult <= BeginResult)
        {
            MappedIndices[ScopeIndex] = MappedParent;
            continue;
        }

        FGPUProfilerInterval Interval;
        Interval.Name                 = Scope.Name;
        Interval.StartNanoseconds     = BeginResult;
        Interval.EndNanoseconds       = EndResult;
        Interval.InclusiveNanoseconds = EndResult - BeginResult;
        Interval.ExclusiveNanoseconds = Interval.InclusiveNanoseconds;
        Interval.ParentIndex          = MappedParent;
        Interval.Depth                = MappedParent >= 0 ? Collected[MappedParent].Depth + 1 : 0;

        if (Scope.bOwnsPipelineStats)
        {
            const String ScopeName = Scope.Name ? Scope.Name : "";
            if (FGPUProfileScopeQueries* Queries = ScopeQueries.Find(ScopeName))
            {
                if (Queries->PipelineStatsQuery[ReadIndex])
                {
                    FRHIPipelineStatistics ScopeStats;
                    bool bStatsReady = RHI::Device->GetPipelineStatisticsResult(
                        Queries->PipelineStatsQuery[ReadIndex].Get(), ScopeStats);

                    if (!bStatsReady)
                    {
                        bStatsReady = RHI::Device->GetPipelineStatisticsResult(
                            Queries->PipelineStatsQuery[ReadIndex].Get(), ScopeStats, EQueryResultMode::Wait);
                    }

                    if (bStatsReady)
                    {
                        Interval.PipelineStats     = ScopeStats;
                        Interval.bHasPipelineStats = true;
                    }
                }
            }
        }

        MappedIndices[ScopeIndex] = Collected.Size();
        Collected.Add(Interval);

        EarliestStart = Math::Min(EarliestStart, BeginResult);
    }

    if (!Collected.IsEmpty())
    {
        const uint64 Base = (FrameBegin != 0 && FrameBegin <= EarliestStart) ? FrameBegin : EarliestStart;
        for (FGPUProfilerInterval& Interval : Collected)
        {
            Interval.StartNanoseconds -= Math::Min(Interval.StartNanoseconds, Base);
            Interval.EndNanoseconds   -= Math::Min(Interval.EndNanoseconds, Base);
        }

        ApplyExclusiveTimesFromParentIndex(Collected);
    }

    if (FrameMs <= 0.0f && Collected.IsEmpty())
    {
        return;
    }

    FProfilerGpuFrame Stored;
    Stored.CpuFrameIndex   = PendingCpuFrameIndex[ReadIndex];
    Stored.GpuMilliseconds = FrameMs;
    Stored.Intervals       = Move(Collected);

    StoredFrames.Push(Move(Stored));
}

void FGPUProfiler::BeginGPUFrame(FRHICommandList& CmdList)
{
    TScopedLock Lock(StateLock);
    if (bEnabled)
    {
        CollectResults();

        WriteIndex = (WriteIndex + 1) % GPU_PROFILER_BUFFER_COUNT;

        NumPendingScopes[WriteIndex]     = 0;
        PendingCpuFrameIndex[WriteIndex] = FFrameProfiler::Get().GetLatestFinishedFrameIndex();

        OpenTraceStack.Clear();

        OpenPipelineStatsQuery  = nullptr;
        PipelineStatsScopeIndex = -1;
        bFrameOpen              = true;

        if (!FrameBeginQuery[WriteIndex])
        {
            FrameBeginQuery[WriteIndex] = RHI::CreateQuery(EQueryType::Timestamp);
        }

        CmdList.QueryTimestamp(FrameBeginQuery[WriteIndex].Get());
    }
}

void FGPUProfiler::EndGPUFrame(FRHICommandList& CmdList)
{
    TScopedLock Lock(StateLock);
    if (bFrameOpen)
    {
        if (!FrameEndQuery[WriteIndex])
        {
            FrameEndQuery[WriteIndex] = RHI::CreateQuery(EQueryType::Timestamp);
        }

        CmdList.QueryTimestamp(FrameEndQuery[WriteIndex].Get());
        bFrameOpen = false;
    }
}

void FGPUProfiler::BeginGPUTrace(FRHICommandList& CmdList, const CHAR* Name)
{
    TScopedLock Lock(StateLock);
    if (!bEnabled || !bFrameOpen)
    {
        return;
    }

    TArray<FGPUPendingScope>& Pending    = PendingScopes[WriteIndex];
    const int32               ScopeIndex = NumPendingScopes[WriteIndex]++;

    if (ScopeIndex >= Pending.Size())
    {
        FGPUPendingScope NewScope;
        NewScope.BeginQuery = RHI::CreateQuery(EQueryType::Timestamp);
        NewScope.EndQuery   = RHI::CreateQuery(EQueryType::Timestamp);
        Pending.Add(NewScope);
    }

    FGPUPendingScope& Scope = Pending[ScopeIndex];
    Scope.Name               = Name;
    Scope.Depth              = OpenTraceStack.Size();
    Scope.ParentIndex        = OpenTraceStack.IsEmpty() ? -1 : OpenTraceStack.Last();
    Scope.bOwnsPipelineStats = false;
    OpenTraceStack.Add(ScopeIndex);

    if (Scope.BeginQuery)
    {
        CmdList.QueryTimestamp(Scope.BeginQuery.Get());
    }

    if (bPipelineStatsEnabled && !OpenPipelineStatsQuery)
    {
        const String ScopeName = Name;

        FGPUProfileScopeQueries* Queries = ScopeQueries.Find(ScopeName);
        if (!Queries)
        {
            Queries = &ScopeQueries.Add(ScopeName);
        }

        if (!Queries->PipelineStatsQuery[WriteIndex])
        {
            Queries->PipelineStatsQuery[WriteIndex] = RHI::CreateQuery(EQueryType::PipelineStatistics);
        }

        OpenPipelineStatsQuery   = Queries->PipelineStatsQuery[WriteIndex].Get();
        PipelineStatsScopeIndex  = ScopeIndex;
        Scope.bOwnsPipelineStats = true;

        CmdList.BeginQuery(OpenPipelineStatsQuery);
    }
}

void FGPUProfiler::EndGPUTrace(FRHICommandList& CmdList, const CHAR* /* Name */)
{

    TScopedLock Lock(StateLock);
    if (!bFrameOpen || OpenTraceStack.IsEmpty())
    {
        return;
    }

    const int32 ScopeIndex = OpenTraceStack.Last();
    OpenTraceStack.RemoveAt(OpenTraceStack.Size() - 1);

    if (ScopeIndex >= 0 && ScopeIndex < PendingScopes[WriteIndex].Size())
    {
        FGPUPendingScope& Scope = PendingScopes[WriteIndex][ScopeIndex];
        if (Scope.EndQuery)
        {
            CmdList.QueryTimestamp(Scope.EndQuery.Get());
        }
    }

    if (OpenPipelineStatsQuery && PipelineStatsScopeIndex == ScopeIndex)
    {
        CmdList.EndQuery(OpenPipelineStatsQuery);

        OpenPipelineStatsQuery  = nullptr;
        PipelineStatsScopeIndex = -1;
    }
}
