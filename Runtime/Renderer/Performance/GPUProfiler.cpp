#include "Core/Threading/ScopedLock.h"
#include "Core/Time/Timespan.h"
#include "RHI/RHI.h"
#include "Renderer/Performance/GPUProfiler.h"

FGPUProfiler FGPUProfiler::GPUProfiler;

FGPUProfiler::FGPUProfiler()
    : FrameTime()
    , Samples()
    , LastPipelineStats()
    , PipelineStatsMinMax()
    , bEnabled(false)
    , bPipelineStatsEnabled(false)
    , WriteIndex(0)
    , PipelineStatsNestingDepth(0)
{
}

FGPUProfiler::~FGPUProfiler()
{
}

void FGPUProfiler::Release()
{
    for (int32 i = 0; i < GPU_PROFILER_BUFFER_COUNT; i++)
    {
        FrameBeginQuery[i].Reset();
        FrameEndQuery[i].Reset();
    }

    for (auto ScopeQuery : ScopeQueries)
    {
        for (int32 i = 0; i < GPU_PROFILER_BUFFER_COUNT; i++)
        {
            ScopeQuery.Second.BeginQuery[i].Reset();
            ScopeQuery.Second.EndQuery[i].Reset();
            ScopeQuery.Second.PipelineStatsQuery[i].Reset();
        }
    }

    ScopeQueries.Clear();
}

void FGPUProfiler::Enable()
{
    bEnabled = true;
}

void FGPUProfiler::Disable()
{
    bEnabled = false;
}

void FGPUProfiler::EnablePipelineStatistics()
{
    bPipelineStatsEnabled = true;
}

void FGPUProfiler::DisablePipelineStatistics()
{
    bPipelineStatsEnabled = false;
}

bool FGPUProfiler::IsPipelineStatisticsEnabled() const
{
    return bPipelineStatsEnabled;
}

void FGPUProfiler::Reset()
{
    FrameTime.Reset();
    PipelineStatsMinMax.Reset();
    
    LastPipelineStats = FRHIPipelineStatistics();

    TScopedLock Lock(SamplesLock);
    for (auto Sample : Samples)
    {
        Sample.Second.Reset();
    }
}

void FGPUProfiler::GetGPUSamples(GPUProfileSamplesMap& OutSamples)
{
    TScopedLock Lock(SamplesLock);
    OutSamples = Samples;
}

void FGPUProfiler::CollectResults()
{
    const int32 ReadIndex = (WriteIndex + 1) % GPU_PROFILER_BUFFER_COUNT;

    if (FrameBeginQuery[ReadIndex] && FrameEndQuery[ReadIndex])
    {
        uint64 BeginResult = 0;
        uint64 EndResult   = 0;

        RHI::Device->GetQueryResult(FrameBeginQuery[ReadIndex].Get(), BeginResult);
        RHI::Device->GetQueryResult(FrameEndQuery[ReadIndex].Get(), EndResult);

        if (BeginResult != 0 && EndResult != 0 && EndResult > BeginResult)
        {
            const double DeltaTime = static_cast<double>(EndResult - BeginResult);
            const double Duration  = DeltaTime / 1000000.0;
            FrameTime.AddSample(static_cast<float>(Duration));
        }
    }

    FRHIPipelineStatistics FrameTotalStats = {};

    TScopedLock Lock(SamplesLock);

    for (auto ScopeQuery : ScopeQueries)
    {
        FGPUProfileSample* Entry = Samples.Find(ScopeQuery.First);
        if (!Entry)
        {
            continue;
        }

        if (ScopeQuery.Second.BeginQuery[ReadIndex] && ScopeQuery.Second.EndQuery[ReadIndex])
        {
            uint64 BeginResult = 0;
            uint64 EndResult   = 0;

            RHI::Device->GetQueryResult(ScopeQuery.Second.BeginQuery[ReadIndex].Get(), BeginResult);
            RHI::Device->GetQueryResult(ScopeQuery.Second.EndQuery[ReadIndex].Get(), EndResult);

            if (BeginResult != 0 && EndResult != 0 && EndResult > BeginResult)
            {
                const double DeltaTime = static_cast<double>(EndResult - BeginResult);
                Entry->AddSample(static_cast<float>(DeltaTime));
            }
        }

        if (bPipelineStatsEnabled && ScopeQuery.Second.PipelineStatsQuery[ReadIndex])
        {
            FRHIPipelineStatistics ScopeStats;
            if (RHI::Device->GetPipelineStatisticsResult(ScopeQuery.Second.PipelineStatsQuery[ReadIndex].Get(), ScopeStats))
            {
                FrameTotalStats.IAVertices    += ScopeStats.IAVertices;
                FrameTotalStats.IAPrimitives  += ScopeStats.IAPrimitives;
                FrameTotalStats.VSInvocations += ScopeStats.VSInvocations;
                FrameTotalStats.GSInvocations += ScopeStats.GSInvocations;
                FrameTotalStats.GSPrimitives  += ScopeStats.GSPrimitives;
                FrameTotalStats.CInvocations  += ScopeStats.CInvocations;
                FrameTotalStats.CPrimitives   += ScopeStats.CPrimitives;
                FrameTotalStats.PSInvocations += ScopeStats.PSInvocations;
                FrameTotalStats.HSInvocations += ScopeStats.HSInvocations;
                FrameTotalStats.DSInvocations += ScopeStats.DSInvocations;
                FrameTotalStats.CSInvocations += ScopeStats.CSInvocations;
                FrameTotalStats.ASInvocations += ScopeStats.ASInvocations;
                FrameTotalStats.MSInvocations += ScopeStats.MSInvocations;
                FrameTotalStats.MSPrimitives  += ScopeStats.MSPrimitives;
            }
        }
    }

    if (bPipelineStatsEnabled && FrameTotalStats.HasAnyActivity())
    {
        LastPipelineStats = FrameTotalStats;
        PipelineStatsMinMax.Update(FrameTotalStats);
    }
}

