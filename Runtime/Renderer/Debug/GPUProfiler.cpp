#include "GPUProfiler.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Time/Timestamp.h"

#include "RHI/RHICoreInterface.h"

FGPUProfiler FGPUProfiler::Instance;

FGPUProfiler::FGPUProfiler()
    : Timequeries(nullptr)
    , FrameTime()
    , Samples()
    , bEnabled(false)
{ }

bool FGPUProfiler::Init()
{
    Instance.Timequeries = RHICreateTimestampQuery();
    if (!Instance.Timequeries)
    {
        return false;
    }

    return true;
}

void FGPUProfiler::Release()
{
    Instance.Timequeries.Reset();
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
    if (Timequeries)
    {
        FRHITimestamp Query;
        Timequeries->GetTimestampFromIndex(Query, FrameTime.TimeQueryIndex);

        const double Frequency = static_cast<double>(Timequeries->GetFrequency());
        const double DeltaTime = static_cast<double>(Query.End - Query.Begin);

        double Duration = (DeltaTime / Frequency) * 1000.0;
        FrameTime.AddSample((float)Duration);
    }
}

void FGPUProfiler::Reset()
{
    FrameTime.Reset();

    {
        TScopedLock Lock(SamplesLock);
        for (auto& Sample : Samples)
        {
            Sample.second.Reset();
        }
    }
}

void FGPUProfiler::GetGPUSamples(GPUProfileSamplesTable& OutSamples)
{
    TScopedLock Lock(SamplesLock);
    OutSamples = Samples;
}

void FGPUProfiler::BeginGPUFrame(FRHICommandList& CmdList)
{
    if (Timequeries && bEnabled)
    {
        CmdList.BeginTimeStamp(Timequeries.Get(), FrameTime.TimeQueryIndex);
    }
}

void FGPUProfiler::EndGPUFrame(FRHICommandList& CmdList)
{
    if (Timequeries && bEnabled)
    {
        CmdList.EndTimeStamp(Timequeries.Get(), FrameTime.TimeQueryIndex);
    }
}

void FGPUProfiler::BeginGPUTrace(FRHICommandList& CmdList, const char* Name)
{
    if (Timequeries && bEnabled)
    {
        const FString ScopeName = Name;

        int32 TimeQueryIndex = -1;

        {
            TScopedLock Lock(SamplesLock);

            auto Entry = Samples.find(ScopeName);
            if (Entry == Samples.end())
            {
                auto NewSample = Samples.insert(std::make_pair(ScopeName, FGPUProfileSample()));
                NewSample.first->second.TimeQueryIndex = ++CurrentTimeQueryIndex;
                TimeQueryIndex = NewSample.first->second.TimeQueryIndex;
            }
            else
            {
                TimeQueryIndex = Entry->second.TimeQueryIndex;
            }
        }

        if (TimeQueryIndex >= 0)
        {
            CmdList.BeginTimeStamp(Timequeries.Get(), TimeQueryIndex);
        }
    }
}

void FGPUProfiler::EndGPUTrace(FRHICommandList& CmdList, const char* Name)
{
    if (Timequeries && bEnabled)
    {
        const FString ScopeName = Name;

        int32 TimeQueryIndex = -1;

        TScopedLock Lock(SamplesLock);

        auto Entry = Samples.find(ScopeName);
        if (Entry != Samples.end())
        {
            TimeQueryIndex = Entry->second.TimeQueryIndex;
            CmdList.EndTimeStamp(Timequeries.Get(), TimeQueryIndex);

            if (TimeQueryIndex >= 0)
            {
                FRHITimestamp Query;
                Timequeries->GetTimestampFromIndex(Query, TimeQueryIndex);

                const double Frequency = static_cast<double>(Timequeries->GetFrequency());

                double Duration = NTime::ToSeconds<double>(static_cast<double>((Query.End - Query.Begin) / Frequency));
                Entry->second.AddSample((float)Duration);
            }
        }
    }
}
