#include "FrameProfiler.h"

#include "Core/Threading/ScopedLock.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FFrameProfiler

FFrameProfiler& FFrameProfiler::Get()
{
    static FFrameProfiler Instance;
    return Instance;
}

void FFrameProfiler::Tick()
{
    Clock.Tick();

    CurrentFps++;
    if (Clock.GetTotalTime().AsSeconds() > 1.0f)
    {
        Fps = CurrentFps;
        CurrentFps = 0;

        Clock.Reset();
    }

    if (bEnabled)
    {
        const double Delta = Clock.GetDeltaTime().AsMilliSeconds();
        CPUFrameTime.AddSample(float(Delta));
    }
}

void FFrameProfiler::Enable()
{
	FFrameProfiler& Instance = FFrameProfiler::Get();
    Instance.bEnabled = true;
}

void FFrameProfiler::Disable()
{
	FFrameProfiler& Instance = FFrameProfiler::Get();
    Instance.bEnabled = false;
}

void FFrameProfiler::Reset()
{
    CPUFrameTime.Reset();

    {
        TScopedLock Lock(SamplesTableLock);
        for (auto& Sample : SamplesTable)
        {
            Sample.second.Reset();
        }
    }
}

void FFrameProfiler::BeginTraceScope(const char* Name)
{
    if (bEnabled)
    {
        TScopedLock Lock(SamplesTableLock);

        const FString ScopeName = Name;

        auto Entry = SamplesTable.find(ScopeName);
        if (Entry == SamplesTable.end())
        {
            auto NewSample = SamplesTable.insert(std::make_pair(ScopeName, FProfileSample()));
            NewSample.first->second.Begin();
        }
        else
        {
            Entry->second.Begin();
        }
    }
}

void FFrameProfiler::EndTraceScope(const char* Name)
{
    if (bEnabled)
    {
        const FString ScopeName = Name;

        TScopedLock Lock(SamplesTableLock);

        auto Entry = SamplesTable.find(ScopeName);
        if (Entry != SamplesTable.end())
        {
            Entry->second.End();
        }
        else
        {
            Check(false);
        }
    }
}

void FFrameProfiler::GetCPUSamples(ProfileSamplesTable& OutCPUSamples)
{
    TScopedLock Lock(SamplesTableLock);
    OutCPUSamples = SamplesTable;
}