void FGPUProfiler::BeginGPUFrame(FRHICommandList& CmdList)
{
    if (bEnabled)
    {
        CollectResults();

        PipelineStatsNestingDepth = 0;
        WriteIndex = (WriteIndex + 1) % GPU_PROFILER_BUFFER_COUNT;

        if (!FrameBeginQuery[WriteIndex])
        {
            FrameBeginQuery[WriteIndex] = RHI::CreateQuery(EQueryType::Timestamp);
        }

        CmdList.QueryTimestamp(FrameBeginQuery[WriteIndex].Get());
    }
}

void FGPUProfiler::EndGPUFrame(FRHICommandList& CmdList)
{
    if (bEnabled)
    {
        if (!FrameEndQuery[WriteIndex])
        {
            FrameEndQuery[WriteIndex] = RHI::CreateQuery(EQueryType::Timestamp);
        }

        CmdList.QueryTimestamp(FrameEndQuery[WriteIndex].Get());
    }
}

void FGPUProfiler::BeginGPUTrace(FRHICommandList& CmdList, const CHAR* Name)
{
    if (bEnabled)
    {
        const String ScopeName = Name;

        FGPUProfileScopeQueries* Queries = ScopeQueries.Find(ScopeName);
        if (!Queries)
        {
            FGPUProfileScopeQueries& NewQueries = ScopeQueries.Add(ScopeName);
            for (int32 i = 0; i < GPU_PROFILER_BUFFER_COUNT; i++)
            {
                NewQueries.BeginQuery[i] = RHI::CreateQuery(EQueryType::Timestamp);
                NewQueries.EndQuery[i]   = RHI::CreateQuery(EQueryType::Timestamp);
            }
            
            Queries = &NewQueries;

            TScopedLock Lock(SamplesLock);
            Samples.Add(ScopeName);
        }

        CHECK(Queries->BeginQuery[WriteIndex]);
        CmdList.QueryTimestamp(Queries->BeginQuery[WriteIndex].Get());

        if (bPipelineStatsEnabled && PipelineStatsNestingDepth == 0)
        {
            if (!Queries->PipelineStatsQuery[WriteIndex])
            {
                Queries->PipelineStatsQuery[WriteIndex] = RHI::CreateQuery(EQueryType::PipelineStatistics);
            }

            CmdList.BeginQuery(Queries->PipelineStatsQuery[WriteIndex].Get());
        }

        if (bPipelineStatsEnabled)
        {
            PipelineStatsNestingDepth++;
        }
    }
}

void FGPUProfiler::EndGPUTrace(FRHICommandList& CmdList, const CHAR* Name)
{
    if (bEnabled)
    {
        const String ScopeName = Name;

        FGPUProfileScopeQueries* Queries = ScopeQueries.Find(ScopeName);
        if (Queries)
        {
            if (bPipelineStatsEnabled)
            {
                PipelineStatsNestingDepth--;
                CHECK(PipelineStatsNestingDepth >= 0);

                if (PipelineStatsNestingDepth == 0 && Queries->PipelineStatsQuery[WriteIndex])
                {
                    CmdList.EndQuery(Queries->PipelineStatsQuery[WriteIndex].Get());
                }
            }

            CHECK(Queries->EndQuery[WriteIndex]);
            CmdList.QueryTimestamp(Queries->EndQuery[WriteIndex].Get());
        }
        else
        {
            DEBUG_BREAK();
        }
    }
}
