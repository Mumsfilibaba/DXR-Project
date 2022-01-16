#include "GPUProfiler.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Time/Timestamp.h"

#include "RHI/RHIInterface.h"

CGPUProfiler CGPUProfiler::Instance;

CGPUProfiler::CGPUProfiler()
    : Timequeries(nullptr)
    , FrameTime()
    , Samples()
    , bEnabled(false)
{
}

bool CGPUProfiler::Init()
{
    Instance.Timequeries = RHICreateTimestampQuery();
    if (!Instance.Timequeries)
    {
        return false;
    }

    return true;
}

void CGPUProfiler::Release()
{
    Instance.Timequeries.Reset();
}

void CGPUProfiler::Enable()
{
    bEnabled = true;
}

void CGPUProfiler::Disable()
{
    bEnabled = false;
}

void CGPUProfiler::Tick()
{
    if (Timequeries)
    {
        SRHITimestamp Query;
        Timequeries->GetTimestampFromIndex(Query, FrameTime.TimeQueryIndex);

        const double Frequency = static_cast<double>(Timequeries->GetFrequency());
        const double DeltaTime = static_cast<double>(Query.End - Query.Begin);

        double Duration = (DeltaTime / Frequency) * 1000.0;
        FrameTime.AddSample((float)Duration);
    }
}

void CGPUProfiler::Reset()
{
    FrameTime.Reset();

    {
        TScopedLock Lock(Samples);
        for (auto& Sample : Samples.Get())
        {
            Sample.second.Reset();
        }
    }
}

void CGPUProfiler::GetGPUSamples(GPUProfileSamplesTable& OutSamples)
{
    TScopedLock Lock(Samples);
    OutSamples = Samples.Get();
}

void CGPUProfiler::BeginGPUFrame(CRHICommandList& CmdList)
{
    if (Timequeries && bEnabled)
    {
        CmdList.BeginTimeStamp(Timequeries.Get(), FrameTime.TimeQueryIndex);
    }
}

void CGPUProfiler::EndGPUFrame(CRHICommandList& CmdList)
{
    if (Timequeries && bEnabled)
    {
        CmdList.EndTimeStamp(Timequeries.Get(), FrameTime.TimeQueryIndex);
    }
}

void CGPUProfiler::BeginGPUTrace(CRHICommandList& CmdList, const char* Name)
{
    if (Timequeries && bEnabled)
    {
        const CString ScopeName = Name;

        int32 TimeQueryIndex = -1;

        {
            TScopedLock Lock(Samples);

            auto Entry = Samples.Get().find(ScopeName);
            if (Entry == Samples.Get().end())
            {
                auto NewSample = Samples.Get().insert(std::make_pair(ScopeName, SGPUProfileSample()));
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

void CGPUProfiler::EndGPUTrace(CRHICommandList& CmdList, const char* Name)
{
    if (Timequeries && bEnabled)
    {
        const CString ScopeName = Name;

        int32 TimeQueryIndex = -1;

        TScopedLock Lock(Samples);

        auto Entry = Samples.Get().find(ScopeName);
        if (Entry != Samples.Get().end())
        {
            TimeQueryIndex = Entry->second.TimeQueryIndex;
            CmdList.EndTimeStamp(Timequeries.Get(), TimeQueryIndex);

            if (TimeQueryIndex >= 0)
            {
                SRHITimestamp Query;
                Timequeries->GetTimestampFromIndex(Query, TimeQueryIndex);

                const double Frequency = static_cast<double>(Timequeries->GetFrequency());

                double Duration = NTime::ToSeconds<double>(static_cast<double>((Query.End - Query.Begin) / Frequency));
                Entry->second.AddSample((float)Duration);
            }
        }
    }
}
