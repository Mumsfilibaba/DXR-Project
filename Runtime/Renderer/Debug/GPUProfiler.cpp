#include "GPUProfiler.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Time/Timespan.h"
#include "RHI/RHI.h"

FGPUProfiler FGPUProfiler::Instance;

FGPUProfiler::FGPUProfiler()
    : FrameTime()
    , Samples()
    , bEnabled(false)
{
}

void FGPUProfiler::Release()
{
    FrameTime.BeginQuery.Reset();
    FrameTime.EndQuery.Reset();

    for (auto Sample : Samples)
    {
        Sample.Second.BeginQuery.Reset();
        Sample.Second.EndQuery.Reset();
    }
}

void FGPUProfiler::Enable()
{
    bEnabled = true;
}

void FGPUProfiler::Disable()
{
    bEnabled = false;
}

void FGPUProfiler::Tick()
{
    if (bEnabled && FrameTime.BeginQuery && FrameTime.EndQuery)
    {
        uint64 BeginQuery;
        GetRHI()->RHIGetQueryResult(FrameTime.BeginQuery.Get(), BeginQuery);
        uint64 EndQuery;
        GetRHI()->RHIGetQueryResult(FrameTime.EndQuery.Get(), EndQuery);

        const double DeltaTime = static_cast<double>(EndQuery - BeginQuery);
        double Duration = DeltaTime / 1000000.0; // To milliseconds
        FrameTime.AddSample((float)Duration);
    }
}

void FGPUProfiler::Reset()
{
    FrameTime.Reset();

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

void FGPUProfiler::BeginGPUFrame(FRHICommandList& CmdList)
{
    if (bEnabled)
    {
        if (!FrameTime.BeginQuery)
            FrameTime.BeginQuery = RHICreateQuery(EQueryType::Timestamp);

        CmdList.QueryTimestamp(FrameTime.BeginQuery.Get());
    }
}

void FGPUProfiler::EndGPUFrame(FRHICommandList& CmdList)
{
    if (bEnabled)
    {
        if (!FrameTime.EndQuery)
            FrameTime.EndQuery = RHICreateQuery(EQueryType::Timestamp);

        CmdList.QueryTimestamp(FrameTime.EndQuery.Get());
    }
}

void FGPUProfiler::BeginGPUTrace(FRHICommandList& CmdList, const CHAR* Name)
{
    if (bEnabled)
    {
        const FString ScopeName = Name;

        FRHIQueryRef Query;
        {
            TScopedLock Lock(SamplesLock);

            if (FGPUProfileSample* Sample = Samples.Find(ScopeName))
            {
                Query = Sample->BeginQuery;
            }
            else
            {
                FGPUProfileSample& NewSample = Samples.Add(ScopeName);
                Query = NewSample.BeginQuery = RHICreateQuery(EQueryType::Timestamp);
            }
        }

        if (Query)
        {
            CmdList.QueryTimestamp(Query.Get());
        }
        else
        {
            DEBUG_BREAK();
        }
    }
}

void FGPUProfiler::EndGPUTrace(FRHICommandList& CmdList, const CHAR* Name)
{
    if (bEnabled)
    {
        const FString ScopeName = Name;

        TScopedLock Lock(SamplesLock);
        if (FGPUProfileSample* Entry = Samples.Find(ScopeName))
        {
            if (!Entry->EndQuery)
                Entry->EndQuery = RHICreateQuery(EQueryType::Timestamp);

            if (Entry->EndQuery)
            {
                CmdList.QueryTimestamp(Entry->EndQuery.Get());

                uint64 BeginQuery;
                GetRHI()->RHIGetQueryResult(Entry->BeginQuery.Get(), BeginQuery);
                uint64 EndQuery;
                GetRHI()->RHIGetQueryResult(Entry->EndQuery.Get(), EndQuery);

                const double DeltaTime = static_cast<double>(EndQuery - BeginQuery);
                Entry->AddSample((float)DeltaTime);
            }
            else
            {
                DEBUG_BREAK();
            }
        }
        else
        {
            DEBUG_BREAK();
        }
    }
}
